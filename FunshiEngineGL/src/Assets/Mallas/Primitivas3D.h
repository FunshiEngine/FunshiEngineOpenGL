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
#ifndef PRIMITIVAS3D_H
#define PRIMITIVAS3D_H

#include <cmath>

#include "Mesh.h"

// Primitivas geometricas 3D en CPU, independientes de OpenGL. Igual que las
// de Primitivas2D, se generan como Mesh (el camino que ya usa el renderer) y
// se pueden probar en los targets headless.
namespace Primitivas3D {

// Esfera de radio 1 con la UV del achatamiento habitual (una "slice" de
// pizza por columna). La esfera se usa como muestra de un material: el
// editor la dibuja en una textura pequena para mostrar como queda.
//
// Se generan un anillo de vertices por fila (mas una fila en cada polo) con
// normales que apuntan al centro y UVs en [0,1]. Los polos no tienen triangulo
// degenerado: cada cara del anillo usa el vertice del polo.
inline Mesh esferaUV(int filas = 16, int columnas = 24, float radio = 1.0f) {
    Mesh malla;
    malla.name = "esferaUV";
    if (filas < 2 || columnas < 3 || radio <= 0.f) return malla;

    // Vertices: polo superior + (filas - 1) anillos + polo inferior.
    const int verticesPorAnillo = columnas + 1; // se repite el ultimo para la UV
    malla.vertices.reserve(2 + (filas - 1) * verticesPorAnillo);
    malla.normals.reserve(malla.vertices.capacity());
    malla.uvs.reserve(malla.vertices.capacity());
    malla.indices.reserve(static_cast<std::size_t>(filas) *
                          static_cast<std::size_t>(columnas) * 6);

    // Polo superior (v = 0).
    malla.vertices.emplace_back(0.f, radio, 0.f);
    malla.normals.emplace_back(0.f, 1.f, 0.f);
    malla.uvs.emplace_back(0.5f, 0.f);

    for (int f = 1; f < filas; ++f) {
        const float v = static_cast<float>(f) / static_cast<float>(filas);
        const float phi = v * 3.14159265358979f; // de 0 (polo norte) a pi
        const float y = radio * std::cos(phi);
        const float r = radio * std::sin(phi);
        for (int c = 0; c <= columnas; ++c) {
            const float u = static_cast<float>(c) / static_cast<float>(columnas);
            const float theta = u * 6.28318530717959f; // vuelta completa
            const float x = r * std::cos(theta);
            const float z = r * std::sin(theta);
            malla.vertices.emplace_back(x, y, z);
            const float largo = radio;
            malla.normals.emplace_back(x / largo, y / largo, z / largo);
            malla.uvs.emplace_back(u, v);
        }
    }

    // Polo inferior (v = 1).
    const int poloInferior = static_cast<int>(malla.vertices.size());
    malla.vertices.emplace_back(0.f, -radio, 0.f);
    malla.normals.emplace_back(0.f, -1.f, 0.f);
    malla.uvs.emplace_back(0.5f, 1.f);

    // Triangulos entre el polo superior y el primer anillo.
    for (int c = 0; c < columnas; ++c) {
        malla.indices.push_back(0);
        malla.indices.push_back(1 + c);
        malla.indices.push_back(1 + c + 1);
    }

    // Triangulos entre anillos consecutivos.
    for (int f = 0; f < filas - 2; ++f) {
        const int base = 1 + f * verticesPorAnillo;
        const int baseSiguiente = base + verticesPorAnillo;
        for (int c = 0; c < columnas; ++c) {
            malla.indices.push_back(base + c);
            malla.indices.push_back(baseSiguiente + c);
            malla.indices.push_back(baseSiguiente + c + 1);
            malla.indices.push_back(base + c);
            malla.indices.push_back(baseSiguiente + c + 1);
            malla.indices.push_back(base + c + 1);
        }
    }

    // Triangulos del ultimo anillo al polo inferior.
    const int baseUltimo = 1 + (filas - 2) * verticesPorAnillo;
    for (int c = 0; c < columnas; ++c) {
        malla.indices.push_back(baseUltimo + c);
        malla.indices.push_back(poloInferior);
        malla.indices.push_back(baseUltimo + c + 1);
    }

    return malla;
}

} // namespace Primitivas3D

#endif // PRIMITIVAS3D_H
