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
#include "../../Herramientas/IconosGUI/IconosGUI.h"

SceneMenuBarInterface::SceneMenuBarInterface(bool state)
    : GeneralUserInterface("MenuBar", state, ImGuiWindowFlags_MenuBar), toggleBool(nullptr) {}
void SceneMenuBarInterface::setActivador(bool* target) { toggleBool = target; }

void SceneMenuBarInterface::setPausa(bool* pausada) noexcept {
    pausada_ = pausada;
}

void SceneMenuBarInterface::setModoJuego(bool* modoJuego) noexcept {
    modoJuego_ = modoJuego;
}

void SceneMenuBarInterface::setAccionSimulacion(
    std::function<void(AccionSimulacion)> accion) {
    accionSimulacion_ = std::move(accion);
}

void SceneMenuBarInterface::setIconosGUI(IconosGUI* iconos) noexcept {
    iconosGUI_ = iconos;
}

void SceneMenuBarInterface::setGizmoGlobal(bool* target) { gizmoGlobal = target; }
bool* SceneMenuBarInterface::getActivador() { return toggleBool; }
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

    // El dialogo de exportacion se renderiza aqui y el dueno lo destruye
    // DESPUES de render(), nunca dentro: hacerlo dentro dejaria el frame
    // corriendo sobre el objeto liberado.
    if (mostrarExportDialog_ && exportDialog_) {
        exportDialog_->render();
        if (exportDialog_->debeCerrarse()) {
            mostrarExportDialog_ = false;
            exportDialog_.reset();
        }
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

static bool botonSimulacion(const char* id, const char* etiqueta,
                            ImTextureID icono, const ImVec2& tamano) {
    const bool activado = ImGui::Button(id, tamano);
    const ImVec2 minimo = ImGui::GetItemRectMin();
    const bool tieneIcono = icono != ImTextureID_Invalid;
    if (tieneIcono) {
        const float lado = 20.0f;
        const ImVec2 posicion(minimo.x + (tamano.x - lado) * 0.5f,
                              minimo.y + (tamano.y - lado) * 0.5f);
        ImGui::GetWindowDrawList()->AddImage(
            icono, posicion, ImVec2(posicion.x + lado, posicion.y + lado));
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", etiqueta);
    return activado;
}

void SceneMenuBarInterface::contentGUI() {
    ImGui::BeginDisabled(modoJuego_ && *modoJuego_);
    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("Archivo")) {
        if (ImGui::MenuItem("Exportar juego")) {
            if (!exportDialog_) {
                exportDialog_ = std::make_unique<ExportDialog>(proyectoActual_);
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
    ImGui::EndDisabled();
    if (!toggleBool) return;
    const bool activo = *toggleBool;
    ImGui::SetCursorPosX(
        (ImGui::GetWindowWidth() - (activo ? 300.0f : 100.0f)) * 0.5f);
    if (activo) {
        const bool pausada = pausada_ && *pausada_;
        const ImTextureID iconoPausa =
            pausada && iconosGUI_ ? iconosGUI_->getIconoPlay()
                                  : iconosGUI_ ? iconosGUI_->getIconoPausa()
                                               : ImTextureID_Invalid;
        if (botonSimulacion("##pausa", pausada ? "Reanudar" : "Pausa",
                            iconoPausa,
                            ImVec2(40, 32)) &&
            accionSimulacion_) {
            accionSimulacion_(AccionSimulacion::Pausa);
        }
        ImGui::SameLine();
        if (botonSimulacion("##reset", "Reset",
                            iconosGUI_ ? iconosGUI_->getIconoReset()
                                       : ImTextureID_Invalid,
                            ImVec2(40, 32)) &&
            accionSimulacion_)
            accionSimulacion_(AccionSimulacion::Reset);
        ImGui::SameLine();
        if (botonSimulacion("##terminar", "Terminar",
                            iconosGUI_ ? iconosGUI_->getIconoStop()
                                       : ImTextureID_Invalid,
                            ImVec2(40, 32)) &&
            accionSimulacion_)
            accionSimulacion_(AccionSimulacion::Terminar);
        ImGui::SameLine();
        ImGui::Text("Estado: %s%s",
                    pausada ? "PAUSADO" : "ACTIVO",
                    modoJuego_ && *modoJuego_ ? " (Juego)" : " (Depuracion)");
    } else {
        if (botonSimulacion("##depuracion", "Depuración",
                            iconosGUI_ ? iconosGUI_->getIconoDepuracion()
                                       : ImTextureID_Invalid,
                            ImVec2(40, 32)) &&
            accionSimulacion_) {
            accionSimulacion_(AccionSimulacion::IniciarDepuracion);
        }
        ImGui::SameLine();
        if (botonSimulacion("##juego", "Juego",
                            iconosGUI_ ? iconosGUI_->getIconoPlay()
                                       : ImTextureID_Invalid,
                            ImVec2(40, 32)) &&
            accionSimulacion_) {
            accionSimulacion_(AccionSimulacion::IniciarJuego);
        }
    }
}
void SceneMenuBarInterface::endGUI() { ImGui::PopID(); ImGui::End(); }
void SceneMenuBarInterface::printGUI() {
    if (stateGUI) { initGUI(); contentGUI(); endGUI(); }
}
