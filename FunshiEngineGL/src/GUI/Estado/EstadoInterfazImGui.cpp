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
#include "EstadoInterfazImGui.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace {

// Que widget sostiene la captura del teclado. El orden importa: un popup
// modal se lleva el teclado aunque haya un campo de texto en edicion debajo,
// y un campo de texto en edicion es mas especifico que un widget cualquiera
// con el foco.
std::string motivoDeCaptura() {
    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::GetTopMostPopupModal() != nullptr) {
        return std::string("popup modal ") + ImGui::GetTopMostPopupModal()->Name;
    }
    if (io.WantTextInput) {
        ImGuiWindow* ventana = ImGui::GetCurrentContext()->ActiveIdWindow;
        return std::string("campo de texto en edicion en ") +
               (ventana ? ventana->Name : "(ventana desconocida)");
    }
    if (ImGui::IsAnyItemActive()) {
        ImGuiWindow* ventana = ImGui::GetCurrentContext()->ActiveIdWindow;
        return std::string("widget activo en ") +
               (ventana ? ventana->Name : "(ventana desconocida)");
    }
    return "sin captura";
}

} // namespace

bool EstadoInterfazImGui::capturaTeclado() const {
    // Sin contexto de ImGui no hay interfaz que capture nada: el motor puede
    // arrancar antes de crearlo (la ventana se registra antes que el
    // contexto) y el juego exportado puede no tener interfaz alguna.
    if (ImGui::GetCurrentContext() == nullptr) return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool EstadoInterfazImGui::capturaMouse() const {
    if (ImGui::GetCurrentContext() == nullptr) return false;
    return ImGui::GetIO().WantCaptureMouse;
}

std::string EstadoInterfazImGui::motivoCapturaTeclado() const {
    if (ImGui::GetCurrentContext() == nullptr) return "sin captura";
    return motivoDeCaptura();
}
