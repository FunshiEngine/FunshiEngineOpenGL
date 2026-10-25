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
#include "SettingsSombras.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Sombras/Sombras.h"
#include <imgui.h>

SettingsSombras::SettingsSombras(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(Sombras).name())), gameObject(gameObject) {}

void SettingsSombras::showDataComponent() {
    Sombras* sombras = gameObject->getComponent<Sombras>();
    if (!sombras) return;

    ImGui::Text("===Sombras===");

    bool activado = sombras->estaActivado();
    if (ImGui::Checkbox("Proyecta sombra", &activado)) {
        sombras->setActivado(activado);
    }

    int resolucion = sombras->getResolucion();
    if (ImGui::SliderInt("Resolucion", &resolucion, 256, 4096)) {
        sombras->setResolucion(resolucion);
    }

    float sesgo = sombras->getSesgo();
    if (ImGui::SliderFloat("Sesgo", &sesgo, 0.f, 0.02f)) {
        sombras->setSesgo(sesgo);
    }

    float fuerza = sombras->getFuerza();
    if (ImGui::SliderFloat("Fuerza", &fuerza, 0.f, 1.f)) {
        sombras->setFuerza(fuerza);
    }

    float radio = sombras->getRadio();
    if (ImGui::SliderFloat("Radio", &radio, 1.f, 200.f)) {
        sombras->setRadio(radio);
    }

    float distancia = sombras->getDistancia();
    if (ImGui::SliderFloat("Distancia", &distancia, 1.f, 300.f)) {
        sombras->setDistancia(distancia);
    }
}
