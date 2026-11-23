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
#ifndef RAYOPANTALLA_H
#define RAYOPANTALLA_H

#include <glm/glm.hpp>

// Rayo del mundo que pasa por un pixel de la pantalla (CPU puro, testeable).
// Lo comparten el picking de objetos, el pintado de tiles y el arrastre de
// archivos a la escena: los tres necesitan "que hay debajo del mouse".
struct Rayo {
    glm::vec3 origen{0.f};
    glm::vec3 direccion{0.f, 0.f, -1.f};  // unitaria
};

namespace RayoPantalla {

// Rayo por el pixel (x, y) de una pantalla de ancho x alto (origen arriba a la
// izquierda), con las matrices de vista y proyeccion de la camara. Devuelve
// false si los datos no definen un rayo (pantalla vacia, matriz singular).
bool desdePixel(float x, float y, float ancho, float alto, const glm::mat4& vista,
                const glm::mat4& proyeccion, Rayo& salida);

// Interseccion con el plano dot(normal, p) = distancia. Devuelve false si el
// rayo es paralelo al plano o el corte queda detras del origen.
bool cortarPlano(const Rayo& rayo, const glm::vec3& normal, float distancia,
                 glm::vec3& punto);

}  // namespace RayoPantalla

#endif
