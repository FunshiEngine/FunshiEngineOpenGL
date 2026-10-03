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
#ifndef BARRA_PROGRESO_TEXTO_H
#define BARRA_PROGRESO_TEXTO_H

#include <algorithm>
#include <cstddef>
#include <string>

// Formato de la barra de progreso en texto de la ventana "Estado". Puro (solo
// std) para poder probarlo headless y para concentrar en un solo sitio el
// acotado de valores: un `hecha > total` o un `total == 0` no deben producir ni
// longitudes negativas convertidas a `size_t` ni una division por cero.
namespace BarraProgresoTexto {

// Porcentaje [0, 100] de `hecha` sobre `total`, acotando `hecha` a `[0, total]`.
// Con `total == 0` no hay proporcion posible: devuelve 0.
inline int porcentaje(std::size_t hecha, std::size_t total) {
    if (total == 0) return 0;
    if (hecha > total) hecha = total;
    const int pct =
        static_cast<int>((static_cast<float>(hecha) / static_cast<float>(total)) *
                         100.0f);
    return std::min(100, std::max(0, pct));
}

// Barra de `ancho` caracteres: '#' proporcional al porcentaje y el resto
// espacios. `total == 0` devuelve cadena vacia (no hay barra que dibujar). El
// llenado usa resize, que no puede recibir una longitud negativa.
inline std::string formatear(std::size_t hecha, std::size_t total,
                             std::size_t ancho = 10) {
    if (total == 0) return {};
    const std::size_t llenos =
        static_cast<std::size_t>(porcentaje(hecha, total)) * ancho / 100;
    std::string barra(llenos, '#');
    barra.resize(ancho, ' ');
    return barra;
}

}  // namespace BarraProgresoTexto

#endif  // BARRA_PROGRESO_TEXTO_H
