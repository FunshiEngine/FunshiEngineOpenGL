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
#ifndef PARTICULA_H
#define PARTICULA_H

#include <memory>

#include "PerfilParticula.h"

// Estado de una particula viva.
//
// Es un dato plano: el sistema de particulas lo mueve y el renderer lo
// dibuja, y nadie mas lo toca. La particula guarda su propia gravedad y su
// rozamiento, de modo que el sistema puede actualizarla sin volver al emisor
// que la creo (un emisor puede desaparecer de la escena sin arrastrar a sus
// particulas).
struct Particula {
    // Posicion en el mundo.
    float posicion[3] = {0.0f, 0.0f, 0.0f};
    // Velocidad en unidades de mundo por segundo.
    float velocidad[3] = {0.0f, 0.0f, 0.0f};
    // Aceleracion constante que se le suma a la velocidad.
    float gravedad[3] = {0.0f, 0.0f, 0.0f};
    // Color RGBA.
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    // Segundos que le quedan de vida (0 o menos = muerta).
    float vida = 0.0f;
    // Segundos que vivio en total: el denominador de la fraccion de vida.
    float vidaMaxima = 0.0f;
    // Tamano base del quad en unidades de mundo (el que le dio el emisor).
    float tamanoBase = 1.0f;
    // Tamano efectivo: el base por el multiplicador que da la curva del
    // perfil en este instante de su vida.
    float tamano = 1.0f;
    // Rozamiento: fraccion de velocidad que se pierde por segundo.
    float rozamiento = 0.0f;
    // Rotacion del quad y su velocidad (radianes por segundo).
    float rotacion = 0.0f;
    float velocidadRotacion = 0.0f;
    // Perfil de efecto que la creo (nulo si el emisor no tenia perfil). Se
    // comparte con el emisor: si el emisor desaparece, la particula sigue
    // viva con su perfil.
    std::shared_ptr<const PerfilParticula> perfil;
    // Ruta opcional de la textura de particula que sobreescribe la del perfil.
    // Vacia = usar la del perfil.
    std::string texturaOverride;

    bool viva() const noexcept { return vida > 0.0f; }

    // Fraccion de vida vivida: 0 al nacer, 1 al morir. Es lo que consultan las
    // curvas del perfil de efecto para escalar el tamano y el color.
    float fraccionVida() const noexcept {
        if (vidaMaxima <= 0.0f) return 1.0f;
        const float f = 1.0f - (vida / vidaMaxima);
        if (f < 0.0f) return 0.0f;
        if (f > 1.0f) return 1.0f;
        return f;
    }
};

#endif
