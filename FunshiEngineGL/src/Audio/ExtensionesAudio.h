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
#ifndef EXTENSIONES_AUDIO_H
#define EXTENSIONES_AUDIO_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

// Extensiones de audio que entiende el decoder de miniaudio (wav, ogg, mp3 y
// flac).
//
// La comparacion es NORMALIZADA: la extension se pasa a minusculas antes de
// mirar. El nombre del archivo lo pone el usuario y ningun sistema de
// archivos obliga a una Extension concreta, asi que "tema.WAV" y "tema.wav"
// tienen que valer lo mismo (en Windows es facil acabar con la extension en
// mayusculas).
inline bool esArchivoAudio(const std::filesystem::path& archivo) {
    std::string extension = archivo.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char caracter) {
                       return static_cast<char>(std::tolower(caracter));
                   });
    for (const char* sufijo : {".wav", ".ogg", ".mp3", ".flac"})
        if (extension == sufijo) return true;
    return false;
}

#endif // EXTENSIONES_AUDIO_H
