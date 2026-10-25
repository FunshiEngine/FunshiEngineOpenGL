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
#ifndef SPRITERENDERER_H
#define SPRITERENDERER_H

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

namespace Rendering {

// Datos planos de un sprite a dibujar. El renderer no conoce el componente
// SpriteRenderer (capa de entidades): recibe solo las matrices y los valores ya
// resueltos, de modo que un modulo no acopla al otro (comunicacion por datos).
struct SpriteDraw {
    const float* model = nullptr;       // matriz mundo ya calculada (col-major)
    const float* view = nullptr;
    const float* projection = nullptr;
    const char* rutaTextura = nullptr;  // puede ser null o vacio: color plano
    const float* tint = nullptr;        // RGBA
    const float* tamano = nullptr;      // ancho, alto en unidades de mundo
    const float* uvRect = nullptr;      // u0, v0, u1, v1
};

// Pasada de sprites 2D: un quad texturizado sin iluminacion, dibujado con el
// pipeline moderno sobre el mismo backend que el resto del engine (no agrega
// metodos a IRenderBackend: sube la malla con createMesh y la dibuja con
// drawMesh). La malla y el shader se crean una sola vez y se reutilizan; las
// texturas (path -> Image compartida via TextureManager) se suben a GPU una vez
// por imagen y se cachean por identidad, igual que MeshRenderer. Se dibuja con
// blend activo, depth test on y depth mask off, en orden de Z creciente
// (atras->adelante): asi un sprite no escribe profundidad y el orden de dibujo
// decide el solapamiento entre sprites.
class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer();

    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    // Manager de imagenes compartidas (flyweight). Sin el, los sprites sin
    // textura se dibujan como color plano (tinte).
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

    // Dibuja un sprite. Devuelve false si no dibujo (pipeline no disponible o
    // datos incompletos); un fallo de textura no es fatal: cae a color plano.
    bool dibujar(const SpriteDraw& sprite);

    // Libera la malla, el shader y las texturas cacheadas (al apagar el engine).
    void destruir();

private:
    bool inicializar();
    bool enlazarTextura(const std::string& path);

    std::shared_ptr<ShaderProgram> shader_;
    Rendering::GestorShaders* gestorShaders_ = nullptr;
    // Backend de render inyectado (crea y destruye los recursos GPU).
    Rendering::Backend::IRenderBackend* backend_ = nullptr;
    bool shaderFallado_ = false;
    Rendering::Backend::Handle quadMalla_ = Rendering::Backend::kInvalidHandle;
    TextureManager* textureManager_ = nullptr;
    std::unordered_map<const void*, std::unique_ptr<TextureGL>> gpuTexturas_;
    std::unordered_set<std::string> texturasFallidas_;
};

} // namespace Rendering

#endif
