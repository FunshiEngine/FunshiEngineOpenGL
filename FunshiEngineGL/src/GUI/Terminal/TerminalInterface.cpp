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
#include "TerminalInterface.h"
#include "../Tema/TemaEditor.h"

#include "../WindowNames.h"
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include "../../Localizacion/Localizacion.h"

using Localizacion::Texto;

namespace {

std::string secuenciaTeclaTerminal(ImGuiKey tecla) {
    switch (tecla) {
        case ImGuiKey_UpArrow: return "\x1b[A";
        case ImGuiKey_DownArrow: return "\x1b[B";
        case ImGuiKey_RightArrow: return "\x1b[C";
        case ImGuiKey_LeftArrow: return "\x1b[D";
        case ImGuiKey_Home: return "\x1b[H";
        case ImGuiKey_End: return "\x1b[F";
        case ImGuiKey_Delete: return "\x1b[3~";
        case ImGuiKey_Backspace: return "\x7f";
        case ImGuiKey_Escape: return "\x1b";
        default: return {};
    }
}

std::string caracterUtf8(unsigned int caracter) {
    std::string resultado;
    if (caracter <= 0x7Fu) {
        resultado.push_back(static_cast<char>(caracter));
    } else if (caracter <= 0x7FFu) {
        resultado.push_back(static_cast<char>(0xC0u | (caracter >> 6)));
        resultado.push_back(static_cast<char>(0x80u | (caracter & 0x3Fu)));
    } else if (caracter <= 0xFFFFu &&
               (caracter < 0xD800u || caracter > 0xDFFFu)) {
        resultado.push_back(static_cast<char>(0xE0u | (caracter >> 12)));
        resultado.push_back(
            static_cast<char>(0x80u | ((caracter >> 6) & 0x3Fu)));
        resultado.push_back(static_cast<char>(0x80u | (caracter & 0x3Fu)));
    } else if (caracter > 0xFFFFu && caracter <= 0x10FFFFu) {
        resultado.push_back(static_cast<char>(0xF0u | (caracter >> 18)));
        resultado.push_back(
            static_cast<char>(0x80u | ((caracter >> 12) & 0x3Fu)));
        resultado.push_back(
            static_cast<char>(0x80u | ((caracter >> 6) & 0x3Fu)));
        resultado.push_back(static_cast<char>(0x80u | (caracter & 0x3Fu)));
    }
    return resultado;
}

ImU32 colorTerminal(int indice, bool fondo) {
    static const ImU32 paleta[16] = {
        IM_COL32(0, 0, 0, 255), IM_COL32(205, 49, 49, 255), IM_COL32(13, 188, 121, 255), IM_COL32(229, 229, 16, 255),
        IM_COL32(36, 114, 200, 255), IM_COL32(188, 63, 188, 255), IM_COL32(17, 168, 205, 255), IM_COL32(229, 229, 229, 255),
        IM_COL32(102, 102, 102, 255), IM_COL32(241, 76, 76, 255), IM_COL32(35, 209, 139, 255), IM_COL32(245, 245, 67, 255),
        IM_COL32(59, 142, 234, 255), IM_COL32(214, 112, 214, 255), IM_COL32(41, 184, 219, 255), IM_COL32(255, 255, 255, 255)};
    if (fondo && indice == 0) return IM_COL32(20, 20, 20, 255);
    return paleta[std::clamp(indice, 0, 15)];
}

}

TerminalInterface::TerminalInterface(bool visible)
    : GeneralUserInterface(WindowNames::Terminal, visible, ImGuiWindowFlags_None) {
#if defined(_WIN32)
    shells_ = {"cmd.exe", "powershell.exe", "pwsh.exe"};
    shellActual_ = "powershell.exe";
#else
    shells_ = {"/bin/bash", "/bin/sh", "/usr/bin/zsh"};
    shellActual_ = "/bin/bash";
#endif
    emulador_.redimensionar(80, 24);
}

TerminalInterface::~TerminalInterface() { sesion_.detener(); }

int TerminalInterface::filtrarCaracter(ImGuiInputTextCallbackData* datos) {
    auto* interfaz = static_cast<TerminalInterface*>(datos->UserData);
    if (!interfaz->sesion_.estaActiva()) return 0;

    switch (datos->EventFlag) {
        case ImGuiInputTextFlags_CallbackCharFilter: {
            // Caracteres imprimibles (incluye UTF-8 via InputQueueCharacters del backend)
            if (datos->EventChar >= 0x20) {
                const std::string utf8 = caracterUtf8(static_cast<unsigned int>(datos->EventChar));
                if (!utf8.empty()) interfaz->sesion_.escribir(utf8);
            }
            // Retornar 1 = filtrar (no agregar al buffer del InputText)
            return 1;
        }

        case ImGuiInputTextFlags_CallbackCompletion: {
            // Tab - enviar \t al terminal
            interfaz->sesion_.escribir("\t");
            return 0;
        }

        case ImGuiInputTextFlags_CallbackHistory: {
            // Flechas arriba/abajo - enviar secuencias ANSI
            const std::string secuencia = (datos->EventKey == ImGuiKey_UpArrow ||
                                           datos->EventKey == ImGuiKey_DownArrow)
                                              ? secuenciaTeclaTerminal(datos->EventKey)
                                              : std::string{};
            if (!secuencia.empty()) interfaz->sesion_.escribir(secuencia);
            return 0;
        }

        case ImGuiInputTextFlags_CallbackEdit: {
            // Cualquier edición del buffer (Backspace, Delete, Ctrl+X, etc.)
            // El buffer ya refleja el cambio; aquí solo reenviamos teclas
            // de control que no generan carácter imprimible y no son
            // manejadas por Completion/History.
            return 0;
        }
    }
    return 0;
}

