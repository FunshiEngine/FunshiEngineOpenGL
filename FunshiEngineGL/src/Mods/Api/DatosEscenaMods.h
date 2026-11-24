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

// Fuente de datos de la escena que consume la tabla de servicios de los mods.
//
// Es la UNICA unidad de translational del modulo de mods que conoce las
// clases del motor (GameObject, Transform, Collider): el resto del modulo
// solo ve la tabla de punteros a funcion que esta fuente expone. La logica
// de la tabla de servicios vive en ServiciosMods.cpp, que no incluye nada del
// motor y por eso se puede probar headless con una fuente falsa.

#ifndef DATOS_ESCENA_MODS_H
#define DATOS_ESCENA_MODS_H

#include "ServiciosMods.h"

class GameScene;

namespace FunshiMods {

// Conecta la escena real del motor con la tabla de servicios de los mods
// (con nullptr se desconecta).
void inyectarEscenaMods(GameScene* escena);

// Fuente de datos de la escena (la que consume la tabla de servicios).
const FuenteDatosEscena* fuenteDatosEscenaMods();

} // namespace FunshiMods

#endif // DATOS_ESCENA_MODS_H
