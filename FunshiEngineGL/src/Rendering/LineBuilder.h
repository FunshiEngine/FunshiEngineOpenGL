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
#ifndef LINEBUILDER_H
#define LINEBUILDER_H

#include <cstddef>
#include <vector>

// Constructor CPU de la geometria de las lineas "gruesas" del engine (grilla,
// marcadores de luz/camara y wireframes de los colliders) para el perfil core.
//
// Por que NO se dibujan con GL_LINES: en un contexto core glLineWidth solo
// garantiza 1 pixel (los anchos de 2/3 px de la grilla no se pueden pedir), y
// ademas las primitivas de linea no se antialiasan de forma portable. La
// solucion es expandir cada segmento a un CUADrilatero (dos triangulos) y
// resolver el ancho en el vertex shader, en pixeles, con la perpendicular de la
// direccion proyectada: asi el ancho es el mismo a cualquier distancia y en
// cualquier resolucion (ver kLineVertexShader).
//
// Cada segmento aporta kVerticesPorSegmento vertices y NO hay buffer de
// indices: los triangulos no comparten vertices, asi que alcanza con
// glDrawArrays y se evita el EBO.
//
// Vertice (kFloatsPorVertice floats, layout del VAO):
//   [0..2]  inicio del segmento (mundo/local)
//   [3..5]  fin del segmento
//   [6]     lado: -1 o +1 (extremo del ancho, perpendicular a la direccion)
//   [7]     avance: 0 en el extremo del inicio, 1 en el del fin
//   [8..11] color RGBA
// El shader interpola (inicio->fin) con "avance" y ensancha hacia "lado", y usa
// el color de cada extremo para conservar los difuminados por vertice (la
// grilla se funde en el horizonte).
//
// Es CPU puro a proposito: no incluye OpenGL ni glm, asi se puede probar en los
// targets headless (tests/RenderingTests.cpp) igual que Mesh::computeTangents.
class LineBuilder {
public:
    // Floats por vertice y vertices por segmento (dos triangulos).
    static const std::size_t kFloatsPorVertice = 12;
    static const std::size_t kVerticesPorSegmento = 6;
    // Tamano del vertice en bytes (stride del VAO).
    static std::size_t stride() { return kFloatsPorVertice * sizeof(float); }
    // Tamano de un endpoint (xyz) dentro del vertice.
    static std::size_t offsetInicio() { return 0; }
    static std::size_t offsetFin() { return 3 * sizeof(float); }
    static std::size_t offsetLado() { return 6 * sizeof(float); }
    static std::size_t offsetAvance() { return 7 * sizeof(float); }
    static std::size_t offsetColor() { return 8 * sizeof(float); }

    // Descarta la geometria acumulada (reutiliza la capacidad reservada).
    void limpiar() { vertices_.clear(); }

    // Segmento con un unico color (RGBA) en ambos extremos.
    void agregarSegmento(const float inicio[3], const float fin[3],
                         const float rgba[4]) {
        agregarSegmento(inicio, fin, rgba, rgba);
    }

    // Segmento con color por extremo (RGBA): el quad interpola de un color al
    // otro a lo largo del segmento, que es lo que necesita el difuminado radial
    // de la grilla.
    void agregarSegmento(const float inicio[3], const float fin[3],
                         const float rgbaInicio[4], const float rgbaFin[4]);

    // Polilinea de "cantidad" endpoints (xyz contiguos, 3 floats cada uno).
    // "cerrada" une el ultimo con el primero (GL_LINE_LOOP). Con cantidad < 2
    // no agrega nada.
    void agregarPolilinea(const float* puntos, std::size_t cantidad,
                          bool cerrada, const float rgba[4]);
    // Igual, con color por vertice: "rgba" tiene 4 floats por punto.
    void agregarPolilineaVertices(const float* puntosRGBA, std::size_t cantidad,
                                  bool cerrada);

    // Aristas sobre un conjunto de vertices ya calculado: "aristas" son pares
    // de indices (edgeCount pares), como en el wireframe de los colliders.
    // Los indices fuera de rango se ignoran. Con color por vertice: "rgba" son
    // 4 floats por vertice de "vertices".
    void agregarAristas(const float* vertices, std::size_t cantidad,
                        const int* aristas, std::size_t pares,
                        const float rgba[4]);
    // Igual, con color por vertice (4 floats por vertice de "vertices").
    void agregarAristasColoreadas(const float* verticesRGBA,
                                  std::size_t cantidad, const int* aristas,
                                  std::size_t pares);

    // Caja alineada a los ejes: las 12 aristas de un wireframe. "centro" y
    // "semilado" van en el mismo espacio que el resto de la geometria.
    void agregarCaja(const float centro[3], const float semilado[3],
                     const float rgba[4]);

    bool vacio() const { return vertices_.empty(); }
    std::size_t cantidadVertices() const {
        return vertices_.size() / kFloatsPorVertice;
    }
    std::size_t cantidadSegmentos() const {
        return cantidadVertices() / kVerticesPorSegmento;
    }
    // Puntero a los floats (nullptr si vacio) y su tamano en bytes, que es lo
    // que consume LineBatch::upload/update.
    const float* vertices() const {
        return vertices_.empty() ? nullptr : vertices_.data();
    }
    std::size_t bytes() const { return vertices_.size() * sizeof(float); }

private:
    std::vector<float> vertices_;

    // Empuja el quad de los dos triangulos que cubren el segmento a->b.
    void agregarQuad(const float a[3], const float b[3], const float rgbaA[4],
                     const float rgbaB[4]);
    // Copia un endpoint (xyz) a los 3 primeros floats del vertice en curso.
    void copiarEndpoint(std::size_t base, const float punto[3]);
};

#endif
