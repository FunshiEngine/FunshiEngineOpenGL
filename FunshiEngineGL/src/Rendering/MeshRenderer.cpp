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
#include "MeshRenderer.h"
#include "../Diagnosticos/RegistroDiagnosticos.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

#include "../Assets/Mesh.h"
#include "../Assets/TextureManager.h"
#include "../Iluminacion/LightSystem.h"
#include "../Objetos/Componentes/Color.h"
#include "../Objetos/Componentes/IMaterial.h"
#include "../Objetos/Componentes/IResolutorMateriales.h"
#include "../Objetos/Componentes/Material.h"
#include "../Objetos/Componentes/Model.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/Modelos3D.h"
#include "MeshGPU.h"
#include "Shaders/ShaderProgram.h"
#include "Shaders/GestorShaders.h"
#include "Sombras/MapaSombras.h"
#include "TextureGL.h"
#include "Backend/IRenderBackend.h"

namespace {

bool matrizFinita(const glm::mat3& m) {
    const float* p = glm::value_ptr(m);
    for (int i = 0; i < 9; ++i)
        if (!std::isfinite(p[i])) return false;
    return true;
}

// Unidades de textura reservadas para los mapas de sombra. Las del material
// ocupan las cuatro primeras (difusa, especular, emision y normal), asi que
// los mapas de sombra arrancan en la quinta.
constexpr int kUnidadTexturaSombra = 4;

} // namespace

MeshRenderer::MeshRenderer() = default;

MeshRenderer::~MeshRenderer() = default;

bool MeshRenderer::inicializar() {
    if (shader_) return true;
    // El fuente del shader va embebido: si el driver no lo compilo una vez, no
    // lo compila nunca. Recordar el fallo evita reintentar la compilacion y
    // releer el error en cada pasada de dibujo (el reintento por frame llego a
    // escribir 319.030 lineas de log en una sesion y compilar el programa una
    // vez por frame).
    if (falloShader_) return false;
    if (!backend_ || !backend_->init()) return false;
    // Sin gestor de shaders no hay de donde pedir el programa.
    if (!gestorShaders_) return false;
    try {
        shader_ = gestorShaders_->obtener("malla");
        // El programa del pase de profundidad es opcional: si no compila, el
        // motor dibuja sin sombras en vez de no dibujar nada.
        try {
            shaderSombra_ = gestorShaders_->obtener("profundidad");
        } catch (const std::exception& e) {
            registrarExcepcionDiagnostica("Rendering/MeshRenderer", e,
                                          NivelDiagnostico::Advertencia);
            std::cerr << "[MeshRenderer] Programa de profundidad no disponible: "
                      << e.what() << '\n';
            shaderSombra_.reset();
        }
        aplicarLuces();
        // Los mapas de sombra tambien se suben aqui: el programa puede
        // aparecer despues de que la escena ya tuviera proyectores (el
        // renderer de la escena prepara las sombras antes del primer dibujado).
        aplicarSombras();
    } catch (const std::exception& e) {
        registrarExcepcionDiagnostica("Rendering/MeshRenderer", e,
                                      NivelDiagnostico::Error);
        // Sin shader no hay pipeline: available() queda en false y la pasada
        // dibuja menos, sin objeto que avise uno por uno. El error va al log
        // porque es la causa raiz de una escena vacia.
        std::cerr << "[MeshRenderer] Shader por defecto no disponible: "
                  << e.what() << '\n';
        shader_.reset();
        falloShader_ = true;
    }
    return shader_ != nullptr;
}

void MeshRenderer::clearCache() {
    gpu_.clear();
    gpuTexturas_.clear();
    texturasFallidas_.clear();
    mallasSinNormales_.clear();
}

void MeshRenderer::setLuces(const LightData* luces, int lucesCount,
                            const float* globalAmbient) {
    // El ambiente global puede no venir (el contexto de frame lo deja en
    // null por defecto): leerlo sin comprobar seria desreferenciar un
    // puntero nulo. Sin ambiente se usa el mismo valor por defecto que
    // declara el sistema de luces.
    static const float kAmbientePorDefecto[3] = {0.15f, 0.15f, 0.15f};
    const float* ambiente = globalAmbient ? globalAmbient : kAmbientePorDefecto;
    globalAmbient_[0] = ambiente[0];
    globalAmbient_[1] = ambiente[1];
    globalAmbient_[2] = ambiente[2];

    luces_.clear();
    if (luces && lucesCount > 0)
        luces_.assign(luces, luces + lucesCount);

    if (shader_) aplicarLuces();
}

