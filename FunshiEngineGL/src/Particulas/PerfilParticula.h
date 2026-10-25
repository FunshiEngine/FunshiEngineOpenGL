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
#ifndef PERFILPARTICULA_H
#define PERFILPARTICULA_H

#include <array>
#include <string>
#include <vector>

// Perfil de efecto: el guion de una particula a lo largo de su vida.
//
// Es un efecto completo (textura, vida, velocidad, gravedad y las curvas de
// tamano y color). Un emisor puede usarlo en vez de sus propios parametros:
// con perfil, la particula nace con los datos del perfil y sus curvas
// mandan sobre el tamano y el color; sin perfil, valen los parametros del
// emisor.
//
// Las curvas se muestrean por la fraccion de vida (0 al nacer, 1 al morir) y se
// interpolan de forma lineal entre sus puntos.
struct PerfilParticula {
    // Solo para identificar el efecto en el inspector.
    std::string nombre;
    // Ruta de la textura, relativa a la raiz de assets del proyecto.
    std::string textura;
    // Rango de vida de la particula, en segundos.
    float vidaMinima = 0.6f;
    float vidaMaxima = 1.2f;
    // Rango de velocidad inicial, en unidades de mundo por segundo.
    float velocidadMinima = 1.0f;
    float velocidadMaxima = 2.0f;
    // Aceleracion constante.
    float aceleracion[3] = {0.0f, -1.0f, 0.0f};
    // Fraccion de velocidad que se pierde por segundo.
    float rozamiento = 0.0f;
    // Multiplicadores de tamano por punto de la curva (1 = el tamano del
    // emisor). El primero corresponde al nacimiento y el ultimo a la muerte.
    std::vector<float> tamanos;
    // Colores RGBA por punto de la curva, con la misma convencion.
    std::vector<std::array<float, 4>> colores;

    // Un perfil sin curvas no cambia el tamano ni el color: es valido, pero
    // se comporta como un emisor sin perfil.
    bool tieneCurvas() const { return !tamanos.empty() || !colores.empty(); }

    // Multiplicador de tamano en la fraccion de vida f (0..1). Sin curva de
    // tamanos devuelve 1.
    float multiplicadorTamano(float f) const;

    // Color RGBA en la fraccion de vida f (0..1). Sin curva de colores
    // devuelve blanco opaco.
    void colorEn(float f, float salida[4]) const;
};

#endif
