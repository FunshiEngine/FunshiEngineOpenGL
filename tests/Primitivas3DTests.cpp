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
// Pruebas headless de las primitivas 3D en CPU.
//
// La esfera es la muestra de un material (el editor la dibuja en una textura
// para mostrar como queda el material). Es geometria pura, sin OpenGL, asi que
// se comprueba entera en headless: que los vertices esten en la esfera, que
// las normales sean unitarias, que las UVs esten en [0,1] y que no haya
// triangulos degenerados (un vertice repetido haria invisible un trozo).
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <cmath>
#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/Assets/Mesh.h"
#include "../FunshiEngineGL/src/Assets/Primitivas3D.h"

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Area de un triangulo en 3D: si es cero, el triangulo es degenerado.
float areaTriangulo(const Mesh& malla, unsigned int a, unsigned int b,
                    unsigned int c) {
    const vec3& p0 = malla.vertices[a];
    const vec3& p1 = malla.vertices[b];
    const vec3& p2 = malla.vertices[c];
    vec3 e1 = p1 - p0;
    vec3 e2 = p2 - p0;
    const vec3 cr = e1.prodVetorial(e2);
    return cr.magnitude() * 0.5f;
}

void esferaGenerada() {
    const int filas = 16;
    const int columnas = 24;
    const float radio = 1.0f;
    const Mesh malla = Primitivas3D::esferaUV(filas, columnas, radio);

    const int verticesPorAnillo = columnas + 1;
    CHECK(malla.vertices.size() ==
              static_cast<std::size_t>(2 + (filas - 1) * verticesPorAnillo),
          "la esfera tiene la cantidad de vertices esperada");
    CHECK(malla.normals.size() == malla.vertices.size(),
          "hay una normal por vertice");
    CHECK(malla.uvs.size() == malla.vertices.size(),
          "hay una UV por vertice");
    CHECK(!malla.indices.empty(), "la esfera tiene triangulos");

    // Todos los vertices estan en la superficie de la esfera.
    bool todosEnLaEsfera = true;
    for (const vec3& v : malla.vertices) {
        if (std::fabs(v.magnitude() - radio) > 1e-3f) todosEnLaEsfera = false;
    }
    CHECK(todosEnLaEsfera, "todos los vertices estan en la esfera");

    // Normales unitarias y UVs en [0,1].
    bool normalesUnitarias = true;
    bool uvsEnRango = true;
    for (std::size_t i = 0; i < malla.normals.size(); ++i) {
        if (std::fabs(malla.normals[i].magnitude() - 1.f) > 1e-3f)
            normalesUnitarias = false;
        if (malla.uvs[i].x < 0.f || malla.uvs[i].x > 1.f ||
            malla.uvs[i].y < 0.f || malla.uvs[i].y > 1.f)
            uvsEnRango = false;
    }
    CHECK(normalesUnitarias, "las normales son unitarias");
    CHECK(uvsEnRango, "las UVs estan en [0,1]");

    // Indices dentro de rango.
    bool indicesEnRango = true;
    for (unsigned int indice : malla.indices) {
        if (indice >= malla.vertices.size()) indicesEnRango = false;
    }
    CHECK(indicesEnRango, "todos los indices apuntan a un vertice");

    // Ningun triangulo degenerado (ningun vertice repetido).
    bool sinDegenerados = true;
    for (std::size_t t = 0; t + 2 < malla.indices.size(); t += 3) {
        const unsigned int a = malla.indices[t];
        const unsigned int b = malla.indices[t + 1];
        const unsigned int c = malla.indices[t + 2];
        if (a == b || b == c || a == c) sinDegenerados = false;
    }
    CHECK(sinDegenerados, "ningun triangulo repite vertice");

    // Los triangulos tienen area (no son solo aristas).
    bool conArea = true;
    for (std::size_t t = 0; t + 2 < malla.indices.size(); t += 3) {
        if (areaTriangulo(malla, malla.indices[t], malla.indices[t + 1],
                          malla.indices[t + 2]) <= 1e-6f)
            conArea = false;
    }
    CHECK(conArea, "todos los triangulos tienen area");
}

// Parametros invalidos: la malla sale vacia (el renderer la descarta).
void parametrosInvalidos() {
    CHECK(Primitivas3D::esferaUV(1, 24, 1.f).isEmpty(),
          "con menos de dos filas la esfera sale vacia");
    CHECK(Primitivas3D::esferaUV(16, 2, 1.f).isEmpty(),
          "con menos de tres columnas la esfera sale vacia");
    CHECK(Primitivas3D::esferaUV(16, 24, 0.f).isEmpty(),
          "con radio cero la esfera sale vacia");
    CHECK(Primitivas3D::esferaUV(16, 24, -1.f).isEmpty(),
          "con radio negativo la esfera sale vacia");
}

int main() {
    esferaGenerada();
    parametrosInvalidos();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
