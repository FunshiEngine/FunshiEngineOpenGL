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
#include "MatrizLuz.h"

#include <cmath>

// glm::lookAt y glm::ortho no estan en el nucleo de glm (glm/glm.hpp): viven
// en la extension de transformaciones de matriz.
#include <glm/gtc/matrix_transform.hpp>

namespace {

// Direccion auxiliar de la vista. Se usa el (0,1,0) salvo que la luz apunte
// casi vertical: ahi seria paralelo a la direccion de la vista y el producto
// vectorial daria un vector nulo (el NaN del lookAt).
glm::vec3 auxiliarVista(const glm::vec3& direccion) {
    if (std::fabs(direccion.y) > 0.999f) return glm::vec3(0.f, 0.f, 1.f);
    return glm::vec3(0.f, 1.f, 0.f);
}

}  // namespace

glm::mat4 vistaLuz(const glm::vec3& posicion, const glm::vec3& direccion) {
    glm::vec3 dir = direccion;
    const float largo = glm::length(dir);
    // Una direccion de luz de longitud cero no define una vista: se usa la
    // direccion por defecto (hacia abajo) en vez de propagar un NaN.
    if (largo < 1e-6f) {
        dir = glm::vec3(0.f, -1.f, 0.f);
    } else {
        dir = dir / largo;
    }
    return glm::lookAt(posicion, posicion + dir, auxiliarVista(dir));
}

glm::mat4 proyeccionLuz(float radio, float cerca, float lejos) {
    // El frustum tiene que tener volumen: con el plano lejano por delante del
    // cercano la proyeccion se invierte y el mapa de sombras sale vacio.
    if (radio < 0.001f) radio = 0.001f;
    if (lejos <= cerca) lejos = cerca + 0.001f;
    return glm::ortho(-radio, radio, -radio, radio, cerca, lejos);
}

glm::mat4 matrizLuz(const FocoLuz& foco) {
    return proyeccionLuz(foco.radio, foco.cerca, foco.lejos) *
           vistaLuz(foco.posicion, foco.direccion);
}

FocoLuz focoPosicional(const glm::vec3& posicion, const glm::vec3& direccion,
                       float radio, float alcance) {
    FocoLuz foco;
    foco.posicion = posicion;
    foco.direccion = direccion;
    foco.radio = radio;
    foco.cerca = 0.05f;
    foco.lejos = alcance;
    return foco;
}

FocoLuz focoDireccional(const glm::vec3& centro, const glm::vec3& direccion,
                        float radio, float alcance) {
    glm::vec3 dir = direccion;
    const float largo = glm::length(dir);
    if (largo < 1e-6f) {
        dir = glm::vec3(0.f, -1.f, 0.f);
    } else {
        dir = dir / largo;
    }
    FocoLuz foco;
    // La luz direccional no esta en ningun sitio: se coloca 'alcance' unidades
    // atras del centro para que el frustum abarque desde ese punto hasta el
    // doble de alcance por delante del centro.
    foco.posicion = centro - dir * alcance;
    foco.direccion = dir;
    foco.radio = radio;
    foco.cerca = 0.05f;
    foco.lejos = alcance * 2.f;
    return foco;
}
