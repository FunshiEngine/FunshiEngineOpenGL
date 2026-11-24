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

// Tabla de servicios que el motor entrega a los mods.
//
// La tabla es POD (punteros a funcion) y la implementa este modulo, que NO
// conoce las clases del motor: los datos de la escena llegan por una fuente
// inyectada (tabla de punteros a funcion) y el contexto de ImGui por un
// puntero opaco que fija la capa de GUI. Asi el modulo se puede probar
// headless con una fuente de datos falsa.

#ifndef SERVICIOS_MODS_H
#define SERVICIOS_MODS_H

#include "ApiMod.h"

namespace FunshiMods {

// Fuente de datos de la escena que el motor inyecta en el modulo de mods.
// Los callbacks reciben el contexto opaco de la fuente.
struct FuenteDatosEscena {
    int (*contarObjetos)(void* contexto) = nullptr;
    bool (*obtenerObjeto)(void* contexto, int indice,
                          ApiModGameObject* salida) = nullptr;
    int (*contarComponentes)(void* contexto, int indice) = nullptr;
    bool (*obtenerComponente)(void* contexto, int indiceObjeto,
                               int indiceComponente,
                               ApiModComponente* salida) = nullptr;
    void* contexto = nullptr;
};

// Conecta (o desconecta, con nullptr) la fuente de datos de la escena.
void inyectarDatosEscena(const FuenteDatosEscena* fuente);

// Nombre del proyecto abierto que se le muestra a los mods.
void fijarNombreProyecto(const char* nombre);
const char* nombreProyectoActual();

// Contexto de ImGui del motor (opaco para el mod). Lo fija la capa de GUI
// cuando el contexto existe; mientras tanto es nullptr.
void fijarContextoImGui(void* contexto);
void* contextoImGuiActual();

// Tabla de servicios que el motor entrega a los mods (la misma para todos:
// el estado que necesita un mod vive en el propio mod).
const ApiModServicios* tablaServicios();

} // namespace FunshiMods

#endif // SERVICIOS_MODS_H
