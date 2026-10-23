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
// Pruebas headless del componente AudioSource (fuente de sonido).
//
// Casos:
//   - AudioSource sin motor inyectado: reproducir() no hace nada
//     (no encola comando, no cambia estado).
//   - AudioSource con motor inyectado y archivo existente: reproducible()
//     encola comando y pasa a estado sonando.
//   - AudioSource con motor inyectado y archivo inexistente: reproducible()
//     no encola comando (AudioEngine devuelve -1).
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "../FunshiEngineGL/src/Audio/AudioEngine.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/AudioSource.h"
#include "TempPruebas.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

void probarSinMotor() {
    TempPruebas::CarpetaPrueba carpeta("funshi_audio_source_sin_motor");
    const std::string archivo = (carpeta.ruta() / "clip.wav").string();
    std::ofstream(archivo) << "contenido";

    AudioSource source;
    source.setRutaClip(archivo);
    // Sin motor: reproducir() debe retornar sin encolar nada y sin cambiar
    // el estado (isReproduciendo() sigue false).
    source.reproducir();
    CHECK(!source.isReproduciendo(),
          "sin motor, reproducir() no pone en estado sonando");
    CHECK(source.getRutaClip() == archivo,
          "sin motor, la ruta del clip se conserva");
}

void probarConMotorArchivoExistente() {
    TempPruebas::CarpetaPrueba carpeta("funshi_audio_source_con_motor");
    const std::string archivo = (carpeta.ruta() / "clip.wav").string();
    std::ofstream(archivo) << "contenido";

    AudioEngine motor;  // NullAudioBackend por defecto
    motor.esperarPilaVacia();  // drena el comando Iniciar del ctor

    AudioSource source;
    source.setRutaClip(archivo);
    source.setMotor(&motor);

    source.reproducir();
    CHECK(source.isReproduciendo(),
          "con motor y archivo existente, reproducir() pone en estado sonando");

    source.detener();
    CHECK(!source.isReproduciendo(),
          "despues de detener(), isReproduciendo() es false");
}

void probarConMotorArchivoInexistente() {
    TempPruebas::CarpetaPrueba carpeta("funshi_audio_source_inexistente");
    const std::string inexistente =
        (carpeta.ruta() / "no_existe.wav").string();

    AudioEngine motor;
    motor.esperarPilaVacia();

    AudioSource source;
    source.setRutaClip(inexistente);
    source.setMotor(&motor);

    source.reproducir();
    CHECK(!source.isReproduciendo(),
          "con archivo inexistente, reproducir() no pone en estado sonando");
}

void probarBanderaEspacial() {
    AudioSource source;
    CHECK(!source.isEspacial(),
          "por defecto el sonido es global (2D), no espacial");
    source.setEspacial(true);
    CHECK(source.isEspacial(),
          "setEspacial(true) se lee con isEspacial()");
    source.setEspacial(false);
    CHECK(!source.isEspacial(),
          "setEspacial(false) vuelve al comportamiento global (2D)");
}

void probarAudioEngineConPosicion() {
    TempPruebas::CarpetaPrueba carpeta("funshi_audio_engine_posicion");
    const std::string archivo = (carpeta.ruta() / "clip.wav").string();
    std::ofstream(archivo) << "contenido";

    AudioEngine motor;
    motor.esperarPilaVacia();

    // Listener y posicion de un sonido inexistente: no fallan.
    motor.setListenerPosition(1.f, 2.f, 3.f);
    motor.setPosicionSonido(999, 4.f, 5.f, 6.f);

    // Reproducir con posicion: el motor copia la posicion en el comando y
    // devuelve handle.
    float pos[3] = {7.f, 8.f, 9.f};
    const int handle = motor.reproducir(archivo, 0.5f, true, pos);
    CHECK(handle >= 0, "reproducir() con posicion devuelve handle");
    motor.detener(handle);
    motor.esperarPilaVacia();
    CHECK(motor.pendientes() == 0, "la cola de comando de audio queda vacia");
}

}  // namespace

int main() {
    std::cout << "== AUDIO SOURCE TESTS ==" << std::endl;
    probarSinMotor();
    probarConMotorArchivoExistente();
    probarConMotorArchivoInexistente();
    probarBanderaEspacial();
    probarAudioEngineConPosicion();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "AUDIO SOURCE TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
