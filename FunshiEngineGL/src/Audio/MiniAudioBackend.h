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
#include "IMusicaBackend.h"

#include <cstdint>
#include <unordered_map>

// Adaptador concreto: reproduce con miniaudio (libreria single-file vendoriada
// en src/Audio/miniaudio.h, MIT-0/public domain).
//
// Concurrencia: miniaudio mezcla en el HILO INTERNO de su device callback
// (hilo de audio en tiempo real). Como ma_engine ya es seguro entre hilos y
// AudioEngine serializa todos los accesos del adaptador en su propio hilo de
// trabajo, el hilo del juego nunca toca el hardware de audio directamente.
//
// Tambien implementa el contrato de musica: las pistas se cuelgan de un
// grupo de miniaudio (el bus), que tiene volumen propio. El bus solo afecta
// a la musica: los efectos de sonido no pasan por el.
class MiniAudioBackend : public IAudioBackend, public IMusicaBackend {
public:
    MiniAudioBackend();
    ~MiniAudioBackend() override;

    bool iniciar() override;
    void cerrar() override;
    int reproducirArchivo(const std::string& ruta, float volumen,
                          bool bucle,
                          const float* posicion = nullptr) override;
    void detener(int id) override;
    void detenerTodo() override;
    void setVolumenMaestro(float volumen) override;
    void despachar(float deltaTime) override;

    void setPosicionSonido(int id, float x, float y, float z) override;
    void setListenerPosition(float x, float y, float z) override;

    IMusicaBackend* backendMusica() override { return this; }

    int abrirPista(const std::string& ruta) override;
    void cerrarPista(int id) override;
    bool reproducirPista(int id, float volumen) override;
    void detenerPista(int id) override;
    void volumenPista(int id, float volumen) override;
    void volumenBus(float volumen) override;
    void silenciarBus(bool silenciado) override;
    EstadoPista estadoPista(int id) override;
    float posicionPista(int id) override;
    float duracionPista(int id) override;

private:
    void aplicarVolumenBus();

    struct Impl;
    Impl* impl_;
};

#endif // MINIAUDIO_BACKEND_H