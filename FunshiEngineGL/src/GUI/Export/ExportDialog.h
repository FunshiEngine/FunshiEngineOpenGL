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
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include "Exportador/GameExporter.h"
#include "Configuracion/EditorConfig.h"

class ExportDialog {
public:
    struct Resultado {
        bool exportar = false;
        GameExporter::Config config;
    };

    using Callback = std::function<void(const Resultado&)>;

    explicit ExportDialog(Callback onCerrar, const std::string& proyectoActual = "");
    ~ExportDialog();

    void render();

    void setProyectoActual(const std::string& proyecto) { proyectoActual_ = proyecto; }

private:
    struct LogMsg { std::string msg; };
    struct ProgresoMsg { float p; std::string etapa; };
    struct FinalizadoMsg { bool exito; std::string msg; };

    Callback onCerrar_;
    std::string proyectoActual_;
    bool abierto_ = true;
    char nombreEjecutable_[256] = "MiJuego";
    char nombreProyectoExportado_[256] = "Exportacion";
    int plataformaIdx_ = 0; // 0 = Linux, 1 = Windows
    bool exportando_ = false;
    std::unique_ptr<GameExporter> exporter_;
    float progreso_ = 0.0f;
    std::string etapaActual_;
    std::string ultimoLog_;

    // Colas thread-safe para comunicacion hilo exportacion -> hilo UI
    std::mutex mtx_;
    std::queue<LogMsg> logQueue_;
    std::queue<ProgresoMsg> progresoQueue_;
    std::queue<FinalizadoMsg> finalizadoQueue_;

    void iniciarExportacion();
    void actualizarProgreso(float p, const std::string& etapa);
    void finalizarExportacion(bool exito, const std::string& msg);
    void agregarLog(const std::string& msg);

    // Encolar desde hilo de exportacion
    void encolarLog(const std::string& msg);
    void encolarProgreso(float p, const std::string& etapa);
    void encolarFinalizado(bool exito, const std::string& msg);

    // Procesar colas en hilo UI (llamado desde render)
    void procesarColas();

    void renderSpinner(float radius = 16.0f, float thickness = 3.0f);
};

#endif // EXPORTDIALOG_H