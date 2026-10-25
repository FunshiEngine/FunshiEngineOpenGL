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
#include "ExportDialog.h"

#include "Exportador/PrecondicionesExportacion.h"

#include <imgui.h>
#include <chrono>
#include <memory>
#include <cmath>
#include <filesystem>

ExportDialog::ExportDialog(const std::string& proyectoActual)
    : proyectoActual_(proyectoActual) {}

ExportDialog::~ExportDialog() {
    // Unir el hilo de exportacion ANTES de que se destruyan el mutex y las
    // colas (van declarados despues de exporter_, asi que sus destructores
    // correrian primero): mientras el hilo siga encolando, esa memoria tiene
    // que seguir viva.
    exporter_.reset();
}

ExportDialog::Vista ExportDialog::vista() const noexcept {
    switch (estado_.fase()) {
    case EstadoExportacion::Fase::Exportando:
        return Vista::Progreso;
    case EstadoExportacion::Fase::Resultado:
        return Vista::Resultado;
    case EstadoExportacion::Fase::Configuracion:
        break;
    }
    return Vista::Formulario;
}

void ExportDialog::render() {
    if (!abierto_) return;

    // Procesar mensajes del hilo de exportacion en el hilo UI
    procesarColas();

    ImGui::OpenPopup("Exportar Juego");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // El boton X del titulo solo esta disponible fuera de una exportacion:
    // cerrar a mitad uniria el hilo en el destructor y congelaria el editor.
    if (ImGui::BeginPopupModal("Exportar Juego",
                               estado_.exportando() ? nullptr : &abierto_,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        switch (vista()) {
        case Vista::Progreso:
            renderProgreso();
            break;
        case Vista::Resultado:
            renderResultado();
            break;
        case Vista::Formulario:
            renderFormulario();
            break;
        }
        // Cerrar el popup de ImGui junto con el dialogo: sin esto, el estado
        // del popup quedaria abierto en el contexto de ImGui aunque el
        // dialogo deje de renderizarse.
        if (!abierto_) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (!abierto_) cerrado_ = true;
}

void ExportDialog::renderFormulario() {
    ImGui::Text("Configuración de exportación");
    ImGui::Separator();
    ImGui::Spacing();

    // De que proyecto se exporta: sin esta linea no se ve de donde sale la
    // exportacion (y un proyecto sin nombre es un fallo silencioso).
    if (proyectoActual_.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f),
                           "Proyecto: (ninguno)");
    } else {
        ImGui::Text("Proyecto: %s", proyectoActual_.c_str());
    }

    ImGui::InputText("Nombre del ejecutable", nombreEjecutable_, IM_ARRAYSIZE(nombreEjecutable_));
    ImGui::InputText("Nombre del proyecto exportado", nombreProyectoExportado_, IM_ARRAYSIZE(nombreProyectoExportado_));

    // La plataforma destino depende de la maquina: en Windows el toolchain
    // es el nativo (sale un .exe con sus DLL al lado); en Linux se puede
    // ademas cruzar a Windows.
    const std::vector<GameExporter::Plataforma> plataformas =
        GameExporter::plataformasDisponibles();
    std::vector<const char*> etiquetas;
    etiquetas.reserve(plataformas.size());
    for (GameExporter::Plataforma plataforma : plataformas) {
        etiquetas.push_back(GameExporter::etiquetaPlataforma(plataforma));
    }
    ImGui::Combo("Plataforma objetivo", &plataformaIdx_, etiquetas.data(),
                 static_cast<int>(etiquetas.size()));

    // El motivo de un intento anterior que no llego a exportarse: sin esto el
    // fallo se pierde y el dialogo parece que no respondio.
    if (estado_.hayMensaje() &&
        estado_.resultado() == EstadoExportacion::Resultado::Error) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s",
                           estado_.mensaje().c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("Exportar", ImVec2(120, 0))) {
        iniciarExportacion();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
        abierto_ = false;
    }
}

