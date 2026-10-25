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
#include "SidebarOrbitaGUI.h"

#include <imgui.h>

float SidebarOrbitaGUI::progreso(float radio, float radioMinimo,
                                float radioMaximo) {
    // Sin rango no hay progreso que dibujar: la barra queda vacia.
    if (radioMaximo <= radioMinimo) return 0.0f;
    float progreso = (radio - radioMinimo) / (radioMaximo - radioMinimo);
    if (progreso < 0.0f) progreso = 0.0f;
    if (progreso > 1.0f) progreso = 1.0f;
    return progreso;
}

void SidebarOrbitaGUI::dibujar(const EstadoOrbita& estado) {
    if (!estado.activa) return;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Orbita", nullptr, flags)) {
        ImGui::Text("Radio Orbita");
        ImGui::Separator();
        // Barra vertical: altura fija, progreso = (radio - min) / (max - min).
        float avance = SidebarOrbitaGUI::progreso(estado.radio,
                                                  estado.radioMinimo,
                                                  estado.radioMaximo);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size(30, 200);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
        // Relleno con el acento del tema (mismo criterio que la barra de
        // StatusBarInterface): el amarillo de fabrica de ImGuiCol_PlotHistogram
        // quedaba fuera del color elegido en Opciones.
        ImU32 fgCol = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
        // Fondo
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgCol);
        // Relleno (de abajo hacia arriba)
        float fillH = size.y * avance;
        draw->AddRectFilled(
            ImVec2(pos.x, pos.y + size.y - fillH),
            ImVec2(pos.x + size.x, pos.y + size.y),
            fgCol);
        // Borde
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      ImGui::GetColorU32(ImGuiCol_Border));
        // Avanzar cursor por el tamano de la barra
        ImGui::Dummy(size);
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%.1f", estado.radio);
        ImGui::Text("min: %.1f", estado.radioMinimo);
        ImGui::Text("max: %.1f", estado.radioMaximo);
        ImGui::EndGroup();
        ImGui::End();
    }
}
