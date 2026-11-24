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
            shaderSombraCubo_ = gestorShaders_->obtener("profundidad_cubo");
        } catch (const std::exception& e) {
            registrarExcepcionDiagnostica("Rendering/MeshRenderer", e,
                                          NivelDiagnostico::Advertencia);
            std::cerr << "[MeshRenderer] Programa de profundidad no disponible: "
                      << e.what() << '\n';
            shaderSombra_.reset();
            shaderSombraCubo_.reset();
        }
        configurarSamplers();
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

void MeshRenderer::configurarSamplers() {
    if (!shader_) return;
    shader_->use();

    // 1. Texturas 2D del material (unidades 0..3)
    shader_->setInt("uDiffuseTex", kUnidadTexturaDifusa);
    shader_->setInt("uSpecularTex", kUnidadTexturaEspecular);
    shader_->setInt("uEmissionTex", kUnidadTexturaEmision);
    shader_->setInt("uNormalTex", kUnidadTexturaNormal);

    // Mapas de sombra (unidades 4..11).
    Rendering::UniformsSombras::configurarSamplers(*shader_);
}

void MeshRenderer::aplicarSombras() {
    if (!shader_) return;
    Rendering::UniformsSombras::subir(*shader_, sombras_);
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
                    enlazarSlotTextura(material->getDiffuseMapPath(),
                                       kUnidadTexturaDifusa, "uDiffuseTex")
                        ? 1
                        : 0);
    shader_->setInt("uUseSpecularMap",
                    enlazarSlotTextura(material->getSpecularMapPath(),
                                       kUnidadTexturaEspecular, "uSpecularTex")
                        ? 1
                        : 0);
    shader_->setInt("uUseEmissionMap",
                    enlazarSlotTextura(material->getEmissionMapPath(),
                                       kUnidadTexturaEmision, "uEmissionTex")
                        ? 1
                        : 0);

    // El normal map ademas exige el marco tangente de la malla.
    shader_->setInt("uUseNormalMap",
                    mesh->hasTangents()
                        ? (enlazarSlotTextura(material->getNormalMapPath(),
                                              kUnidadTexturaNormal,
                                              "uNormalTex")
                               ? 1
                               : 0)
                        : 0);
}