void ExportDialog::renderProgreso() {
    // UI de progreso con spinner indeterminado
    ImGui::Text("Exportando: %s", nombreProyectoExportado_);
    ImGui::Separator();
    ImGui::Spacing();

    // Spinner animado
    renderSpinner();
    ImGui::SameLine();
    ImGui::Text("  %s", estado_.etapa().c_str());

    ImGui::ProgressBar(estado_.progreso(), ImVec2(300, 0), "");

    if (estado_.hayMensaje()) {
        ImGui::Spacing();
        ImGui::BeginChild("Log", ImVec2(400, 150), true);
        ImGui::TextWrapped("%s", estado_.mensaje().c_str());
        ImGui::EndChild();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::BeginDisabled(estado_.exportando());
    if (ImGui::Button("Cerrar", ImVec2(120, 0))) {
        abierto_ = false;
    }
    ImGui::EndDisabled();
    if (estado_.exportando() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("La exportacion en curso debe terminar antes de cerrar");
    }
}

void ExportDialog::renderResultado() {
    const bool exito = estado_.resultado() == EstadoExportacion::Resultado::Exito;
    ImGui::Text("%s", exito ? "Exportación terminada" : "La exportación falló");
    ImGui::Separator();
    ImGui::Spacing();

    // El motivo (de exito o de error) queda siempre a la vista: es lo que
    // explica al usuario que paso con lo que pidio.
    if (estado_.hayMensaje()) {
        const ImVec4 color = exito ? ImVec4(0.45f, 0.95f, 0.55f, 1.0f)
                                   : ImVec4(1.0f, 0.45f, 0.45f, 1.0f);
        ImGui::TextColored(color, "%s", estado_.mensaje().c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("Cerrar", ImVec2(120, 0))) {
        abierto_ = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Exportar otra vez", ImVec2(180, 0))) {
        // Vuelta al formulario: el resultado anterior se descarta y el
        // proyecto sigue siendo el mismo.
        estado_.reiniciar();
    }
}

void ExportDialog::iniciarExportacion() {
    estado_.iniciar();

    // Precondiciones: sin proyecto (o sin su carpeta) no hay nada que
    // exportar. El motivo se conserva en el estado para que la interfaz lo
    // muestre en vez de dejar al usuario sin saber que paso.
    const std::string rutaProyecto =
        EditorConfig::directorioProyecto(proyectoActual_);
    std::string error;
    if (!PrecondicionesExportacion::comprobar(proyectoActual_, rutaProyecto,
                                              error)) {
        estado_.finalizar(false, error);
        return;
    }

    GameExporter::Config cfg;
    cfg.proyectoOrigen = rutaProyecto;
    cfg.nombreProyecto = proyectoActual_;
    cfg.nombreEjecutable = nombreEjecutable_;
    cfg.nombreProyectoExportado = nombreProyectoExportado_;
    const std::vector<GameExporter::Plataforma> plataformas =
        GameExporter::plataformasDisponibles();
    const int indice = (plataformaIdx_ >= 0 &&
                        plataformaIdx_ < static_cast<int>(plataformas.size()))
                           ? plataformaIdx_
                           : 0;
    cfg.plataforma = plataformas[static_cast<std::size_t>(indice)];
    cfg.directorioSalida = EditorConfig::directorioExportaciones() + "/" + std::string(nombreProyectoExportado_);

    // Callbacks thread-safe: usan cola para pasar mensajes al hilo principal
    cfg.onLog = [this](const std::string& msg) { encolarLog(msg); };
    cfg.onProgreso = [this](float p, const std::string& etapa) { encolarProgreso(p, etapa); };
    cfg.onFinalizado = [this](bool exito, const std::string& msg) { encolarFinalizado(exito, msg); };

    exporter_ = std::make_unique<GameExporter>(cfg);
    // La exportacion corre en el pool de hilos del motor: el trabajo pesado
    // (generar el proyecto, compilar, empaquetar) no bloquea el hilo del
    // editor, que se entera por los callbacks.
    if (!motorHilos_) motorHilos_ = std::make_unique<MotorHilos>(1);
    exporter_->setMotorHilos(motorHilos_.get());
    exporter_->iniciar();
}

// Thread-safe enqueue desde hilo de exportacion
void ExportDialog::encolarLog(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    logQueue_.push({msg});
}

void ExportDialog::encolarProgreso(float p, const std::string& etapa) {
    std::lock_guard<std::mutex> lock(mtx_);
    progresoQueue_.push({p, etapa});
}

void ExportDialog::encolarFinalizado(bool exito, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    finalizadoQueue_.push({exito, msg});
}

// Procesar colas en hilo UI
void ExportDialog::procesarColas() {
    std::lock_guard<std::mutex> lock(mtx_);
    while (!logQueue_.empty()) {
        estado_.registrarLog(logQueue_.front().msg);
        logQueue_.pop();
    }
    while (!progresoQueue_.empty()) {
        estado_.registrarProgreso(progresoQueue_.front().p,
                                  progresoQueue_.front().etapa);
        progresoQueue_.pop();
    }
    while (!finalizadoQueue_.empty()) {
        estado_.finalizar(finalizadoQueue_.front().exito,
                           finalizadoQueue_.front().msg);
        finalizadoQueue_.pop();
    }
}

void ExportDialog::renderSpinner(float radius, float thickness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 center = ImGui::GetCursorScreenPos();
    center.x += radius;
    center.y += radius;

    float t = ImGui::GetTime() * 8.0f;
    float a_min = 3.14159f * 2.0f * fmodf(t, 1.0f);
    float a_max = a_min + 3.14159f * 2.0f * 0.75f;

    draw_list->PathArcTo(center, radius - thickness, a_min, a_max);
    draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_ButtonHovered), 0, thickness);
    ImGui::Dummy(ImVec2(radius * 2, radius * 2));
}
