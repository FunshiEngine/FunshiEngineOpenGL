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

#include <vector>

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
// interior al circulo y se subdivide; cada vertice lleva su propio alpha (7
// floats: xyz + rgba) via IRenderBackend::drawLinePairsRGBA, opaco cerca de la
// camara y transparente en el borde. El color efectivo se calcula afuera
// (AparienciaUtil::grillaEfectiva); los ejes X/Z/Y se pintan con colores de
// base (rojo, verde, amarillo) ajustados por contraste contra ese color
// (AparienciaUtil::ejeContraste). El ancho de linea distingue secundarias
// (1px), principales (2px) y ejes (3px).
class GrillaRenderer {
public:
    ~GrillaRenderer() { destruir(); }

    // Dibuja la grilla infinita en el espacio local del objeto "Grilla" (la
    // matriz model del componente, aplicada como pushMatrix/multMatrix), con el
    // color efectivo y la posicion de la camara EN EL MUNDO (se transforma a
    // local aqui) para extender el plano y calcular el difuminado.
    void dibujar(const float model[16], const float colorGrilla[3],
                 const float camaraMundo[3]);

    // Descarta los vectores CPU cacheados (higiene defensiva).
    void destruir();

private:
    // Vectores intercalados xyz+rgba, reutilizados frame a frame.
    std::vector<float> minorVertices_;
    std::vector<float> majorVertices_;
    std::vector<float> ejeXVertices_;
    std::vector<float> ejeZVertices_;
    std::vector<float> ejeYVertices_;
};

#endif