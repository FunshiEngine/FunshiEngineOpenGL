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
#include "PerfilParticula.h"

#include <algorithm>
#include <cmath>

namespace {

// Interpola linealmente el valor de una curva en la fraccion f (0..1).
// El primer punto es el nacimiento y el ultimo la muerte.
template <typename Valor, typename Lectura>
Valor interpolar(const std::vector<Valor>& puntos, float f, Lectura leer,
                 const Valor& porDefecto) {
    if (puntos.empty()) return porDefecto;
    if (puntos.size() == 1) return puntos[0];
    const float acotada = std::min(1.0f, std::max(0.0f, f));
    const float posicion = acotada * static_cast<float>(puntos.size() - 1);
    const std::size_t indice = std::min(
        puntos.size() - 2, static_cast<std::size_t>(std::floor(posicion)));
    const float t = posicion - static_cast<float>(indice);
    const Valor& a = puntos[indice];
    const Valor& b = puntos[indice + 1];
    return leer(a, b, t);
}

}  // namespace

float PerfilParticula::multiplicadorTamano(float f) const {
    return interpolar<float>(
        tamanos, f,
        [](float a, float b, float t) { return a + (b - a) * t; }, 1.0f);
}

void PerfilParticula::colorEn(float f, float salida[4]) const {
    const std::array<float, 4> blanco{1.0f, 1.0f, 1.0f, 1.0f};
    const std::array<float, 4> color = interpolar<std::array<float, 4>>(
        colores, f,
        [](const std::array<float, 4>& a, const std::array<float, 4>& b,
           float t) {
            std::array<float, 4> mezcla{};
            for (std::size_t i = 0; i < 4; ++i)
                mezcla[i] = a[i] + (b[i] - a[i]) * t;
            return mezcla;
        },
        blanco);
    for (int i = 0; i < 4; ++i) salida[i] = color[static_cast<std::size_t>(i)];
}
