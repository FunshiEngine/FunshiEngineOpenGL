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
#include "LineBuilder.h"

void LineBuilder::copiarEndpoint(std::size_t base, const float punto[3]) {
    for (int i = 0; i < 3; ++i) vertices_[base + static_cast<std::size_t>(i)] = punto[i];
}

void LineBuilder::agregarQuad(const float a[3], const float b[3],
                              const float rgbaA[4], const float rgbaB[4]) {
    // Los 6 vertices del quad, como (avance, lado). Los dos triangulos son
    // (0,-1)(0,+1)(1,+1) y (0,-1)(1,+1)(1,-1): cubren el rectangulo a->b sin
    // compartir vertices, asi que se dibuja con glDrawArrays(GL_TRIANGLES).
    static const float kLados[6] = {-1.f, 1.f, 1.f, -1.f, 1.f, -1.f};
    static const float kAvances[6] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f};

    const std::size_t base = vertices_.size();
    vertices_.resize(base + kVerticesPorSegmento * kFloatsPorVertice);

    for (std::size_t v = 0; v < kVerticesPorSegmento; ++v) {
        std::size_t o = base + v * kFloatsPorVertice;
        copiarEndpoint(o, a);
        copiarEndpoint(o + 3, b);
        vertices_[o + 6] = kLados[v];
        vertices_[o + 7] = kAvances[v];
        // Cada extremo toma su color: los cuatro corners de "avance" 0 usan el
        // del inicio y los de "avance" 1 el del fin.
        const float* color = (kAvances[v] == 0.f) ? rgbaA : rgbaB;
        for (int c = 0; c < 4; ++c) vertices_[o + 8 + static_cast<std::size_t>(c)] = color[c];
    }
}

void LineBuilder::agregarSegmento(const float inicio[3], const float fin[3],
                                  const float rgbaInicio[4],
                                  const float rgbaFin[4]) {
    if (!inicio || !fin || !rgbaInicio || !rgbaFin) return;
    agregarQuad(inicio, fin, rgbaInicio, rgbaFin);
}

void LineBuilder::agregarPolilinea(const float* puntos, std::size_t cantidad,
                                   bool cerrada, const float rgba[4]) {
    if (!puntos || !rgba || cantidad < 2) return;
    for (std::size_t i = 0; i + 1 < cantidad; ++i)
        agregarSegmento(&puntos[i * 3], &puntos[(i + 1) * 3], rgba);
    if (cerrada)
        agregarSegmento(&puntos[(cantidad - 1) * 3], &puntos[0], rgba);
}

void LineBuilder::agregarPolilineaVertices(const float* puntosRGBA,
                                           std::size_t cantidad, bool cerrada) {
    if (!puntosRGBA || cantidad < 2) return;
    for (std::size_t i = 0; i + 1 < cantidad; ++i)
        agregarSegmento(&puntosRGBA[i * 7], &puntosRGBA[(i + 1) * 7],
                        &puntosRGBA[i * 7 + 3], &puntosRGBA[(i + 1) * 7 + 3]);
    if (cerrada)
        agregarSegmento(&puntosRGBA[(cantidad - 1) * 7], &puntosRGBA[0],
                        &puntosRGBA[(cantidad - 1) * 7 + 3], &puntosRGBA[3]);
}

void LineBuilder::agregarAristas(const float* vertices, std::size_t cantidad,
                                 const int* aristas, std::size_t pares,
                                 const float rgba[4]) {
    if (!vertices || !aristas || !rgba) return;
    for (std::size_t e = 0; e < pares; ++e) {
        const int i0 = aristas[e * 2 + 0];
        const int i1 = aristas[e * 2 + 1];
        if (i0 < 0 || i1 < 0) continue;
        if (static_cast<std::size_t>(i0) >= cantidad ||
            static_cast<std::size_t>(i1) >= cantidad)
            continue;
        agregarSegmento(&vertices[static_cast<std::size_t>(i0) * 3],
                        &vertices[static_cast<std::size_t>(i1) * 3], rgba);
    }
}

void LineBuilder::agregarAristasColoreadas(const float* verticesRGBA,
                                           std::size_t cantidad,
                                           const int* aristas,
                                           std::size_t pares) {
    if (!verticesRGBA || !aristas) return;
    for (std::size_t e = 0; e < pares; ++e) {
        const int i0 = aristas[e * 2 + 0];
        const int i1 = aristas[e * 2 + 1];
        if (i0 < 0 || i1 < 0) continue;
        if (static_cast<std::size_t>(i0) >= cantidad ||
            static_cast<std::size_t>(i1) >= cantidad)
            continue;
        agregarSegmento(&verticesRGBA[static_cast<std::size_t>(i0) * 7],
                        &verticesRGBA[static_cast<std::size_t>(i1) * 7],
                        &verticesRGBA[static_cast<std::size_t>(i0) * 7 + 3],
                        &verticesRGBA[static_cast<std::size_t>(i1) * 7 + 3]);
    }
}

void LineBuilder::agregarCaja(const float centro[3], const float semilado[3],
                              const float rgba[4]) {
    if (!centro || !semilado || !rgba) return;

    // Los 8 corners, en el orden habitual: 0..3 cara -Z, 4..7 cara +Z.
    float v[8][3];
    for (int i = 0; i < 8; ++i) {
        v[i][0] = centro[0] + ((i & 1) ? semilado[0] : -semilado[0]);
        v[i][1] = centro[1] + ((i & 2) ? semilado[1] : -semilado[1]);
        v[i][2] = centro[2] + ((i & 4) ? semilado[2] : -semilado[2]);
    }

    // 12 aristas: 4 en cada cara XY/XZ/YZ y 4 verticales.
    static const int kAristas[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0}, // cara -Z
        {4, 5}, {5, 7}, {7, 6}, {6, 4}, // cara +Z
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // verticales
    };
    for (const auto& e : kAristas)
        agregarSegmento(v[e[0]], v[e[1]], rgba);
}
