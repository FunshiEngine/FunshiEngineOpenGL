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
#include "AudioClipsManager.h"

#include "AudioEngine.h"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace {

// Extensiones de audio soportadas por el decoder de miniaudio.
bool esAudio(const fs::path& archivo) {
    const std::string ext = archivo.extension().string();
    for (const char* sufijo : {".wav", ".ogg", ".mp3", ".flac"})
        if (ext == sufijo) return true;
    return false;
}

} // namespace

void AudioClipsManager::configurarCarpeta(const std::string& directorio,
                                          AudioEngine* motor) {
    directorio_ = directorio;
    clips_.clear();
    if (motor) motor->limpiarClips();
    if (directorio.empty() || !motor) return;

    std::error_code ec;
    if (!fs::is_directory(directorio, ec)) return;

    for (const auto& entrada : fs::directory_iterator(directorio, ec)) {
        if (ec) break;
        if (!entrada.is_regular_file() || !esAudio(entrada.path())) continue;
        AudioClip clip;
        clip.nombre = entrada.path().stem().string(); // sin extension
        clip.ruta = entrada.path().string();
        clips_.push_back(std::move(clip));
    }
    std::sort(clips_.begin(), clips_.end(),
              [](const AudioClip& a, const AudioClip& b) {
                  return a.nombre < b.nombre;
              });

    for (const AudioClip& clip : clips_) motor->registrarClip(clip.nombre, clip.ruta);
}