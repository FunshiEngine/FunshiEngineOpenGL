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

#include "IAudioBackend.h"
#include "NullAudioBackend.h"

#include <chrono>
#include <condition_variable>
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
    Cierre        // apaga el hilo
};

struct Comando {
    TipoComando tipo = TipoComando::Iniciar;
    int handle = -1;
    std::string clip;
    float volumen = 1.0f;
    bool bucle = false;
};

void ejecutarComando(struct AudioEngine::Impl* imp, const Comando& c);

} // namespace

struct AudioEngine::Impl {
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::queue<Comando> pila;
    // nombre del clip -> ruta (registro del asset; el hilo del juego lo lee).
    std::unordered_map<std::string, std::string> clips;
    // handle propio del motor -> id devuelto por el backend.
    std::unordered_map<int, int> handleAId;
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
            break;
        case TipoComando::Reproducir:
            // El handle del motor ya esta asignado; se traduce al id del backend
            // (que puede fallar con 0 si el archivo no existe o no hay driver).
            if (c.handle >= 0) {
                const int id = imp->backend->reproducirArchivo(
                    c.clip, c.volumen, c.bucle);
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
    c.clip.clear();
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->cerrado = true;
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_all();
    if (impl_->hilo.joinable()) impl_->hilo.join();
}

void AudioEngine::registrarClip(const std::string& nombre,
                                const std::string& ruta) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    impl_->clips[nombre] = ruta;
}

bool AudioEngine::clipRegistrado(const std::string& nombre) const {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    return impl_->clips.count(nombre) > 0;
}

bool AudioEngine::eliminarClip(const std::string& nombre) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    return impl_->clips.erase(nombre) > 0;
}

void AudioEngine::limpiarClips() {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    impl_->clips.clear();
}

std::vector<std::string> AudioEngine::nombresClips() const {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    std::vector<std::string> nombres;
    nombres.reserve(impl_->clips.size());
    for (const auto& [nombre, ruta] : impl_->clips) nombres.push_back(nombre);
    return nombres;
}

int AudioEngine::reproducir(const std::string& clip, float volumen, bool bucle) {
    Comando c;
    c.tipo = TipoComando::Reproducir;
    c.volumen = volumen;
    c.bucle = bucle;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        const auto it = impl_->clips.find(clip);
        if (it == impl_->clips.end()) return -1; // clip no registrado
        c.clip = it->second;
        c.handle = impl_->proximoHandle++;
        impl_->pila.push(std::move(c));
    }
    impl_->cv.notify_one();
    return c.handle;
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

void AudioEngine::esperarPilaVacia() {
    std::unique_lock<std::mutex> lock(impl_->mtx);
    impl_->cv.wait(lock, [this] { return impl_->pila.empty() && !impl_->enEjecucion; });
}

std::size_t AudioEngine::pendientes() const {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    return impl_->pila.size() + (impl_->enEjecucion ? 1u : 0u);
}