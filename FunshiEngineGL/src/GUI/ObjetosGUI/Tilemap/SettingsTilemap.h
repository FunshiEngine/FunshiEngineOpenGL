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

#include "../../../Objetos/Componentes/Juego2D/Tilemap.h"
#include "../SettingsComponent.h"

class GameObject;

// Inspector del componente Tilemap: asigna el tileset, administra las capas y
// ofrece el pintado basico por celda con una paleta de ids.
class SettingsTilemap : public SettingsComponent {
public:
    explicit SettingsTilemap(GameObject* objeto);

    Component* getComponent() override;
    void showDataComponent() override;

private:
    void dibujarTileset(Tilemap* tilemap);
    void dibujarCapas(Tilemap* tilemap);
    void dibujarPaleta(Tilemap* tilemap);
    void dibujarGrilla(Tilemap* tilemap);

    GameObject* gameObject_ = nullptr;
    int capaActiva_ = 0;
    std::uint32_t pincel_ = 1;
};

#endif
