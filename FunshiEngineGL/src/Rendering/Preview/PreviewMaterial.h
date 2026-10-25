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
#ifndef PREVIEWMATERIAL_H
#define PREVIEWMATERIAL_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "../../Assets/Mesh.h"
#include "../MeshRenderer.h"
#include "../RenderTarget.h"
#include "IPreviewMaterial.h"

class TextureManager;

namespace Rendering {
class GestorShaders;
}  // namespace Rendering

// Vista previa de un material: dibuja una esfera de muestra con el material
// que se esta editando en un render target propio y expone su textura.
//
// Cada material editado tiene su propia vista (una clave la identifica), de
// modo que un panel con varias regiones puede mostrar varias vistas a la vez.
//
// Usa un MeshRenderer propio, con sus propias luces fijas: asi la vista
// previa no depende de las luces de la escena y no las altera (la escena las
// vuelve a fijar en su pasada).
class PreviewMaterial : public IPreviewMaterial {
public:
    // Lado de la textura de la vista previa (cuadrada).
    static constexpr int kLado = 128;
    // Vistas vivas como maximo: cada una tiene su render target, asi que se
    // sueltan las que sobran (se vuelven a crear bajo demanda).
    static constexpr std::size_t kMaxVistas = 16;

    // gestorShaders y texturas pueden ser nullptr: sin gestor no hay programa
    // (la vista previa no dibuja) y sin gestor de imagenes los materiales con
    // textura se ven sin ella.
    PreviewMaterial(Rendering::Backend::IRenderBackend& backend,
                    Rendering::GestorShaders* gestorShaders,
                    TextureManager* texturas);
    ~PreviewMaterial() override;

    bool disponible() const override;
    int lado() const override { return kLado; }
    const void* descriptorTextura(const std::string& clave) const override;
    bool renderizar(const std::string& clave, const IMaterial& material) override;
    bool sucia(const std::string& clave) const override;
    void invalidar(const std::string& clave) override;

private:
    struct Vista {
        // Render target de la vista (con su textura de color).
        std::unique_ptr<RenderTarget> target;
        // La vista hay que redibujarla (la primera vez, y tras cada cambio
        // del material).
        bool sucia = true;
    };

    Rendering::Backend::IRenderBackend* backend_ = nullptr;
    // Renderer propio de la vista previa (luces fijas, cache propia).
    MeshRenderer renderer_;
    // Esfera de muestra (geometria pura, en CPU).
    Mesh esfera_;
    // Vistas por clave (un material editado por vista).
    std::unordered_map<std::string, Vista> vistas_;
    // Camara de producto: enquadra la esfera desde el frente.
    glm::mat4 vista_;
    glm::mat4 proyeccion_;

    // Devuelve la vista de la clave, creandola si no existe.
    Vista* obtenerVista(const std::string& clave);
    // Luces fijas de la vista previa (clave + relleno).
    void configurarLuces();
};

#endif // PREVIEWMATERIAL_H
