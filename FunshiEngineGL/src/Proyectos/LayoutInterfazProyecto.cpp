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
#include "LayoutInterfazProyecto.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "../Configuracion/ProjectPaths.h"

void LayoutInterfazProyecto::aplicarProyecto(const std::string& nombreProyecto) {
    // El layout (posiciones de ventanas y docks) se guarda en Memory del
    // proyecto del usuario, no en el directorio de lanzamiento. Sin proyecto se
    // deja la interfaz sin archivo de layout: el primer arranque no crea
    // carpetas de "Nuevo Proyecto" por el camino.
    ini_ = nombreProyecto.empty() ? std::string()
                                   : ProjectPaths::rutaImguiIni(nombreProyecto);
    if (io_)
        io_->IniFilename = ini_.empty() ? nullptr : ini_.c_str();
}

void LayoutInterfazProyecto::conmutarProyecto(
    const std::string& nombreProyecto) {
    volcarLayoutVigente();
    aplicarProyecto(nombreProyecto);
    // La carga del entrante se difiere: se ejecuta en el frame siguiente al
    // cambio, cuando sus ventanas ya existen.
    cargaPendiente_ = !ini_.empty();
    rutaCarga_ = ini_;
}

void LayoutInterfazProyecto::volcarLayoutVigente() {
    // ImGui solo carga el archivo una vez por proceso (UpdateSettings, en el
    // primer NewFrame) y su guardado diferido escribe sobre la ruta ACTIVA en
    // el momento del volcado. Sin volcar antes de conmutar, el layout no viaja
    // con el proyecto: el entrante nunca se restaura (las ventanas quedan en las
    // posiciones del anterior) y el volcado en curso contamina el layout ajeno.
    if (!io_ || io_->IniFilename == nullptr) return;
    ImGui::SaveIniSettingsToDisk(io_->IniFilename);
}

void LayoutInterfazProyecto::procesarCargaDiferida() {
    if (!cargaPendiente_ || rutaCarga_.empty() || !io_) return;
    // Ejecutar DESPUÉS de NewFrame(): g.Windows ya tiene ventanas del nuevo
    // proyecto con stateGUI restaurado. LoadIniSettingsFromDisk itera g.Windows
    // y re-aplica DockId correctamente.
    ImGui::LoadIniSettingsFromDisk(rutaCarga_.c_str());
    // Fix LastFrameAlive: nodos creados desde ini empiezan en 0 -> undock.
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < g.DockContext.Nodes.Data.Size; ++n) {
        if (ImGuiDockNode* node =
                (ImGuiDockNode*)g.DockContext.Nodes.Data[n].val_p) {
            node->LastFrameAlive = g.FrameCount;
        }
    }
    cargaPendiente_ = false;
    rutaCarga_.clear();
}
