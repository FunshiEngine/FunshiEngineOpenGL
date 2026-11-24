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
#include "SettingsColliderTilemap.h"

#include "../Transform/SettingsTransform.h"
#include "../../../Objetos/GameObject.h"
#include <imgui.h>

SettingsColliderTilemap::SettingsColliderTilemap(GameObject* objeto)
    : SettingsComponent(demangle(typeid(TilemapCollider).name())),
      myCollider(objeto ? objeto->getComponent<TilemapCollider>() : nullptr) {
    this->settingsTransform =
        new SettingsTransform(myCollider ? myCollider->getTransform() : nullptr,
                              objeto);
}

SettingsColliderTilemap::~SettingsColliderTilemap() { delete settingsTransform; }

void SettingsColliderTilemap::setEditor(EditorController* editor) {
    this->editor = editor;
}

void SettingsColliderTilemap::showDataComponent() {
    if (!myCollider) return;
    bool visible = myCollider->estaVisibleEnEscena();
    if (ImGui::Checkbox("Visible en escena", &visible))
        myCollider->setVisibleEnEscena(visible);
    ImGui::TextDisabled("La forma se arma con los tiles solidos de las capas");
    ImGui::TextDisabled("con colision del Tilemap del objeto.");
    if (ImGui::Button("Recargar colision")) {
        // La shape se cachea: al pintar tiles hay que reconstruirla desde los
        // tiles actuales. refreshRigidBody invalida la shape, saca el cuerpo
        // viejo del mundo, lo recrea y lo re-registra.
        if (editor && myCollider->getOwner())
            editor->refreshRigidBody(myCollider->getOwner());
        else
            myCollider->invalidateCollisionShape();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Reconstruye las cajas desde los tiles solidos actuales.");
    settingsTransform->showDataComponent();
}
