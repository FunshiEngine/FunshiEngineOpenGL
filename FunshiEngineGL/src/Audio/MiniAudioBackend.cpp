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
    return true;
}

void MiniAudioBackend::cerrar() {
    if (!impl_->inicializado) return;
    detenerTodo();
    ma_engine_uninit(&impl_->motor);
    impl_->inicializado = false;
}

int MiniAudioBackend::reproducirArchivo(const std::string& ruta, float volumen,
                                        bool bucle) {
    if (!impl_->inicializado || ruta.empty()) return 0;

    ma_sound sonido;
    // MA_SOUND_FLAG_DECODE: decodifica el archivo completo a memoria, ideal
    // para efectos cortos: el stream no depende de que el archivo siga
    // abierto en disco y el loop/volumen responden al instante.
    if (ma_sound_init_from_file(&impl_->motor, ruta.c_str(),
                                MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                                &sonido) != MA_SUCCESS) {
        return 0;
    }
    ma_sound_set_volume(&sonido, volumen);
    ma_sound_set_looping(&sonido, bucle ? MA_TRUE : MA_FALSE);
    if (ma_sound_start(&sonido) != MA_SUCCESS) {
        ma_sound_uninit(&sonido);
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