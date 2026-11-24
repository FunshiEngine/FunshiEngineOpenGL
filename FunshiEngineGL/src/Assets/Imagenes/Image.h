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
#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <vector>

// Imagen decodificada en CPU, independiente de OpenGL: es la "malla" de las
// texturas. La genera el loader (StbImageLoader en la integracion con el motor;
// un loader artificial en las pruebas headless) y la comparte el TextureManager
// entre Material que apunten al mismo archivo. El renderer moderno la sube a
// GPU en un TextureGL, de la misma manera que Mesh -> MeshGPU.
struct Image {
    int width = 0;
    int height = 0;
    // Canales ya forzados a RGBA (4) por el loader; el formato se estandariza
    // para que el upload a GPU sea siempre RGBA8.
    int channels = 4;
    std::vector<std::uint8_t> pixels;

    bool isEmpty() const { return pixels.empty(); }
};

#endif