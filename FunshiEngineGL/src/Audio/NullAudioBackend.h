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
#ifndef NULL_AUDIO_BACKEND_H
#define NULL_AUDIO_BACKEND_H

#include "IAudioBackend.h"
#include "IMusicaBackend.h"

// Backend sin dispositivo (no-op): permite montar AudioEngine sin hardware de
// audio (tests headless, CI, o si el driver falla). La cola de comandos del
// motor sigue funcionando normal: reproducir() devuelve 0 => "no se pudo".
//
// Tambien implementa el contrato de musica: sin dispositivo no hay pistas
// que abrir, asi que abrirPista() devuelve 0 y el reproductor informa que no
// pudo reproducir. La cola de musica del motor se ejercita igual.
class NullAudioBackend : public IAudioBackend, public IMusicaBackend {
public:
    bool iniciar() override { return false; }
    void cerrar() override {}
    int reproducirArchivo(const std::string&, float, bool,
                          const float* = nullptr) override {
        return 0;
    }
    void detener(int) override {}
    void detenerTodo() override {}
    void setVolumenMaestro(float) override {}
    void despachar(float) override {}
    void setPosicionSonido(int, float, float, float) override {}
    void setListenerPosition(float, float, float) override {}

    IMusicaBackend* backendMusica() override { return this; }

    int abrirPista(const std::string&) override { return 0; }
    void cerrarPista(int) override {}
    bool reproducirPista(int, float) override { return false; }
    void detenerPista(int) override {}
    void volumenPista(int, float) override {}
    void volumenBus(float) override {}
    void silenciarBus(bool) override {}
    EstadoPista estadoPista(int) override { return EstadoPista::Detenida; }
    float posicionPista(int) override { return 0.0f; }
    float duracionPista(int) override { return 0.0f; }
};

#endif // NULL_AUDIO_BACKEND_H
