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
#include "../../../Localizacion/Localizacion.h"

using Localizacion::Texto;

SettingsSombras::SettingsSombras(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(Sombras).name())), gameObject(gameObject) {}

void SettingsSombras::showDataComponent() {
    Sombras* sombras = objetivo<Sombras>(gameObject);
    if (!sombras) return;

    bool activado = sombras->estaActivado();
    if (ImGui::Checkbox(Localizacion::etiqueta(Texto::ProyectaSombra).c_str(), &activado)) {
        sombras->setActivado(activado);
    }

    // Resolucion: el componente solo admite potencias de dos entre 256 y 4096.
    static const int kResoluciones[] = {256, 512, 1024, 2048, 4096};
    static const char* kEtiquetas[] = {"256", "512", "1024", "2048", "4096"};
    int actual = 0;
    for (int i = 0; i < 5; ++i)
        if (kResoluciones[i] == sombras->getResolucion()) actual = i;
    if (ImGui::Combo(Localizacion::etiqueta(Texto::Resolucion).c_str(), &actual, kEtiquetas, 5)) {
        sombras->setResolucion(kResoluciones[actual]);
    }

    float sesgo = sombras->getSesgo();
    if (ImGui::DragFloat(Localizacion::etiqueta(Texto::Sesgo).c_str(), &sesgo, 0.0001f, 0.f, 0.1f, "%.4f")) {
        sombras->setSesgo(sesgo);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localizacion::tr(Texto::SubeSiAparecenRayasAcne));

    float fuerza = sombras->getFuerza();
    if (ImGui::SliderFloat(Localizacion::etiqueta(Texto::Fuerza).c_str(), &fuerza, 0.f, 1.f)) {
        sombras->setFuerza(fuerza);
    }

    float radio = sombras->getRadio();
    if (ImGui::DragFloat(Localizacion::etiqueta(Texto::Radio).c_str(), &radio, 0.25f, 0.5f, 1000.f)) {
        sombras->setRadio(radio);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localizacion::tr(Texto::SoloLuzDireccionalSemiladoDel));

    float distancia = sombras->getDistancia();
    if (ImGui::DragFloat(Localizacion::etiqueta(Texto::Distancia).c_str(), &distancia, 0.5f, 1.f, 5000.f)) {
        sombras->setDistancia(distancia);
    }
}
