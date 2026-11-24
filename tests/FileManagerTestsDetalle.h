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
// Preambulo compartido por las unidades de FileManagerTests.cpp (generado al dividirlo).
#ifndef FILEMANAGERTESTSDETALLE_H
#define FILEMANAGERTESTSDETALLE_H

// Pruebas headless del subsistema FileManager (explorador de archivos).
// Sin pila grafica: solo std C++17 + los headers de ImGui que arrastra
// TreeGUI.h. Se ejecutan contra un proyecto temporal en una carpeta unica por
// proceso (TempPruebas::CarpetaPrueba) y se corren con ctest.
//
// Casos:
//   - Construccion del arbol y re-resolucion de FileSelection::carpetaActual
//     por ruta tras un rescaneo.
//   - Operaciones de dominio: crear carpeta/archivo, renombrar (incluye el
//     rechazo de separadores), copiar carpeta/archivo, mover (incluye el
//     rechazo de pisar un destino existente y de meter una carpeta en si
//     misma), eliminar.
//   - Plantilla de script C++: la fabrica que el motor busca con GetProcAddress
//     tiene que viajar exportada en MSVC (H-15): la plantilla debe declarar el
//     macro portable de exportacion.
//   - Busqueda por ruta en el arbol vigente.
//   - Arrastre-y-suelta (soltarEnCarpeta): mueve con Ctrl copia, y solo el
//     movimiento publica ArchivosReubicados (lo que reescribe las rutas de la
//     escena).
//   - Renombre por click derecho (RenombrarElemento, compartido por el arbol y
//     el grid): la ruta nueva es hermana de la vieja, el disco lo hace
//     FileManager y solo un cambio real publica ArchivosReubicados.
//   - Busqueda por ruta en el arbol vigente.
//   - FileSystemWatcher: deteccion de cambios externos (inotify en Linux,
//     polling temporal en el resto, con el intervalo inyectado).

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "TempPruebas.h"
#include "../FunshiEngineGL/src/FileManager/FileManager.h"
#include "../FunshiEngineGL/src/FileManager/FileSystemWatcher.h"
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/CrearCarpeta.h"
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/RenombrarElemento.h"
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/SoltarEnCarpeta.h"
#include "../FunshiEngineGL/src/GUI/ObjetosGUI/Skybox/SelectorArchivoCubemap.h"

namespace fs = std::filesystem;

namespace {

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

std::string contenidoDe(const fs::path& archivo) {
    std::ifstream f(archivo, std::ios::binary);
    std::string contenido((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    return contenido;
}

std::string unir(const fs::path& base, const std::string& resto) {
    return (base / resto).string();
}

}  // namespace

// Estado de la unidad original: una sola instancia (se define en FileManagerTests.cpp).

namespace FileManagerTestsDetalle {

extern int total;
extern int fallos;

}  // namespace FileManagerTestsDetalle
using namespace FileManagerTestsDetalle;

// Funciones repartidas entre las unidades.

void seccion01(const fs::path& proy);
void seccion02(const fs::path& proyw);
void seccion03(const fs::path& proyw);
void seccion04(const fs::path& proy, FileManager& fm, const std::string& rutaNueva);
void seccion05(const fs::path& proy, FileManager& fm, FileSelection*& sel);
void seccion06(const fs::path& proy, FileManager& fm, FileSelection*& sel, const std::string& rutaNueva);
void seccion07(const fs::path& proy, FileManager& fm, const std::string& rutaNueva);
void seccion08();

#endif
