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
#include "PanelModInterface.h"

#include <imgui.h>

#include "../../EngineTime.h"
#include "../../Mods/Api/RegistroPanelesMods.h"

PanelModInterface::PanelModInterface(const std::string& nombreVentana,
                                     const std::string& titulo)
    : GeneralUserInterface(nombreVentana, false, ImGuiWindowFlags_None),
      nombreVentana_(nombreVentana),
      titulo_(titulo) {
}

void PanelModInterface::printGUI() {
    auto& registro = FunshiMods::RegistroPanelesMods::global();
    const FunshiMods::PanelMod* panel = registro.buscarPorVentana(nombreVentana_);
    if (!panel) return;

    // El registro es la fuente de verdad del estado (lo alterna el menu y lo
    // persisten); la ventana solo refleja lo que ImGui reporte.
    stateGUI = panel->abierta;

    const std::string tituloConId = titulo_ + "##" + nombreVentana_;
    const bool mostrar = ImGui::Begin(tituloConId.c_str(), &stateGUI, flagGUI);
    if (mostrar) contentGUI();
    ImGui::End();

    // El usuario pudo cerrar la ventana con la 'X': se devuelve al registro.
    registro.establecerEstado(nombreVentana_, stateGUI);
}

void PanelModInterface::contentGUI() {
    auto& registro = FunshiMods::RegistroPanelesMods::global();
    const FunshiMods::PanelMod* panel = registro.buscarPorVentana(nombreVentana_);
    if (!panel || !panel->dibujar) return;

    FunshiMods::ContextoMod contexto;
    // El mod dibuja con SU propia copia de ImGui: el contexto del motor se le
    // entrega opaco y el mod lo fija con ImGui::SetCurrentContext().
    contexto.ctxImgui = ImGui::GetCurrentContext();
    const ImVec2 disponible = ImGui::GetContentRegionAvail();
    contexto.areaAncho = disponible.x;
    contexto.areaAlto = disponible.y;
    contexto.deltaTime = Time::getDeltaTime();

    // El mod en ejecucion es el dueno del panel: si desde el callback
    // registra otro panel, se le atribuye a el.
    const std::string anterior = registro.modEnEjecucion();
    registro.fijarModEnEjecucion(panel->modId);
    panel->dibujar(&contexto, panel->datos);
    registro.fijarModEnEjecucion(anterior);
}
