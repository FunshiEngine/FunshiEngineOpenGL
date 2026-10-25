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
#include <queue>
#include <memory>
#include <mutex>
#include "Exportador/GameExporter.h"
#include "Exportador/EstadoExportacion.h"
#include "Configuracion/EditorConfig.h"
#include "../../Hilos/MotorHilos.h"

// Dialogo modal de exportacion de un juego.
//
// El dialogo no guarda el estado de la exportacion: lo delega en
// EstadoExportacion (dominio, sin ImGui) y solo dibuja. Asi las reglas del
// proceso (que el resultado no se pierda, que la fase dependa del hilo de
// exportacion) se pueden comprobar en headless.
class ExportDialog {
public:
    // Lo que el popup esta mostrando ahora. La vista se deduce del estado:
    // formulario mientras se configura, progreso mientras corre la
    // exportacion y resultado cuando termino (con exito o con error).
    enum class Vista { Formulario, Progreso, Resultado };

    explicit ExportDialog(const std::string& proyectoActual = "");
    ~ExportDialog();

    void render();

    void setProyectoActual(const std::string& proyecto) { proyectoActual_ = proyecto; }

    // Proyecto del que se exporta (el que la barra de menu reflejo). Vacio
    // cuando no hay ninguno: entonces la exportacion no puede lanzarse.
    const std::string& proyecto() const noexcept { return proyectoActual_; }

    // Estado de la exportacion (fase, resultado, progreso, mensaje).
    const EstadoExportacion& estado() const noexcept { return estado_; }

    // Lanza la exportacion con los datos del formulario. Si no se puede
    // lanzar (no hay proyecto, o su carpeta no existe), la peticion se
    // rechaza y el motivo queda en el estado (la interfaz lo muestra).
    void iniciarExportacion();

    // Vista que corresponde al estado actual.
    Vista vista() const noexcept;

    // El dueno lo consulta INMEDIATAMENTE despues de render() y recien ahi
    // destruye el dialogo: destruirlo dentro de render() (por ejemplo desde un
    // callback) dejaria el frame corriendo sobre memoria liberada.
    bool debeCerrarse() const noexcept { return cerrado_; }

private:
    struct LogMsg { std::string msg; };
    struct ProgresoMsg { float p; std::string etapa; };
    struct FinalizadoMsg { bool exito; std::string msg; };

    std::string proyectoActual_;
    bool abierto_ = true;
    bool cerrado_ = false;
    char nombreEjecutable_[256] = "MiJuego";
    char nombreProyectoExportado_[256] = "Exportacion";
    int plataformaIdx_ = 0; // 0 = Linux, 1 = Windows
    std::unique_ptr<GameExporter> exporter_;
    // Pool de hilos donde corre la exportacion (trabajo pesado en segundo
    // plano). Se crea al exportar y se inyecta en el exportador.
    std::unique_ptr<MotorHilos> motorHilos_;
    // Estado de la exportacion (dominio, sin ImGui).
    EstadoExportacion estado_;

    // Colas thread-safe para comunicacion hilo exportacion -> hilo UI
    std::mutex mtx_;
    std::queue<LogMsg> logQueue_;
    std::queue<ProgresoMsg> progresoQueue_;
    std::queue<FinalizadoMsg> finalizadoQueue_;

    // Encolar desde hilo de exportacion
    void encolarLog(const std::string& msg);
    void encolarProgreso(float p, const std::string& etapa);
    void encolarFinalizado(bool exito, const std::string& msg);

    // Procesar colas en hilo UI (llamado desde render)
    void procesarColas();

    // Partes del popup (una por vista).
    void renderFormulario();
    void renderProgreso();
    void renderResultado();

    void renderSpinner(float radius = 16.0f, float thickness = 3.0f);
};

#endif // EXPORTDIALOG_H
