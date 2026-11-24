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
// Preambulo compartido por las unidades de EditorConfigTests.cpp (generado al dividirlo).
#ifndef EDITORCONFIGTESTSDETALLE_H
#define EDITORCONFIGTESTSDETALLE_H

// Pruebas headless de EditorConfig (la configuracion del editor en JSON):
// tolerancia ante archivo ausente/corrupto/parcial y round-trip escrito-leido.
// Sin pila grafica: solo std C++17 + nlohmann/json del intermedio.
//
// Separacion de archivos (nueva arquitectura):
//   - Configuracion.json (general): apariencia, idioma, sensibilidad, ultimo proyecto.
//   - ConfiguracionProyecto.json (por proyecto): ventanas, gizmo, camara activa.
// Los metodos guardarGeneral/cargarGeneral y guardarProyecto/cargarProyecto
// permiten escribir/leer cada archivo por separado.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>
#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Configuracion/Proyecto/ArchivoProyecto.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Configuracion/ProjectManager.h"
#include "../FunshiEngineGL/src/Configuracion/ProjectPaths.h"
#include "../FunshiEngineGL/src/Exportador/RutasExportacion.h"
#include "../FunshiEngineGL/src/Configuracion/ProyectoInicial.h"
#include "../FunshiEngineGL/src/Configuracion/Apariencia.h"
#include "../FunshiEngineGL/src/Configuracion/RutasLog.h"

namespace fs = std::filesystem;

namespace {

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"  \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

}  // namespace

// Estado de la unidad original: una sola instancia (se define en EditorConfigTests.cpp).

namespace EditorConfigTestsDetalle {

extern int total;
extern int fallos;

}  // namespace EditorConfigTestsDetalle
using namespace EditorConfigTestsDetalle;

// Funciones repartidas entre las unidades.

void seccion01(const std::string& rutaGeneral, const std::string& proyNombreTest, const std::string& rutaProyecto);
void seccion02(const std::string& rutaGeneral);
void seccion03();
void seccion04(const std::string& rutaGeneral);
void seccion05(const std::string& rutaGeneral, const std::string& proyNombreTest, const std::string& rutaProyecto);
void seccion06(const fs::path& base, const std::string& proyNombreTest);
void seccion07(const fs::path& base, const std::string& rutaGeneral, const std::string& proyNombreTest);
void seccion08(const std::string& proyNombreTest, const std::string& rutaProyecto);
void seccion09(const std::string& rutaGeneral);
void seccion10(const std::string& rutaGeneral);
void seccion11();
void seccion12();
void seccion13();
void seccion14(const fs::path& base);
void seccion15();
void seccion16();
void seccion17();
void seccion18();
void seccion19();
void seccion20(const fs::path& base);
void seccion21(const fs::path& base);
void seccion22();
void seccion23();
void seccion24();
void seccion25();

#endif
