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
#ifndef ESTADOORBITA_H
#define ESTADOORBITA_H

// Estado de la orbita de la camara del editor.
//
// Lo produce el modulo de input (que es quien lleva la orbita) y lo dibuja la
// GUI: la barra lateral recibe los datos y no necesita conocer el input.
struct EstadoOrbita {
    bool activa = false;       // hay orbita en curso
    float radio = 0.0f;        // radio actual de la orbita
    float radioMinimo = 0.0f;  // radio minimo admitido
    float radioMaximo = 0.0f;  // radio maximo admitido
};

#endif // ESTADOORBITA_H
