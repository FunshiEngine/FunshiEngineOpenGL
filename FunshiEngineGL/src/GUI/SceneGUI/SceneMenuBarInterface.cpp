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
#include "SceneMenuBarInterface.h"

#include "../WindowNames.h"

SceneMenuBarInterface::SceneMenuBarInterface(bool state)
    : GeneralUserInterface("MenuBar", state, ImGuiWindowFlags_MenuBar), toggleBool(nullptr) {}
void SceneMenuBarInterface::setActivador(bool* target) { toggleBool = target; }
void SceneMenuBarInterface::setGizmoGlobal(bool* target) { gizmoGlobal = target; }
bool* SceneMenuBarInterface::getActivador() { return toggleBool; }
bool SceneMenuBarInterface::getCargarScripts() { return cargarScripts; }
void SceneMenuBarInterface::setCargarScripts(bool value) { cargarScripts = value; }
void SceneMenuBarInterface::setEditorEventBus(EditorEventBus* bus) {
    busEditor = bus;
}
void SceneMenuBarInterface::setVentanas(const std::map<std::string, bool>& estados) {
    ventanas_ = estados;
}

void SceneMenuBarInterface::setProyectoActual(const std::string& proyecto) {
    proyectoActual_ = proyecto;
    if (exportDialog_) {
        exportDialog_->setProyectoActual(proyecto);
    }
}
void SceneMenuBarInterface::initGUI() {
    ImGui::Begin(getNameGui().c_str(), &stateGUI, getFlagGui());
    ImGui::PushID(this);

    // Actualizar dialogo de exportación si está abierto
    if (mostrarExportDialog_ && exportDialog_) {
        exportDialog_->render();
    }
}
// Etiqueta en espanol por WindowName para el menu "Ventanas"; nullptr para las
// ventanas que no se pueden alternar desde aqui (dock, propia barra, settings).
static const char* etiquetaVentana(const std::string& nombre) {
    if (nombre == WindowNames::BrowseFile) return "Explorador de archivos";
    if (nombre == WindowNames::ShowFolder) return "Vista de contenido";
    if (nombre == WindowNames::SelectedObjects) return "Objetos seleccionados";
    if (nombre == WindowNames::Status) return "Barra de estado";
    return nullptr;
}
void SceneMenuBarInterface::contentGUI() {
    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("Archivo")) {
        if (ImGui::MenuItem("Exportar juego")) {
            if (!exportDialog_) {
                exportDialog_ = std::make_unique<ExportDialog>([this](const ExportDialog::Resultado& r) {
                    mostrarExportDialog_ = false;
                    exportDialog_.reset();
                }, proyectoActual_);
            }
            mostrarExportDialog_ = true;
        }
        ImGui::EndMenu();
    }
    // Menu "Ventanas": permite reabrir los paneles cerrados (p. ej. el
    // explorador, que la config persistia cerrado y no se podia volver a
    // mostrar). Al tildar/destildar se publica VentanaEstadoCambio; main lo
    // aplica a la ventana y lo persiste en la config del proyecto.
    if (ImGui::BeginMenu("Ventanas")) {
        static const char* kVentanasEditables[] = {
            WindowNames::BrowseFile, WindowNames::ShowFolder,
            WindowNames::SelectedObjects, WindowNames::Status};
        for (const char* nombre : kVentanasEditables) {
            const char* etiqueta = etiquetaVentana(nombre);
            if (!etiqueta) continue;
            auto it = ventanas_.find(nombre);
            const bool abierta = it != ventanas_.end() ? it->second : true;
            if (ImGui::MenuItem(etiqueta, nullptr, abierta) && busEditor) {
                EditorEvent ev;
                ev.type = EditorEventType::VentanaEstadoCambio;
                ev.nombreVentana = nombre;
                ev.abierta = !abierta;
                busEditor->publish(ev);
            }
        }
        ImGui::EndMenu();
    }
    if (gizmoGlobal && ImGui::BeginMenu("Gizmo")) {
        // Sistema de coordenadas del gizmo: LOCAL (los ejes rotan con el
        // objeto seleccionado) o GLOBAL (ejes del mundo fijos, el gizmo no
        // rota con el objeto). Tambien se alterna con la tecla G.
        if (*gizmoGlobal) {
            if (ImGui::MenuItem("Local (ejes del objeto)", "G")) *gizmoGlobal = false;
            ImGui::MenuItem("Global (ejes del mundo)", "G", true);
        } else {
            ImGui::MenuItem("Local (ejes del objeto)", "G", true);
            if (ImGui::MenuItem("Global (ejes del mundo)", "G")) *gizmoGlobal = true;
        }
        ImGui::Separator();
        ImGui::TextDisabled("La tecla G alterna entre ambos.\nOperacion: 1/T mover, 2/R rotar, 3/Y escalar");
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
    if (!toggleBool) return;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 100) * 0.5f);
    // Boton play/stop: el texto y el color cambian segun el estado para que
    // quede claro que el mismo boton activa y detiene la simulacion.
    const bool activo = *toggleBool;
    const ImVec4 color =
        activo ? ImVec4(0.72f, 0.22f, 0.22f, 1.0f)   // Detener (rojo)
               : ImVec4(0.16f, 0.55f, 0.24f, 1.0f);  // Activar (verde)
    const ImVec4 hover = activo ? ImVec4(0.85f, 0.30f, 0.30f, 1.0f)
                                : ImVec4(0.24f, 0.68f, 0.32f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    if (ImGui::Button(activo ? "Detener" : "Activar", ImVec2(100, 30))) {
        *toggleBool = !*toggleBool;
        cargarScripts = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::Text(activo ? "Estado: ACTIVO" : "Estado: INACTIVO");
}
void SceneMenuBarInterface::endGUI() { ImGui::PopID(); ImGui::End(); }
void SceneMenuBarInterface::printGUI() {
    if (stateGUI) { initGUI(); contentGUI(); endGUI(); }
}
