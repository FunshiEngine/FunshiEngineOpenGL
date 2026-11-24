/*
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    SPDX-License-Identifier: Apache-2.0
*/
#include "MeshRendererDetalle.h"

namespace {

bool matrizFinita(const glm::mat3& m) {
    const float* p = glm::value_ptr(m);
    for (int i = 0; i < 9; ++i)
        if (!std::isfinite(p[i])) return false;
    return true;
}

}  // namespace

bool MeshRenderer::intentarRenderMesh(GameObject* contexto, const Mesh* mesh,
                                      const float modelo[16],
                                      const float view[16],
                                      const float projection[16],
                                      float deltaTime) {
    // El render NO simula: los scripts y la fisica se actualizan una sola vez
    // por frame en GameScene::update (y solo en play). Antes se llamaba aca a
    // objeto->update(), lo que corria los scripts tambien en el editor, los
    // duplicaba en play y reactivaba el loop al detener el play.
    (void)deltaTime;

    if (!contexto) return false;

    // Cada region de la malla se dibuja con el material que le corresponde: lo
    // decide el resolvedor con el objeto como contexto.
    return renderMesh(contexto, mesh, modelo, view, projection,
                      [this, contexto](const std::string& nombreRegion) {
                          return resolvedorMateriales_
                                     ? resolvedorMateriales_->materialDeRegion(
                                           contexto, nombreRegion)
                                     : nullptr;
                      });
}

bool MeshRenderer::renderMeshConMaterial(const Mesh* mesh,
                                         const IMaterial* material,
                                         const float modelo[16],
                                         const float view[16],
                                         const float projection[16]) {
    // Un unico material para toda la malla: es la vista previa de un material
    // (la esfera de muestra), donde las regiones no aplican.
    return renderMesh(nullptr, mesh, modelo, view, projection,
                      [material](const std::string&) { return material; });
}

bool MeshRenderer::renderMesh(GameObject* contexto, const Mesh* mesh,
                              const float modelo[16], const float view[16],
                              const float projection[16],
                              const ProveedorMaterial& proveedor) {
    if (!inicializar()) return false;

    // Sin malla, vacia o sin normales no hay nada que pintar. Ya no existe el
    // modo inmediato de respaldo, asi que se avisa una vez por malla (el loader
    // rellena las normales que faltan, de modo que esto solo deberia tocar a
    // mallas construidas a mano).
    if (!mesh || mesh->isEmpty() || !mesh->hasNormals()) {
        if (mesh && mallasSinNormales_.insert(mesh).second) {
            std::cerr << "[MeshRenderer] Malla '" << mesh->name
                      << "' sin normales por vertice: no se dibuja. Use "
                         "Mesh::computeNormals() para generarlas."
                      << '\n';
        }
        return false;
    }

    const glm::mat4 model = glm::make_mat4(modelo);
    const glm::mat4 viewMat = glm::make_mat4(view);
    const glm::mat4 proj = glm::make_mat4(projection);
    const glm::mat4 modelView = viewMat * model;

    // Deja la matriz normal como transpose(inverse(model)) pero identidad si la
    // inversa degenero (fracciones no finitas); evita NaN en el shader.
    glm::mat3 normalMatrix =
        glm::mat3(glm::transpose(glm::inverse(model)));
    if (!matrizFinita(normalMatrix)) normalMatrix = glm::mat3(1.0f);

    shader_->use();
    shader_->setMat4("uModel", model);
    shader_->setMat4("uView", viewMat);
    shader_->setMat4("uProjection", proj);
    shader_->setMat4("uModelView", modelView);
    shader_->setMat3("uNormalMatrix", normalMatrix);

    const glm::mat4 invView = glm::inverse(viewMat);
    shader_->setVec3("uCameraPosition",
                     glm::vec3(invView[3][0], invView[3][1], invView[3][2]));
    shader_->setFloat("uDistanceFadeStart", horizonteInicio_);
    shader_->setFloat("uDistanceFadeEnd", horizonteFin_);

    // El relleno de poligonos es el unico estado valido en un contexto core
    // (no hay glPolygonMode) y el depth test ya lo activa applyBaseState().
    if (!backend_) return false;
    auto& backend = *backend_;
    backend.setBlendEnabled(true);

    const MeshGPU* gpu = obtenerGpu(mesh);
    if (!gpu) {
        backend.setBlendEnabled(false);
        return false;
    }

    // Malla con regiones (materiales por parte): cada region se dibuja con su
    // material, en el rango de indices que le corresponde. Sin regiones, la
    // malla entera se dibuja de una vez con el material del objeto.
    if (mesh->subMallas.empty()) {
        const IMaterial* material = proveedor("");
        aplicarMaterial(contexto, material);
        aplicarTexturas(material, mesh);
        enlazarTexturasSombra();
        gpu->draw();
    } else {
        for (const SubMalla& region : mesh->subMallas) {
            if (region.indiceCantidad == 0) continue;
            const IMaterial* material = proveedor(region.nombreMaterial);
            aplicarMaterial(contexto, material);
            aplicarTexturas(material, mesh);
            enlazarTexturasSombra();
            gpu->draw(region.indiceInicio, region.indiceCantidad);
        }
    }

    backend.setBlendEnabled(false);
    ShaderProgram::unbind(backend);
    return true;
}

