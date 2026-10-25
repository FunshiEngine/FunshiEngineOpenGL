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
#include "StbImageLoader.h"

// Solo las declaraciones de stb_image: su implementacion esta en la unidad
// propia de la libreria (External/stb_image/stb_image_impl.cpp).
#include <stb_image/stb_image.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include "TextureException.h"

std::shared_ptr<Image> StbImageLoader::load(const std::string& path) {
    int width = 0, height = 0, channels = 0;
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels,
                              STBI_rgb_alpha);
    if (!data) {
        const char* reason = stbi_failure_reason();
        throw TextureLoadException(
            path, (reason && std::strlen(reason) > 0)
                      ? reason
                      : "el archivo no existe o no decodifica");
    }

    auto image = std::make_shared<Image>();
    image->width = width;
    image->height = height;
    image->channels = 4; // pedimos explícitamente RGBA
    const std::size_t fila = static_cast<std::size_t>(width) * 4;
    image->pixels.assign(data, data + fila * static_cast<std::size_t>(height));

    // Voltea la imagen en vertical (el origen abajo-izquierda que espera GL).
    voltearVertical(*image);

    stbi_image_free(data);
    return image;
}

void StbImageLoader::voltearVertical(Image& imagen) {
    if (imagen.width <= 0 || imagen.height <= 0) return;
    const std::size_t fila = static_cast<std::size_t>(imagen.width) * 4;
    if (imagen.pixels.size() < fila * static_cast<std::size_t>(imagen.height)) {
        return;
    }
    std::vector<std::uint8_t> copia(imagen.pixels);
    for (int y = 0; y < imagen.height; ++y) {
        const std::size_t origen = static_cast<std::size_t>(y) * fila;
        const std::size_t destino =
            static_cast<std::size_t>(imagen.height - 1 - y) * fila;
        std::copy(copia.begin() + static_cast<std::ptrdiff_t>(origen),
                  copia.begin() + static_cast<std::ptrdiff_t>(origen + fila),
                  imagen.pixels.begin() +
                      static_cast<std::ptrdiff_t>(destino));
    }
}

bool StbImageLoader::dimensiones(const std::string& path, int& ancho,
                                  int& alto) {
    int x = 0, y = 0, canales = 0;
    if (!stbi_info(path.c_str(), &x, &y, &canales) || x <= 0 || y <= 0) {
        return false;
    }
    ancho = x;
    alto = y;
    return true;
}