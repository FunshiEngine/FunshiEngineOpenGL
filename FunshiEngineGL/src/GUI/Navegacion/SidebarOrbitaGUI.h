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
#ifndef SIDEBARORBITAGUI_H
#define SIDEBARORBITAGUI_H

#include "../../Input/EstadoOrbita.h"

// Barra lateral con el radio de la orbita de la camara (editor oculto + clic
// derecho). Es una ayuda de navegacion del editor: la dibuja la GUI y el
// estado lo produce el modulo de input.
class SidebarOrbitaGUI {
public:
    // Dibuja la barra. Sin orbita en curso no dibuja nada.
    void dibujar(const EstadoOrbita& estado);

    // Progreso del radio dentro de su rango (0..1), acotado a los extremos:
    // un radio fuera de rango no puede dibujar una barra mas llena ni mas
    // vacia de lo que corresponde.
    static float progreso(float radio, float radioMinimo, float radioMaximo);
};

#endif // SIDEBARORBITAGUI_H
