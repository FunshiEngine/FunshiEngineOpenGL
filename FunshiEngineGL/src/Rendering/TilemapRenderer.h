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
#ifndef TILEMAPRENDERER_H
#define TILEMAPRENDERER_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Backend/IRenderBackend.h"

class ShaderProgram;

namespace Rendering {
class GestorShaders;
}  // namespace Rendering
class TextureGL;
class TextureManager;
struct Mesh;

namespace Rendering {

// Datos planos de un tilemap a dibujar. El renderer no conoce el componente
// Tilemap (capa de entidades): recibe la malla ya horneada en CPU, su revision
// y la imagen del atlas, para que un modulo no acople al otro.
struct TilemapDraw {
    const void* id = nullptr;           // identidad del tilemap (clave de cache)
    std::uint64_t revision = 0;         // cambia al editar tiles/capas
    const Mesh* malla = nullptr;        // geometria horneada en CPU
    const char* rutaTextura = nullptr;  // imagen del atlas (puede ser null)
    const float* model = nullptr;       // matriz mundo ya calculada (col-major)
    const float* view = nullptr;
    const float* projection = nullptr;
    const float* tint = nullptr;        // RGBA
};

// Pasada de tilemaps 2D: sube una malla horneada por tilemap (un draw call por
// atlas) y la reutiliza hasta que cambia su revision, igual que MeshRenderer
// cachea sus sub-mallas. Usa el mismo shader y pipeline de sprites (blend on,
// depth test on, depth mask off) para que el orden de dibujo decida el
// solapamiento. Las texturas se comparten por identidad via TextureManager.
class TilemapRenderer {
public:
    TilemapRenderer();
    ~TilemapRenderer();

    TilemapRenderer(const TilemapRenderer&) = delete;
    TilemapRenderer& operator=(const TilemapRenderer&) = delete;

    void setTextureManager(TextureManager* textureManager) noexcept {
        textureManager_ = textureManager;
    }

    // Backend de render: con el se crean y destruyen los recursos GPU.
    void setBackend(Rendering::Backend::IRenderBackend& backend) noexcept {
        backend_ = &backend;
    }

    // Gestor de shaders del motor (dueno de los programas): sin el, el
    // renderer no tiene programa y no dibuja.
    void setGestorShaders(Rendering::GestorShaders* gestorShaders) noexcept {
        gestorShaders_ = gestorShaders;
    }

    // Dibuja un tilemap. Devuelve false si no dibujo (pipeline no disponible,
    // malla vacia o datos incompletos); un fallo de textura no es fatal: cae a
    // color plano (tinte).
    bool dibujar(const TilemapDraw& draw);

    // Libera las mallas subidas, el shader y las texturas cacheadas.
    void destruir();

private:
    bool inicializar();
    bool enlazarTextura(const std::string& path);
    // Sube la malla a GPU si el tilemap es nuevo o su revision cambio; devuelve
    // el handle y el numero de indices a dibujar.
    bool asegurarMalla(const void* id, const Mesh& malla, std::uint64_t revision,
                       Backend::Handle& outMalla, int& outIndices);

    struct MallaCacheada {
        Backend::Handle malla = Backend::kInvalidHandle;
        std::uint64_t revision = 0;
        int indices = 0;
    };

    std::shared_ptr<ShaderProgram> shader_;
    Rendering::GestorShaders* gestorShaders_ = nullptr;
    // Backend de render inyectado (crea y destruye los recursos GPU).
    Rendering::Backend::IRenderBackend* backend_ = nullptr;
    bool shaderFallado_ = false;
    TextureManager* textureManager_ = nullptr;
    std::unordered_map<const void*, MallaCacheada> mallas_;
    std::unordered_map<const void*, std::unique_ptr<TextureGL>> gpuTexturas_;
    std::unordered_set<std::string> texturasFallidas_;
};

} // namespace Rendering

#endif
