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
// Pruebas headless de la fachada de audio (AudioEngine) montando el backend
// sin hardware (NullAudioBackend). Sin pila grafica ni dispositivo: el hilo de
// audio del motor procesa la cola de comandos igual (no-op), asi que se prueba
// el contrato de la fachada: registro de clips, encolado no bloqueante,
// handles, detencion, volumen y limpieza.
//
// Casos:
//   - Creacion con/in sin backend explicito (nullptr monta NullAudioBackend).
//   - Registro y consulta de clips (clipRegistrado, nombresClips, eliminar).
//   - reproducir() de un clip desconocido devuelve -1 sin encolar nada.
//   - reproducir() de un clip conocido devuelve un handle >= 0 y el motor
//     drena la cola (esperarPilaVacia / pendientes()).
//   - detener()/detenerTodo()/setVolumenMaestro() encolan y se processan.
//   - limpiarClips() vacia el catalogo.

#include <iostream>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Audio/AudioEngine.h"

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

bool contiene(const std::vector<std::string>& v, const std::string& s) {
    for (const std::string& x : v)
        if (x == s) return true;
    return false;
}

void probarCicloComandos() {
    AudioEngine motor;  // nullptr => NullAudioBackend
    // El ctor encola "Iniciar"; se drena antes de medir pendientes().
    motor.esperarPilaVacia();
    motor.registrarClip("click", "/tmp/funshi_click.wav");
    motor.registrarClip("musica", "/tmp/funshi_musica.wav");

    CHECK(motor.clipRegistrado("click"), "clip registrado visible");
    CHECK(motor.clipRegistrado("musica"), "clip registrado visible 2");
    CHECK(!motor.clipRegistrado("inexistente"),
          "clip no registrado no se ve");

    std::vector<std::string> nombres = motor.nombresClips();
    CHECK(nombres.size() == 2, "nombresClips tiene los dos clips");
    CHECK(contiene(nombres, "click") && contiene(nombres, "musica"),
          "nombresClips contiene ambos nombres");

    // Clip desconocido: -1 y no se encola (no bloquea).
    CHECK(motor.reproducir("no_existo") == -1,
          "reproducir clip desconocido devuelve -1");
    CHECK(motor.pendientes() == 0,
          "reproducir desconocido no encola comandos");

    // Clip conocido: handle valido y la cola se drena.
    const int h1 = motor.reproducir("click");
    CHECK(h1 >= 0, "reproducir clip conocido devuelve handle");
    const int h2 = motor.reproducir("musica", 0.5f, true);
    CHECK(h2 >= 0 && h2 != h1, "handles distintos por reproduccion");
    motor.esperarPilaVacia();
    CHECK(motor.pendientes() == 0,
          "la cola queda vacia tras esperarPilaVacia");

    motor.detener(h1);
    motor.detener(h2);
    motor.setVolumenMaestro(0.75f);
    motor.detenerTodo();
    motor.esperarPilaVacia();
    CHECK(motor.pendientes() == 0,
          "detener/volumen drenan la cola");

    CHECK(motor.eliminarClip("click"), "eliminarClip borra el clip");
    CHECK(!motor.clipRegistrado("click"),
          "el clip eliminado ya no se consulta");
    motor.limpiarClips();
    CHECK(motor.nombresClips().empty(), "limpiarClips vacia el catalogo");
}

void probarBackendExplicito() {
    auto motor = std::make_unique<AudioEngine>();
    motor->registrarClip("sonido", "/tmp/funshi_s.wav");
    const int h = motor->reproducir("sonido", 1.0f, false);
    CHECK(h >= 0, "backend por defecto reproduce y devuelve handle");
    motor->esperarPilaVacia();
    CHECK(motor->pendientes() == 0, "cola drenada con backend por defecto");
}

}  // namespace

int main() {
    std::cout << "== AUDIO ENGINE TESTS ==" << std::endl;
    probarCicloComandos();
    probarBackendExplicito();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "AUDIO ENGINE TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}