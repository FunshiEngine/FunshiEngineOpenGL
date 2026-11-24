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
#ifndef SETTINGS_TILEMAP_H
#define SETTINGS_TILEMAP_H

#include <cstdint>
#include <string>

#include "../../../Objetos/Componentes/Juego2D/Tilemap.h"
#include "../../../Rendering/Backend/IRenderBackend.h"
#include "../SettingsComponent.h"

class EditorController;
class GameObject;
class HerramientaTilemap;

// Inspector del componente Tilemap: asigna el tileset, administra las capas y
// elige que se pinta (tile de la paleta, herramienta, capa). El pintado se hace
// clickeando en la escena (HerramientaTilemap), no en el panel.
class SettingsTilemap : public SettingsComponent {
public:
    SettingsTilemap(GameObject* objeto, EditorController* editor);
    ~SettingsTilemap() override;

    Component* getComponent() override;
    void showDataComponent() override;

private:
    void dibujarTileset(Tilemap* tilemap);
    void dibujarCapas(Tilemap* tilemap, HerramientaTilemap* herramienta);
    void dibujarHerramienta(HerramientaTilemap& herramienta);
    void dibujarPaleta(Tilemap* tilemap, HerramientaTilemap* herramienta);
    // Textura del atlas para la paleta (se recarga si cambia la imagen).
    void asegurarAtlas(Tilemap* tilemap);
    void liberarAtlas();

    GameObject* gameObject_ = nullptr;
    EditorController* editor_ = nullptr;
    int capaActiva_ = 0;
    std::uint32_t tileLocal_ = 1;
    int anchoCapaNueva_ = 16;
    int altoCapaNueva_ = 16;
    std::string rutaAtlas_;
    Rendering::Backend::Handle atlas_ = Rendering::Backend::kInvalidHandle;
};

#endif
