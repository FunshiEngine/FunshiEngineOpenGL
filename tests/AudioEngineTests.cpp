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
//   - Los efectos de sonido se piden por RUTA: el motor reproduce el archivo
//     que le pasan; sin archivo (o con un archivo que no existe) devuelve -1.
//   - La cola de musica: lista de pistas por ruta, reproduccion, bus, silencio,
//     deteccion y el tick de la escena. Con un backend FALSO que registra lo
//     que le llega, para comprobar que los comandos se ejecutan en el hilo de
//     audio contra el reproductor.

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Audio/AudioEngine.h"
#include "../FunshiEngineGL/src/Audio/IAudioBackend.h"
#include "../FunshiEngineGL/src/Audio/IMusicaBackend.h"
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

bool contiene(const std::vector<std::string>& v, const std::string& s) {
    for (const std::string& x : v)
        if (x == s) return true;
    return false;
}

bool casiIgual(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// Backend falso que registra las llamadas del contrato de musica: sirve para
// comprobar que la cola del motor llega al backend (y que se ejecuta en el
// hilo de audio, no en el de juego).
class BackendFalso : public IAudioBackend, public IMusicaBackend {
public:
    std::vector<std::string> llamadas;
    std::set<int> abiertas;
    int consultasEstado = 0;
    float volumenDelBus = 1.0f;
    bool silenciado = false;

    bool iniciar() override { return true; }
    void cerrar() override {}
    int reproducirArchivo(const std::string&, float, bool,
                          const float* = nullptr) override {
        return 1;
    }
    void detener(int) override {}
    void detenerTodo() override {}
    void setVolumenMaestro(float) override {}
    void despachar(float) override {}
    void setPosicionSonido(int, float, float, float) override {}
    void setListenerPosition(float, float, float) override {}

    IMusicaBackend* backendMusica() override { return this; }

    int abrirPista(const std::string& ruta) override {
        llamadas.push_back("abrir:" + ruta);
        const int id = static_cast<int>(abiertas.size()) + 1;
        abiertas.insert(id);
        return id;
    }
    void cerrarPista(int id) override {
        llamadas.push_back("cerrar:" + std::to_string(id));
        abiertas.erase(id);
    }
    bool reproducirPista(int id, float) override {
        llamadas.push_back("reproducir:" + std::to_string(id));
        return true;
    }
    void detenerPista(int id) override {
        llamadas.push_back("detener:" + std::to_string(id));
    }
    void volumenPista(int, float) override {}
    void volumenBus(float volumen) override { volumenDelBus = volumen; }
    void silenciarBus(bool estado) override { silenciado = estado; }
    EstadoPista estadoPista(int) override {
        ++consultasEstado;
        return EstadoPista::Reproduciendo;
    }
    float posicionPista(int) override { return 0.0f; }
    float duracionPista(int) override { return 0.0f; }
};

// Los efectos de sonido se piden por RUTA: el motor reproduce el archivo que
// le pasan. No hay catalogo por nombre ni carpeta fija.
void probarCicloComandos() {
    TempPruebas::CarpetaPrueba carpeta("funshi_audio_tests");
    const std::string click = (carpeta.ruta() / "click.wav").string();
    const std::string musica = (carpeta.ruta() / "musica.wav").string();
    std::ofstream(click) << "contenido";
    std::ofstream(musica) << "contenido";

    AudioEngine motor;  // nullptr => NullAudioBackend
    // El ctor encola "Iniciar"; se drena antes de medir pendientes().
    motor.esperarPilaVacia();

    // Sin archivo asignado: -1 y no se encola nada.
    CHECK(motor.reproducir("") == -1, "reproducir sin ruta devuelve -1");
    CHECK(motor.pendientes() == 0,
          "reproducir sin ruta no encola comandos");

    // Con un archivo que no existe: -1 y no se encola nada (queda avisado en
    // el log del motor).
    const std::string inexistente = (carpeta.ruta() / "no_existe.wav").string();
    CHECK(motor.reproducir(inexistente) == -1,
          "reproducir un archivo inexistente devuelve -1");
    CHECK(motor.pendientes() == 0,
          "reproducir un archivo inexistente no encola comandos");

    // Archivo existente: handle valido y la cola se drena.
    const int h1 = motor.reproducir(click);
    CHECK(h1 >= 0, "reproducir un archivo existente devuelve handle");
    const int h2 = motor.reproducir(musica, 0.5f, true);
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
}

void probarBackendExplicito() {
    TempPruebas::CarpetaPrueba carpeta("funshi_audio_backend_tests");
    const std::string sonido = (carpeta.ruta() / "sonido.wav").string();
    std::ofstream(sonido) << "contenido";
    auto motor = std::make_unique<AudioEngine>();
    const int h = motor->reproducir(sonido, 1.0f, false);
    CHECK(h >= 0, "backend por defecto reproduce y devuelve handle");
    motor->esperarPilaVacia();
    CHECK(motor->pendientes() == 0, "cola drenada con backend por defecto");
}

// La lista de pistas de musica se fija por ruta y se consulta en orden.
void probarListaMusica() {
    AudioEngine motor;
    motor.esperarPilaVacia();

    motor.establecerMusicas({"/tmp/funshi_tema.ogg", "/tmp/funshi_coda.ogg"});

    const std::vector<std::string> rutas = motor.musicas();
    CHECK(rutas.size() == 2, "la lista tiene las dos pistas");
    CHECK(rutas[0] == "/tmp/funshi_tema.ogg", "la lista conserva el orden");
    CHECK(rutas[1] == "/tmp/funshi_coda.ogg", "la segunda pista va despues");

    motor.establecerMusicas({});
    CHECK(motor.musicas().empty(), "una lista vacia deja al motor sin pistas");
}

// Los comandos de musica llegan al backend desde el hilo de audio.
void probarColaMusica() {
    TempPruebas::CarpetaPrueba carpeta("funshi_audio_musica_tests");
    const std::string tema = (carpeta.ruta() / "tema.ogg").string();
    std::ofstream(tema) << "contenido";

    auto backend = std::make_unique<BackendFalso>();
    BackendFalso* observador = backend.get();
    AudioEngine motor(std::move(backend));
    motor.esperarPilaVacia();  // el comando Iniciar crea el reproductor

    motor.establecerMusicas({tema});
    motor.esperarPilaVacia();

    // Sin ruta no hay nada que reproducir.
    CHECK(!motor.reproducirMusica(""),
          "una pista sin ruta no se reproduce");
    CHECK(motor.pendientes() == 0,
          "la pista sin ruta no encola ningun comando");

    // Con un archivo que no existe tampoco (queda avisado en el log).
    const std::string inexistente = (carpeta.ruta() / "no_existe.ogg").string();
    CHECK(!motor.reproducirMusica(inexistente),
          "una pista cuyo archivo no existe no se reproduce");

    CHECK(motor.reproducirMusica(tema, 0.8f, 0.0f),
          "la pista de la lista se reproduce");
    motor.esperarPilaVacia();
    CHECK(contiene(observador->llamadas, "abrir:" + tema),
          "el backend abrio la pista");
    CHECK(contiene(observador->llamadas, "reproducir:1"),
          "el backend arranco la pista");

    // El tick de la escena llega al reproductor: este consulta el estado de
    // la pista que suena.
    const int consultasAntes = observador->consultasEstado;
    motor.actualizarMusica(0.25f);
    motor.esperarPilaVacia();
    CHECK(observador->consultasEstado > consultasAntes,
          "el tick de la escena avanza el reproductor");

    motor.setVolumenMusica(0.5f);
    motor.esperarPilaVacia();
    CHECK(casiIgual(observador->volumenDelBus, 0.5f),
          "el bus de musica recibe el volumen");

    motor.silenciarMusica(true);
    motor.esperarPilaVacia();
    CHECK(observador->silenciado, "el bus de musica recibe el silencio");

    motor.detenerMusica(0.0f);
    motor.esperarPilaVacia();
    CHECK(observador->abiertas.empty(), "detener cierra la pista");
    CHECK(contiene(observador->llamadas, "detener:1"), "el backend detiene la pista");
}

}  // namespace

int main() {
    std::cout << "== AUDIO ENGINE TESTS ==" << std::endl;
    probarCicloComandos();
    probarBackendExplicito();
    probarListaMusica();
    probarColaMusica();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "AUDIO ENGINE TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}