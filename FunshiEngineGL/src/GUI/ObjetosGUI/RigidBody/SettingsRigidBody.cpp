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
#include "SettingsRigidBody.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../../../Scenes/Editor/EditorController.h"
#include <imgui.h>
#include "../../../Localizacion/Localizacion.h"

using Localizacion::Texto;

SettingsRigidBody::SettingsRigidBody(GameObject* objeto)
    : SettingsComponent(demangle(typeid(RigidBody).name())),
      myCollider(objeto ? objeto->getComponent<RigidBody>() : nullptr) {}

void SettingsRigidBody::setEditor(EditorController* editor) {
    this->editor = editor;
}

void SettingsRigidBody::showDataComponent() {
    bool activo = myCollider->estaActivo();
    if (ImGui::Checkbox(Localizacion::etiqueta(Texto::Activo).c_str(), &activo)) myCollider->setActivo(activo);
    bool sensor = myCollider->esSensor();
    if (ImGui::Checkbox(Localizacion::etiqueta(Texto::SensorTrigger).c_str(), &sensor))
        myCollider->fijarSensor(sensor);
    float masa = myCollider->masa();
    if (ImGui::DragFloat(Localizacion::etiqueta(Texto::Masa).c_str(), &masa, 0.1f, 0.0f, 100.0f)) {
        const bool eraEstatico = myCollider->masa() == 0.0f;
        if (myCollider->fijarMasa(masa) && editor &&
            myCollider->getCollider() &&
            myCollider->getCollider()->getOwner() &&
            (eraEstatico != (masa == 0.0f))) {
            // Cruzar entre estatico y dinamico cambia la naturaleza del
            // cuerpo en Bullet: reconstruirlo via el editor lo re-registra.
            editor->refreshRigidBody(
                myCollider->getCollider()->getOwner());
        }
    }
    bool usaGravedad = myCollider->usaGravedad();
    if (ImGui::Checkbox(Localizacion::etiqueta(Texto::UsaGravedad).c_str(), &usaGravedad))
        myCollider->fijarUsoGravedad(usaGravedad);
    float escala = myCollider->escalaGravedad();
    if (ImGui::DragFloat(Localizacion::etiqueta(Texto::EscalaGravedad).c_str(), &escala, 0.05f, -5.0f, 5.0f))
        myCollider->fijarEscalaGravedad(escala);
    float friccion = myCollider->friccion();
    if (ImGui::DragFloat(Localizacion::etiqueta(Texto::Friccion).c_str(), &friccion, 0.05f, 0.0f, 10.0f))
        myCollider->fijarFriccion(friccion);
    if (ImGui::TreeNode(Localizacion::etiqueta(Texto::BloquearPosicion).c_str())) {
        bool x = myCollider->posicionCongelada(0);
        bool y = myCollider->posicionCongelada(1);
        bool z = myCollider->posicionCongelada(2);
        bool cambio = false;
        cambio |= ImGui::Checkbox("X", &x);
        ImGui::SameLine();
        cambio |= ImGui::Checkbox("Y", &y);
        ImGui::SameLine();
        cambio |= ImGui::Checkbox("Z", &z);
        if (cambio) myCollider->fijarFreezePosicion(x, y, z);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode(Localizacion::etiqueta(Texto::BloquearRotacion).c_str())) {
        bool x = myCollider->rotacionCongelada(0);
        bool y = myCollider->rotacionCongelada(1);
        bool z = myCollider->rotacionCongelada(2);
        bool cambio = false;
        cambio |= ImGui::Checkbox("X##rot", &x);
        ImGui::SameLine();
        cambio |= ImGui::Checkbox("Y##rot", &y);
        ImGui::SameLine();
        cambio |= ImGui::Checkbox("Z##rot", &z);
        if (cambio) myCollider->fijarFreezeRotacion(x, y, z);
        ImGui::TreePop();
    }
}