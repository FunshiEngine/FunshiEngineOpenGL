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
#include "SettingsInterface.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/InterfaceComponent.h"

#include <imgui.h>
#include <cstring>

SettingsInterface::SettingsInterface(GameObject* gameObject)
    : gameObject(gameObject) {}

void SettingsInterface::showDataComponent() {
    InterfaceComponent* comp = gameObject->getComponent<InterfaceComponent>();
    if (!comp) return;

    ImGui::Text("===Interfaz del jugador===");

    // Nombre del asset (texto editable): debe coincidir con un asset de
    // Memory/Interfaces/ del proyecto. Al entrar en play se muestra a pantalla
    // completa delante de la camara principal.
    char buffer[256] = {};
    std::strncpy(buffer, comp->getInterfaz().c_str(), sizeof(buffer) - 1);
    if (ImGui::InputText("Nombre de interfaz", buffer, sizeof(buffer)))
        comp->setInterfaz(buffer);

    ImGui::TextDisabled("Se muestra en modo play, a pantalla completa");
}

Component* SettingsInterface::getComponent() {
    return gameObject->getComponent<InterfaceComponent>();
}