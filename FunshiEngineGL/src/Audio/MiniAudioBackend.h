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
#ifndef MINIAUDIO_BACKEND_H
#define MINIAUDIO_BACKEND_H

#include "IAudioBackend.h"

#include <cstdint>
#include <unordered_map>

// Adaptador concreto: reproduce con miniaudio (libreria single-file vendoriada
// en src/Audio/miniaudio.h, MIT-0/public domain).
//
// Concurrencia: miniaudio mezcla en el HILO INTERNO de su device callback
// (hilo de audio en tiempo real). Como ma_engine ya es seguro entre hilos y
// AudioEngine serializa todos los accesos del adaptador en su propio hilo de
// trabajo, el hilo del juego nunca toca el hardware de audio directamente.
class MiniAudioBackend : public IAudioBackend {
public:
    MiniAudioBackend();
    ~MiniAudioBackend() override;

    bool iniciar() override;
    void cerrar() override;
    int reproducirArchivo(const std::string& ruta, float volumen,
                          bool bucle) override;
    void detener(int id) override;
    void detenerTodo() override;
    void setVolumenMaestro(float volumen) override;
    void despachar(float deltaTime) override;

private:
    struct Impl;
    Impl* impl_;
};

#endif // MINIAUDIO_BACKEND_H