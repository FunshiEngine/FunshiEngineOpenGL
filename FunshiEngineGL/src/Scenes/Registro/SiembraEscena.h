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
#ifndef SIEMBRA_ESCENA_H
#define SIEMBRA_ESCENA_H

#include "../../Configuracion/DimensionProyecto.h"

// Politica de siembra de una escena segun la dimension del proyecto, sin disco
// ni UI: decide si a un proyecto le corresponde la grilla de rejilla 3D y desde
// donde arranca la camara principal. Header-only e inline para poder probarla
// headless sin depender de la escena ni del editor.
namespace SiembraEscena {

// Posicion inicial de la camara sembrada por la escena: en el plano XY (2D) la
// camara mira el plano de frente desde +Z; con profundidad (3D) se eleva para
// ver la grilla.
struct PosicionInicial {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

// La grilla de rejilla es un auxiliar del mundo 3D: en un proyecto 2D el plano
// de juego es el XY y no se siembra (la escena dibuja el plano por otro camino).
inline bool sembrarGrilla(DimensionProyecto dimension) {
    return dimension == DimensionProyecto::TresD;
}

inline PosicionInicial posicionCamaraInicial(DimensionProyecto dimension) {
    if (dimension == DimensionProyecto::DosD) return {0.f, 0.f, 40.f};
    return {0.f, 8.f, 40.f};
}

}  // namespace SiembraEscena

#endif  // SIEMBRA_ESCENA_H