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
// Pruebas headless de "Importar proyecto" (Configurar proyecto): copiar a la
// carpeta de proyectos un proyecto que esta en otro lugar.
//
// Sin pila grafica: carpetas temporales reales (TempPruebas::CarpetaPrueba).
//
// Casos:
//   - Se reconoce un proyecto por su carpeta y por su .fnep.
//   - Una carpeta que no es proyecto, una ruta inexistente y un proyecto que
//     ya esta en la carpeta de proyectos se rechazan con su motivo.
//   - Una carpeta renombrada a mano se reconoce por el nombre de su .fnep.
//   - Importar copia todo, deja un solo .fnep con el nombre correcto, no toca
//     el origen y no deja la carpeta temporal.
//   - Importar con otro nombre renombra src<Nombre> y el .fnep.
//   - Un nombre repetido o invalido no importa nada.

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Configuracion/Proyecto/ArchivoProyecto.h"
#include "../FunshiEngineGL/src/Configuracion/Proyecto/ImportarProyecto.h"

namespace fs = std::filesystem;
using ImportarProyecto::Problema;

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

void escribirArchivo(const fs::path& ruta, const std::string& contenido) {
    fs::create_directories(ruta.parent_path());
    std::ofstream(ruta, std::ios::binary) << contenido;
}

