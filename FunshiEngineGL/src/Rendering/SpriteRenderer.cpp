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
#include "SpriteRenderer.h"
#include "../Diagnosticos/RegistroDiagnosticos.h"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "../Assets/Image.h"
#include "../Assets/Mesh.h"
#include "../Assets/Primitivas2D.h"
#include "../Assets/TextureManager.h"
#include "Shaders/ShaderProgram.h"
#include "Shaders/GestorShaders.h"
#include "TextureGL.h"

namespace Rendering {

SpriteRenderer::SpriteRenderer() = default;

SpriteRenderer::~SpriteRenderer() = default;

void SpriteRenderer::destruir() {
    if (quadMalla_ != Backend::kInvalidHandle) {
        backend_->destroyMesh(quadMalla_);
        quadMalla_ = Backend::kInvalidHandle;
    }
    // Los TextureGL son RAII: al vaciar el mapa se liberan sus handles.
    gpuTexturas_.clear();
    texturasFallidas_.clear();
    shader_.reset();
}

bool SpriteRenderer::inicializar() {
    if (shader_) return true;
    if (shaderFallado_) return false;
    if (!backend_) return false;
    auto& backend = *backend_;
    if (!backend.init()) return false;
    // Sin gestor de shaders no hay de donde pedir el programa.
    if (!gestorShaders_) return false;

    try {
        shader_ = gestorShaders_->obtener("sprite");
    } catch (const std::exception& e) {
        registrarExcepcionDiagnostica("Rendering/SpriteRenderer", e,
                                      NivelDiagnostico::Error);
        std::cerr << "[SpriteRenderer] Shader de sprites no disponible: "
                  << e.what() << '\n';
        shaderFallado_ = true;
        shader_.reset();
        return false;
    }

    const Mesh quad = Primitivas2D::quad();
    Backend::MeshData data;
    data.vertices = &quad.vertices[0].x;
    data.vertexCount = quad.vertices.size();
    // Sin normales ni tangentes: el shader solo usa posicion (atributo 0) y UV
    // (atributo 2); el UV debe ir en su slot para respetar el layout del VAO.
    data.uvs = quad.uvs.empty() ? nullptr : &quad.uvs[0].x;
    data.uvCount = quad.uvs.size();
    data.indices = quad.indices.empty() ? nullptr : quad.indices.data();
    data.indexCount = quad.indices.size();

    quadMalla_ = backend.createMesh(data);
    if (quadMalla_ == Backend::kInvalidHandle) {
        std::cerr << "[SpriteRenderer] No se pudo crear la malla del quad 2D\n";
        shader_.reset();
        return false;
    }
    return true;
}

bool SpriteRenderer::enlazarTextura(const std::string& path) {
    if (path.empty() || texturasFallidas_.count(path) != 0) return false;
    if (!textureManager_) return false;
    try {
        std::shared_ptr<const Image> image = textureManager_->getTexture(path);
        auto it = gpuTexturas_.find(image.get());
        if (it == gpuTexturas_.end()) {
            if (!backend_) return false;
            auto gpu = std::make_unique<TextureGL>(*backend_);
            gpu->upload(*image);
            it = gpuTexturas_.emplace(image.get(), std::move(gpu)).first;
        }
        it->second->bindUnit(0);
        shader_->setInt("uTexture", 0);
        return true;
    } catch (const std::exception& e) {
        texturasFallidas_.insert(path); // no reintentar por frame
        registrarExcepcionDiagnostica("Rendering/SpriteRenderer", e,
                                      NivelDiagnostico::Advertencia);
        std::cerr << "[SpriteRenderer] Textura '" << path << "': " << e.what()
                  << '\n';
        return false;
    }
}

bool SpriteRenderer::dibujar(const SpriteDraw& sprite) {
    if (!sprite.model || !sprite.view || !sprite.projection) return false;
    if (!inicializar()) return false;

    const bool conTextura = sprite.rutaTextura && sprite.rutaTextura[0] != '\0' &&
                            enlazarTextura(sprite.rutaTextura);

    shader_->use();

    glm::mat4 model = glm::make_mat4(sprite.model);
    if (sprite.tamano) {
        // El quad base es 1x1 centrado: el tamano del componente escala las
        // columnas X e Y de la matriz mundo (la escala del Transform ya viene
        // incluida en ella).
        model[0] *= sprite.tamano[0];
        model[1] *= sprite.tamano[0];
        model[2] *= sprite.tamano[0];
        model[4] *= sprite.tamano[1];
        model[5] *= sprite.tamano[1];
        model[6] *= sprite.tamano[1];
    }

    shader_->setMat4("uModel", model);
    shader_->setMat4("uView", glm::make_mat4(sprite.view));
    shader_->setMat4("uProjection", glm::make_mat4(sprite.projection));

    const float uvDefault[4] = {0.f, 0.f, 1.0f, 1.0f};
    const float tinteBlanco[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    shader_->setVec4("uUvRect", sprite.uvRect ? sprite.uvRect : uvDefault);
    shader_->setVec4("uTint", sprite.tint ? sprite.tint : tinteBlanco);
    shader_->setInt("uUseTexture", conTextura ? 1 : 0);

    if (!backend_) return false;
    auto& backend = *backend_;
    backend.setBlendEnabled(true);
    // Depth test on (de applyBaseState) + depth mask off: los sprites no
    // escriben z-buffer, asi el orden de dibujo (Z creciente, atras->adelante)
    // decide el solapamiento y un sprite detras de la geometria se descarta.
    backend.setDepthMask(false);
    backend.drawMesh(quadMalla_, 0, 6);
    backend.setDepthMask(true);
    backend.setBlendEnabled(false);
    ShaderProgram::unbind(backend);
    return true;
}

} // namespace Rendering
