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
#ifndef MATRIZLUZ_H
#define MATRIZLUZ_H

#include <glm/glm.hpp>

// Foco que proyecta el mapa de sombras: desde donde mira la luz, hacia donde
// apunta y que volumen abarca. Todo es CPU puro (no toca GL): la matriz
// resultante la usa el pase de sombras para proyectar y el shader de mallas
// para consultar el mapa.
struct FocoLuz {
    // Posicion desde la que mira la luz (en el caso de una luz direccional es
    // sintetica: se coloca atras del centro de la escena).
    glm::vec3 posicion{0.f, 10.f, 0.f};
    // Direccion hacia la que apunta la luz (unitaria).
    glm::vec3 direccion{0.f, -1.f, 0.f};
    // Semi-lado del frustum ortografico: la sombra abarca una caja de lado
    // 2*radio alrededor del foco.
    float radio = 10.f;
    // Plano cercano y lejano del frustum, en unidades del mundo.
    float cerca = 0.05f;
    float lejos = 50.f;
};

// Vista de la luz: mira desde su posicion hacia su direccion. El vector
// auxiliar se elige para que la vista no se degenere cuando la luz apunta
// justo hacia arriba o hacia abajo (en ese caso el (0,1,0) seria paralelo a
// la direccion y el resultado seria un NaN).
glm::mat4 vistaLuz(const glm::vec3& posicion, const glm::vec3& direccion);

// Proyeccion ortografica del frustum de la luz: una caja de lado 2*radio
// entre los planos cercano y lejano. El sesgo de la sombra se aplica en el
// shader, no aqui.
glm::mat4 proyeccionLuz(float radio, float cerca, float lejos);

// Matriz completa de la luz: proyeccion * vista. Es la que proyecta los
// vertices al mapa de sombras y la que consulta el fragmento sombreado.
glm::mat4 matrizLuz(const FocoLuz& foco);

// Foco de una luz posicional (spot o puntual): la luz esta donde dice la luz
// y el frustum arranca en su plano cercano.
FocoLuz focoPosicional(const glm::vec3& posicion, const glm::vec3& direccion,
                       float radio, float alcance);

// Foco de una luz direccional: la luz no esta en ningun sitio, asi que se
// coloca atras del centro de la escena y el frustum abarca el tramo que va
// desde el centro hasta 'alcance' unidades mas alla.
FocoLuz focoDireccional(const glm::vec3& centro, const glm::vec3& direccion,
                        float radio, float alcance);

// Matriz de una luz direccional estabilizada: igual que matrizLuz(focoDireccional)
// pero el centro se ajusta a la grilla de texeles del mapa en el espacio de la
// luz. Asi, cuando la camara (y con ella el centro) se mueve, la sombra no
// "nada": el mapa solo se desplaza de a texeles enteros.
glm::mat4 matrizDireccionalEstable(const glm::vec3& centro,
                                   const glm::vec3& direccion, float radio,
                                   float alcance, int resolucion);

// Matriz de una luz spot: perspectiva cuyo campo cubre el cono de la luz
// (angulo de corte en grados, medido desde el eje) con un margen para que el
// PCF no lea fuera del mapa en el borde del cono.
glm::mat4 matrizLuzSpot(const glm::vec3& posicion, const glm::vec3& direccion,
                        float anguloCorteGrados, float alcance);

// Plano cercano de las proyecciones en perspectiva de las luces. Es
// proporcional al alcance para no gastar la precision del mapa cerca de la luz.
float planoCercanoLuz(float alcance);

// Caras de un cubemap en el orden de OpenGL (+X, -X, +Y, -Y, +Z, -Z).
constexpr int kCarasCuboLuz = 6;

// Las seis matrices (proyeccion 90 grados x vista) de una luz puntual, con los
// vectores "arriba" de la convencion de cubemaps de OpenGL: la cara que se
// dibuja con la matriz i es la que el hardware muestrea en la direccion de esa
// cara, sin volteos.
void matricesCaraCubo(const glm::vec3& posicion, float alcance,
                      glm::mat4 salida[kCarasCuboLuz]);

#endif
