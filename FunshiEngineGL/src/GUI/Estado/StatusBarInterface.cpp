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
#include "../../Diagnosticos/RegistroDiagnosticos.h"

#include <cstdio>
#include <fstream>
#include <imgui.h>
#include <nlohmann/json.hpp>

StatusBarInterface::StatusBarInterface(bool stateGUI)
    : GeneralUserInterface(WindowNames::Status, stateGUI,
                           ImGuiWindowFlags_MenuBar),
      estadoPublicado_(stateGUI),
      rutaRespaldo_(RegistroDiagnosticos::archivoPorDefecto().parent_path() /
                    "FunshiEngineGL_diagnosticos.respaldo.jsonl") {}

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

    dibujarDiagnosticos();

    // Mensaje temporal (progreso de compilacion o exportacion)
    if (!mensajeTemporal_.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s",
                           mensajeTemporal_.c_str());
    }
}

void StatusBarInterface::dibujarDiagnosticos() {
    ImGui::SeparatorText("Consola de estado/log");

    // Botones de gestion del log: limpiar (con respaldo) y recuperar.
    auto& registro = RegistroDiagnosticos::global();
    bool hayEventos = !registro.ultimos(1).empty();
    ImGui::SetNextItemWidth(140.0f);
    if (hayEventos && ImGui::Button("Limpiar log")) {
        // Leer el contenido actual del archivo de log.
        std::ifstream entrada(registro.archivo(),
                              std::ios::in | std::ios::binary);
        std::string contenidoActual(
            (std::istreambuf_iterator<char>(entrada)),
            std::istreambuf_iterator<char>());

        std::ofstream salida(rutaRespaldo_,
                             std::ios::out | std::ios::binary | std::ios::trunc);
        if (salida) {
            salida << contenidoActual;
            salida.flush();
            hayRespaldo_ = true;
        }
        // Vaciar el log principal (memoria + archivo).
        registro.limpiarTodo();
    }
    ImGui::SameLine();
    if (hayRespaldo_ && ImGui::Button("Recuperar")) {
        // Insertar el contenido del respaldo ANTES del contenido actual,
        // para que se muestre en orden cronologico correcto.
        auto eventosRespaldo =
            RegistroDiagnosticos::leerArchivo(rutaRespaldo_, 10000);
        if (!eventosRespaldo.empty()) {
            registro.insertarAntes(std::move(eventosRespaldo));
        }
        // Vaciar el respaldo tras recuperar.
        std::ofstream salida(rutaRespaldo_,
                             std::ios::out | std::ios::binary | std::ios::trunc);
        hayRespaldo_ = false;
    }
    if (hayRespaldo_) {
        ImGui::SameLine();
        ImGui::TextDisabled("(hay respaldo de log)");
    }

    const auto eventos = registro.ultimos(100);
    if (eventos.empty()) {
        ImGui::TextDisabled("No hay diagnósticos registrados");
        return;
    }

    const bool mostrarContenido =
        ImGui::BeginChild("DiagnosticosRecientes", ImVec2(0.0f, 220.0f), true);
    if (mostrarContenido) {
        for (std::size_t i = 0; i < eventos.size(); ++i) {
            const EventoDiagnostico& evento = eventos[i];
            ImGui::PushID(static_cast<int>(i));
            const std::string nivel =
                RegistroDiagnosticos::nombreNivel(evento.nivel);
            ImVec4 color(0.82f, 0.84f, 0.88f, 1.0f);
            if (evento.nivel == NivelDiagnostico::Advertencia)
                color = ImVec4(1.0f, 0.72f, 0.24f, 1.0f);
            else if (evento.nivel == NivelDiagnostico::Error ||
                     evento.nivel == NivelDiagnostico::Critico)
                color = ImVec4(1.0f, 0.35f, 0.32f, 1.0f);
            ImGui::TextColored(color, "[%s] [%s] %s", evento.fechaUtc.c_str(),
                               nivel.c_str(), evento.modulo.c_str());
            ImGui::TextWrapped("%s", evento.mensaje.c_str());
            if (!evento.proyecto.empty())
                ImGui::TextDisabled("Proyecto: %s", evento.proyecto.c_str());
            if (!evento.traza.empty() && ImGui::TreeNode("Traza")) {
                ImGui::TextWrapped("%s", evento.traza.c_str());
                ImGui::TreePop();
            }
            ImGui::Separator();
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
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
