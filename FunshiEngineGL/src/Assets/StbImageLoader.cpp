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

// stb_image se usa con su implementacion en una unica TU: IconosGUI.cpp ya la
// define (STB_IMAGE_IMPLEMENTATION). Aqui se incluye solo el header con las
// declaraciones (STBIDEF = extern) y se enlazan esas funciones.
#include "../Herramientas/IconosGUI/stb_image.h"

#include <cstring>

#include "TextureException.h"

std::shared_ptr<Image> StbImageLoader::load(const std::string& path) {
    stbi_set_flip_vertically_on_load(1);

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
    image->pixels.assign(data, data + static_cast<std::size_t>(width) *
                                          static_cast<std::size_t>(height) * 4);
    stbi_image_free(data);
    stbi_set_flip_vertically_on_load(0);
    return image;
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