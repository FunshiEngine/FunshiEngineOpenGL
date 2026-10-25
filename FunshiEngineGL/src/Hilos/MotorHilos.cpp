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
#include "MotorHilos.h"

#include "Diagnosticos/RegistroDiagnosticos.h"

#include <algorithm>
#include <exception>

namespace {
// Tope de hilos del pool por defecto. El pool es para trabajo en segundo
// plano: el motor reserva un nucleo para el hilo principal (render, interfaz
// y entrada) y no hace falta mas para el trabajo en segundo plano.
constexpr std::size_t kHilosMaximosPorDefecto = 4;

std::size_t numeroDeHilosPorDefecto() {
    const unsigned nucleos = std::thread::hardware_concurrency();
    if (nucleos <= 1) return 1;
    const std::size_t disponibles = static_cast<std::size_t>(nucleos) - 1u;
    return std::min(disponibles, kHilosMaximosPorDefecto);
}
}  // namespace

MotorHilos::MotorHilos(std::size_t numHilos, RegistroDiagnosticos* registro)
    : registro_(registro) {
    const std::size_t cantidad =
        numHilos > 0 ? numHilos : numeroDeHilosPorDefecto();
    hilos_.reserve(cantidad);
    for (std::size_t i = 0; i < cantidad; ++i)
        hilos_.emplace_back(&MotorHilos::bucleHilo, this);
}

MotorHilos::~MotorHilos() {
    cerrar();
}

bool MotorHilos::encolar(Tarea tarea) {
    if (!tarea) return false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cerrando_) return false;
        cola_.push_back(std::move(tarea));
    }
    cvTrabajo_.notify_one();
    return true;
}

void MotorHilos::esperar() {
    std::unique_lock<std::mutex> lock(mutex_);
    cvVacio_.wait(lock,
                  [this] { return cola_.empty() && enEjecucion_ == 0; });
}

void MotorHilos::cerrar() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cerrando_) return;
        cerrando_ = true;
    }
    // Los hilos esperan a que se vacie la cola: con cerrando_ puesto, drenan
    // lo pendiente y solo entonces salen.
    cvTrabajo_.notify_all();

    const std::thread::id propio = std::this_thread::get_id();
    for (std::thread& hilo : hilos_) {
        if (!hilo.joinable()) continue;
        if (hilo.get_id() == propio) {
            // Cerrar desde una tarea del propio pool: ese hilo no puede
            // esperarse a si mismo. Se abandona (ya esta saliendo).
            hilo.detach();
            continue;
        }
        hilo.join();
    }
    hilos_.clear();
}

std::size_t MotorHilos::numHilos() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return hilos_.size();
}

std::size_t MotorHilos::pendientes() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return cola_.size() + enEjecucion_;
}

std::size_t MotorHilos::tareasEjecutadas() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return tareasEjecutadas_;
}

std::size_t MotorHilos::tareasFallidas() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return tareasFallidas_;
}

bool MotorHilos::cerrado() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return cerrando_;
}

void MotorHilos::bucleHilo() {
    while (true) {
        Tarea tarea;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cvTrabajo_.wait(lock,
                            [this] { return cerrando_ || !cola_.empty(); });
            if (cola_.empty()) {
                // Solo se sale cuando ya no queda nada pendiente.
                if (cerrando_) return;
                continue;
            }
            tarea = std::move(cola_.front());
            cola_.pop_front();
            ++enEjecucion_;
        }

        ejecutarTarea(tarea);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            --enEjecucion_;
            ++tareasEjecutadas_;
        }
        cvVacio_.notify_all();
    }
}

void MotorHilos::ejecutarTarea(const Tarea& tarea) {
    try {
        tarea();
    } catch (const std::exception& excepcion) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++tareasFallidas_;
        }
        registrarFallo("hilos-tarea",
                       std::string("La tarea lanzo una excepcion: ") +
                           excepcion.what());
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++tareasFallidas_;
        }
        registrarFallo("hilos-tarea",
                       "La tarea lanzo una excepcion desconocida");
    }
}

void MotorHilos::registrarFallo(const std::string& codigo,
                                 const std::string& mensaje) {
    if (registro_)
        registrarFalloDiagnostico(*registro_, "Hilos", codigo, mensaje);
    else
        registrarFalloDiagnostico("Hilos", codigo, mensaje);
}
