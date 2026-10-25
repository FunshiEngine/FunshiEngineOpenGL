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
// Pruebas headless del pool de hilos del motor:
//
//  - cada tarea encolada se ejecuta exactamente una vez,
//  - una tarea que lanza una excepcion no tumba el pool (se registra y se
//    sigue),
//  - al cerrar se drena lo pendiente y se esperan los hilos,
//  - esperar() no vuelve hasta que no queda nada por hacer,
//  - el hilo principal se reconoce (y un hilo del pool no lo es).

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Diagnosticos/RegistroDiagnosticos.h"
#include "../FunshiEngineGL/src/Hilos/HiloActual.h"
#include "../FunshiEngineGL/src/Hilos/MotorHilos.h"

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Cada tarea encolada se ejecuta exactamente una vez.
static void testTareasSeEjecutanUnaVez() {
    std::cout << "-- pool: cada tarea se ejecuta exactamente una vez"
              << std::endl;

    MotorHilos motor(4);
    std::atomic<int> ejecutadas{0};
    constexpr int kTareas = 500;
    for (int i = 0; i < kTareas; ++i)
        motor.encolar([&ejecutadas] { ++ejecutadas; });
    motor.esperar();

    CHECK(ejecutadas.load() == kTareas, "se ejecutan todas las tareas");
    CHECK(motor.tareasEjecutadas() == static_cast<std::size_t>(kTareas),
          "el contador de tareas ejecutadas cuadra");
    CHECK(motor.tareasFallidas() == 0, "no hay tareas fallidas");
    CHECK(motor.pendientes() == 0, "no queda nada pendiente");
}

// Una tarea que lanza no tumba el pool: se registra y se sigue.
static void testTareaQueLanzaNoTumbaElPool() {
    std::cout << "-- pool: una tarea que lanza no tumba el pool" << std::endl;

    TempPruebas::CarpetaPrueba carpeta("funshi_hilos_diagnostico");
    const std::filesystem::path archivo =
        carpeta.ruta() / "diagnosticos.jsonl";
    RegistroDiagnosticos registro(archivo);

    MotorHilos motor(2, &registro);
    std::atomic<int> ejecutadas{0};

    CHECK(motor.encolar([] { throw std::runtime_error("fallo de prueba"); }),
          "la tarea que falla se encola");
    for (int i = 0; i < 20; ++i)
        motor.encolar([&ejecutadas] { ++ejecutadas; });
    motor.esperar();

    CHECK(ejecutadas.load() == 20,
          "las tareas posteriores a la que falla se ejecutan");
    CHECK(motor.tareasEjecutadas() == 21,
          "la tarea que lanzo tambien cuenta como ejecutada");
    CHECK(motor.tareasFallidas() == 1, "la tarea que lanzo se cuenta como fallida");

    const std::vector<EventoDiagnostico> ultimos = registro.ultimos(10);
    bool hayFallo = false;
    for (const EventoDiagnostico& evento : ultimos)
        hayFallo = hayFallo || evento.modulo == "Hilos";
    CHECK(hayFallo, "el fallo de la tarea queda registrado en el diagnostico");
}

// Al cerrar se drena lo pendiente.
static void testCierreDrenaLoPendiente() {
    std::cout << "-- pool: al cerrar se drena lo pendiente" << std::endl;

    std::atomic<int> ejecutadas{0};
    {
        MotorHilos motor(2);
        for (int i = 0; i < 100; ++i)
            motor.encolar([&ejecutadas] { ++ejecutadas; });
        motor.cerrar();
        CHECK(ejecutadas.load() == 100, "las tareas encoladas se ejecutan al cerrar");
        CHECK(motor.cerrado(), "el motor queda cerrado");
        CHECK(!motor.encolar([&ejecutadas] { ++ejecutadas; }),
              "despues de cerrar no se aceptan tareas");
        CHECK(ejecutadas.load() == 100,
              "la tarea rechazada no se ejecuta");
    }
}

// esperar() no vuelve hasta que no queda nada por hacer.
static void testEspera() {
    std::cout << "-- pool: esperar no vuelve antes de tiempo" << std::endl;

    MotorHilos motor(2);
    std::mutex mutex;
    std::condition_variable cv;
    bool liberar = false;
    std::atomic<bool> terminada{false};

    motor.encolar([&] {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return liberar; });
        terminada = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK(!terminada.load(), "la tarea sigue en ejecucion");

    {
        std::lock_guard<std::mutex> lock(mutex);
        liberar = true;
    }
    cv.notify_all();
    motor.esperar();
    CHECK(terminada.load(), "esperar devuelve cuando la tarea termino");
}

// El hilo principal se reconoce; un hilo del pool, no.
static void testHiloPrincipal() {
    std::cout << "-- hilos: el hilo principal se reconoce" << std::endl;

    Hilos::fijarHiloPrincipal();
    CHECK(Hilos::esHiloPrincipal(),
          "tras registrarse, el hilo principal se reconoce");

    MotorHilos motor(1);
    std::atomic<bool> enPoolEsPrincipal{true};
    motor.encolar([&] { enPoolEsPrincipal = Hilos::esHiloPrincipal(); });
    motor.esperar();

    CHECK(!enPoolEsPrincipal.load(),
          "un hilo del pool no se confunde con el principal");
}

int main() {
    testTareasSeEjecutanUnaVez();
    testTareaQueLanzaNoTumbaElPool();
    testCierreDrenaLoPendiente();
    testEspera();
    testHiloPrincipal();

    std::cout << "HilosTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}