std::string leerArchivo(const fs::path& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// Proyecto minimo como lo deja el motor: Memory/, src<Nombre>/ y <Nombre>.fnep.
void crearProyecto(const fs::path& carpeta, const std::string& nombre) {
    escribirArchivo(carpeta / "Memory" / "ConfiguracionProyecto.json", "{}");
    escribirArchivo(carpeta / ("src" + nombre) / "Scripts" / "Jugador.cpp", "// jugador");
    ArchivoProyecto::Datos datos;
    datos.nombre = nombre;
    datos.motor = "0.4.0";
    std::string error;
    ArchivoProyecto::escribir((carpeta / (nombre + ".fnep")).string(), datos, error);
}

std::string nombreEnFnep(const fs::path& ruta) {
    std::string error;
    auto datos = ArchivoProyecto::leer(ruta.string(), error);
    return datos ? datos->nombre : std::string();
}

bool hayTemporales(const fs::path& raiz) {
    std::error_code ec;
    for (fs::directory_iterator it(raiz, ec), fin; !ec && it != fin; it.increment(ec))
        if (it->path().filename().string().rfind(".importando-", 0) == 0) return true;
    return false;
}

void reconoceElOrigen(const fs::path& base) {
    const fs::path raiz = base / "Proyects";
    const fs::path origen = base / "Traido" / "Juego";
    crearProyecto(origen, "Juego");

    auto porCarpeta = ImportarProyecto::analizar(origen.string(), raiz.string());
    CHECK(porCarpeta.valido(), "una carpeta de proyecto se reconoce");
    CHECK(porCarpeta.nombre == "Juego", "el nombre sale de src<Nombre>");
    CHECK(porCarpeta.motor == "0.4.0", "se conserva la version del motor del .fnep");

    auto porFnep = ImportarProyecto::analizar((origen / "Juego.fnep").string(), raiz.string());
    CHECK(porFnep.valido() && porFnep.carpeta == porCarpeta.carpeta,
          "elegir el .fnep equivale a elegir su carpeta");

    fs::create_directories(base / "Vacia");
    CHECK(ImportarProyecto::analizar((base / "Vacia").string(), raiz.string()).problema ==
              Problema::NoEsProyecto,
          "una carpeta sin Memory ni src<Nombre> no es un proyecto");
    CHECK(ImportarProyecto::analizar((base / "NoHay").string(), raiz.string()).problema ==
              Problema::NoExiste,
          "una ruta inexistente se rechaza");

    crearProyecto(raiz / "YaEsta", "YaEsta");
    CHECK(ImportarProyecto::analizar((raiz / "YaEsta").string(), raiz.string()).problema ==
              Problema::YaEstaEnProyectos,
          "un proyecto que ya esta en la carpeta de proyectos no se importa");

    // La carpeta se renombro a mano: src<Nombre> y el .fnep siguen diciendo "Nave".
    crearProyecto(base / "Copia de Nave", "Nave");
    auto renombrada = ImportarProyecto::analizar((base / "Copia de Nave").string(), raiz.string());
    CHECK(renombrada.valido() && renombrada.nombre == "Nave",
          "una carpeta renombrada a mano se reconoce por su .fnep");
}

void importaConElMismoNombre(const fs::path& base) {
    const fs::path raiz = base / "Proyects";
    const fs::path origen = base / "Otra maquina" / "Juego";
    crearProyecto(origen, "Juego");

    auto o = ImportarProyecto::analizar(origen.string(), raiz.string());
    CHECK(ImportarProyecto::importar(o, "Juego", raiz.string()) == Problema::Ninguno,
          "el proyecto se importa");
    const fs::path destino = raiz / "Juego";
    CHECK(fs::is_directory(destino / "Memory") && fs::is_directory(destino / "srcJuego"),
          "la copia tiene Memory y srcJuego");
    CHECK(leerArchivo(destino / "srcJuego" / "Scripts" / "Jugador.cpp") == "// jugador",
          "el contenido del proyecto se copia intacto");
    CHECK(nombreEnFnep(destino / "Juego.fnep") == "Juego", "el .fnep queda con su nombre");
    CHECK(fs::exists(origen / "Juego.fnep") && fs::exists(origen / "srcJuego"),
          "el origen no se toca");
    CHECK(!hayTemporales(raiz), "no queda la carpeta temporal de la copia");

    CHECK(ImportarProyecto::importar(o, "Juego", raiz.string()) == Problema::NombreRepetido,
          "importar otra vez con el mismo nombre no pisa el proyecto");
}

void importaConOtroNombre(const fs::path& base) {
    const fs::path raiz = base / "Proyects";
    const fs::path origen = base / "Usb" / "Juego";
    crearProyecto(origen, "Juego");
    crearProyecto(raiz / "Juego", "Juego");  // ya hay uno con ese nombre

    auto o = ImportarProyecto::analizar(origen.string(), raiz.string());
    CHECK(ImportarProyecto::validarNombre("Juego", raiz.string()) == Problema::NombreRepetido,
          "el nombre del origen esta ocupado");
    CHECK(ImportarProyecto::importar(o, "Juego 2", raiz.string()) == Problema::Ninguno,
          "se importa con otro nombre");
    const fs::path destino = raiz / "Juego 2";
    CHECK(fs::is_directory(destino / "srcJuego 2") && !fs::exists(destino / "srcJuego"),
          "src<Nombre> se renombra al nombre nuevo");
    CHECK(nombreEnFnep(destino / "Juego 2.fnep") == "Juego 2" && !fs::exists(destino / "Juego.fnep"),
          "queda un solo .fnep, con el nombre nuevo");
    CHECK(leerArchivo(destino / "srcJuego 2" / "Scripts" / "Jugador.cpp") == "// jugador",
          "los archivos del src renombrado se conservan");

    CHECK(ImportarProyecto::validarNombre("", raiz.string()) == Problema::NombreInvalido,
          "un nombre vacio no sirve");
    CHECK(ImportarProyecto::validarNombre("a/b", raiz.string()) == Problema::NombreInvalido,
          "un nombre con barra no sirve");
    CHECK(ImportarProyecto::validarNombre(".oculto", raiz.string()) == Problema::NombreInvalido,
          "un nombre que empieza con punto no sirve");
    CHECK(ImportarProyecto::importar(o, "a/b", raiz.string()) == Problema::NombreInvalido &&
              !hayTemporales(raiz),
          "un nombre invalido no importa nada");
}

}  // namespace

int main() {
    TempPruebas::CarpetaPrueba carpeta("importar_proyecto");
    reconoceElOrigen(carpeta.ruta() / "reconoce");
    importaConElMismoNombre(carpeta.ruta() / "mismo");
    importaConOtroNombre(carpeta.ruta() / "otro");

    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    std::cout << (fallos == 0 ? "IMPORTAR PROYECTO TESTS OK" : "IMPORTAR PROYECTO TESTS FALLO")
              << std::endl;
    return fallos == 0 ? 0 : 1;
}