// Sube la malla a GPU si no estaba ya (la cache es por identidad de la malla
// CPU, que el AssetManager comparte entre objetos). Devuelve nullptr si la
// malla no se puede dibujar.
const MeshGPU* MeshRenderer::obtenerGpu(const Mesh* mesh) {
    if (!mesh) return nullptr;
    auto it = gpu_.find(mesh);
    if (it != gpu_.end()) return it->second.get();

    if (!backend_) return nullptr;
    auto gpu = std::make_unique<MeshGPU>(*backend_);
    gpu->upload(&mesh->vertices[0].x, mesh->vertices.size(),
                mesh->hasNormals() ? &mesh->normals[0].x : nullptr,
                mesh->normals.size(),
                mesh->hasUvs() ? &mesh->uvs[0].x : nullptr,
                mesh->uvs.size(),
                mesh->hasTangents() ? &mesh->tangents[0].x : nullptr,
                mesh->tangents.size(),
                mesh->hasTangents() ? &mesh->bitangents[0].x : nullptr,
                mesh->bitangents.size(),
                mesh->indices.empty() ? nullptr : &mesh->indices[0],
                mesh->indices.size());
    it = gpu_.emplace(mesh, std::move(gpu)).first;
    return it->second.get();
}

void MeshRenderer::enlazarTexturasSombra() {
    if (!shader_ || !backend_) return;
    Rendering::UniformsSombras::enlazarTexturas(*backend_, sombras_);
}

bool MeshRenderer::intentarRenderSombra(const Mesh* mesh, const float modelo[16],
                                        const ParametrosProfundidad& pase) {
    if (!mesh || mesh->isEmpty()) return false;
    if (!inicializar()) return false;
    // Sin programa de profundidad no hay pase de sombras, pero el resto de la
    // escena se dibuja igual.
    ShaderProgram* programa =
        pase.esCubo ? shaderSombraCubo_.get() : shaderSombra_.get();
    if (!programa || !backend_) return false;

    const glm::mat4 model = glm::make_mat4(modelo);
    if (!matrizFinita(glm::mat3(model))) return false;

    const MeshGPU* gpu = obtenerGpu(mesh);
    if (!gpu) return false;

    programa->use();
    programa->setMat4("uModelLight", pase.matrizLuz * model);
    if (pase.esCubo) {
        programa->setMat4("uModel", model);
        programa->setVec3("uPosLuz", pase.posicionLuz);
        programa->setFloat("uAlcance", pase.alcance > 0.f ? pase.alcance : 1.f);
    }

    // El pase de profundidad no escribe color: el blend se apaga.
    backend_->setBlendEnabled(false);
    gpu->draw();
    ShaderProgram::unbind(*backend_);
    return true;
}
