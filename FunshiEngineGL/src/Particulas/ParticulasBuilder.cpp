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
#include "ParticulasBuilder.h"

#include <cmath>

namespace {

// Vértices del quad unitario centrado, con su uv.
constexpr float kEsquinas[6][2] = {
    {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f},
    {-0.5f, -0.5f}, {0.5f, 0.5f},  {-0.5f, 0.5f},
};
constexpr float kUv[6][2] = {
    {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    {0.0f, 0.0f}, {1.0f, 1.0f},  {0.0f, 1.0f},
};

}  // namespace

void ParticulasBuilder::limpiar() {
    vertices_.clear();
    particulas_ = 0;
}

void ParticulasBuilder::agregarParticula(const Particula& particula,
                                          const float uv[4]) {
    // El tamano es el lado del quad: la mitad es el semilado.
    const float semilado = particula.tamano * 0.5f;
    const float coseno = std::cos(particula.rotacion);
    const float seno = std::sin(particula.rotacion);

    for (std::size_t v = 0; v < kVerticesPorParticula; ++v) {
        const float x = kEsquinas[v][0] * semilado;
        const float y = kEsquinas[v][1] * semilado;
        // La rotacion va en el plano XY: las particulas son billboards.
        const float rx = x * coseno - y * seno;
        const float ry = x * seno + y * coseno;
        vertices_.push_back(particula.posicion[0] + rx);
        vertices_.push_back(particula.posicion[1] + ry);
        vertices_.push_back(particula.posicion[2]);
        // La uv se interpola dentro de la region pedida.
        const float u = kUv[v][0] == 0.0f ? uv[0] : uv[2];
        const float w = kUv[v][1] == 0.0f ? uv[1] : uv[3];
        vertices_.push_back(u);
        vertices_.push_back(w);
        for (int c = 0; c < 4; ++c) vertices_.push_back(particula.color[c]);
    }
    ++particulas_;
}
