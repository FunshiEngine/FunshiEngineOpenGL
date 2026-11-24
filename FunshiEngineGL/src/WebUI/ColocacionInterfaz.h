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
#ifndef COLOCACIONINTERFAZ_H
#define COLOCACIONINTERFAZ_H

#include "../Objetos/Componentes/Interfaz/InterfaceComponent.h"

// Donde se dibuja una interfaz dentro de su contenedor (la pantalla del juego),
// segun su ancla y su desplazamiento. Es CPU puro: el overlay lo usa para
// dibujar la vista y para saber si el mouse esta sobre ella.
//
// El ancla elige el punto del contenedor contra el que se apoya la vista
// (esquina, borde o centro) y el desplazamiento es un corrimiento en pixeles
// desde ahi (x hacia la derecha, y hacia abajo). Con el ancla Centro, que es
// la de fabrica, el resultado es el mismo que antes: centrada + desplazamiento.
struct RectInterfaz {
    float x = 0.f;
    float y = 0.f;
    float ancho = 0.f;
    float alto = 0.f;

    bool contiene(float px, float py) const {
        return px >= x && py >= y && px < x + ancho && py < y + alto;
    }
};

namespace ColocacionInterfaz {

// Fraccion horizontal / vertical del ancla: 0 = izquierda/arriba,
// 0.5 = centro, 1 = derecha/abajo.
void fraccionAncla(AnclaInterfaz ancla, float& fx, float& fy);

RectInterfaz calcular(AnclaInterfaz ancla, float desplazamientoX,
                      float desplazamientoY, float anchoVista, float altoVista,
                      float anchoContenedor, float altoContenedor);

}  // namespace ColocacionInterfaz

#endif
