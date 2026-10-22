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
#include "AudioEngine.h"

#include "GestorMusica.h"
#include "IAudioBackend.h"
#include "IMusicaBackend.h"
#include "NullAudioBackend.h"

#include "Configuracion/EditorConfig.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <utility>

namespace {

// Comando de la cola de audio. El hilo de juego nunca ejecuta audio; solo
// arma comandos y los encola (non-blocking).
enum class TipoComando {
    Iniciar,      // abre el backend (lo maneja el hilo de audio)
    Reproducir,   // clip (ruta), handle, volumen, bucle
    Detener,      // handle
    DetenerTodo,
    Volumen,      // volumen maestro
    Cierre,       // apaga el hilo
    // --- musica (se ejecutan en el hilo de audio, sobre el reproductor) ---
    EstablecerMusicas, // lista de rutas (en orden) y si se repite
    ReproducirMusica,   // ruta, volumen, duracion del fundido
    DetenerMusica,      // duracion del fundido de salida
    VolumenMusica,      // volumen del bus
    SilenciarMusica,    // silencio del bus
    TickMusica          // deltaTime de la escena
};

struct Comando {
    TipoComando tipo = TipoComando::Iniciar;
    int handle = -1;
    std::string ruta;   // ruta del archivo (clip de efecto o pista de musica)
    std::vector<std::string> rutas; // lista de pistas de musica (en orden)
    float volumen = 1.0f;
    float duracionFundido = 0.0f;
    float deltaTime = 0.0f;
    bool bucle = false;
    bool silenciado = false;
    bool repetirLista = true;
};

void ejecutarComando(struct AudioEngine::Impl* imp, const Comando& c);

} // namespace

struct AudioEngine::Impl {
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::queue<Comando> pila;
    // Lista de pistas de musica (rutas, en orden de reproduccion).
    std::vector<std::string> musicas;
    bool repetirMusicas = true;
    // handle propio del motor -> id devuelto por el backend.
    std::unordered_map<int, int> handleAId;
    // Reproductor de musica. Vive en el hilo de audio (lo crea el comando
    // Iniciar y solo se usa desde ese hilo), asi que su estado no necesita
    // candados. Se declara DESPUES de backend para destruirse antes que el
    // backend del que depende.
    std::unique_ptr<GestorMusica> gestorMusica;
    std::unique_ptr<IAudioBackend> backend;
    std::thread hilo;
    int proximoHandle = 0;
    bool cerrado = false;
    bool enEjecucion = false;

    explicit Impl(std::unique_ptr<IAudioBackend> b) : backend(std::move(b)) {}
};

namespace {

void ejecutarComando(AudioEngine::Impl* imp, const Comando& c) {
    if (!imp->backend) return;
    switch (c.tipo) {
        case TipoComando::Iniciar:
            imp->backend->iniciar();
            // El reproductor de musica necesita el backend de musica: se crea
            // aqui, en el hilo de audio, y a partir de aqui solo se usa desde
            // este hilo.
            if (IMusicaBackend* musica = imp->backend->backendMusica()) {
                imp->gestorMusica = std::make_unique<GestorMusica>(*musica);
            }
            break;
        case TipoComando::Reproducir:
            // El handle del motor ya esta asignado; se traduce al id del backend
            // (que puede fallar con 0 si el archivo no existe o no hay driver).
            if (c.handle >= 0) {
                const int id = imp->backend->reproducirArchivo(
                    c.ruta, c.volumen, c.bucle);
                imp->handleAId[c.handle] = id;
            }
            break;
        case TipoComando::Detener:
            if (c.handle >= 0) {
                const auto it = imp->handleAId.find(c.handle);
                if (it != imp->handleAId.end()) {
                    if (it->second > 0) imp->backend->detener(it->second);
                    imp->handleAId.erase(it);
                }
            }
            break;
        case TipoComando::DetenerTodo:
            imp->backend->detenerTodo();
            imp->handleAId.clear();
            break;
        case TipoComando::Volumen:
            imp->backend->setVolumenMaestro(c.volumen);
            break;
        case TipoComando::Cierre:
            imp->backend->cerrar();
            break;
        // --- musica ---
        case TipoComando::EstablecerMusicas:
            if (imp->gestorMusica) {
                imp->gestorMusica->establecerPistas(c.rutas);
                imp->gestorMusica->establecerRepetirLista(c.repetirLista);
            }
            break;
        case TipoComando::ReproducirMusica:
            if (imp->gestorMusica)
                imp->gestorMusica->reproducir(c.ruta, c.volumen,
                                              c.duracionFundido);
            break;
        case TipoComando::DetenerMusica:
            if (imp->gestorMusica) imp->gestorMusica->detener(c.duracionFundido);
            break;
        case TipoComando::VolumenMusica:
            if (imp->gestorMusica) imp->gestorMusica->volumenBus(c.volumen);
            break;
        case TipoComando::SilenciarMusica:
            if (imp->gestorMusica) imp->gestorMusica->silenciarBus(c.silenciado);
            break;
        case TipoComando::TickMusica:
            if (imp->gestorMusica) imp->gestorMusica->actualizar(c.deltaTime);
            break;
    }
}

// Hilo de trabajo: proceso comandos y, cada ~200 ms incluso en reposo, llama al
// despacho del backend para recolectar reproducciones que terminaron solas.
void bucleAudio(AudioEngine::Impl* imp) {
    const auto tick = std::chrono::milliseconds(200);
    while (true) {
        Comando c;
        {
            std::unique_lock<std::mutex> lock(imp->mtx);
            imp->cv.wait_for(lock, tick, [imp] {
                return imp->cerrado || !imp->pila.empty();
            });
            if (imp->cerrado && imp->pila.empty()) break;
            if (!imp->pila.empty()) {
                c = std::move(imp->pila.front());
                imp->pila.pop();
                imp->enEjecucion = true;
            }
        }

        if (imp->enEjecucion) ejecutarComando(imp, c);
        if (imp->backend) imp->backend->despachar(0.2f);

        {
            std::lock_guard<std::mutex> lock(imp->mtx);
            imp->enEjecucion = false;
        }
        imp->cv.notify_all();
    }
}

} // namespace

