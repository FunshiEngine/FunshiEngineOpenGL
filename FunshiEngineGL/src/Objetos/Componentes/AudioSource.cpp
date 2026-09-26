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
#include "AudioSource.h"

#include "../../Audio/AudioEngine.h"

void AudioSource::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    const std::size_t len = clip.size();
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) file->write(clip.data(), static_cast<std::streamsize>(len));
    file->write(reinterpret_cast<const char*>(&volume), sizeof(float));
    file->write(reinterpret_cast<const char*>(&loop), sizeof(bool));
    file->write(reinterpret_cast<const char*>(&reproduccionAutomatica),
                sizeof(bool));
}

void AudioSource::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;
    std::size_t len = 0;
    file->read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > 0) {
        clip.resize(len);
        file->read(clip.data(), static_cast<std::streamsize>(len));
    } else {
        clip.clear();
    }
    file->read(reinterpret_cast<char*>(&volume), sizeof(float));
    file->read(reinterpret_cast<char*>(&loop), sizeof(bool));
    file->read(reinterpret_cast<char*>(&reproduccionAutomatica), sizeof(bool));
}

void AudioSource::saveComponent(std::ofstream* file) { serializeComponent(file); }

void AudioSource::loadComponent(std::ifstream* file) { deserializeComponent(file); }

void AudioSource::reproducir() {
    if (!motor) return;
    detener(); // no apilar probabilidades: un clip, una reproduccion
    const int handle = motor->reproducir(clip, volume, loop);
    handleActual = handle;
    sonando = (handle >= 0);
}

void AudioSource::detener() {
    if (motor && handleActual >= 0) motor->detener(handleActual);
    handleActual = -1;
    sonando = false;
}