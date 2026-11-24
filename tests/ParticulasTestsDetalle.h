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
// Preambulo compartido por las unidades de ParticulasTests.cpp (generado al dividirlo).
#ifndef PARTICULASTESTSDETALLE_H
#define PARTICULASTESTSDETALLE_H

// Pruebas headless del sistema de particulas (src/Particulas).
//
// La simulacion es CPU pura: no toca OpenGL ni glm, asi que estas pruebas
// compilan y corren tambien en el build sin motor (BUILD_ENGINE=OFF).
//
// Cubren: la emision (vida, tamano, color y cono de dispersion), el
// presupuesto de particulas vivas, la integracion (rozamiento, gravedad y
// posicion), la muerte por vida agotada y la reproducibilidad de la
// simulacion con la misma semilla.

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include "../FunshiEngineGL/src/Particulas/Perfil/LectorPerfil.h"
#include "../FunshiEngineGL/src/Particulas/ParticulasBuilder.h"
#include "../FunshiEngineGL/src/Particulas/ParametrosEmisor.h"
#include "../FunshiEngineGL/src/Particulas/Particula.h"
#include "../FunshiEngineGL/src/Particulas/Perfil/PerfilParticula.h"
#include "../FunshiEngineGL/src/Particulas/SistemaParticulas.h"
#include "TempPruebas.h"
#include <fstream>
#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Estado de la unidad original: una sola instancia (se define en ParticulasTests.cpp).

namespace ParticulasTestsDetalle {

extern int total;
extern int fallos;

}  // namespace ParticulasTestsDetalle
using namespace ParticulasTestsDetalle;

// Funciones repartidas entre las unidades.

bool cerca(float a, float b, float tolerancia = 1e-4f);
ParametrosEmisor parametrosFijos();
void probarEmision();
void probarCono();
void probarPresupuesto();
void probarIntegracion();
void probarMuerte();
void probarReproducibilidad();
void probarCurvas();
void escribirArchivo(const std::string& ruta, const std::string& contenido);
void probarLectorPerfil();
void probarGeometria();

#endif
