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
#include "SettingsSkybox.h"

#include "../../../Objetos/Componentes/Skybox.h"
#include "../../../Objetos/GameObject.h"

#include <imgui.h>

SettingsSkybox::SettingsSkybox(GameObject* gameObject) : gameObject(gameObject) {}

void SettingsSkybox::showDataComponent() {
    Skybox* skybox = gameObject ? gameObject->getComponent<Skybox>() : nullptr;
    if (!skybox) return;

    bool visible = skybox->getVisible();
    if (ImGui::Checkbox("Visible", &visible)) skybox->setVisible(visible);
    ImGui::Separator();

    // Seis selectores de archivo para las caras del cubemap
    // Orden: +X, -X, +Y, -Y, +Z, -Z
    const char* labels[6] = {
        "Cara +X (Right)",
        "Cara -X (Left)",
        "Cara +Y (Top)",
        "Cara -Y (Bottom)",
        "Cara +Z (Front)",
        "Cara -Z (Back)"
    };
    const char* getters[6] = {
        "getCaraMasX",
        "getCaraMenosX",
        "getCaraMasY",
        "getCaraMenosY",
        "getCaraMasZ",
        "getCaraMenosZ"
    };
    const char* setters[6] = {
        "setCaraMasX",
        "setCaraMenosX",
        "setCaraMasY",
        "setCaraMenosY",
        "setCaraMasZ",
        "setCaraMenosZ"
    };

    for (int i = 0; i < 6; ++i) {
        char buf[512];
        // Usar reflexion mediante switch en lugar de punteros directos
        std::string value;
        switch (i) {
            case 0: value = skybox->getCaraMasX(); break;
            case 1: value = skybox->getCaraMenosX(); break;
            case 2: value = skybox->getCaraMasY(); break;
            case 3: value = skybox->getCaraMenosY(); break;
            case 4: value = skybox->getCaraMasZ(); break;
            case 5: value = skybox->getCaraMenosZ(); break;
        }
        strncpy(buf, value.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText(labels[i], buf, sizeof(buf))) {
            switch (i) {
                case 0: skybox->setCaraMasX(buf); break;
                case 1: skybox->setCaraMenosX(buf); break;
                case 2: skybox->setCaraMasY(buf); break;
                case 3: skybox->setCaraMenosY(buf); break;
                case 4: skybox->setCaraMasZ(buf); break;
                case 5: skybox->setCaraMenosZ(buf); break;
            }
        }
    }
    ImGui::TextDisabled("Rutas relativas al proyecto. Formatos: PNG, JPG, TGA, BMP, PSD, HDR.");
}

Component* SettingsSkybox::getComponent() {
    return gameObject ? gameObject->getComponent<Skybox>() : nullptr;
}