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
#ifndef LECTORPERFIL_H
#define LECTORPERFIL_H

#include <string>

#include "PerfilParticula.h"

// Carga un perfil de efecto desde un JSON.
//
// Formato (todos los campos son opcionales salvo que se indique lo contrario):
//
//   {
//     "version": 1,
//     "nombre": "Humo",
//     "textura": "particulas/humo.png",
//     "vida": [0.6, 1.2],
//     "velocidad": [1.0, 2.0],
//     "aceleracion": [0.0, -1.0, 0.0],
//     "rozamiento": 0.2,
//     "tamanos": [0.4, 1.0, 1.2],
//     "colores": [[1, 1, 1, 0.6], [0.5, 0.5, 0.5, 0.0]]
//   }
//
// "vida" y "velocidad" son pares [minimo, maximo]; "aceleracion" es una
// terna; "tamanos" es la curva de multiplicadores de tamano y "colores" la de
// colores RGBA, ambos por puntos de la curva (el primero al nacer y el
// ultimo al morir).
//
// Devuelve false y describe el motivo en 'error' si el archivo no abre, si el
// JSON esta mal formado o si un campo tiene un tipo o un rango invalido.
bool cargarPerfilDesdeJson(const std::string& rutaAbsoluta, PerfilParticula& out,
                            std::string& error);

#endif
