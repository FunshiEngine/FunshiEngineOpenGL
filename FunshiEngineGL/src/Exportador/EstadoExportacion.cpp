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
#include "EstadoExportacion.h"

void EstadoExportacion::iniciar() {
    fase_ = Fase::Exportando;
    resultado_ = Resultado::Pendiente;
    progreso_ = 0.0f;
    etapa_ = "Iniciando...";
    mensaje_.clear();
}

void EstadoExportacion::registrarProgreso(float progreso,
                                           const std::string& etapa) {
    progreso_ = progreso;
    if (!etapa.empty()) etapa_ = etapa;
}

void EstadoExportacion::registrarLog(const std::string& mensaje) {
    mensaje_ = mensaje;
}

void EstadoExportacion::finalizar(bool exito, const std::string& mensaje) {
    // El resultado se conserva: la fase pasa a Resultado y el motivo queda
    // disponible para que la interfaz lo muestre. Perderlo aqui es lo que
    // hace que un fallo parezca que no paso nada.
    fase_ = Fase::Resultado;
    resultado_ = exito ? Resultado::Exito : Resultado::Error;
    progreso_ = exito ? 1.0f : progreso_;
    etapa_ = exito ? "Completado" : "Error";
    if (!mensaje.empty()) mensaje_ = mensaje;
}

void EstadoExportacion::reiniciar() {
    fase_ = Fase::Configuracion;
    resultado_ = Resultado::Pendiente;
    progreso_ = 0.0f;
    etapa_.clear();
    mensaje_.clear();
}
