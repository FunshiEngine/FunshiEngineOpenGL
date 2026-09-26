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
#ifndef AUDIOCLIPSMANAGER_H
#define AUDIOCLIPSMANAGER_H

#include <cstddef>
#include <string>
#include <vector>

#include "AudioClip.h"

class AudioEngine;

// Explorador de la carpeta de sonidos del proyecto (Sonidos/): descubre los
// archivos de audio y los registra por NOMBRE en el AudioEngine. Al cambiar de
// proyecto se re-escanea: se limpia el registro y se vuelve a poblar.
class AudioClipsManager {
public:
    void configurarCarpeta(const std::string& directorio, AudioEngine* motor);

    // Snapshot de los clips descubiertos en la ultima exploracion (para GUI).
    const std::vector<AudioClip>& clips() const noexcept { return clips_; }
    std::size_t cantidad() const noexcept { return clips_.size(); }

private:
    std::vector<AudioClip> clips_;
    std::string directorio_;
};

#endif // AUDIOCLIPSMANAGER_H