void MeshRenderer::aplicarLuces() {
    // Los seters de ShaderProgram no hacen glUseProgram: sin este use() los
    // uniforms de luz se descartarian (no hay programa activo al venir desde
    // setLuces/prepararLucesFrame antes de dibujar).
    if (shader_) shader_->use();
    const int count =
        std::min(LightSystem::kMaxLights, static_cast<int>(luces_.size()));

    shader_->setInt("uLightCount", count);
    shader_->setVec3("uGlobalAmbient",
                     glm::vec3(globalAmbient_[0], globalAmbient_[1],
                               globalAmbient_[2]));

    for (int i = 0; i < count; ++i) {
        const LightData& l = luces_[static_cast<std::size_t>(i)];
        char name[32];

        std::snprintf(name, sizeof(name), "uLightTypes[%d]", i);
        shader_->setInt(name, l.type);
        std::snprintf(name, sizeof(name), "uLightWorldPos[%d]", i);
        shader_->setVec3(name, glm::vec3(l.worldPos[0], l.worldPos[1],
                                         l.worldPos[2]));
        std::snprintf(name, sizeof(name), "uLightDir[%d]", i);
        shader_->setVec3(name, glm::vec3(l.direction[0], l.direction[1],
                                         l.direction[2]));
        std::snprintf(name, sizeof(name), "uLightAmbient[%d]", i);
        shader_->setVec3(name, glm::vec3(l.ambient[0], l.ambient[1],
                                         l.ambient[2]));
        std::snprintf(name, sizeof(name), "uLightDiffuse[%d]", i);
        shader_->setVec3(name, glm::vec3(l.diffuse[0], l.diffuse[1],
                                         l.diffuse[2]));
        std::snprintf(name, sizeof(name), "uLightSpecular[%d]", i);
        shader_->setVec3(name, glm::vec3(l.specular[0], l.specular[1],
                                         l.specular[2]));
        std::snprintf(name, sizeof(name), "uLightConstant[%d]", i);
        shader_->setFloat(name, l.constant);
        std::snprintf(name, sizeof(name), "uLightLinear[%d]", i);
        shader_->setFloat(name, l.linear);
        std::snprintf(name, sizeof(name), "uLightQuadratic[%d]", i);
        shader_->setFloat(name, l.quadratic);
        std::snprintf(name, sizeof(name), "uSpotCutoffCos[%d]", i);
        shader_->setFloat(
            name, std::cos(l.spotCutoffDegrees * 3.14159265358979f / 180.f));
    }
}

void MeshRenderer::setSombras(const Rendering::ProyectorSombra* proyectores,
                              int cantidad) {
    sombras_.clear();
    if (proyectores && cantidad > 0) {
        for (int i = 0; i < cantidad; ++i) {
            const Rendering::ProyectorSombra& proyector = proyectores[i];
            // Un proyector sin mapa (el backend no pudo crearlo) no sombrea:
            // dejarlo en la lista haria que el shader consultara una unidad de
            // textura sin mapa, con lo que la sombra saldria de otra escena.
            if (proyector.textura == Rendering::Backend::kInvalidHandle) continue;
            sombras_.push_back(proyector);
        }
    }

    if (shader_) aplicarSombras();
}

