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
#ifndef TEXTUREEXCEPTION_H
#define TEXTUREEXCEPTION_H

#include "../../ExcepcionesCPP/RuntimeException.h"

// Jerarquia de errores del TextureManager. Mismo patron que AssetException:
// excepcion explicita (con stack trace) en lugar de "std::cerr + estado a
// medio cargar" cuando un archivo de imagen falta o no decodifica.
class TextureNotFoundException : public RuntimeException {
public:
    explicit TextureNotFoundException(const std::string& texturePath)
        : RuntimeException("Textura no encontrada: " + texturePath,
                           "TextureNotFoundException") {}
};

// La imagen pudo localizarse pero fallo su decode (archivo corrupto, formato
// no soportado por stb_image, etc.).
class TextureLoadException : public RuntimeException {
public:
    TextureLoadException(const std::string& texturePath,
                         const std::string& detail = "")
        : RuntimeException("No se pudo cargar la textura '" + texturePath + "'" +
                           (detail.empty() ? std::string() : ": " + detail),
                           "TextureLoadException") {}
};

#endif
