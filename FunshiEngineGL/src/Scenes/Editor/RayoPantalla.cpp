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
#include "RayoPantalla.h"

#include <cmath>

namespace RayoPantalla {

bool desdePixel(float x, float y, float ancho, float alto, const glm::mat4& vista,
                const glm::mat4& proyeccion, Rayo& salida) {
    if (ancho <= 0.f || alto <= 0.f) return false;
    const float ndcX = (2.f * x) / ancho - 1.f;
    const float ndcY = 1.f - (2.f * y) / alto;
    const glm::mat4 inversa = glm::inverse(proyeccion * vista);

    glm::vec4 cerca = inversa * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 lejos = inversa * glm::vec4(ndcX, ndcY, 1.f, 1.f);
    if (std::fabs(cerca.w) < 1e-6f || std::fabs(lejos.w) < 1e-6f) return false;
    cerca /= cerca.w;
    lejos /= lejos.w;

    const glm::vec3 direccion = glm::vec3(lejos - cerca);
    const float largo = glm::length(direccion);
    if (!(largo > 1e-6f)) return false;
    salida.origen = glm::vec3(cerca);
    salida.direccion = direccion / largo;
    return true;
}

bool cortarPlano(const Rayo& rayo, const glm::vec3& normal, float distancia,
                 glm::vec3& punto) {
    const float denominador = glm::dot(normal, rayo.direccion);
    if (std::fabs(denominador) < 1e-6f) return false;
    const float t = (distancia - glm::dot(normal, rayo.origen)) / denominador;
    if (t < 0.f) return false;
    punto = rayo.origen + rayo.direccion * t;
    return true;
}

}  // namespace RayoPantalla