void MeshRenderer::aplicarSombras() {
    if (!shader_) return;
    // Los uniforms se suben con el programa activo: sin use() se descartan.
    shader_->use();

    const int count =
        std::min(Rendering::kMaxProyectoresSombra,
                 static_cast<int>(sombras_.size()));
    shader_->setInt("uShadowCount", count);

    for (int i = 0; i < count; ++i) {
        const Rendering::ProyectorSombra& proyector =
            sombras_[static_cast<std::size_t>(i)];
        char name[32];

        // El sampler de cada mapa (uShadowMap0..3: GLSL 3.30 no deja indexar un
        // array de samplers con una variable).
        std::snprintf(name, sizeof(name), "uShadowMap%d", i);
        shader_->setInt(name, kUnidadTexturaSombra + i);

        std::snprintf(name, sizeof(name), "uShadowMatrix[%d]", i);
        shader_->setMat4(name, proyector.matrizLuz);
        std::snprintf(name, sizeof(name), "uShadowBias[%d]", i);
        shader_->setFloat(name, proyector.sesgo);
        std::snprintf(name, sizeof(name), "uShadowStrength[%d]", i);
        shader_->setFloat(name, proyector.fuerza);
        std::snprintf(name, sizeof(name), "uShadowTexel[%d]", i);
        shader_->setFloat(name,
                          1.f / static_cast<float>(proyector.resolucion > 0
                                                       ? proyector.resolucion
                                                       : 1));
        std::snprintf(name, sizeof(name), "uShadowLightIndex[%d]", i);
        shader_->setInt(name, proyector.indiceLuz);
    }

    // Luces puntuales (cubemap): subir las 6 matrices por luz, la distancia
    // de alcance y vincular el sampler de cubemap a su unidad de textura.
    for (int i = 0; i < count; ++i) {
        const Rendering::ProyectorSombra& proyector =
            sombras_[static_cast<std::size_t>(i)];
        if (!proyector.esCubemap) continue;
        char name[48];
        const int base = proyector.indiceLuz * Rendering::kCarasCubemap;
        for (int cara = 0; cara < Rendering::kCarasCubemap; ++cara) {
            std::snprintf(name, sizeof(name), "uShadowMatrixCubo[%d]", base + cara);
            shader_->setMat4(name, proyector.matricesLuz[cara]);
        }
        std::snprintf(name, sizeof(name), "uShadowDistancia[%d]", proyector.indiceLuz);
        shader_->setFloat(name, proyector.distancia);
        std::snprintf(name, sizeof(name), "uShadowCube%d", proyector.indiceLuz);
        shader_->setInt(name, kUnidadTexturaSombra + Rendering::kMaxProyectoresSombra + proyector.indiceLuz);
    }
}

void MeshRenderer::aplicarMaterial(GameObject* objeto, const IMaterial* material) {
    if (material) {
        shader_->setVec4("uMaterialAmbient", material->getAmbient());
        shader_->setVec4("uMaterialDiffuse", material->getDiffuse());
        shader_->setVec4("uMaterialSpecular", material->getSpecular());
        shader_->setVec4("uMaterialEmission", material->getEmission());
        shader_->setFloat("uMaterialShininess", material->getShininess());
        return;
    }

    if (objeto) {
        if (Color* color = objeto->getComponent<Color>()) {
            // El pipeline inmediato solo seteaba GL_DIFFUSE al color; el resto
            // quedaba con el default GL (ambiente gris tenue, sin especular).
            const float* c = color->getColor();
            const float ambient[4] = {0.2f, 0.2f, 0.2f, 1.f};
            const float specular[4] = {0.f, 0.f, 0.f, 1.f};
            const float emission[4] = {0.f, 0.f, 0.f, 1.f};
            shader_->setVec4("uMaterialAmbient", ambient);
            shader_->setVec4("uMaterialDiffuse", c);
            shader_->setVec4("uMaterialSpecular", specular);
            shader_->setVec4("uMaterialEmission", emission);
            shader_->setFloat("uMaterialShininess", 0.f);
            return;
        }
    }

    // Sin material ni color: defaults del componente Material.
    const Material defaults;
    shader_->setVec4("uMaterialAmbient", defaults.getAmbient());
    shader_->setVec4("uMaterialDiffuse", defaults.getDiffuse());
    shader_->setVec4("uMaterialSpecular", defaults.getSpecular());
    shader_->setVec4("uMaterialEmission", defaults.getEmission());
    shader_->setFloat("uMaterialShininess", defaults.getShininess());
}

bool MeshRenderer::enlazarSlotTextura(const std::string& path, int unit,
                                      const char* samplerUniform) {
    if (path.empty() || texturasFallidas_.count(path) != 0) return false;
    try {
        std::shared_ptr<const Image> image = textureManager_->getTexture(path);
        auto it = gpuTexturas_.find(image.get());
        if (it == gpuTexturas_.end()) {
            if (!backend_) return false;
            auto gpu = std::make_unique<TextureGL>(*backend_);
            gpu->upload(*image);
            it = gpuTexturas_.emplace(image.get(), std::move(gpu)).first;
        }
        it->second->bindUnit(unit);
        shader_->setInt(samplerUniform, unit);
        return true;
    } catch (const std::exception& e) {
        texturasFallidas_.insert(path); // no reintentar por frame
        registrarExcepcionDiagnostica("Rendering/MeshRenderer", e,
                                      NivelDiagnostico::Advertencia);
        std::cerr << "[MeshRenderer] Textura '" << path << "': " << e.what()
                  << '\n';
        return false;
    }
}

