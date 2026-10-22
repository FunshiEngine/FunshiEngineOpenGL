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
#include "MiniAudioBackend.h"

// miniaudio se usa con su implementacion en una UNICA TU (igual que stb_image
// en IconosGUI.cpp): el header trae los decoders WAV/OGG(MP3/FLAC) embebidos.
// El define va ANTES del include para generar la implementacion aqui mismo.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <utility>

struct MiniAudioBackend::Impl {
    ma_engine motor;
    bool inicializado = false;
    int proximoId = 1;
    // id del backend -> sonido activo. miniaudio mezcla en el hilo de su
    // device; los accesos a esta estructura ocurren solo desde el hilo de
    // trabajo del AudioEngine (serializados por su cola).
    std::unordered_map<int, ma_sound> activas;

    // Musica: el bus es un grupo de miniaudio (un "sonido" sin fuente de
    // datos) con volumen propio. Las pistas se cuelgan de el, asi que el bus
    // solo afecta a la musica y los efectos de sonido quedan fuera.
    ma_sound_group bus;
    bool busInicializado = false;
    float volumenBus = 1.0f;
    bool busSilenciado = false;
    // id de pista -> sonido de la pista. Las pistas se abren en streaming
    // (una pista de musica puede durar minutos: no se decodifica entera a
    // memoria como se hace con los efectos).
    std::unordered_map<int, ma_sound> pistas;
    int proximoIdPista = 1;
};

MiniAudioBackend::MiniAudioBackend() : impl_(new Impl()) {}

MiniAudioBackend::~MiniAudioBackend() { cerrar(); delete impl_; }

bool MiniAudioBackend::iniciar() {
    if (impl_->inicializado) return true;
    ma_engine_config cfg = ma_engine_config_init();
    cfg.channels = 2;
    // Sin device explicito: usa el device por defecto del sistema.
    if (ma_engine_init(&cfg, &impl_->motor) != MA_SUCCESS) return false;
    impl_->inicializado = true;

    // El bus de musica: un grupo de miniaudio, que es un sonido sin fuente de
    // datos. Sin el, las pistas no tendrian un volumen conjunto.
    ma_sound_group_config cfgGrupo = ma_sound_group_config_init_2(&impl_->motor);
    if (ma_sound_group_init_ex(&impl_->motor, &cfgGrupo, &impl_->bus) != MA_SUCCESS) {
        ma_engine_uninit(&impl_->motor);
        impl_->inicializado = false;
        return false;
    }
    impl_->busInicializado = true;
    aplicarVolumenBus();
    return true;
}

void MiniAudioBackend::cerrar() {
    if (!impl_->inicializado) return;
    detenerTodo();
    // Las pistas se cierran antes que el bus: estan colgadas de el.
    for (auto& [id, pista] : impl_->pistas) {
        ma_sound_stop(&pista);
        ma_sound_uninit(&pista);
    }
    impl_->pistas.clear();
    if (impl_->busInicializado) {
        ma_sound_group_uninit(&impl_->bus);
        impl_->busInicializado = false;
    }
    ma_engine_uninit(&impl_->motor);
    impl_->inicializado = false;
}

int MiniAudioBackend::reproducirArchivo(const std::string& ruta, float volumen,
                                        bool bucle) {
    if (ruta.empty()) return 0;
    // El comando Iniciar se encola de forma asincronica en el ctor de
    // AudioEngine; si el hilo de audio aun no lo ha procesado, inicializar
    // de forma sincronica aqui evita que el primer Reproducir se pierda por
    // condicion de carrera (impl_->inicializado == false).
    if (!impl_->inicializado && !iniciar()) return 0;

    ma_sound sonido;
    // MA_SOUND_FLAG_DECODE: decodifica el archivo completo a memoria, ideal
    // para efectos cortos: el stream no depende de que el archivo siga
    // abierto en disco y el loop/volumen responden al instante.
    if (ma_sound_init_from_file(&impl_->motor, ruta.c_str(),
                                MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                                &sonido) != MA_SUCCESS) {
        std::cerr << "[Audio] MiniAudioBackend::reproducirArchivo: no se pudo "
                     "inicializar sonido desde '" << ruta << "'\n";
        return 0;
    }
    ma_sound_set_volume(&sonido, volumen);
    ma_sound_set_looping(&sonido, bucle ? MA_TRUE : MA_FALSE);
    if (ma_sound_start(&sonido) != MA_SUCCESS) {
        ma_sound_uninit(&sonido);
        std::cerr << "[Audio] MiniAudioBackend::reproducirArchivo: ma_sound_start "
                     "fallo para '" << ruta << "'\n";
        return 0;
    }

    const int id = impl_->proximoId++;
    impl_->activas.emplace(id, sonido);
    return id;
}

void MiniAudioBackend::detener(int id) {
    const auto it = impl_->activas.find(id);
    if (it == impl_->activas.end()) return;
    ma_sound_stop(&it->second);
    ma_sound_uninit(&it->second);
    impl_->activas.erase(it);
}

