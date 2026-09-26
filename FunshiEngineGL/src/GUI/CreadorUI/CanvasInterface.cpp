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
#include "CanvasInterface.h"

#include "../../Audio/AudioEngine.h"
#include "../WindowNames.h"

#include <imgui.h>
#include <cstring>

namespace {

// Reproduce el clip de un widget si tiene uno asignado. No bloquea: encola en
// el hilo de audio (AudioEngine) y devuelve de inmediato.
void tocarSonido(WidgetUI& widget, AudioEngine* motor) {
    if (!motor || widget.sonido.empty() || widget.sonido == kSinSonido) return;
    motor->reproducir(widget.sonido, 1.0f, false);
}

} // namespace

CanvasInterface::CanvasInterface(bool stateGUI)
    : GeneralUserInterface(WindowNames::CanvasUI, stateGUI, ImGuiWindowFlags_None) {}

void CanvasInterface::initGUI() {
    ImGuiWindowFlags flags = getFlagGui();
    if (modoPlay_ && ui_) {
        // Overlay a pantalla completa en modo play (sin barra ni bordes).
        const ImVec2 tam = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
        flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
    } else if (ui_) {
        ImGui::SetNextWindowSize(ImVec2(ui_->ancho + 24.f, ui_->alto + 44.f),
                                 ImGuiCond_Appearing);
    }
    ImGui::Begin(getNameGui().c_str(), &stateGUI, flags);
}

void CanvasInterface::contentGUI() {
    if (!ui_) {
        ImGui::TextWrapped(
            "Sin interfaz activa.\n\nElige una interfaz en \"Creador de "
            "Interfaces\" y pulsa \"Activar en canvas\".");
        return;
    }

    // Area de la interfaz: se centra dentro de la ventana (u overlay).
    const float w = ui_->ancho;
    const float h = ui_->alto;
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    // Titulo estilizado + marco del canvas.
    if (!ui_->titulo.empty()) {
        ImGui::TextUnformatted(ui_->titulo.c_str());
        ImGui::Separator();
    }

    const float margenX = (avail.x - w) * 0.5f;
    const float margenY = ImGui::GetCursorPosY() + 8.f;
    if (margenX > 0.f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margenX);
    ImGui::BeginChild("CanvasUI", ImVec2(w, h), true);

    for (WidgetUI& widget : ui_->widgets) {
        const bool haySonido =
            !widget.sonido.empty() && widget.sonido != kSinSonido;
        if (haySonido) {
            // Pequeno indicador de que el control tiene audio asignado.
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                ImVec4(0.85f, 0.7f, 0.15f, 1.f));
        }

        switch (widget.tipo) {
            case TipoWidget::Etiqueta:
                if (!widget.etiqueta.empty())
                    ImGui::TextUnformatted(widget.etiqueta.c_str());
                break;
            case TipoWidget::Boton:
                if (ImGui::Button(
                        widget.etiqueta.empty() ? widget.nombre.c_str()
                                                : widget.etiqueta.c_str(),
                        ImVec2(-1, 0)))
                    tocarSonido(widget, motor_);
                break;
            case TipoWidget::Checkbox: {
                bool valor = widget.activado;
                if (ImGui::Checkbox(widget.etiqueta.c_str(), &valor)) {
                    widget.activado = valor;
                    tocarSonido(widget, motor_);
                }
                break;
            }
            case TipoWidget::Slider: {
                float valor = widget.valor;
                if (ImGui::SliderFloat(widget.etiqueta.c_str(), &valor,
                                       widget.minimo, widget.maximo, "%.2f")) {
                    if (widget.valor != valor) {
                        widget.valor = valor;
                        tocarSonido(widget, motor_);
                    }
                }
                break;
            }
            case TipoWidget::EntradaTexto: {
                char buf[256] = {};
                std::strncpy(buf, widget.texto.c_str(), sizeof(buf) - 1);
                if (ImGui::InputText(
                        widget.etiqueta.c_str(), buf, sizeof(buf),
                        ImGuiInputTextFlags_EnterReturnsTrue)) {
                    widget.texto = buf;
                    tocarSonido(widget, motor_);
                }
                break;
            }
        }

        if (haySonido) ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    ImGui::EndChild();
}

void CanvasInterface::printGUI() {
    if (!stateGUI) return;
    initGUI();
    contentGUI();
    endGUI();
}