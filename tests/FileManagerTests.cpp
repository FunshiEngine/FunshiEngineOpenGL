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
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/CrearCarpeta.h"
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/RenombrarElemento.h"
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/SoltarEnCarpeta.h"
#include "../FunshiEngineGL/src/GUI/ObjetosGUI/Skybox/SelectorArchivoCubemap.h"

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

    // --- Crear carpeta: "se creo" en vez de "existe" ------------------------
    // Un destino ya ocupado tiene que informar que esta vez no se creo nada; si
    // no, el explorador cierra el modal creyendo que la carpeta nacio.
    const std::string rutaRepetida = unir(proy, "Assets/Repetida");
    CHECK(fm.crearCarpeta(rutaRepetida), "la primera creacion si crea");
    CHECK(!fm.crearCarpeta(rutaRepetida),
          "crearCarpeta no reporta exito si la carpeta ya existe");
    CHECK(fs::is_directory(rutaRepetida),
          "la carpeta existente sigue en su sitio");

    CHECK(!fm.crearCarpeta(""), "crearCarpeta rechaza la ruta vacia");
    const std::string archivoOcupado = unir(proy, "Assets/ocupado.txt");
    CHECK(fm.crearArchivo(archivoOcupado, "x"), "archivo que ocupa el nombre");
    CHECK(!fm.crearCarpeta(archivoOcupado),
          "crearCarpeta falla si el destino es un archivo");

    // El helper compartido por arbol y grid valida el nombre y decide el cierre.
    CHECK(CrearCarpeta::nombreValido("Carpeta con espacios"),
          "un nombre normal es valido");
    CHECK(!CrearCarpeta::nombreValido(""), "el nombre vacio no es valido");
    CHECK(!CrearCarpeta::nombreValido("."), "'.' no es un nombre de carpeta");
    CHECK(!CrearCarpeta::nombreValido(".."), "'..' no es un nombre de carpeta");
    CHECK(!CrearCarpeta::nombreValido("con/separador"),
          "un nombre con '/' no es un nombre de carpeta");
    CHECK(!CrearCarpeta::nombreValido("con\\separador"),
          "un nombre con '\\' no es un nombre de carpeta");

    CHECK(!CrearCarpeta::crear(nullptr, unir(proy, "Assets"), "SinProyecto").creada,
          "sin FileManager no se crea nada");
    CHECK(!CrearCarpeta::crear(&fm, "", "SinPadre").creada,
          "sin carpeta padre no se crea nada");

    const std::string rutaPadre = unir(proy, "Assets");
    const unsigned long contadorAntes = sel->contadorCambios;
    const CrearCarpeta::Resultado creadaOk =
        CrearCarpeta::crear(&fm, rutaPadre, "Creada");
    CHECK(creadaOk.creada && creadaOk.error.empty(),
          "el helper crea con un nombre libre");
    CHECK(fs::is_directory(unir(rutaPadre, "Creada")),
          "la carpeta creada queda donde se pidio");
    CHECK(sel->contadorCambios == contadorAntes + 1,
          "crear una carpeta sube el contador exactamente una vez");

    const unsigned long contadorTrasCrear = sel->contadorCambios;
    const CrearCarpeta::Resultado repetida =
        CrearCarpeta::crear(&fm, rutaPadre, "Creada");
    CHECK(!repetida.creada, "el helper no reporta exito si el nombre se repite");
    CHECK(repetida.error.find("Ya existe") != std::string::npos,
          "el helper explica que el nombre ya esta en uso");
    CHECK(sel->contadorCambios == contadorTrasCrear,
          "un intento fallido no sube el contador");

    CHECK(fm.crearCarpeta(unir(proy, "Assets/pruebas")),
          "carpeta padre para el nombre con separador");
    const CrearCarpeta::Resultado conSeparador =
        CrearCarpeta::crear(&fm, rutaPadre, "pruebas/nueva");
    CHECK(!conSeparador.creada,
          "un nombre con separador no crea una carpeta anidada");
    CHECK(!fs::exists(unir(proy, "Assets/pruebas/nueva")),
          "no aparece la carpeta que el nombre pedia en otra ruta");


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

    // Copiar una carpeta dentro de si misma (Ctrl+arrastrar sobre una
    // subcarpeta del propio arbol): el recorrido se copia a si mismo y se
    // reproduce hasta que la ruta deja de caber, dejando un arbol basura a
    // medias. Se usa un arbol propio para que el resto de la prueba no dependa
    // de lo que quede colgando del caso anidado.
    CHECK(fm.crearCarpeta(unir(proy, "Assets/anidado")),
          "crea la carpeta padre del caso anidado");
    CHECK(fm.crearCarpeta(unir(proy, "Assets/anidado/origen")),
          "crea el origen del caso anidado");
    CHECK(fm.crearArchivo(unir(proy, "Assets/anidado/origen/dato.txt"), "x"),
          "archivo dentro del origen del caso anidado");
    const std::string raizAnidada = unir(proy, "Assets/anidado/origen");
    // Un destino que solo empieza por el nombre del origen NO esta dentro de
    // el: el cotejo tiene que cerrar en un separador.
    CHECK(fm.copiarCarpeta(raizAnidada, unir(proy, "Assets/anidado/origenCopia")),
          "copiarCarpeta acepta un destino que solo empieza igual que el origen");
    CHECK(fs::is_regular_file(unir(proy, "Assets/anidado/origenCopia/dato.txt")),
          "la copia legitima al lado del origen esta entera");
    const std::string anidado = unir(raizAnidada, "dentro");
    CHECK(!fm.copiarCarpeta(raizAnidada, anidado),
          "copiarCarpeta rechaza un destino dentro del propio origen");
    CHECK(!fs::exists(anidado), "el destino anidado ni siquiera se crea");
    CHECK(!fs::exists(unir(anidado, "dato.txt")),
          "no se copia nada al destino anidado");
    CHECK(!fm.copiarCarpeta(raizAnidada, raizAnidada),
          "copiarCarpeta rechaza el origen sobre si mismo");
    CHECK(contenidoDe(unir(raizAnidada, "dato.txt")) == "x",
          "el origen queda intacto tras rechazar la copia anidada");

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

    // --- Plantilla de script C++ (H-15: la fabrica debe viajar exportada) -----
    // La plantilla declara la fabrica con el macro portable de exportacion,
    // para que en Windows/MSVC la .dll la exporte y GetProcAddress la
    // encuentre. Sin el macro la .dll compila pero el simbolo no existe y el
    // script nunca carga (en MinGW/ELF no se nota: ahi se exporta todo solo).
    const std::string plantilla = FileManager::plantillaScript("MiScript", false);
    CHECK(plantilla.find("FUNSHI_COMPORTAMIENTO_EXPORT") != std::string::npos,
          "la plantilla del script declara el macro de exportacion");
    CHECK(plantilla.find("extern \"C\" " + std::string("FUNSHI_COMPORTAMIENTO_EXPORT")) !=
              std::string::npos,
          "el macro de exportacion aparece en la declaracion de la fabrica");
    CHECK(plantilla.find("FUNSHI_CREAR_COMPORTAMIENTO") != std::string::npos,
          "la plantilla conserva la fabrica con su nombre para GetProcAddress");
    // El camino Java no lleva exportacion (la carga el JVM, no LoadLibrary).
    const std::string plantillaJava = FileManager::plantillaScript("MiJava", true);
    CHECK(plantillaJava.find("FUNSHI_COMPORTAMIENTO_EXPORT") == std::string::npos,
          "la plantilla Java no lleva el macro de exportacion nativa");

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

