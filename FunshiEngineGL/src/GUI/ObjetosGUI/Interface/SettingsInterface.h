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
#ifndef SETTINGSINTERFACE_H
#define SETTINGSINTERFACE_H

#include "../SettingsComponent.h"

class GameObject;
class InterfaceComponent;

// Inspector del InterfaceComponent: nombre del asset de interfaz
// (Memory/Interfaces/<nombre>.json) que se muestra delante de la camara
// principal al entrar en modo play.
class SettingsInterface : public SettingsComponent {
    GameObject* gameObject;

public:
    explicit SettingsInterface(GameObject* gameObject);

    void showDataComponent() override;
    Component* getComponent() override;
};

#endif // SETTINGSINTERFACE_H