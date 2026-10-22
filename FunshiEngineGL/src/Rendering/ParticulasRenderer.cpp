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
#include "ParticulasRenderer.h"

#include "../Assets/TextureManager.h"
#include "Shaders/GestorShaders.h"
#include "Shaders/ShaderProgram.h"
#include "TextureGL.h"

#include <algorithm>
#include <exception>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Rendering {

bool ParticulasRenderer::inicializar() {
    if (shader_) return true;
    if (!backend_ || !gestorShaders_) return false;
    try {
        shader_ = gestorShaders_->obtener("particulas");
    } catch (const std::exception& excepcion) {
        std::cerr << "[particulas] no se pudo obtener el shader: "
                  << excepcion.what() << std::endl;
        shader_.reset();
        return false;
    }
    return true;
}

bool ParticulasRenderer::enlazarTextura(const PerfilParticula* perfil,
                                        const std::string& texturaOverride) {
    const std::string& ruta = !texturaOverride.empty() ? texturaOverride
                                                        : (perfil ? perfil->textura : std::string());
    if (ruta.empty()) return false;
    if (!textureManager_) return false;
    if (std::find(texturasFallidas_.begin(), texturasFallidas_.end(),
                  ruta) != texturasFallidas_.end())
        return false;
    try {
        std::shared_ptr<const Image> imagen = textureManager_->getTexture(ruta);
        const auto it = texturas_.find(imagen.get());
        if (it == texturas_.end()) {
            if (!backend_) return false;
            auto gpu = std::make_unique<TextureGL>(*backend_);
            gpu->upload(*imagen);
            texturas_.emplace(imagen.get(), std::move(gpu));
        }
        texturas_.at(imagen.get())->bindUnit(0);
        shader_->setInt("uTexture", 0);
        return true;
    } catch (const std::exception& excepcion) {
        // La textura que falla no se reintenta cada frame: quedaria el log
        // lleno del mismo error.
        texturasFallidas_.push_back(ruta);
        std::cerr << "[particulas] no se pudo cargar la textura '"
                  << ruta << "': " << excepcion.what() << std::endl;
        return false;
    }
}

void ParticulasRenderer::dibujar(const SistemaParticulas& sistema,
                                 const float view[16], const float projection[16]) {
    if (sistema.particulas().empty()) return;
    if (!inicializar()) return;

    // Agrupar por (perfil, texturaOverride): las particulas de un perfil
    // comparten textura a menos que tengan un override por particula.
    std::map<std::pair<const PerfilParticula*, std::string>,
             std::vector<const Particula*>>
        grupos;
    for (const Particula& particula : sistema.particulas()) {
        grupos[{particula.perfil.get(), particula.texturaOverride}]
            .push_back(&particula);
    }

    const float uvRect[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    for (auto& grupo : grupos) {
        const PerfilParticula* perfil = grupo.first.first;
        const std::string& texturaOverride = grupo.first.second;
        builder_.limpiar();
        for (const Particula* particula : grupo.second)
            builder_.agregarParticula(*particula, uvRect);
        if (builder_.vacio()) continue;

        Lote& lote = lotes_[perfil];
        if (lote.handle == Backend::kInvalidHandle) {
            lote.handle = backend_->createParticulasBatch(
                builder_.vertices(), builder_.cantidadVertices());
            if (lote.handle == Backend::kInvalidHandle) continue;
        } else {
            backend_->updateParticulasBatch(lote.handle, builder_.vertices(),
                                            builder_.cantidadVertices());
        }
        lote.vertices = builder_.cantidadVertices();

        shader_->use();
        shader_->setMat4("uView", glm::make_mat4(view));
        shader_->setMat4("uProjection", glm::make_mat4(projection));
        const bool conTextura = enlazarTextura(perfil, texturaOverride);
        shader_->setInt("uUseTexture", conTextura ? 1 : 0);

        backend_->setBlendEnabled(true);
        backend_->setDepthMask(false);
        backend_->drawParticulasBatch(lote.handle,
                                      static_cast<unsigned int>(lote.vertices));
        backend_->setDepthMask(true);
        backend_->setBlendEnabled(false);
        ShaderProgram::unbind(*backend_);
    }
}

ParticulasRenderer::ParticulasRenderer() = default;

ParticulasRenderer::~ParticulasRenderer() {
    destruir();
}

void ParticulasRenderer::destruir() {
    if (backend_) {
        for (auto& par : lotes_) {
            if (par.second.handle != Backend::kInvalidHandle)
                backend_->destroyParticulasBatch(par.second.handle);
        }
    }
    lotes_.clear();
    texturas_.clear();
    shader_.reset();
}

}  // namespace Rendering
