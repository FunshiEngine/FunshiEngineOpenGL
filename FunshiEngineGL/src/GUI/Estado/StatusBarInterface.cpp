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
#include "StatusBarInterface.h"

#include "BarraProgresoTexto.h"
#include "../WindowNames.h"
#include "../../Events/EditorEventBus.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Script.h"
#include "../../Scenes/SceneRegistry.h"
#include "../../EngineTime.h"
#include "../../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

#include <cstdio>
#include <imgui.h>

StatusBarInterface::StatusBarInterface(bool stateGUI)
    : GeneralUserInterface(WindowNames::Status, stateGUI,
                           ImGuiWindowFlags_MenuBar),
      estadoPublicado_(stateGUI) {}

void StatusBarInterface::bindScene(SceneRegistry* scene) { scene_ = scene; }

void StatusBarInterface::setEditorEventBus(EditorEventBus* bus) {
    busEditor = bus;
}

void StatusBarInterface::setEstadoCompilacion(
    bool enCurso, const std::string& actual, std::size_t hecha,
    std::size_t total,
    const std::vector<ScriptRuntime::ResultadoCarga>& resultados,
    bool /*overlayProgreso*/, bool /*overlayResultado*/) {
    // Convertir a log simple: "cargando scripts: #######42%". El formato acota
    // hecha a total, asi que un productor desincronizado no puede construir una
    // longitud negativa (length_error).
    if (enCurso && total > 0) {
        const std::string barra = BarraProgresoTexto::formatear(hecha, total);
        const int pct = BarraProgresoTexto::porcentaje(hecha, total);
        mensajeTemporal_ = "cargando scripts: " + barra + std::to_string(pct) + "%";
        temporizadorMensaje_ = 0.5f; // visible medio segundo por frame
    }
    // Guardar estado para mostrar en contentGUI. Con total 0 no hay progreso
    // valido: se apaga el indicador para que la lista no divida por cero.
    compilando_ = enCurso && total > 0;
    actual_ = actual;
    hecha_ = hecha;
    total_ = total;
    resultados_ = resultados;
}

void StatusBarInterface::mostrarMensaje(const std::string& mensaje) {
    mensajeTemporal_ = mensaje;
    temporizadorMensaje_ = 4.0f;
}

void StatusBarInterface::dibujarToolchain() {
    if (!toolchainListo_) {
        toolchain_ = ScriptRuntime::estadoHerramientas();
        toolchainListo_ = true;
    }

    ImGui::SeparatorText("Herramientas externas");
    ImGui::BulletText("Compilador C++: %s",
                      toolchain_.compiladorCpp.empty()
                          ? "no detectado (defini FUNSHI_CXX)"
                          : toolchain_.compiladorCpp.c_str());
    ImGui::BulletText("Cache de artefactos: %s",
                      toolchain_.cache.empty() ? "-" : toolchain_.cache.c_str());
    if (toolchain_.soporteJava) {
        ImGui::BulletText("javac: %s",
                          toolchain_.javac.empty()
                              ? "no encontrado (defini JAVAC)"
                              : toolchain_.javac.c_str());
        ImGui::BulletText("libjvm: %s",
                          toolchain_.libjvm.empty()
                              ? "no encontrada (JAVA_HOME / FUNSHI_LIBJVM)"
                              : toolchain_.libjvm.c_str());
        ImGui::BulletText("JVM: %s",
                          toolchain_.jvmArrancada
                              ? "arrancada"
                              : "pendiente (se inicia con el primer script Java)");
    } else {
        ImGui::BulletText("Scripts Java: backend no compilado (FUNSHI_JAVA=OFF)");
    }
}

void StatusBarInterface::contentGUI() {
    dibujarToolchain();

    // Estado de scripts como log simple
    ImGui::SeparatorText("Scripts de la escena");

    if (!scene_) {
        ImGui::TextDisabled("Sin escena");
    } else {
        ListaDE<GameObject*>* objs = scene_->getGameObjects();
        if (!objs || objs->isEmpty()) {
            ImGui::TextDisabled("No hay objetos");
        } else {
            bool alguno = false;
            Position<GameObject*>* pos = objs->first();
            while (pos && pos->getElement()) {
                GameObject* objeto = pos->getElement();
                ListaDE<Component*>* componentes = objeto->getComponents();
                if (componentes && !componentes->isEmpty()) {
                    Position<Component*>* posicion = componentes->first();
                    while (posicion) {
                        if (Script* s =
                                dynamic_cast<Script*>(posicion->getElement())) {
                            alguno = true;
                            std::string fuente = s->rutaFuente();
                            std::string estado;
                            if (compilando_ && fuente == actual_)
                                estado = "cargando " +
                                         std::to_string(
                                             BarraProgresoTexto::porcentaje(
                                                 hecha_, total_)) +
                                         "%";
                            else if (s->estaCargado())
                                estado = "cargado";
                            else if (!s->ultimoError().empty())
                                estado = "error";
                            else
                                estado = "esperando";
                            ImGui::BulletText(
                                "%s  [%s]",
                                fuente.empty() ? "(sin fuente)"
                                               : fuente.c_str(),
                                estado.c_str());
                            if (!s->ultimoError().empty())
                                ImGui::TextWrapped("-> %s",
                                                   s->ultimoError().c_str());
                        }
                        posicion = posicion != componentes->last()
                                       ? componentes->next(posicion)
                                       : nullptr;
                    }
                }
                pos = (pos != objs->last()) ? objs->next(pos) : nullptr;
            }
            if (!alguno) ImGui::TextDisabled("No hay scripts en la escena");
        }
    }

    // Mensaje temporal (progreso de compilacion o exportacion)
    if (!mensajeTemporal_.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s",
                           mensajeTemporal_.c_str());
    }
}

void StatusBarInterface::printGUI() {
    if (stateGUI) {
        initGUI();
        contentGUI();
        endGUI();
    }

    // Temporizador para mensaje temporal
    if (temporizadorMensaje_ > 0.0f) {
        temporizadorMensaje_ -= Time::getDeltaTime();
        if (temporizadorMensaje_ <= 0.0f) {
            mensajeTemporal_.clear();
            temporizadorMensaje_ = 0.0f;
        }
    }

    // Notificar cambios de visibilidad al bus de GUI
    if (busEditor && stateGUI != estadoPublicado_) {
        EditorEvent ev;
        ev.type = EditorEventType::VentanaEstadoCambio;
        ev.nombreVentana = WindowNames::Status;
        ev.abierta = stateGUI;
        busEditor->publish(ev);
        estadoPublicado_ = stateGUI;
    }
}