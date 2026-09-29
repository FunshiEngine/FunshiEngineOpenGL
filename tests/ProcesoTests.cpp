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
// Pruebas headless del runner de procesos sin shell (Proceso, H-3 nivel 2).
//
// El nucleo es un round-trip de punta a punta: el binario se COPIA a un
// directorio con espacios y se relanza a si mismo con argumentos hostiles
// (espacios, operadores de shell, comilla, barra final, vacio); el hijo los
// imprime al log y el padre exige que vuelvan BYTE A BYTE. Ese es el
// contrato que std::system + cmd.exe no podia garantizar (H-1/H-2/H-3).
// Ademas cubre exit codes, truncado del log, cwd, entorno extra y — en
// Windows — la receta cruda de cmd.exe que usa el harvest de vcvars.
//
// Patron de autoria (ver AGENTS.md): CHECK definido en este archivo,
// TempPruebas::CarpetaPrueba para la carpeta temporal que se limpia sola, y
// salida final "OK/FALLOS: N comprobaciones" saliendo con 0 o 1.

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/FileManager/Proceso.h"

namespace fs = std::filesystem;

static int total = 0;
static int fallos = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "FALLO: " << (msg) << std::endl;                      \
        }                                                                      \
    } while (0)

// getline de log: el hijo escribe en modo texto, asi que en Windows cada
// linea termina en \r\n (mismo criterio que SceneSerializer::getlineLimpio).
static bool leerLinea(std::ifstream& f, std::string& linea) {
    if (!std::getline(f, linea)) return false;
    if (!linea.empty() && linea.back() == '\r') linea.pop_back();
    return true;
}

static std::vector<std::string> lineasDe(const std::string& ruta) {
    std::vector<std::string> lineas;
    std::ifstream f(ruta);
    std::string linea;
    while (leerLinea(f, linea)) lineas.push_back(linea);
    return lineas;
}

