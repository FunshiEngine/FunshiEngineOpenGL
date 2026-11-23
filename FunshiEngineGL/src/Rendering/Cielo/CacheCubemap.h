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
#ifndef CACHECUBEMAP_H
#define CACHECUBEMAP_H

#include <cstdint>
#include <string>

// Clave de identidad del cubemap de las 6 caras de un Skybox.
//
// El renderer sube el cubemap a GPU una sola vez y solo lo vuelve a subir
// cuando cambia algo que lo altera: otra carpeta de caras o un archivo
// reescrito. Esa decision se toma comparando esta clave contra la que valido la
// textura que esta viva, asi que la regla de invalidacion vive aca y se puede
// probar sin GPU ni imagenes.
//
// Es CPU puro a proposito (sin OpenGL ni stb_image), igual que Cielo o
// Difuminado, para correr en los targets headless.
namespace CacheCubemap {

// Clave a partir de las 6 rutas y las 6 fechas de modificacion de los archivos
// (segundos desde el epoch; 0 = inexistente). Cada componente se separa con su
// longitud para que no haya dos claves distintas que puedan escribir la misma
// cadena (una ruta que contenga '|' o '#' no puede emparejar con otra). El orden
// importa: es el de las caras (+X, -X, +Y, -Y, +Z, -Z), porque intercambiar
// dos rutas produce un cubemap distinto.
inline std::string claveDeCaras(const std::string rutas[6],
                                const std::int64_t mtimes[6]) {
    std::string clave;
    for (int i = 0; i < 6; ++i) {
        clave += std::to_string(rutas[i].size());
        clave += ':';
        clave += rutas[i];
        clave += '#';
        clave += std::to_string(mtimes[i]);
        clave.push_back('|');
    }
    return clave;
}

} // namespace CacheCubemap

#endif // CACHECUBEMAP_H
