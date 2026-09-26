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
#include "SettingsGrid.h"

#include "../../../Objetos/Componentes/Grid.h"
#include "../../../Objetos/GameObject.h"

SettingsGrid::SettingsGrid(GameObject* gameObject) : gameObject(gameObject) {}

void SettingsGrid::showDataComponent() {
    Grid* grid = gameObject ? gameObject->getComponent<Grid>() : nullptr;
    if (!grid) return;

    // El checkbox de la grilla vive aca (en sus settings), no mas como entrada
    // fija en la jerarquia.
    bool visible = grid->getVisible();
    if (ImGui::Checkbox("Visible", &visible)) grid->setVisible(visible);
    ImGui::Separator();

    float color[3] = {grid->getColor()[0], grid->getColor()[1], grid->getColor()[2]};
    if (ImGui::ColorEdit3("Color", color)) {
        grid->setColor(color[0], color[1], color[2]);
    }

    // La grilla es infinita con densidad FIJA de lineas (secundarias cada 1
    // unidad, principales cada 5) y difuminado en el horizonte: NO hay
    // tamano ni separacion configurables (se eliminaron por diseño).
    ImGui::TextDisabled("Grilla infinita con densidad fija");
}

Component* SettingsGrid::getComponent() {
    return gameObject ? gameObject->getComponent<Grid>() : nullptr;
}