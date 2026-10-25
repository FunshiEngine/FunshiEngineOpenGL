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
#ifndef MOTORHILOS_H
#define MOTORHILOS_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class RegistroDiagnosticos;

// Pool de hilos del motor.
//
// El motor es monohilo salvo en tres casos (el hilo de audio, el de
// exportacion y el lector de terminal), y cada uno montaba su propio
// mecanismo. Este pool concentra ese trabajo: se encolan tareas y las
// ejecutan unos pocos hilos creados al arrancar y apagados de forma ordenada.
//
// Reglas del pool:
//   - una tarea encolada se ejecuta exactamente una vez,
//   - una tarea que lanza una excepcion no tumba el pool: se registra por el
//     canal de diagnostico y se sigue con la siguiente,
//   - al cerrar se drena lo pendiente y se espera a los hilos. Nunca se
//     abandonan con detach: un hilo que sobrevive al motor ejecutaria codigo
//     de objetos ya destruidos.
//
// El pool no es un singleton: se inyecta donde se usa, como el resto de los
// servicios del motor.
class MotorHilos {
public:
    using Tarea = std::function<void()>;

    // numHilos = 0: se deduce del hardware (nucleos disponibles menos el del
    // hilo principal, con un maximo y un minimo). registro = nullptr: los
    // fallos se reportan en el registro de diagnostico global.
    explicit MotorHilos(std::size_t numHilos = 0,
                        RegistroDiagnosticos* registro = nullptr);
    ~MotorHilos();

    MotorHilos(const MotorHilos&) = delete;
    MotorHilos& operator=(const MotorHilos&) = delete;

    // Encola una tarea. Devuelve false si el motor ya esta cerrando (la tarea
    // no se ejecutaria nunca) o si la tarea esta vacia.
    bool encolar(Tarea tarea);

    // Espera a que no queden tareas en la cola ni en ejecucion.
    void esperar();

    // Cierra el motor: deja de aceptar tareas, drena la cola y espera a los
    // hilos. Es idempotente.
    void cerrar();

    std::size_t numHilos() const noexcept;
    std::size_t pendientes() const noexcept;
    std::size_t tareasEjecutadas() const noexcept;
    std::size_t tareasFallidas() const noexcept;
    bool cerrado() const noexcept;

private:
    mutable std::mutex mutex_;
    std::condition_variable cvTrabajo_;  // avisa a los hilos que hay tarea
    std::condition_variable cvVacio_;    // avisa a quien espera que acabe
    std::deque<Tarea> cola_;
    std::vector<std::thread> hilos_;
    RegistroDiagnosticos* registro_ = nullptr;
    std::size_t enEjecucion_ = 0;
    std::size_t tareasEjecutadas_ = 0;
    std::size_t tareasFallidas_ = 0;
    bool cerrando_ = false;

    void bucleHilo();
    void ejecutarTarea(const Tarea& tarea);
    void registrarFallo(const std::string& codigo, const std::string& mensaje);
};

#endif  // MOTORHILOS_H
