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
// Pruebas headless del subsistema FileManager (explorador de archivos).
// Sin pila grafica: solo std C++17 + los headers de ImGui que arrastra
// TreeGUI.h. Se ejecutan contra un proyecto temporal en una carpeta unica por
// proceso (TempPruebas::CarpetaPrueba) y se corren con ctest.
//
// Casos:
//   - Construccion del arbol y re-resolucion de FileSelection::carpetaActual
//     por ruta tras un rescaneo.
//   - Operaciones de dominio: crear carpeta/archivo, renombrar (incluye el
//     rechazo de separadores), copiar carpeta/archivo, eliminar.
//   - Busqueda por ruta en el arbol vigente.
//   - FileSystemWatcher (solo en Linux, donde usa inotify): deteccion de
//     cambios externos y de ramas multi-nivel.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/FileManager/FileManager.h"

#if defined(__linux__)
#include "../FunshiEngineGL/src/FileManager/FileSystemWatcher.h"
#endif

namespace fs = std::filesystem;

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Busca en pre-orden un nodo cuyo nombre coincida (la raiz se ignora, igual
// que en el explorador).
bool buscarPorNombre(ArbolEnlazado<File*>* arbol, Position<File*>* current,
                     const std::string& nombre) {
    if (!arbol || !current) return false;
    if (current != arbol->rootOfTree() && current->getElement() &&
        current->getElement()->getPathName() == nombre)
        return true;
    if (arbol->isInternal(current)) {
        auto* hijos = arbol->childsOf(current);
        auto* h = hijos->first();
        bool encontrado = false;
        while (h && !encontrado) {
            encontrado = buscarPorNombre(arbol, h->getElement(), nombre);
            h = (h != hijos->last()) ? hijos->next(h) : nullptr;
        }
        delete hijos;
        return encontrado;
    }
    return false;
}

std::string contenidoDe(const fs::path& archivo) {
    std::ifstream f(archivo, std::ios::binary);
    std::string contenido((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    return contenido;
}

std::string unir(const fs::path& base, const std::string& resto) {
    return (base / resto).string();
}
} // namespace

int main() {
    // Carpeta temporal unica por proceso (TempPruebas::CarpetaPrueba): sin
    // remove_all inicial que pudiera pisar a otras corridas simultaneas de
    // ctest; se limpia sola al salir del scope, incluso si el test falla.
    TempPruebas::CarpetaPrueba carpetaBase("funshi_filemanager_tests");
    const fs::path base = carpetaBase.ruta();

    // --- Proyecto sintetico -------------------------------------------------
    const fs::path proy = base / "proyecto";
    fs::create_directories(proy / "src" / "nucleo");
    fs::create_directories(proy / "Assets" / "Meshes");
    fs::create_directories(proy / "Assets" / "Scripts");
    { std::ofstream f(proy / "README.txt"); f << "hola proyecto"; }

    FileManager fm(proy.string());

    // --- Arbol inicial ------------------------------------------------------
    ArbolEnlazado<File*>* arbol = fm.getArbol();
    CHECK(arbol != nullptr && !arbol->isEmpty(), "el arbol no esta vacio");
    CHECK(buscarPorNombre(arbol, arbol->rootOfTree(), "src"),
          "el arbol contiene 'src'");
    CHECK(buscarPorNombre(arbol, arbol->rootOfTree(), "Assets"),
          "el arbol contiene 'Assets'");
    CHECK(buscarPorNombre(arbol, arbol->rootOfTree(), "nucleo"),
          "el arbol camina recursivamente hasta 'nucleo'");
    CHECK(fm.buscarCarpetaPorRuta(unir(proy, "Assets/Meshes")) != nullptr,
          "buscarCarpetaPorRuta resuelve 'Assets/Meshes'");
    CHECK(fm.buscarCarpetaPorRuta(unir(proy, "NoExiste")) == nullptr,
          "buscarCarpetaPorRuta devuelve nullptr para una ruta inexistente");

    // --- Seleccion compartida y re-resolucion por ruta tras rescaneo --------
    FileSelection* sel = fm.getSelection();
    sel->rutaVisible = unir(proy, "src");
    fm.refrescar();
    CHECK(sel->carpetaActual != nullptr, "carpetaActual se re-resuelve");
    CHECK(sel->carpetaActual != nullptr &&
              sel->carpetaActual->getPathName() == "src",
          "carpetaActual apunta a 'src'");

    // --- Operaciones de dominio --------------------------------------------
    const std::string rutaNueva = unir(proy, "Assets/Nueva");
    CHECK(fm.crearCarpeta(rutaNueva), "crearCarpeta crea en disco");
    CHECK(fs::is_directory(rutaNueva), "la carpeta nueva existe");

    const std::string rutaArchivo = unir(proy, "Assets/Nueva/ok.txt");
    CHECK(fm.crearArchivo(rutaArchivo, "12345"), "crearArchivo crea en disco");
    CHECK(contenidoDe(rutaArchivo) == "12345", "el archivo nuevo tiene contenido");

    // Renombrar carpeta.
    const std::string rutaRenombrada = unir(proy, "Assets/Renombrada");
    CHECK(fm.renombrar(rutaNueva, "Renombrada"), "renombrar mueve la carpeta");
    CHECK(!fs::exists(rutaNueva) && fs::is_directory(rutaRenombrada),
          "el nombre viejo dejo de existir y el nuevo existe");

    // Renombrar rechaza separadores de ruta.
    CHECK(!fm.renombrar(rutaRenombrada, "con/separador"),
          "renombrar rechaza '/'");
    CHECK(!fm.renombrar(rutaRenombrada, "con\\separador"),
          "renombrar rechaza '\\'");

    // Copiar archivo y carpeta (recursivamente).
    const std::string datosOrig = unir(proy, "src/nucleo/datos.txt");
    CHECK(fm.crearArchivo(datosOrig, "abc"), "archivo fuente para copiar");
    CHECK(fm.copiarArchivo(datosOrig, unir(proy, "src/copiaDatos.txt")),
          "copiarArchivo copia el archivo");
    CHECK(fs::is_regular_file(unir(proy, "src/copiaDatos.txt")),
          "la copia del archivo existe");
    const std::string copiaCarpeta = unir(proy, "Assets/srcCopia");
    CHECK(fm.copiarCarpeta(unir(proy, "src"), copiaCarpeta),
          "copiarCarpeta copia la rama");
    CHECK(fs::is_directory(unir(copiaCarpeta, "nucleo")),
          "copiarCarpeta es recursiva (nucleo existe dentro)");

    // Eliminar carpeta.
    CHECK(fm.eliminarCarpeta(rutaRenombrada), "eliminarCarpeta borra en disco");
    CHECK(!fs::exists(rutaRenombrada), "la carpeta eliminada ya no existe");
    fm.refrescar();
    CHECK(fm.buscarCarpetaPorRuta(rutaRenombrada) == nullptr,
          "tras el rescaneo la carpeta eliminada no esta en el arbol");

    // --- Rescaneo refleja carpetas creadas FUERA del editor -----------------
    CHECK(fs::create_directory(proy / "Assets" / "DiscDirecto"),
          "carpeta sembrada externamente");
    sel->rutaVisible = unir(proy, "Assets");
    fm.refrescar();
    CHECK(sel->carpetaActual != nullptr &&
              sel->carpetaActual->getPathName() == "Assets",
          "carpetaActual se re-resuelve a 'Assets'");
    CHECK(fm.buscarCarpetaPorRuta(unir(proy, "Assets/DiscDirecto")) != nullptr,
          "el rescaneo incorpora carpetas creadas fuera del editor");

    // --- FileSystemWatcher (inotify, solo Linux) ----------------------------
#if defined(__linux__)
    const fs::path proyw = base / "proyecto_watch";
    fs::create_directories(proyw / "ok");
    {
        FileSystemWatcher w(proyw.string());
        CHECK(!w.huboCambiosYConsumir(), "sin eventos en el arranque");
        fs::create_directory(proyw / "nueva");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(w.huboCambiosYConsumir(), "detecta la creacion de una carpeta");
        CHECK(!w.huboCambiosYConsumir(), "la marca se consume");
        fs::create_directories(proyw / "nueva" / "a" / "b");
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        CHECK(w.huboCambiosYConsumir(),
              "detecta una rama multi-nivel creada de golpe");
        { std::ofstream f(proyw / "nueva" / "a" / "b" / "x.txt"); f << "x"; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(w.huboCambiosYConsumir(),
              "detecta cambios profundos dentro de la rama nueva");
    }
#endif

    // --- Resultado ----------------------------------------------------------
    fs::remove_all(base);
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "FILEMANAGER TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}