void MeshRenderer::aplicarTexturas(const IMaterial* material, const Mesh* mesh) {
    // Por defecto: sin texturas en ningun slot.
    shader_->setInt("uUseTexture", 0);
    shader_->setInt("uUseSpecularMap", 0);
    shader_->setInt("uUseEmissionMap", 0);
    shader_->setInt("uUseNormalMap", 0);

    if (!textureManager_ || !mesh || !mesh->hasUvs()) {
        return;
    }
    if (!material) return;

    shader_->setInt("uUseTexture",
                    enlazarSlotTextura(material->getDiffuseMapPath(), 0,
                                       "uDiffuseTex")
                        ? 1
                        : 0);
    shader_->setInt("uUseSpecularMap",
                    enlazarSlotTextura(material->getSpecularMapPath(), 1,
                                       "uSpecularTex")
                        ? 1
                        : 0);
    shader_->setInt("uUseEmissionMap",
                    enlazarSlotTextura(material->getEmissionMapPath(), 2,
                                       "uEmissionTex")
                        ? 1
                        : 0);

    // El normal map ademas exige el marco tangente de la malla.
    shader_->setInt("uUseNormalMap",
                    mesh->hasTangents()
                        ? (enlazarSlotTextura(material->getNormalMapPath(), 3,
                                              "uNormalTex")
                               ? 1
                               : 0)
                        : 0);
}

bool MeshRenderer::intentarRender(Modelos3D* objeto, const float view[16],
                                  const float projection[16],
                                  float deltaTime) {
    if (!objeto) return false;

    // El componente Model (path) manda sobre el path actual. Se toma el
    // puntero a la malla DESPUES de este paso, porque setPath() puede
    // reemplazar la malla compartida (y el AssetManager eviccionar la
    // anterior).
    if (Model* model = objeto->getComponent<Model>();
        model && model->getPath() != objeto->getPath())
        objeto->setPath(model->getPath());

    Transform* transform = objeto->getGlobalTransform();
    if (!transform) return false;

    float modelo[16];
    buildMatrixFromTransform(transform, modelo);
    return intentarRenderMesh(objeto, objeto->getMesh(), modelo, view,
                              projection, deltaTime);
}

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

    // Deja la matriz normal como transpose(inverse(model)) pero identidad si la
    // inversa degenero (fracciones no finitas); evita NaN en el shader.
    glm::mat3 normalMatrix =
        glm::mat3(glm::transpose(glm::inverse(model)));
    if (!matrizFinita(normalMatrix)) normalMatrix = glm::mat3(1.0f);

    shader_->use();
    shader_->setMat4("uModel", model);
    shader_->setMat4("uView", glm::make_mat4(view));
    shader_->setMat4("uProjection", glm::make_mat4(projection));
    shader_->setMat3("uNormalMatrix", normalMatrix);

    const glm::mat4 invView = glm::inverse(glm::make_mat4(view));
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
    if (!shader_) return;
    if (!backend_) return;
    auto& backend = *backend_;
    const int count =
        std::min(Rendering::kMaxProyectoresSombra,
                 static_cast<int>(sombras_.size()));
    for (int i = 0; i < count; ++i) {
        const Rendering::Backend::Handle textura =
            sombras_[static_cast<std::size_t>(i)].textura;
        if (textura == Rendering::Backend::kInvalidHandle) continue;
        if (sombras_[static_cast<std::size_t>(i)].esCubemap) {
            backend.bindTextureCube(textura,
                kUnidadTexturaSombra + Rendering::kMaxProyectoresSombra +
                sombras_[static_cast<std::size_t>(i)].indiceLuz);
        } else {
            backend.bindTexture2D(textura, kUnidadTexturaSombra + i);
        }
    }
}

bool MeshRenderer::intentarRenderSombra(const Mesh* mesh, const float modelo[16],
                                        const glm::mat4& matrizLuz) {
    if (!mesh || mesh->isEmpty()) return false;
    if (!inicializar()) return false;
    // Sin programa de profundidad no hay pase de sombras, pero el resto de la
    // escena se dibuja igual.
    if (!shaderSombra_) return false;

    const glm::mat4 model = glm::make_mat4(modelo);
    if (!matrizFinita(glm::mat3(model))) return false;

    const MeshGPU* gpu = obtenerGpu(mesh);
    if (!gpu) return false;

    shaderSombra_->use();
    shaderSombra_->setMat4("uModel", model);
    shaderSombra_->setMat4("uLightMatrix", matrizLuz);

    // El pase de profundidad no escribe color: el blend se apaga.
    if (!backend_) return false;
    auto& backend = *backend_;
    backend.setBlendEnabled(false);
    gpu->draw();
    ShaderProgram::unbind(backend);
    return true;
}