void TerminalInterface::setDirectorioProyecto(const std::string& directorio) {
    if (directorioProyecto_ == directorio) return;
    directorioProyecto_ = directorio;
    if (sesion_.estaActiva()) sesion_.detener();
}

void TerminalInterface::iniciar() {
    if (directorioProyecto_.empty()) {
        error_ = "No hay un proyecto activo para usar como directorio";
        return;
    }
    error_.clear();
    if (!sesion_.iniciar(shellActual_, directorioProyecto_, columnas_, filas_, error_)) return;
    emulador_ = TerminalEmulator{};
    emulador_.redimensionar(columnas_, filas_);
}

void TerminalInterface::contentGUI() {
    ImGui::Text(
            Localizacion::tr(Texto::Proyecto),
            directorioProyecto_.empty() ? "(sin proyecto)" : directorioProyecto_.c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    if (!sesion_.estaActiva() && ImGui::BeginCombo(
            Localizacion::etiqueta(Texto::Shell).c_str(), shellActual_.c_str())) {
        for (const std::string& shell : shells_) {
            const bool seleccionada = shell == shellActual_;
            if (ImGui::Selectable(shell.c_str(), seleccionada)) shellActual_ = shell;
            if (seleccionada) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (!sesion_.estaActiva()) {
        // El arranque se difiere hasta conocer el tamaño real del panel: la
        // sesión debe nacer con la cuadrícula de la vista, no con la del frame
        // anterior ni con la dimensión por defecto.
        if (ImGui::Button(Localizacion::etiqueta(Texto::Iniciar).c_str())) iniciarPendiente_ = true;
    } else if (ImGui::Button(Localizacion::etiqueta(Texto::Interrumpir).c_str())) {
        sesion_.escribir("\x03");
    }
    ImGui::SameLine();
    if (sesion_.estaActiva() && ImGui::Button(Localizacion::etiqueta(Texto::CerrarSesion).c_str())) sesion_.detener();
    if (!error_.empty()) ImGui::TextColored(TemaEditor::color(TemaEditor::RolColor::Error), "%s", error_.c_str());

    const float altoLinea = ImGui::GetTextLineHeight();
    const float anchoCaracter = std::max(ImGui::CalcTextSize("M").x, 1.0f);
    const ImVec2 disponible = ImGui::GetContentRegionAvail();
    const float altoEntrada = ImGui::GetFrameHeightWithSpacing();
    columnas_ = static_cast<unsigned>(std::clamp(std::floor(disponible.x / anchoCaracter), 20.0f, 500.0f));
    filas_ = static_cast<unsigned>(std::clamp(std::floor((disponible.y - altoEntrada) / std::max(altoLinea, 1.0f)), 5.0f, 300.0f));
    if (columnas_ != emulador_.columnas() || filas_ != emulador_.filas()) {
        emulador_.redimensionar(columnas_, filas_);
        if (sesion_.estaActiva()) sesion_.redimensionar(columnas_, filas_);
    }
    if (iniciarPendiente_) {
        iniciarPendiente_ = false;
        iniciar();
    }
    // La salida se drena siempre, también después de que la shell termina: si
    // solo se leyera mientras estaActiva() lo último que emitió la shell antes
    // de salir quedaría sin pintar.
    emulador_.procesar(sesion_.leerSalida());
    const std::string respuestas = emulador_.tomarRespuestas();
    if (sesion_.estaActiva() && !respuestas.empty()) sesion_.escribir(respuestas);

    // Al reaparecer el panel sin sesión (se ocultó o la shell terminó) se
    // borra una sola vez: así la salida final drenada queda visible y no
    // arrastra restos de la terminal anterior.
    if (!sesion_.estaActiva() && !panelMostrado_) {
        panelMostrado_ = true;
        emulador_ = TerminalEmulator{};
        emulador_.redimensionar(columnas_, filas_);
    } else if (sesion_.estaActiva()) {
        panelMostrado_ = false;
    }

    // InputText INVISIBLE para captura de teclado. Se crea ANTES del área de
    // la terminal para que podamos enfocarlo desde el click en el área.
    // Flags: EnterReturnsTrue para detectar Enter, callbacks para chars/Tab/flechas,
    // AllowTabInput para que Tab no navegue sino vaya al callback.
    static bool terminalQuiereFoco = false;
    if (sesion_.estaActiva()) {
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue
                                  | ImGuiInputTextFlags_CallbackCharFilter
                                  | ImGuiInputTextFlags_CallbackCompletion
                                  | ImGuiInputTextFlags_CallbackHistory
                                  | ImGuiInputTextFlags_AllowTabInput
                                  | ImGuiInputTextFlags_NoHorizontalScroll;

        // Foco inicial al aparecer la ventana, o si el área de terminal lo pidió
        if (ImGui::IsWindowAppearing() || terminalQuiereFoco) {
            ImGui::SetKeyboardFocusHere();
            terminalQuiereFoco = false;
        }

        static char inputBuffer[4096] = {};

        // InputText invisible: ancho 1px (no 0, para que sea focusable), sin frame, sin fondo
        ImGui::SetNextItemWidth(1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
        bool enterPressed = ImGui::InputText("##TerminalInputHidden", inputBuffer, IM_ARRAYSIZE(inputBuffer),
                                             flags, TerminalInterface::filtrarCaracter, this);
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        // Manejo de teclas especiales NO cubiertas por callbacks: se envian como
        // secuencias del mapa unico (izquierda/derecha incluidas; el InputText
        // las consumiria moviendo su propio cursor sobre un buffer vacio).
        // Enter se maneja aparte porque EnterReturnsTrue desactiva el item
        if (ImGui::IsItemActive()) {
            ImGuiIO& io = ImGui::GetIO();

            const ImGuiKey teclas[] = {ImGuiKey_LeftArrow, ImGuiKey_RightArrow,
                                       ImGuiKey_Backspace, ImGuiKey_Delete,
                                       ImGuiKey_Home, ImGuiKey_End,
                                       ImGuiKey_Escape};
            for (ImGuiKey tecla : teclas) {
                const std::string secuencia = secuenciaTeclaTerminal(tecla);
                if (!secuencia.empty() && ImGui::IsKeyPressed(tecla, true))
                    sesion_.escribir(secuencia);
            }
            // Ctrl+A-Z (INCLUYE Ctrl+C=3, que ImGui usa para Copy pero nosotros
            // queremos que vaya al terminal como SIGINT)
            if (io.KeyCtrl && !io.KeyAlt) {
                for (int key = ImGuiKey_A; key <= ImGuiKey_Z; ++key) {
                    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), true)) {
                        char ctrlChar = static_cast<char>(key - ImGuiKey_A + 1);
                        // Solo saltamos los que ImGui maneja nativamente en InputText
                        // y que NO queremos en el terminal: V(peg), X(cort), Z(desh), Y(reh), A(sel.todo)
                        // Ctrl+C (3) SÍ lo enviamos al terminal (SIGINT)
                        if (ctrlChar != 22 && ctrlChar != 24 && ctrlChar != 26 &&
                            ctrlChar != 25 && ctrlChar != 1) {
                            sesion_.escribir(std::string(1, ctrlChar));
                        }
                    }
                }
            }
        }

        // Enter: EnterReturnsTrue retorna true y desactiva el item en este frame.
        // enterPressed será true solo en el frame exacto de la pulsación.
        if (enterPressed) {
            sesion_.escribir("\r");
            ImGui::SetKeyboardFocusHere(-1);
            inputBuffer[0] = '\0';
        }
    }

    // Área de la terminal: click para enfocar el input invisible
    ImGui::BeginChild("TerminalPantalla", ImVec2(0, std::max(40.0f, disponible.y - altoEntrada)), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 origen = ImGui::GetCursorScreenPos();
    ImDrawList* dibujo = ImGui::GetWindowDrawList();
    for (unsigned y = 0; y < emulador_.filas(); ++y) {
        for (unsigned x = 0; x < emulador_.columnas(); ++x) {
            const auto& celda = emulador_.celdas()[static_cast<std::size_t>(y) * emulador_.columnas() + x];
            const ImVec2 posicion(origen.x + anchoCaracter * x, origen.y + altoLinea * y);
            if (celda.fondoDefinido) dibujo->AddRectFilled(posicion, ImVec2(posicion.x + anchoCaracter, posicion.y + altoLinea), colorTerminal(celda.fondo, true));
            if (celda.texto != " ") dibujo->AddText(posicion, colorTerminal(celda.primerPlano, false), celda.texto.c_str());
            if (sesion_.estaActiva() && emulador_.cursorVisible() && x == emulador_.cursorX() && y == emulador_.cursorY())
                dibujo->AddRect(posicion, ImVec2(posicion.x + anchoCaracter, posicion.y + altoLinea), IM_COL32(230, 230, 230, 220));
        }
    }
    ImGui::Dummy(ImVec2(anchoCaracter * emulador_.columnas(), altoLinea * emulador_.filas()));
    
    // Click en el área de terminal -> pide foco para el InputText invisible
    if (sesion_.estaActiva() && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        terminalQuiereFoco = true;
    }
    
    ImGui::EndChild();
}

void TerminalInterface::printGUI() {
    if (!stateGUI) return;
    const bool mostrar = ImGui::Begin(nameGUI.c_str(), &stateGUI, flagGUI);
    if (mostrar) contentGUI();
    ImGui::End();
}
