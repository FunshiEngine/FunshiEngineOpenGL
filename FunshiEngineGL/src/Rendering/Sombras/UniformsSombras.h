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
#ifndef UNIFORMSSOMBRAS_H
#define UNIFORMSSOMBRAS_H

#include <vector>

#include "MapaSombras.h"

class ShaderProgram;

namespace Rendering {

// Union entre los proyectores de sombra elegidos en la pasada y el shader de
// mallas. Todo se indexa por SLOT (posicion del proyector en la lista): el
// slot s usa uShadowMap<s> (unidad kUnidadSombra2D + s) si es un mapa 2D, o
// uShadowCube<s> (unidad kUnidadSombraCubo + s) si es un cubemap. Las unidades
// estan separadas por tipo de sampler: mezclar sampler2DShadow y samplerCube en
// la misma unidad es un error de OpenGL al dibujar.
namespace UniformsSombras {

constexpr int kUnidadSombra2D = 4;    // 4..7
constexpr int kUnidadSombraCubo = 8;  // 8..11

// Fija las unidades de todos los samplers de sombra. Una vez por programa.
void configurarSamplers(ShaderProgram& shader);

// Sube los datos de cada slot (matriz, tipo, luz, sesgo, fuerza, texeles,
// posicion y alcance). Se llama cuando cambian los proyectores.
void subir(ShaderProgram& shader, const std::vector<ProyectorSombra>& proyectores);

// Enlaza la textura de cada slot en su unidad. Se llama antes de dibujar: otras
// pasadas (sprites, cielo, previas) pueden haber dejado otras texturas ahi.
void enlazarTexturas(Backend::IRenderBackend& backend,
                     const std::vector<ProyectorSombra>& proyectores);

}  // namespace UniformsSombras
}  // namespace Rendering

#endif
