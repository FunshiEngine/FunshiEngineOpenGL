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

// Backend sin dispositivo (no-op): permite montar AudioEngine sin hardware de
// audio (tests headless, CI, o si el driver falla). La cola de comandos del
// motor sigue funcionando normal: reproducir() devuelve 0 => "no se pudo".
class NullAudioBackend : public IAudioBackend {
public:
    bool iniciar() override { return false; }
    void cerrar() override {}
    int reproducirArchivo(const std::string&, float, bool) override { return 0; }
    void detener(int) override {}
    void detenerTodo() override {}
    void setVolumenMaestro(float) override {}
    void despachar(float) override {}
};

#endif // NULL_AUDIO_BACKEND_H