AudioEngine::AudioEngine()
    : AudioEngine(nullptr) {}

AudioEngine::AudioEngine(std::unique_ptr<IAudioBackend> backend)
    : impl_(std::make_unique<Impl>(
          backend ? std::move(backend)
                  : std::make_unique<NullAudioBackend>())) {
    Comando c;
    c.tipo = TipoComando::Iniciar;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->hilo = std::thread(bucleAudio, impl_.get());
}

AudioEngine::~AudioEngine() {
    Comando c;
    c.tipo = TipoComando::Cierre;
    c.ruta.clear();
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->cerrado = true;
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_all();
    if (impl_->hilo.joinable()) impl_->hilo.join();
}

int AudioEngine::reproducir(const std::string& ruta, float volumen, bool bucle) {
    if (ruta.empty()) return -1; // sin archivo asignado
    // Ruta relativa: resolver contra la raiz de assets del proyecto (si hay)
    // para que el check de existencia funcione desde cualquier cwd.
    std::string rutaResuelta = ruta;
    if (!std::filesystem::path(ruta).is_absolute() && !EditorConfig::raizAssetsFijada().empty())
        rutaResuelta = EditorConfig::absolutizarRuta(ruta);
    // El archivo se comprueba aqui (hilo de juego) para poder avisar al que
    // lo pidio: el backend solo puede fallar en su hilo, y entonces el que
    // reproduce creeria que el sonido salio.
    std::error_code ec;
    if (!std::filesystem::is_regular_file(rutaResuelta, ec)) {
        std::cerr << "[Audio] No se reproduce '" << ruta
                  << "': el archivo no existe\n";
        return -1;
    }
    Comando c;
    c.tipo = TipoComando::Reproducir;
    c.volumen = volumen;
    c.bucle = bucle;
    c.ruta = rutaResuelta;
    int handle = -1;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        handle = impl_->proximoHandle++;
        c.handle = handle;
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
    return handle;
}

void AudioEngine::detener(int handle) {
    Comando c;
    c.tipo = TipoComando::Detener;
    c.handle = handle;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

void AudioEngine::detenerTodo() {
    Comando c;
    c.tipo = TipoComando::DetenerTodo;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

void AudioEngine::setVolumenMaestro(float volumen) {
    Comando c;
    c.tipo = TipoComando::Volumen;
    c.volumen = volumen;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

// ---------------------------------------------------------------------------
// Musica
// ---------------------------------------------------------------------------

void AudioEngine::establecerMusicas(const std::vector<std::string>& rutas,
                                     bool repetir) {
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->musicas = rutas;
        impl_->repetirMusicas = repetir;
    }
    Comando c;
    c.tipo = TipoComando::EstablecerMusicas;
    c.rutas = rutas;
    c.repetirLista = repetir;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

std::vector<std::string> AudioEngine::musicas() const {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    return impl_->musicas;
}

bool AudioEngine::reproducirMusica(const std::string& ruta, float volumen,
                                    float duracionFundido) {
    if (ruta.empty()) return false; // sin archivo asignado
    // El archivo se comprueba aqui (hilo de juego) para poder avisar al que
    // lo pidio: el backend solo puede fallar en su hilo, y entonces el que
    // reproduce creeria que la pista salio.
    std::error_code ec;
    if (!std::filesystem::is_regular_file(ruta, ec)) {
        std::cerr << "[Audio] No se reproduce la pista de musica '" << ruta
                  << "': el archivo no existe\n";
        return false;
    }
    Comando c;
    c.tipo = TipoComando::ReproducirMusica;
    c.ruta = ruta;
    c.volumen = volumen;
    c.duracionFundido = duracionFundido;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
    return true;
}

void AudioEngine::detenerMusica(float duracionFundido) {
    Comando c;
    c.tipo = TipoComando::DetenerMusica;
    c.duracionFundido = duracionFundido;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

void AudioEngine::setVolumenMusica(float volumen) {
    Comando c;
    c.tipo = TipoComando::VolumenMusica;
    c.volumen = volumen;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

void AudioEngine::silenciarMusica(bool silenciado) {
    Comando c;
    c.tipo = TipoComando::SilenciarMusica;
    c.silenciado = silenciado;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

void AudioEngine::actualizarMusica(float deltaTime) {
    if (deltaTime < 0.0f) return;
    Comando c;
    c.tipo = TipoComando::TickMusica;
    c.deltaTime = deltaTime;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
}

void AudioEngine::esperarPilaVacia() {
    std::unique_lock<std::mutex> lock(impl_->mtx);
    // Con el motor cerrado ya nobody drena la cola: seguir esperando seria
    // esperar forever.
    impl_->cv.wait(lock, [this] {
        return impl_->cerrado || (impl_->pila.empty() && !impl_->enEjecucion);
    });
}

std::size_t AudioEngine::pendientes() const {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    return impl_->pila.size() + (impl_->enEjecucion ? 1u : 0u);
}