#ifndef _WIN32
        // En Linux y macOS la barra invertida es un caracter LEGAL del nombre de
        // archivo: "Assets/con\\barra.txt" es un archivo, no una ruta. Si al
        // partir la ruta se trata como separador, el arrastre lo renombra a
        // "barra.txt" y ademas invalida el cache de una carpeta que no existe.
        CHECK(fm.crearCarpeta(unir(proy, "Assets/ConBarra")), "carpeta destino con barra");
        const std::string conBarra = unir(proy, "Assets/con\\barra.txt");
        CHECK(fm.crearArchivo(conBarra, "con barra"), "archivo con barra invertida en el nombre");
        std::string padreReportado;
        CHECK(soltarEnCarpeta(&fm, &bus, conBarra, unir(proy, "Assets/ConBarra"),
                              false, &padreReportado),
              "soltar un archivo con barra invertida lo mueve");
        CHECK(fs::is_regular_file(unir(proy, "Assets/ConBarra/con\\barra.txt")),
              "el archivo conserva su nombre completo (no se renombra)");
        CHECK(!fs::exists(unir(proy, "Assets/ConBarra/barra.txt")),
              "no aparece un archivo truncado en el destino");
        CHECK(padreReportado == unir(proy, "Assets"),
              "la carpeta padre reportada es la real, para invalidar su cache");
        CHECK(recibidos.size() == 2 &&
                  recibidos[1].rutaNueva == unir(proy, "Assets/ConBarra/con\\barra.txt"),
              "el evento lleva la ruta nueva con la barra intacta");
