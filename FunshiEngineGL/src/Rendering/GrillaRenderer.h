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
#ifndef GRILLARENDERER_H
#define GRILLARENDERER_H

#include "LineBatch.h"
#include "LineBuilder.h"

// Render de la grilla del editor (gris infinita con difuminado en el horizonte).
//
// La grilla ya NO tiene tamano ni densidad configurables (diseño): es un plano
// azulejado con separacion FIJA (secundarias cada 1 unidad, principales cada
// 5) que se recorta a un CIRCULO horizonte de radio fijo centrado en la camara
// sobre el plano del suelo (constantes privadas en GrillaRenderer.cpp). Ese
// circulo es el LIMITE DE DIBUJADO: fuera de el no se pinta nada (da la ilusion
// de que la grilla continua mas lejos) y persigue a la camara, asi que moverse
// pinta grilla nueva por delante y deja de pintar lo que queda atras.
//
// El difuminado es radial y POR VERTICE: cada linea se recorta a su trozo
// interior al circulo y se subdivide; cada vertice lleva su propio alpha y la
// geometria se sube al batch de lineas del pipeline moderno (LineBuilder), que
// interpola el color de un extremo al otro dentro de cada segmento. El color
// efectivo se calcula afuera (AparienciaUtil::grillaEfectiva); los ejes X/Z/Y se
// pintan con colores de base (rojo, verde, amarillo) ajustados por contraste
// contra ese color (AparienciaUtil::ejeContraste).
//
// Hay un batch por ANCHO de linea, porque el ancho se resuelve en pixeles en el
// shader: secundarias (1px), principales (2px) y ejes (3px) = 3 draws por
// frame. Los tres batches se reutilizan: solo se re-suben los buffers.
class GrillaRenderer {
public:
    // Escala y horizonte de la grilla, expuestos porque otras partes del editor
    // se alinean a ellos. La guia de eje (X/Y/Z) mide su recta en multiplos de
    // la celda principal para que termine justo sobre lineas de la grilla, y
    // comparte el mismo difuminado radial para desvanecerse en el mismo
    // horizonte. Secundarias cada kSeparacionMenor unidades, una principal cada
    // kMultiploMayor de ellas.
    static constexpr float kSeparacionMenor = 1.0f;
    static constexpr int kMultiploMayor = 5;
    // Difuminado radial: opacidad plena hasta kFadeInicio y caida cuadratica
    // hasta 0 en kFadeFin, que es el radio del circulo-horizonte y por lo tanto
    // el limite de dibujado (fuera de el no se pinta nada).
    static constexpr float kFadeInicio = 40.0f;
    static constexpr float kFadeFin = 150.0f;
    // Trozos en que se subdivide cada linea para que el difuminado quede suave
    // (el batch interpola el alpha entre extremos de cada trozo).
    static constexpr int kSubdivisiones = 6;

    ~GrillaRenderer() { destruir(); }

    // Dibuja la grilla infinita en el espacio local del objeto "Grilla" (la
    // matriz model del componente se pasa al shader de lineas), con el color
    // efectivo y la posicion de la camara EN EL MUNDO (se transforma a local
    // aqui) para extender el plano y calcular el difuminado.
    void dibujar(const float model[16], const float colorGrilla[3],
                 const float camaraMundo[3]);

    // Descarta la geometria CPU cacheada (higiene defensiva).
    void destruir();

private:
    // Un par (geometria CPU + batch de GPU) por ancho de linea. Ambos miembros
    // se reutilizan frame a frame: solo se re-suben los buffers.
    LineBuilder secundario_;
    LineBuilder principal_;
    LineBuilder ejes_;
    LineBatch secundarioBatch_;
    LineBatch principalBatch_;
    LineBatch ejesBatch_;
};

#endif
