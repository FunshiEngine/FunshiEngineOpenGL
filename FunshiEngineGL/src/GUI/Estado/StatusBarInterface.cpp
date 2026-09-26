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
    bool overlayProgreso, bool overlayResultado) {
    compilando_ = enCurso;
    actual_ = actual;
    hecha_ = hecha;
    total_ = total;
    resultados_ = resultados;
    mostrarProgreso_ = overlayProgreso;
    mostrarResultado_ = overlayResultado;
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

void StatusBarInterface::dibujarScripts() {
    ImGui::SeparatorText("Scripts de la escena");

    // Progreso de la compilacion en curso (la cola de GameScene muestra un
    // frame "Mostrar" antes de bloquear con g++/javac, por eso se ve).
    if (compilando_ && total_ > 0) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Compilando... (%zu de %zu)",
                      hecha_ + 1, total_);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                              ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::ProgressBar(total_ > 0 ? static_cast<float>(hecha_) /
                                            static_cast<float>(total_)
                                      : 0.0f,
                           ImVec2(-1.0f, 0.0f), buf);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::TextWrapped("%s", actual_.c_str());
    }

    if (!scene_) {
        ImGui::TextDisabled("Sin escena");
        return;
    }
    ListaDE<GameObject*>* objs = scene_->getGameObjects();
    if (!objs || objs->isEmpty()) {
        ImGui::TextDisabled("No hay objetos");
        return;
    }

    bool alguno = false;
    Position<GameObject*>* pos = objs->first();
    while (pos && pos->getElement()) {
        GameObject* objeto = pos->getElement();
        if (Script* s = objeto->getComponent<Script>()) {
            alguno = true;
            std::string fuente = s->rutaFuente();
            std::string estado = "En espera";
            if (compilando_ && fuente == actual_)
                estado = "Compilando...";
            else if (s->estaCargado())
                estado = "Cargado";
            else if (!s->ultimoError().empty())
                estado = "Error";
            ImGui::BulletText("%s  [%s]",
                              fuente.empty() ? "(sin fuente)" : fuente.c_str(),
                              estado.c_str());
            if (!s->ultimoError().empty())
                ImGui::TextWrapped("-> %s", s->ultimoError().c_str());
        }
        pos = (pos != objs->last()) ? objs->next(pos) : nullptr;
    }
    if (!alguno) ImGui::TextDisabled("No hay scripts en la escena");
}

void StatusBarInterface::dibujarResultados() {
    if (resultados_.empty()) return;
    ImGui::SeparatorText("Ultima compilacion");
    const std::size_t maxMostrar = 8;
    const std::size_t inicio =
        resultados_.size() > maxMostrar ? resultados_.size() - maxMostrar : 0;
    for (std::size_t i = inicio; i < resultados_.size(); ++i) {
        const ScriptRuntime::ResultadoCarga& r = resultados_[i];
        ImGui::BulletText("%s %s", r.ok ? "[OK]" : "[FALLO]",
                          r.nombre.empty() ? "(sin nombre)" : r.nombre.c_str());
        if (!r.mensaje.empty()) {
            std::string corto = r.mensaje;
            if (corto.size() > 300) corto.resize(300);
            ImGui::TextWrapped("   %s", corto.c_str());
        }
    }
}

void StatusBarInterface::contentGUI() {
    dibujarToolchain();
    dibujarScripts();
    dibujarResultados();
    if (!mensajeTemporal_.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s",
                           mensajeTemporal_.c_str());
    }
}

// Overlay de carga que se dibuja centrado al pulsar "Activar": barra de
// progreso bonita mientras la cola trabaja y, al terminar, un aviso breve con
// el resultado para que el usuario no dependa de la consola.
void StatusBarInterface::dibujarOverlayCarga() {
    if (!mostrarProgreso_ && !mostrarResultado_) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 18));
    // Fondo y borde derivados del tema activo (TemaEditor): el overlay deja
    // de tener un azul fijo y acompana el acento elegido por el usuario.
    ImVec4 fondoOverlay = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    fondoOverlay.w = 0.96f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, fondoOverlay);
    ImVec4 bordeOverlay = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    bordeOverlay.w = 0.6f;
    ImGui::PushStyleColor(ImGuiCol_Border, bordeOverlay);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus;
    bool abierto = true;
    ImGui::Begin("##OverlayCargaScripts", &abierto, flags);

    if (mostrarProgreso_) {
        const int puntos = 1 + (static_cast<int>(ImGui::GetTime() * 3.0f) % 3);
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        ImGui::TextUnformatted("Compilando scripts");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextUnformatted(std::string(puntos, '.').c_str());

        char buf[48];
        if (total_ > 0) {
            const float frac =
                static_cast<float>(hecha_) / static_cast<float>(total_);
            std::snprintf(buf, sizeof(buf), "%.0f%%", frac * 100.0f);
        } else {
            std::snprintf(buf, sizeof(buf), "-");
        }

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                              ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::ProgressBar(
            total_ > 0 ? static_cast<float>(hecha_) /
                             static_cast<float>(total_)
                       : 0.0f,
            ImVec2(360, 18), buf);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Spacing();
        if (total_ > 0) {
            ImGui::TextDisabled("Script %zu de %zu", hecha_ + 1, total_);
            ImGui::TextWrapped("%s", actual_.c_str());
        } else {
            ImGui::TextDisabled("Verificando que los scripts esten al dia...");
        }
    } else if (mostrarResultado_) {
        std::size_t ok = 0;
        std::size_t fallo = 0;
        for (const ScriptRuntime::ResultadoCarga& r : resultados_) {
            if (r.ok) ++ok;
            else ++fallo;
        }
        char buf[96];
        if (fallo == 0) {
            std::snprintf(buf, sizeof(buf), ok == 1 ? "Listo: %zu script actualizado"
                                                    : "Listo: %zu scripts actualizados",
                          ok);
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.35f, 0.85f, 0.5f, 1.0f));
        } else {
            std::snprintf(buf, sizeof(buf),
                          ok == 1 ? "Listo: %zu actualizado, %zu con error"
                                  : "Listo: %zu actualizados, %zu con error",
                          ok, fallo);
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        }
        ImGui::TextUnformatted(buf);
        ImGui::PopStyleColor();
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void StatusBarInterface::printGUI() {
    if (stateGUI) {
        initGUI();
        contentGUI();
        endGUI();
    }
    // El overlay de carga es independiente de la ventana "Estado": se muestra
    // aunque el usuario haya cerrado la ventana.
    dibujarOverlayCarga();

    // Temporizador para mensaje temporal (decrementa en segundos).
    if (temporizadorMensaje_ > 0.0f) {
        temporizadorMensaje_ -= Time::getDeltaTime();
        if (temporizadorMensaje_ <= 0.0f) {
            mensajeTemporal_.clear();
            temporizadorMensaje_ = 0.0f;
        }
    }

    // Notificar cambios de visibilidad al bus de GUI: cerrar con la 'X' muda
    // stateGUI por dentro de ImGui::Begin y nadie mas la veria. El overlay no
    // cambia la visibilidad de la ventana, asi que aqui no hay ruido extra.
    if (busEditor && stateGUI != estadoPublicado_) {
        EditorEvent ev;
        ev.type = EditorEventType::VentanaEstadoCambio;
        ev.nombreVentana = WindowNames::Status;
        ev.abierta = stateGUI;
        busEditor->publish(ev);
        estadoPublicado_ = stateGUI;
    }
}

void StatusBarInterface::mostrarMensaje(const std::string& mensaje) {
    mensajeTemporal_ = mensaje;
    temporizadorMensaje_ = 4.0f; // visible 4 segundos
}