void MiniAudioBackend::detenerTodo() {
    for (auto& [id, sonido] : impl_->activas) {
        ma_sound_stop(&sonido);
        ma_sound_uninit(&sonido);
    }
    impl_->activas.clear();
}

void MiniAudioBackend::setVolumenMaestro(float volumen) {
    if (impl_->inicializado) ma_engine_set_volume(&impl_->motor, volumen);
}

void MiniAudioBackend::despachar(float) {
    if (impl_->activas.empty()) return;
    for (auto it = impl_->activas.begin(); it != impl_->activas.end();) {
        if (ma_sound_at_end(&it->second)) {
            ma_sound_uninit(&it->second);
            it = impl_->activas.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// Musica
// ---------------------------------------------------------------------------

int MiniAudioBackend::abrirPista(const std::string& ruta) {
    if (!impl_->inicializado || !impl_->busInicializado || ruta.empty()) return 0;

    ma_sound pista;
    // MA_SOUND_FLAG_STREAM: la pista se decodifica sobre la marcha. Los
    // efectos usan MA_SOUND_FLAG_DECODE (todo a memoria) porque son cortos;
    // una pista de musica puede durar minutos y no caben en memoria
    // decodificadas.
    if (ma_sound_init_from_file(&impl_->motor, ruta.c_str(),
                                MA_SOUND_FLAG_STREAM, &impl_->bus, nullptr,
                                &pista) != MA_SUCCESS) {
        return 0;
    }
    const int id = impl_->proximoIdPista++;
    impl_->pistas.emplace(id, pista);
    return id;
}

void MiniAudioBackend::cerrarPista(int id) {
    const auto it = impl_->pistas.find(id);
    if (it == impl_->pistas.end()) return;
    ma_sound_stop(&it->second);
    ma_sound_uninit(&it->second);
    impl_->pistas.erase(it);
}

bool MiniAudioBackend::reproducirPista(int id, float volumen) {
    const auto it = impl_->pistas.find(id);
    if (it == impl_->pistas.end()) return false;
    ma_sound_set_volume(&it->second, volumen);
    if (ma_sound_start(&it->second) != MA_SUCCESS) return false;
    return true;
}

void MiniAudioBackend::detenerPista(int id) {
    const auto it = impl_->pistas.find(id);
    if (it == impl_->pistas.end()) return;
    ma_sound_stop(&it->second);
}

void MiniAudioBackend::volumenPista(int id, float volumen) {
    const auto it = impl_->pistas.find(id);
    if (it == impl_->pistas.end()) return;
    ma_sound_set_volume(&it->second, volumen);
}

void MiniAudioBackend::volumenBus(float volumen) {
    impl_->volumenBus = volumen;
    aplicarVolumenBus();
}

void MiniAudioBackend::silenciarBus(bool silenciado) {
    impl_->busSilenciado = silenciado;
    aplicarVolumenBus();
}

// El silencio se aplica al alza: el bus queda a volumen 0 sin perder el
// volumen que el usuario habia pedido.
void MiniAudioBackend::aplicarVolumenBus() {
    if (!impl_->busInicializado) return;
    const float efectivo = impl_->busSilenciado ? 0.0f : impl_->volumenBus;
    ma_sound_group_set_volume(&impl_->bus, efectivo);
}

EstadoPista MiniAudioBackend::estadoPista(int id) {
    const auto it = impl_->pistas.find(id);
    if (it == impl_->pistas.end()) return EstadoPista::Detenida;
    if (ma_sound_at_end(&it->second)) return EstadoPista::Terminada;
    return ma_sound_is_playing(&it->second) ? EstadoPista::Reproduciendo
                                            : EstadoPista::Detenida;
}

float MiniAudioBackend::posicionPista(int id) {
    const auto it = impl_->pistas.find(id);
    if (it == impl_->pistas.end()) return 0.0f;
    ma_uint64 cursor = 0;
    if (ma_sound_get_cursor_in_pcm_frames(&it->second, &cursor) != MA_SUCCESS)
        return 0.0f;
    const ma_uint32 tasa = ma_engine_get_sample_rate(&impl_->motor);
    if (tasa == 0) return 0.0f;
    return static_cast<float>(cursor) / static_cast<float>(tasa);
}

float MiniAudioBackend::duracionPista(int id) {
    const auto it = impl_->pistas.find(id);
    if (it == impl_->pistas.end()) return 0.0f;
    ma_uint64 largo = 0;
    // Una pista en streaming puede no conocer su largo hasta que el
    // decodificador ha leido bastante: si no lo sabe, se informa 0.
    if (ma_sound_get_length_in_pcm_frames(&it->second, &largo) != MA_SUCCESS)
        return 0.0f;
    const ma_uint32 tasa = ma_engine_get_sample_rate(&impl_->motor);
    if (tasa == 0) return 0.0f;
    return static_cast<float>(largo) / static_cast<float>(tasa);
}