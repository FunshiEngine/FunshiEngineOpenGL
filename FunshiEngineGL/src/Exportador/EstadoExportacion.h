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
#ifndef ESTADOEXPORTACION_H
#define ESTADOEXPORTACION_H

#include <string>

// Estado de una exportacion de juego, tal como lo refleja el dialogo.
//
// Vive fuera de la interfaz (sin ImGui) para que sus reglas se puedan
// comprobar en headless. La regla que mas importa: el resultado de la
// exportacion (exito o error) no se pierde al terminar, porque es lo unico
// que le dice al usuario que paso con lo que pidio.
//
// Fases:
//   - Configuracion: se esta armando la peticion (formulario del dialogo).
//   - Exportando: la exportacion esta en curso (progreso y bitacora).
//   - Resultado: la exportacion termino; se conserva el mensaje final
//     (exito o error) hasta que el usuario cierra el dialogo o reconfigura.
class EstadoExportacion {
public:
    enum class Fase { Configuracion, Exportando, Resultado };
    enum class Resultado { Pendiente, Exito, Error };

    // Arranca una exportacion: descarta el resultado anterior, vuelve el
    // progreso a cero y pasa a la fase de exportacion.
    void iniciar();

    // Avance de la exportacion (0..1) y etapa en curso.
    void registrarProgreso(float progreso, const std::string& etapa);

    // Linea de la bitacora del exportador. La ultima leida es la que se
    // muestra (al terminar, la sustituye el mensaje final).
    void registrarLog(const std::string& mensaje);

    // La exportacion termino: se conserva el resultado y el motivo para que
    // la interfaz los muestre.
    void finalizar(bool exito, const std::string& mensaje);

    // Vuelve al formulario y descarta el resultado anterior (el usuario
    // reconfigura y reintenta).
    void reiniciar();

    Fase fase() const noexcept { return fase_; }
    Resultado resultado() const noexcept { return resultado_; }
    bool exportando() const noexcept { return fase_ == Fase::Exportando; }
    bool terminado() const noexcept { return fase_ == Fase::Resultado; }
    bool hayMensaje() const noexcept { return !mensaje_.empty(); }
    const std::string& mensaje() const noexcept { return mensaje_; }
    float progreso() const noexcept { return progreso_; }
    const std::string& etapa() const noexcept { return etapa_; }

private:
    Fase fase_ = Fase::Configuracion;
    Resultado resultado_ = Resultado::Pendiente;
    float progreso_ = 0.0f;
    std::string etapa_;
    std::string mensaje_;
};

#endif // ESTADOEXPORTACION_H
