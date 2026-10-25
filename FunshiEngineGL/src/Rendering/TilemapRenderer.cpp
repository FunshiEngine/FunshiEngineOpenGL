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
#include "TilemapRenderer.h"
#include "../Diagnosticos/RegistroDiagnosticos.h"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "../Assets/Image.h"
#include "../Assets/Mesh.h"
#include "../Assets/TextureManager.h"
#include "Shaders/ShaderProgram.h"
#include "Shaders/GestorShaders.h"
#include "TextureGL.h"

namespace Rendering {

TilemapRenderer::TilemapRenderer() = default;

TilemapRenderer::~TilemapRenderer() = default;

void TilemapRenderer::destruir() {
    if (!backend_) return;
    auto& backend = *backend_;
    for (auto& entrada : mallas_) {
        if (entrada.second.malla != Backend::kInvalidHandle)
            backend.destroyMesh(entrada.second.malla);
    }
    mallas_.clear();
    gpuTexturas_.clear();
    texturasFallidas_.clear();
    shader_.reset();
}

bool TilemapRenderer::inicializar() {
    if (shader_) return true;
    if (shaderFallado_) return false;
    if (!backend_) return false;
    auto& backend = *backend_;
    if (!backend.init()) return false;
    // Sin gestor de shaders no hay de donde pedir el programa.
    if (!gestorShaders_) return false;

    try {
        // Comparte programa con el renderer de sprites: es el mismo GLSL.
        shader_ = gestorShaders_->obtener("sprite");
    } catch (const std::exception& e) {
        registrarExcepcionDiagnostica("Rendering/TilemapRenderer", e,
                                      NivelDiagnostico::Error);
        std::cerr << "[TilemapRenderer] Shader de tiles no disponible: "
                  << e.what() << '\n';
        shaderFallado_ = true;
        shader_.reset();
        return false;
    }
    return true;
}

bool TilemapRenderer::enlazarTextura(const std::string& path) {
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
        texturasFallidas_.insert(path);
        registrarExcepcionDiagnostica("Rendering/TilemapRenderer", e,
                                      NivelDiagnostico::Advertencia);
        std::cerr << "[TilemapRenderer] Textura '" << path << "': " << e.what()
                  << '\n';
        return false;
    }
}

bool TilemapRenderer::asegurarMalla(const void* id, const Mesh& malla,
                                    std::uint64_t revision,
                                    Backend::Handle& outMalla,
                                    int& outIndices) {
    auto it = mallas_.find(id);
    if (it != mallas_.end() && it->second.revision == revision) {
        outMalla = it->second.malla;
        outIndices = it->second.indices;
        return outMalla != Backend::kInvalidHandle && outIndices > 0;
    }

    if (!backend_) return false;
    auto& backend = *backend_;
    if (it != mallas_.end()) {
        if (it->second.malla != Backend::kInvalidHandle)
            backend.destroyMesh(it->second.malla);
        mallas_.erase(it);
    }
    if (malla.vertices.empty() || malla.indices.empty()) {
        MallaCacheada entrada;
        entrada.revision = revision;
        mallas_[id] = entrada;
        outMalla = Backend::kInvalidHandle;
        outIndices = 0;
        return false;
    }

    Backend::MeshData data;
    data.vertices = &malla.vertices[0].x;
    data.vertexCount = malla.vertices.size();
    data.uvs = malla.uvs.empty() ? nullptr : &malla.uvs[0].x;
    data.uvCount = malla.uvs.size();
    data.indices = malla.indices.data();
    data.indexCount = malla.indices.size();

    MallaCacheada entrada;
    entrada.malla = backend.createMesh(data);
    entrada.revision = revision;
    entrada.indices = static_cast<int>(malla.indices.size());
    outMalla = entrada.malla;
    outIndices = entrada.indices;
    mallas_[id] = entrada;
    return outMalla != Backend::kInvalidHandle && outIndices > 0;
}

bool TilemapRenderer::dibujar(const TilemapDraw& draw) {
    if (!draw.model || !draw.view || !draw.projection || !draw.malla)
        return false;
    if (draw.malla->isEmpty()) return false;
    if (!inicializar()) return false;

    Backend::Handle malla = Backend::kInvalidHandle;
    int indices = 0;
    if (!asegurarMalla(draw.id, *draw.malla, draw.revision, malla, indices))
        return false;

    const bool conTextura =
        draw.rutaTextura && draw.rutaTextura[0] != '\0' &&
        enlazarTextura(draw.rutaTextura);

    shader_->use();
    shader_->setMat4("uModel", glm::make_mat4(draw.model));
    shader_->setMat4("uView", glm::make_mat4(draw.view));
    shader_->setMat4("uProjection", glm::make_mat4(draw.projection));

    // La geometria del tilemap ya trae los UV horneados por vertice; el shader
    // de sprites los multiplica por uUvRect, asi que se fija la region completa.
    const float uvCompleto[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    const float tinteBlanco[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    shader_->setVec4("uUvRect", uvCompleto);
    shader_->setVec4("uTint", draw.tint ? draw.tint : tinteBlanco);
    shader_->setInt("uUseTexture", conTextura ? 1 : 0);

    if (!backend_) return false;
    auto& backend = *backend_;
    backend.setBlendEnabled(true);
    backend.setDepthMask(false);
    backend.drawMesh(malla, 0, indices);
    backend.setDepthMask(true);
    backend.setBlendEnabled(false);
    ShaderProgram::unbind(backend);
    return true;
}

} // namespace Rendering