static std::string contenidoDe(const std::string& ruta) {
    std::ifstream f(ruta);
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

// --- Modo hijo: este binario relanzado a si mismo ------------------------------
// `--hijo <modo> ...`: cada modo deja una senal verificable en el log o en el
// cwd. El guard evita la recursion cuando el padre nos sondea con `--hijo`.
static int modoHijo(int argc, char** argv) {
    const std::string modo = argv[2];
    if (modo == "args") {
        for (int i = 3; i < argc; ++i) std::cout << "ARG=" << argv[i] << "\n";
        return 0;
    }
    if (modo == "codigo")
        return argc >= 4 ? std::atoi(argv[3]) : 0;
    if (modo == "cwd") {
        std::ofstream f("archivo_en_cwd.txt");
        f << "ok";
        return f ? 0 : 1;
    }
    if (modo == "env") {
        const char* v = std::getenv("FUNSHI_SONDA_ENV");
        std::cout << "ENV=" << (v ? v : "(nula)") << "\n";
        return 0;
    }
    return 2;
}

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--hijo")
        return modoHijo(argc, argv);

    TempPruebas::CarpetaPrueba carpetaDir("funshi_proceso_test");
    const fs::path dir = carpetaDir.ruta();
    std::error_code ec;

    // --- Copia propia a un directorio con espacios (el caso real del usuario) --
    fs::path propio = fs::absolute(argv[0], ec);
    if (ec) propio = argv[0];
    fs::create_directories(dir / "con espacios", ec);
    const fs::path copia = dir / "con espacios" / propio.filename();
    fs::copy_file(propio, copia, fs::copy_options::overwrite_existing, ec);
    CHECK(!ec, "se copia el propio binario a un directorio con espacios");

    const std::string logArgs = (dir / "args.log").string();

    // --- Round-trip de argv: los argumentos vuelven byte a byte ----------------
    const std::vector<std::string> argsHijo = {
        copia.string(), "--hijo", "args",
        "hola mundo",         // espacio: el shell de antes lo cortaba
        "a&b^c%d",            // operadores y expansiones de shell
        "com\"illa",          // comilla: el escape que la CRT si entiende
        "C:\\fin con barra\\", // barra final: el \" que se comia el hijo
        ""                    // argumento vacio
    };
    const int rcArgs = Proceso::ejecutar(argsHijo, logArgs);
    CHECK(rcArgs == 0, "el hijo corre y termina en 0 con argumentos hostiles");
    const std::vector<std::string> lineas = lineasDe(logArgs);
    CHECK(lineas.size() == 5, "el hijo recibe exactamente los 5 argumentos");
    CHECK(!lineas.empty() && lineas[0] == "ARG=hola mundo",
          "un argumento con espacios vuelve completo");
    CHECK(lineas.size() > 1 && lineas[1] == "ARG=a&b^c%d",
          "operadores de shell (& ^ %) sobreviven sin que nadie los expanda");
    CHECK(lineas.size() > 2 && lineas[2] == "ARG=com\"illa",
          "una comilla interior vuelve intacta (escape de la CRT, no del "
          "shell)");
    CHECK(lineas.size() > 3 && lineas[3] == "ARG=C:\\fin con barra\\",
          "la barra final no se pierde ni se come la comilla de cierre");
    CHECK(lineas.size() > 4 && lineas[4] == "ARG=",
          "un argumento vacio no desaparece");

    // --- Exit codes ------------------------------------------------------------
    const std::string logCodigo = (dir / "codigo.log").string();
    CHECK(Proceso::ejecutar({copia.string(), "--hijo", "codigo", "0"},
                             logCodigo) == 0,
          "el exit code 0 llega como 0");
    CHECK(Proceso::ejecutar({copia.string(), "--hijo", "codigo", "7"},
                             logCodigo) == 7,
          "el exit code 7 llega como 7 (no un wait status crudo)");

    // --- El log se trunca en cada arranque, como el `> log` del shell ----------
    // (codigo no imprime nada: si el archivo conserva el contenido viejo de
    // args.log, no se trunco).
    Proceso::ejecutar({copia.string(), "--hijo", "codigo", "0"}, logArgs);
    CHECK(contenidoDe(logArgs).empty(),
          "el log se trunca en cada arranque (hereda la semantica de `>` )");

    // --- cwd: el hijo trabaja donde se le pide, no donde vive el motor ---------
    fs::create_directories(dir / "trabajo", ec);
    const std::string logCwd = (dir / "cwd.log").string();
    const int rcCwd = Proceso::ejecutar({copia.string(), "--hijo", "cwd"},
                                         logCwd, (dir / "trabajo").string());
    CHECK(rcCwd == 0, "el hijo termina en 0 con cwd pedido");
    CHECK(fs::exists(dir / "trabajo" / "archivo_en_cwd.txt"),
          "el hijo crea su archivo en el cwd pedido");
    CHECK(!fs::exists(dir / "archivo_en_cwd.txt"),
          "y no en el directorio de trabajo del motor");

    // --- Entorno extra ----------------------------------------------------------
    const std::string logEnv = (dir / "env.log").string();
    const int rcEnv = Proceso::ejecutar(
        {copia.string(), "--hijo", "env"}, logEnv, std::string(),
        {{"FUNSHI_SONDA_ENV", "valor con espacios"}});
    CHECK(rcEnv == 0, "el hijo termina en 0 con entorno extra");
    CHECK(contenidoDe(logEnv).find("ENV=valor con espacios") !=
              std::string::npos,
          "la variable extra llega al hijo, con sus espacios");

    // --- Casos de error ---------------------------------------------------------
    CHECK(Proceso::ejecutar({"funshi_no_existe_xyz_123"},
                             (dir / "fantasma.log").string()) != 0,
          "un programa que no existe no devuelve 0");
    CHECK(Proceso::ejecutar({}) == -1, "sin argumentos no se lanza nada");

    // --- Contrato de citacion (H-3, movido desde scripts-tests) -----------------
    // La regla no la pone cmd.exe (ya no hay shell): la pone el parser del
    // HIJO (CRT / CommandLineToArgvW). Solo se duplica el tramo final de
    // backslashes, que es el que el hijo leeria como \" pegado a la comilla
    // de cierre; los internos son literales.
    CHECK(Proceso::citar("C:\\Users\\gianf\\x.cpp") ==
              "\"C:\\Users\\gianf\\x.cpp\"",
          "una ruta con backslashes internos se cita sin duplicarlos");
    CHECK(Proceso::citar("C:\\a\\b\\") == "\"C:\\a\\b\\\\\"",
          "el tramo final de backslashes si se duplica");
    CHECK(Proceso::citar("\\\\server\\share\\x.cpp") ==
              "\"\\\\server\\share\\x.cpp\"",
          "una ruta UNC conserva sus backslashes iniciales");
    CHECK(Proceso::citar("C:\\Proyectos\\Nuevo Proyecto\\") ==
              "\"C:\\Proyectos\\Nuevo Proyecto\\\\\"",
          "ruta con espacios y barra final: se citan los espacios, no los "
          "backslashes internos");
    CHECK(Proceso::citar("") == "\"\"",
          "el argumento vacio se cita como cadena vacia");
    CHECK(Proceso::lineaDeArgumentos({"C:\\Program Files\\cl.exe", "/c", "a b"}) ==
              "\"C:\\Program Files\\cl.exe\" \"/c\" \"a b\"",
          "la linea completa cita cada argumento y los une con espacios");

#if defined(_WIN32)
    // --- Receta cruda de cmd.exe (el unico uso: harvest de vcvars) --------------
    // Sin citar: cmd.exe no entiende el escape \" de la CRT, por eso esa
    // receta viene balanceada a mano (ver ComandoCompilacionCpp::
    // comandoEntornoVcvars) y esta funcion no la toca.
    const std::string logCrudo = (dir / "crudo.log").string();
    CHECK(Proceso::ejecutarCmdCrudo("/d /c echo hola_cruda", logCrudo) == 0,
          "la receta cruda de cmd.exe corre y devuelve 0");
    CHECK(contenidoDe(logCrudo).find("hola_cruda") != std::string::npos,
          "el output de la receta cruda queda en el log");
#endif

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
