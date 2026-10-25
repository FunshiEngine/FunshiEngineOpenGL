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
#ifndef UIRENDERER_H
#define UIRENDERER_H

#include <map>
#include <string>

#include <imgui.h>

#include "Backend/IRenderBackend.h"
#include "../UI/LienzoUI.h"
#include "../UI/SistemaUI.h"

class TextureManager;

// Dibuja la interfaz de juego nativa (HUD) con el draw list de ImGui.
//
// Es el unico punto del sistema que sabe de ImGui: recibe el lienzo ya
// colocado (los rectangulos los calculo el modulo puro) y traduce cada widget
// a primitivas de dibujo. Las imagenes se suben a GPU una sola vez por ruta y
// se cachean; el cache de imagenes en CPU es el TextureManager de la escena.
class UIRenderer {
public:
    explicit UIRenderer(Rendering::Backend::IRenderBackend& backend) noexcept;
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    // Cache de imagenes en CPU de la escena (para decodificar los PNG del
    // proyecto). Puede ser nullptr: entonces los widgets de imagen no se
    // dibujan.
    void setTextureManager(TextureManager* manager) noexcept {
        textureManager_ = manager;
    }

    // Dibuja los widgets del lienzo en el draw list dado. El lienzo tiene que
    // tener su layout calculado (LienzoUI::layout) para saber donde cae cada
    // widget. El sistema aporta el estado de interaccion (para resaltar el
    // boton que tiene el raton encima).
    void dibujar(const LienzoUI& lienzo, const SistemaUI& sistema,
                 ImDrawList* dibujo);

private:
    // Sube la imagen del proyecto a GPU (una vez por ruta) y devuelve su
    // descriptor para ImGui. Devuelve ImTextureID_Invalid si no se pudo.
    ImTextureID texturaDe(const std::string& ruta);

    Rendering::Backend::IRenderBackend& backend_;
    TextureManager* textureManager_ = nullptr;
    // Texturas ya subidas, por ruta del proyecto.
    std::map<std::string, Rendering::Backend::Handle> texturas_;
};

#endif // UIRENDERER_H
