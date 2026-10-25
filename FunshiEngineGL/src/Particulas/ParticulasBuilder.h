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
#ifndef PARTICULASBUILDER_H
#define PARTICULASBUILDER_H

#include <cstddef>
#include <vector>

#include "Particula.h"

// Constructor CPU de la geometria de las particulas.
//
// Cada particula se expande a un CUADrilatero de dos triangulos, con el tamano
// y la rotacion ya aplicados: el vertex shader solo tiene que proyectar. No
// hay buffer de indices (los triangulos no comparten vertices) asi que alcanza
// con glDrawArrays.
//
// Vertice (kFloatsPorVertice floats, layout del VAO):
//   [0..2]  posicion del vertice en el mundo
//   [3..4]  uv dentro de la textura
//   [5..8]  color RGBA
//
// Es CPU puro a proposito: no incluye OpenGL ni glm, asi se puede probar en
// los targets headless (tests/ParticulasTests.cpp) igual que LineBuilder.
class ParticulasBuilder {
public:
    // Floats por vertice y vertices por particula (dos triangulos).
    static const std::size_t kFloatsPorVertice = 9;
    static const std::size_t kVerticesPorParticula = 6;
    // Tamano del vertice en bytes (stride del VAO).
    static std::size_t stride() { return kFloatsPorVertice * sizeof(float); }
    // Offset de cada atributo dentro del vertice.
    static std::size_t offsetPosicion() { return 0; }
    static std::size_t offsetUv() { return 3 * sizeof(float); }
    static std::size_t offsetColor() { return 5 * sizeof(float); }

    // Descarta la geometria acumulada (reutiliza la capacidad reservada).
    void limpiar();

    // Agrega el quad de una particula. 'uv' es la region de la textura que
    // se dibuja (u0, v0, u1, v1).
    void agregarParticula(const Particula& particula, const float uv[4]);

    bool vacio() const { return vertices_.empty(); }
    std::size_t cantidadVertices() const { return vertices_.size() / kFloatsPorVertice; }
    std::size_t cantidadParticulas() const { return particulas_; }
    const float* vertices() const { return vertices_.empty() ? nullptr : vertices_.data(); }
    std::size_t bytes() const { return vertices_.size() * sizeof(float); }

private:
    std::vector<float> vertices_;
    std::size_t particulas_ = 0;
};

#endif
