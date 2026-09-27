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
//     rechazo de separadores), copiar carpeta/archivo, mover (incluye el
//     rechazo de pisar un destino existente y de meter una carpeta en si
//     misma), eliminar.
//   - Busqueda por ruta en el arbol vigente.
//   - Arrastre-y-suelta (soltarEnCarpeta): mueve con Ctrl copia, y solo el
//     movimiento publica ArchivosReubicados (lo que reescribe las rutas de la
//     escena).
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
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/FileManager/FileManager.h"
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/SoltarEnCarpeta.h"

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

    // --- Mover (drag&drop del explorador) -----------------------------------
    // Es la operacion que usa el arrastre: por defecto mueve, con Ctrl copia.
    // Los casos que importan son los que un rename Ingenuo no cubre.

    // Mover un archivo: desaparece el origen y aparece el destino con su
    // contenido intacto.
    const std::string movible = unir(proy, "Assets/movible.txt");
    CHECK(fm.crearArchivo(movible, "contenido a conservar"), "archivo para mover");
    const std::string movido = unir(proy, "src/movible.txt");
    CHECK(fm.mover(movible, movido), "mover traslada el archivo");
    CHECK(!fs::exists(movible), "tras mover, el origen ya no existe");
    CHECK(fs::is_regular_file(movido), "tras mover, el destino existe");
    CHECK(contenidoDe(movido) == "contenido a conservar",
          "mover conserva el contenido del archivo");

    // Mover una carpeta completa, con su contenido y su estructura.
    const std::string rama = unir(proy, "Assets/rama");
    CHECK(fm.crearCarpeta(rama), "crea la rama");
    CHECK(fm.crearCarpeta(unir(proy, "Assets/rama/interior")),
          "crea la subcarpeta de la rama");
    CHECK(fm.crearArchivo(unir(proy, "Assets/rama/interior/dato.txt"), "x"),
          "archivo dentro de la rama");
    const std::string ramaDestino = unir(proy, "src/rama");
    CHECK(fm.mover(rama, ramaDestino), "mover traslada la carpeta");
    CHECK(!fs::exists(rama), "tras mover la carpeta, el origen ya no existe");
    CHECK(fs::is_regular_file(unir(ramaDestino, "interior/dato.txt")),
          "mover arrastra el contenido de la carpeta");

    // No se pisa un destino existente: es la proteccion contra un arrastre
    // accidental encima de algo que ya estaba ahi.
    const std::string ocupado = unir(proy, "src/ocupado.txt");
    const std::string hueco = unir(proy, "Assets/hueco.txt");
    CHECK(fm.crearArchivo(ocupado, "no tocar"), "destino ocupado");
    CHECK(fm.crearArchivo(hueco, "el que se quiere mover"), "origen a mover");
    CHECK(!fm.mover(hueco, ocupado), "mover se niega a pisar un destino existente");
    CHECK(contenidoDe(ocupado) == "no tocar",
          "el archivo ocupado quedo intacto tras el mover rechazado");
    CHECK(fs::is_regular_file(hueco),
          "el origen sigue en su sitio tras el mover rechazado");

    // Carpeta dentro de si misma: se rechaza antes de tocar disco, porque si
    // se dejara que el error_code lo cortara a mitad, quedaria un arbol a
    // medias.
    CHECK(!fm.mover(unir(proy, "src"), unir(proy, "src")),
          "mover se niega a mover una carpeta sobre si misma");
    CHECK(!fm.mover(unir(proy, "src"), unir(proy, "src/rama/dentro")),
          "mover se niega a meter una carpeta en un descendiente suyo");
    CHECK(fs::is_directory(unir(proy, "src")),
          "la carpeta origen sigue intacta tras los rechazos");

    // Origen inexistente: false sin tocar nada.
    CHECK(!fm.mover(unir(proy, "no/existe.txt"), unir(proy, "src/x.txt")),
          "mover devuelve false si el origen no existe");
    CHECK(!fm.mover("", unir(proy, "src/x.txt")),
          "mover devuelve false con origen vacio");
    CHECK(!fm.mover(unir(proy, "src/rama"), ""),
          "mover devuelve false con destino vacio");

    // Eliminar archivo.
    const std::string rutaBorrable = unir(proy, "Assets/borrable.txt");
    CHECK(fm.crearArchivo(rutaBorrable, "bye"), "archivo fuente para eliminar");
    CHECK(fm.eliminarArchivo(rutaBorrable), "eliminarArchivo borra en disco");
    CHECK(!fs::exists(rutaBorrable), "el archivo eliminado ya no existe");
    CHECK(!fm.eliminarArchivo(rutaBorrable),
          "eliminarArchivo devuelve false si la ruta no existe");
    CHECK(!fm.eliminarArchivo(""),
          "eliminarArchivo devuelve false con ruta vacia");
    CHECK(fm.eliminarArchivo(unir(copiaCarpeta, "nucleo/datos.txt")),
          "eliminarArchivo borra un archivo dentro de una rama");
    CHECK(!fs::exists(unir(copiaCarpeta, "nucleo/datos.txt")),
          "el archivo dentro de la rama ya no existe");

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

    // --- Arrastre: el evento que reescribe las rutas de la escena ------------
    // Es el motivo de que soltar notifique: sin ArchivosReubicados, mover un
    // asset dejaria la escena apuntando a la ruta vieja.
    {
        EditorEventBus bus;
        std::vector<EditorEvent> recibidos;
        bus.subscribe([&recibidos](const EditorEvent& ev) {
            recibidos.push_back(ev);
        });

        const std::string origen = unir(proy, "Assets/arrastrado.txt");
        const std::string destinoCarpeta = unir(proy, "Assets/Movidos");
        CHECK(fm.crearArchivo(origen, "x"), "archivo para arrastrar");
        CHECK(fm.crearCarpeta(destinoCarpeta), "carpeta destino del arrastre");

        // Mover: publica el evento con la ruta anterior y la nueva.
        CHECK(soltarEnCarpeta(&fm, &bus, origen, destinoCarpeta, false),
              "soltarEnCarpeta mueve el elemento");
        CHECK(recibidos.size() == 1, "mover publica un unico evento");
        CHECK(recibidos.size() == 1 &&
                  recibidos[0].type == EditorEventType::ArchivosReubicados,
              "el evento es ArchivosReubicados");
        CHECK(recibidos.size() == 1 && recibidos[0].rutaAnterior == origen,
              "el evento lleva la ruta anterior");
        CHECK(recibidos.size() == 1 &&
                  recibidos[0].rutaNueva == unir(destinoCarpeta, "arrastrado.txt"),
              "el evento lleva la ruta nueva");
        CHECK(fs::is_regular_file(unir(destinoCarpeta, "arrastrado.txt")),
              "el archivo esta en el destino tras el arrastre");

        // Copiar (Ctrl): mueve nada y no publica, porque no cambia ninguna de
        // las dos rutas y no hay nada que reescribir en la escena.
        const std::string aCopiar = unir(proy, "Assets/paraCopiar.txt");
        CHECK(fm.crearArchivo(aCopiar, "y"), "archivo para copiar");
        CHECK(soltarEnCarpeta(&fm, &bus, aCopiar, destinoCarpeta, true),
              "soltarEnCarpeta copia el elemento");
        CHECK(recibidos.size() == 1,
              "copiar no publica evento (sigue habiendo uno solo)");
        CHECK(fs::is_regular_file(aCopiar),
              "tras copiar, el original sigue en su sitio");
        CHECK(fs::is_regular_file(unir(destinoCarpeta, "paraCopiar.txt")),
              "tras copiar, la copia esta en el destino");

        // Soltar sobre la carpeta que ya lo contiene: no hace nada, ni mueve ni
        // publica (mover intentaria renombrar el archivo sobre si mismo).
        CHECK(!soltarEnCarpeta(&fm, &bus, unir(destinoCarpeta, "arrastrado.txt"),
                                destinoCarpeta, false),
              "soltar sobre la carpeta de origen se cancela");
        CHECK(recibidos.size() == 1, "la operacion cancelada no publica evento");

        // Rechazos: destino ocupado y carpeta dentro de si misma. No deben
        // publicar evento, porque no se toco disco.
        CHECK(fm.crearCarpeta(unir(proy, "Assets/Ocupada")), "carpeta ocupada");
        // El conflicto es por NOMBRE: el destino ya tiene un "paraCopiar.txt".
        CHECK(fm.crearArchivo(unir(proy, "Assets/Ocupada/paraCopiar.txt"),
                              "el que ya estaba"),
              "el destino ocupado ya tiene un archivo con ese nombre");
        CHECK(!soltarEnCarpeta(&fm, &bus,
                               unir(destinoCarpeta, "paraCopiar.txt"),
                               unir(proy, "Assets/Ocupada"), false),
              "destino ocupado: la operacion se cancela");
        CHECK(contenidoDe(unir(proy, "Assets/Ocupada/paraCopiar.txt")) ==
                  "el que ya estaba",
              "el archivo ocupado quedo intacto");
        CHECK(recibidos.size() == 1,
              "un destino ocupado no publica evento");
        CHECK(!soltarEnCarpeta(&fm, &bus, unir(proy, "src"),
                               unir(proy, "src/dentro"), false),
              "carpeta dentro de si misma: la operacion se cancela");
        CHECK(recibidos.size() == 1,
              "una carpeta en si misma no publica evento");
    }

    // --- Resultado ----------------------------------------------------------
    fs::remove_all(base);
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "FILEMANAGER TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}