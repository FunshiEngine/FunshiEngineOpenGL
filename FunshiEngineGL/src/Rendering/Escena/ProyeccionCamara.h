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
#ifndef PROYECCION_CAMARA_H
#define PROYECCION_CAMARA_H

#include <cmath>

// Consultas sobre una matriz de proyeccion 4x4 guardada en un float[16] en
// orden column-major (el de glm/OpenGL). Header-only y puro: no depende de glm
// ni de la camara, de modo que el renderer de lineas y cualquier consumidor
// pueden clasificar la proyeccion sin arrastrar las clases del motor.
namespace ProyeccionCamara {

// Una proyeccion es ortografica cuando la ultima fila del bloque 3x4 no lleva
// la componente en perspectiva. En la perspectiva de glm/OpenGL el elemento
// (fila 2, columna 3) vale -1 (m[2][3] -> indice 11 en column-major); en la
// ortografica vale 0, porque el rayo de vista no converge.
inline bool esOrtografica(const float p[16]) {
    return std::fabs(p[11]) < 1e-6f;
}

// Distancia al plano cercano que el shader de lineas usa para recortar los
// segmentos que quedan por delante de la camara. En la perspectiva de glm se
// cumple m[2][2] = -(f+n)/(f-n) (indice 10) y m[3][2] = -2fn/(f-n) (indice 14),
// de donde n = m32 / (m22 - 1). Devuelve 0 en ortografica (sin recorte: la
// geometria no converge al ojo) y el valor por defecto si la matriz no es
// utilizable, para no dejar el uniform en un numero sin sentido.
inline float nearDesdeProyeccion(const float p[16]) {
    if (esOrtografica(p)) return 0.0f;

    const float den = p[10] - 1.0f;
    if (std::fabs(den) < 1e-6f) return 0.01f;

    const float near = p[14] / den;
    if (!std::isfinite(near) || near <= 0.0f) return 0.01f;
    return near;
}

}  // namespace ProyeccionCamara

#endif  // PROYECCION_CAMARA_H