#endif
    }

    // --- Drag&Drop: invalidacion de vistas (arbol + grid) ---------------------
    // Soltar un ARCHIVO sobre una carpeta del arbol debe:
    //   - mover el archivo en disco
    //   - incrementar contadorCambios (para que el arbol se rescanee)
    //   - publicar ArchivosReubicados (para reescribir referencias de la escena)
    {
        EditorEventBus bus;
        std::vector<EditorEvent> recibidos;
        bus.subscribe([&recibidos](const EditorEvent& ev) {
            recibidos.push_back(ev);
        });

        const std::string origenArchivo = unir(proy, "Assets/archivo_suelto.txt");
        const std::string destinoCarpeta = unir(proy, "Assets/Meshes");
        CHECK(fm.crearArchivo(origenArchivo, "contenido"), "archivo para arrastrar al arbol");

        // Estado antes: contadorCambios actual
        FileSelection* sel = fm.getSelection();
        unsigned long contadorAntes = sel->contadorCambios;

        CHECK(soltarEnCarpeta(&fm, &bus, origenArchivo, destinoCarpeta, false),
              "soltar archivo sobre carpeta del arbol mueve el archivo");

        // Verificar disco
        CHECK(!fs::exists(origenArchivo), "archivo ya no existe en origen");
        CHECK(fs::is_regular_file(unir(destinoCarpeta, "archivo_suelto.txt")),
              "archivo existe en destino");

        // Verificar evento
        CHECK(recibidos.size() == 1, "mover archivo publica un evento");
        CHECK(recibidos[0].type == EditorEventType::ArchivosReubicados,
              "evento es ArchivosReubicados");

        // Verificar que contadorCambios subio (invalida arbol)
        CHECK(sel->contadorCambios == contadorAntes + 1,
              "contadorCambios incrementa al mover archivo (invalida arbol)");

        // Verificar que el arbol rescaneado refleja el cambio (carpeta Meshes sigue ahi)
        fm.refrescar();
        CHECK(fm.buscarCarpetaPorRuta(destinoCarpeta) != nullptr,
              "arbol rescaneado conserva la carpeta destino");
    }

    // Soltar una CARPETA sobre otra carpeta del arbol debe:
    //   - mover la carpeta con TODO su subarbol (hijos, nietos)
    //   - incrementar contadorCambios
    //   - publicar ArchivosReubicados con la ruta de la carpeta (prefijo)
    //   - tras refrescar, el arbol tiene la carpeta en la nueva ubicacion
    {
        EditorEventBus bus;
        std::vector<EditorEvent> recibidos;
        bus.subscribe([&recibidos](const EditorEvent& ev) {
            recibidos.push_back(ev);
        });

        const std::string ramaOrigen = unir(proy, "Assets/RamaAMover");
        CHECK(fm.crearCarpeta(ramaOrigen), "crea carpeta origen");
        CHECK(fm.crearCarpeta(unir(ramaOrigen, "hijo")), "crea subcarpeta");
        CHECK(fm.crearCarpeta(unir(ramaOrigen, "hijo/nieto")), "crea sub-subcarpeta");
        CHECK(fm.crearArchivo(unir(ramaOrigen, "hijo/nieto/dato.txt"), "x"),
              "archivo en nivel profundo");

        const std::string destinoCarpeta = unir(proy, "Assets/Meshes");
        FileSelection* sel = fm.getSelection();
        unsigned long contadorAntes = sel->contadorCambios;

        CHECK(soltarEnCarpeta(&fm, &bus, ramaOrigen, destinoCarpeta, false),
              "soltar carpeta sobre carpeta del arbol mueve la rama");

        // Verificar disco: origen desaparecio, destino tiene la rama completa
        CHECK(!fs::exists(ramaOrigen), "carpeta origen ya no existe");
        const std::string ramaNueva = unir(destinoCarpeta, "RamaAMover");
        CHECK(fs::is_directory(ramaNueva), "carpeta movida existe en destino");
        CHECK(fs::is_regular_file(unir(ramaNueva, "hijo/nieto/dato.txt")),
              "subarbol completo se movio (archivo profundo existe)");

        // Verificar evento: rutaAnterior = ramaOrigen, rutaNueva = ramaNueva
        CHECK(recibidos.size() == 1, "mover carpeta publica un evento");
        CHECK(recibidos[0].rutaAnterior == ramaOrigen, "evento lleva ruta anterior");
        CHECK(recibidos[0].rutaNueva == ramaNueva, "evento lleva ruta nueva");

        // Verificar contadorCambios
        CHECK(sel->contadorCambios == contadorAntes + 1,
              "contadorCambios incrementa al mover carpeta");

        // Verificar arbol rescaneado
        fm.refrescar();
        CHECK(fm.buscarCarpetaPorRuta(ramaNueva) != nullptr,
              "arbol rescaneado tiene la carpeta en nueva ubicacion");
        CHECK(fm.buscarCarpetaPorRuta(unir(ramaNueva, "hijo")) != nullptr,
              "arbol rescaneado tiene subcarpeta");
        CHECK(fm.buscarCarpetaPorRuta(unir(ramaNueva, "hijo/nieto")) != nullptr,
              "arbol rescaneado tiene sub-subcarpeta (subarbol completo)");
        CHECK(fm.buscarCarpetaPorRuta(ramaOrigen) == nullptr,
              "arbol rescaneado ya no tiene la ruta vieja");
    }

    // --- Renombre por click derecho: helper compartido arbol/grid ------------
    // Renombrar no cambia de carpeta: la ruta nueva es hermana de la vieja. El
    // disco lo hace FileManager (que rechaza separadores en el nombre) y el
    // exito se avisa una sola vez, porque de ese evento depende que la escena
    // reescriba las referencias que apuntaban a la ruta vieja.
    {
        CHECK(RenombrarElemento::rutaConNombreNuevo("a/b/c.txt", "d.txt") ==
                  "a/b/d.txt",
              "la ruta nueva es hermana de la vieja");
        CHECK(RenombrarElemento::rutaConNombreNuevo("suelto.txt", "otro.txt") ==
                  "otro.txt",
              "sin separadores, la ruta nueva es el nombre nuevo");
#ifdef _WIN32
        // En Windows el motor mezcla separadores: la parte de carpeta se
        // conserva byte a byte, no se reescribe con el separador nativo.
        CHECK(RenombrarElemento::rutaConNombreNuevo("C:\\a\\b", "c") ==
                  "C:\\a\\c",
              "el separador de la ruta original se conserva");
#endif
        EditorEventBus bus;
        std::vector<EditorEvent> recibidos;
        bus.subscribe([&recibidos](const EditorEvent& ev) {
            recibidos.push_back(ev);
        });

        const std::string carpetaVieja = unir(proy, "Assets/Renombrable");
        CHECK(fm.crearCarpeta(carpetaVieja), "carpeta para renombrar");
        CHECK(fm.crearArchivo(unir(carpetaVieja, "dato.txt"), "x"),
              "contenido de la carpeta a renombrar");

        CHECK(RenombrarElemento::ejecutar(&fm, &bus, carpetaVieja, "Renombrada"),
              "renombrar la carpeta se completa");
        const std::string carpetaNueva =
            RenombrarElemento::rutaConNombreNuevo(carpetaVieja, "Renombrada");
        CHECK(recibidos.size() == 1, "renombrar publica un unico evento");
        CHECK(recibidos.size() == 1 &&
                  recibidos[0].type == EditorEventType::ArchivosReubicados,
              "el evento es ArchivosReubicados");
        CHECK(recibidos.size() == 1 && recibidos[0].rutaAnterior == carpetaVieja,
              "el evento lleva la ruta anterior");
        CHECK(recibidos.size() == 1 && recibidos[0].rutaNueva == carpetaNueva,
              "el evento lleva la ruta nueva");
        CHECK(fs::is_directory(carpetaNueva),
              "renombrar renombra la carpeta en disco");
        CHECK(!fs::exists(carpetaVieja), "la ruta vieja ya no existe");
        CHECK(fs::is_regular_file(unir(carpetaNueva, "dato.txt")),
              "renombrar arrastra el contenido de la carpeta");

        // El arbol vigente refleja el nombre nuevo tras el rescaneo.
        fm.refrescar();
        CHECK(fm.buscarCarpetaPorRuta(carpetaNueva) != nullptr,
              "tras el rescaneo el arbol tiene la carpeta con el nombre nuevo");
        CHECK(fm.buscarCarpetaPorRuta(carpetaVieja) == nullptr,
              "tras el rescaneo la ruta vieja no esta en el arbol");

        // Rechazos: nada de esto toca disco ni publica.
        CHECK(!RenombrarElemento::ejecutar(&fm, &bus, carpetaNueva, ""),
              "un nombre vacio no renombra");
        CHECK(!RenombrarElemento::ejecutar(&fm, &bus, carpetaNueva, "a/b"),
              "un nombre con separadores no renombra (no crea una ruta nueva)");
        CHECK(!RenombrarElemento::ejecutar(&fm, &bus, carpetaNueva, "Renombrada"),
              "el mismo nombre no es un cambio");
        CHECK(recibidos.size() == 1,
              "los renombres rechazados no publican evento");
        CHECK(fs::is_directory(carpetaNueva),
              "los rechazos dejan la carpeta intacta");

        // Renombrar no pisa un destino existente (mismo criterio que mover):
        // rename reemplazaria en silencio el archivo destino y se perderia su
        // contenido con un renombre accidental. Se permite el renombre al MISMO
        // elemento (cambiar mayusculas/minusculas) mediante equivalent.
        const std::string destinoProtegido = unir(proy, "Assets/protegido.txt");
        const std::string archivoARenombrar = unir(proy, "Assets/cambiable.txt");
        CHECK(fm.crearArchivo(destinoProtegido, "conservar"),
              "archivo destino que no debe tocarse");
        CHECK(fm.crearArchivo(archivoARenombrar, "cambiar"),
              "archivo fuente para el renombre rechazado");
        CHECK(!RenombrarElemento::ejecutar(&fm, &bus, archivoARenombrar,
                                           "protegido.txt"),
              "renombrar se niega a pisar un archivo existente");
        CHECK(contenidoDe(destinoProtegido) == "conservar",
              "el archivo existente quedo intacto tras el renombre rechazado");
        CHECK(fs::is_regular_file(archivoARenombrar),
              "el origen sigue en su sitio tras el renombre rechazado");
        CHECK(recibidos.size() == 1,
              "el renombre a un destino ocupado no publica evento");
    }

    // --- Selector de caras del cubemap: filtro y validacion ------------------
    // La logica del modal vive en funciones puras aparte del dibujo, para poder
    // ejercitarla sin ventana. Cubre las dos condiciones que hacen que el motor
    // descarte el cubemap sin avisar en pantalla: una cara sin asignar y dos
    // caras que no midan lo mismo.
    {
        CHECK(SelectorArchivoCubemap::esImagenCubemap("cielo_px.png"),
              "una .png sirve como cara del cubemap");
        CHECK(SelectorArchivoCubemap::esImagenCubemap("cielo_px.PNG"),
              "el filtro no distingue mayusculas");
        CHECK(SelectorArchivoCubemap::esImagenCubemap("a/b/cielo.tga"),
              "una .tga anidada en carpetas sirve");
        CHECK(SelectorArchivoCubemap::esImagenCubemap("cielo.hdr"),
              "una .hdr sirve");
        CHECK(!SelectorArchivoCubemap::esImagenCubemap("cielo.obj"),
              "una malla no es una cara del cubemap");
        CHECK(!SelectorArchivoCubemap::esImagenCubemap("notas.txt"),
              "un texto no es una cara del cubemap");
        CHECK(!SelectorArchivoCubemap::esImagenCubemap("cielo"),
              "un archivo sin extension no es una cara");

        const std::string vacias[6] = {"", "", "", "", "", ""};
        CHECK(SelectorArchivoCubemap::carasSinAsignar(vacias) == 6,
              "sin nada asignado faltan las seis caras");

        const SelectorArchivoCubemap::Dimensiones iguales[6] = {
            {512, 512}, {512, 512}, {512, 512},
            {512, 512}, {512, 512}, {512, 512},
        };
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(iguales) == -1,
              "seis caras de las mismas dimensiones no se descartan");

        // El caso que la comparacion por peso de archivo nocia: dos archivos con
        // distinto peso pero los mismos pixeles. Con el criterio de pixeles no
        // hay nada que avisar.
        SelectorArchivoCubemap::Dimensiones mismoTamano[6];
        for (int i = 0; i < 6; ++i) mismoTamano[i] = {1024, 1024};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(mismoTamano) == -1,
              "caras con los mismos pixeles no se descartan aunque pesen distinto");

        // Una cara exportada a otra resolucion: el motor descarta el cubemap
        // entero, asi que el selector avisa cual es.
        SelectorArchivoCubemap::Dimensiones dispares[6];
        for (int i = 0; i < 6; ++i) dispares[i] = {512, 512};
        dispares[3] = {256, 256};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(dispares) == 3,
              "avisa la cara cuya resolucion no coincide");

        // Ancho y alto se comparan por separado: 512x256 no es lo mismo que
        // 256x512 aunque los dos produzcan el mismo numero de pixeles.
        SelectorArchivoCubemap::Dimensiones transpuesta[6];
        for (int i = 0; i < 6; ++i) transpuesta[i] = {512, 256};
        transpuesta[1] = {256, 512};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(transpuesta) == 1,
              "avisa una cara transpuesta aunque tenga igual cantidad de pixeles");

        // Una cara que no se pudo medir (no existe, o formato que no se pudo
        // leer) queda en 0x0 y no falsea la comparacion.
        SelectorArchivoCubemap::Dimensiones conIlegible[6];
        for (int i = 0; i < 6; ++i) conIlegible[i] = {512, 512};
        conIlegible[5] = {};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(conIlegible) == -1,
              "una cara ilegible no se reporta como dimension distinta");

        // Si la +X no se pudo medir no hay contra que comparar: el motor igual
        // va a avisar por su cuenta al no poder decodificar.
        SelectorArchivoCubemap::Dimensiones sinReferencia[6];
        for (int i = 0; i < 6; ++i) sinReferencia[i] = {512, 512};
        sinReferencia[0] = {};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(sinReferencia) == -1,
              "sin dimensiones de la +X no se reporta desajuste");
    }

    // --- Resultado ----------------------------------------------------------
    fs::remove_all(base);
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "FILEMANAGER TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}