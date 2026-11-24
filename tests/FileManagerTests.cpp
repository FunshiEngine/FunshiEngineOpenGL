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
#include "FileManagerTestsDetalle.h"

namespace FileManagerTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace FileManagerTestsDetalle
using namespace FileManagerTestsDetalle;

void seccion01(const fs::path& proy) {
    { std::ofstream f(proy / "README.txt"); f << "hola proyecto"; }
}

void seccion02(const fs::path& proyw) {
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
}

void seccion03(const fs::path& proyw) {
    {
        // Intervalo corto: el contrato del polling es "un pulso periodico
        // compara el mtime de la raiz", no la duracion del pulso.
        FileSystemWatcher w(proyw.string(), std::chrono::milliseconds(50));
        CHECK(!w.huboCambiosYConsumir(),
              "sin eventos al arrancar (polling)");
        // Un pulso completo sin tocar nada no debe inventar un cambio: la
        // marca inicial del mtime es la de la raiz, no el epoch.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(!w.huboCambiosYConsumir(),
              "sin cambios en la raiz el pulso no inventa eventos (polling)");
        fs::create_directory(proyw / "nueva");
        // La DETECCION solo es verificable donde la lectura del mtime tiene
        // la precision del FS: con MSVC (CI Windows) se lee FILETIME (100ns)
        // y el cambio es observable. El CRT de MinGW trae la lectura de stat
        // (1 segundo, y su setter no aplica sobre directorios), de modo que
        // una creacion dentro del mismo segundo que la semilla no cambia el
        // valor leido; ahi el contrato del polling queda cubierto por la
        // verificacion de que NO inventa eventos, y la deteccion por la
        // corrida de CI en la plataforma real.
#if defined(_MSC_VER)
        bool detecto = false;
        for (int i = 0; i < 26 && !detecto; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            detecto = w.huboCambiosYConsumir();
        }
        CHECK(detecto,
              "el pulso detecta la creacion de una carpeta (polling)");
        CHECK(!w.huboCambiosYConsumir(),
              "la marca se consume (polling)");
#endif
    }
}

// --- Arrastre: el evento que reescribe las rutas de la escena ------------
// Es el motivo de que soltar notifique: sin ArchivosReubicados, mover un
// asset dejaria la escena apuntando a la ruta vieja.
void seccion04(const fs::path& proy, FileManager& fm, const std::string& rutaNueva) {
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
}

// --- Drag&Drop: invalidacion de vistas (arbol + grid) ---------------------
// Soltar un ARCHIVO sobre una carpeta del arbol debe:
//   - mover el archivo en disco
//   - incrementar contadorCambios (para que el arbol se rescanee)
//   - publicar ArchivosReubicados (para reescribir referencias de la escena)
void seccion05(const fs::path& proy, FileManager& fm, FileSelection*& sel) {
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
}

// Soltar una CARPETA sobre otra carpeta del arbol debe:
//   - mover la carpeta con TODO su subarbol (hijos, nietos)
//   - incrementar contadorCambios
//   - publicar ArchivosReubicados con la ruta de la carpeta (prefijo)
//   - tras refrescar, el arbol tiene la carpeta en la nueva ubicacion
void seccion06(const fs::path& proy, FileManager& fm, FileSelection*& sel, const std::string& rutaNueva) {
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
}

// --- Renombre por click derecho: helper compartido arbol/grid ------------
// Renombrar no cambia de carpeta: la ruta nueva es hermana de la vieja. El
// disco lo hace FileManager (que rechaza separadores en el nombre) y el
// exito se avisa una sola vez, porque de ese evento depende que la escena
// reescriba las referencias que apuntaban a la ruta vieja.
void seccion07(const fs::path& proy, FileManager& fm, const std::string& rutaNueva) {
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
}
