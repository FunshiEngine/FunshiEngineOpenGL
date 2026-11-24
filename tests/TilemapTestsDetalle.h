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
// Preambulo compartido por las unidades de TilemapTests.cpp (generado al dividirlo).
#ifndef TILEMAPTESTSDETALLE_H
#define TILEMAPTESTSDETALLE_H

// Pruebas headless del tileset (JSON estilo Tiled) y del componente Tilemap:
//   - parseo del tileset, geometria de la grilla y flag de solidez por tile,
//   - capas, pintado por celda y revision,
//   - horneado de la malla (una malla por tilemap, cuatriangulos por tile),
//   - round-trip binario del componente.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI/OpenGL, y salida final "OK/FALLOS: N comprobaciones".

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <btBulletDynamicsCommon.h>
#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Assets/Mallas/Mesh.h"
#include "../FunshiEngineGL/src/Assets/Tiles/Tileset.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Colliders/TilemapCollider.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Colliders/TiposShapeBullet.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Juego2D/Tilemap.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Juego2D/PincelTilemap.h"
#include "../FunshiEngineGL/src/Comandos/GestorComandos.h"
#include "../FunshiEngineGL/src/Comandos/Tilemap/PintarTilesComando.h"
#include "../FunshiEngineGL/src/Scenes/Registro/SceneRegistry.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Escena/Transform.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

namespace {

std::string ruta(const TempPruebas::CarpetaPrueba& carpeta,
                 const std::string& nombre) {
    return (carpeta.ruta() / nombre).string();
}

void escribir(const std::string& ruta, const std::string& contenido) {
    std::ofstream file(ruta, std::ios::binary);
    file << contenido;
}

}  // namespace

// Estado de la unidad original: una sola instancia (se define en TilemapTests.cpp).

namespace TilemapTestsDetalle {

extern int total;
extern int fallos;

}  // namespace TilemapTestsDetalle
using namespace TilemapTestsDetalle;

// Funciones repartidas entre las unidades.

void pincelTilemap(const std::string& rutaTileset);

#endif
