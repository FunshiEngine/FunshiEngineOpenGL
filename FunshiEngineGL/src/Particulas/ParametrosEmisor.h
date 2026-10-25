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
#ifndef PARAMETROSEMISOR_H
#define PARAMETROSEMISOR_H

// Parametros de un emisor de particulas: los datos que el inspector edita y
// que la escena guarda.
//
// Son datos, no comportamiento: el sistema de particulas los lee al emitir y
// copia los que necesita a cada particula (gravedad, rozamiento), asi una
// particula sobrevive a la disappearance de su emisor.
struct ParametrosEmisor {
    // Cuantas particulas suelta por segundo.
    int cantidadPorSegundo = 20;
    // Vida de cada particula, en segundos (rango).
    float vidaMinima = 0.6f;
    float vidaMaxima = 1.2f;
    // Velocidad inicial, en unidades de mundo por segundo (rango).
    float velocidadMinima = 1.0f;
    float velocidadMaxima = 2.0f;
    // Tamano del quad de cada particula, en unidades de mundo (rango).
    float tamanoMinimo = 0.2f;
    float tamanoMaximo = 0.6f;
    // Color RGBA con el que nacen.
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    // Eje del cono de emision. Si es (0,0,0), la direccion es aleatoria.
    float direccion[3] = {0.0f, 1.0f, 0.0f};
    // Apertura del cono: 0 = todas en la direccion, 1 = cono de 90 grados.
    float dispersion = 0.25f;
    // Aceleracion constante que se aplica a las particulas.
    float gravedad[3] = {0.0f, -1.0f, 0.0f};
    // Fraccion de velocidad que se pierde por segundo.
    float rozamiento = 0.0f;
    // Rotacion inicial y su velocidad (radianes por segundo).
    float rotacionInicial = 0.0f;
    float velocidadRotacion = 0.0f;
};

#endif
