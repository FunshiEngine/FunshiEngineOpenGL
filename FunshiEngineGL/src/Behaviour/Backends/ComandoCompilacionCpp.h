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
#ifndef COMANDOCOMPILACIONCPP_H
#define COMANDOCOMPILACIONCPP_H

// Flags con los que BackendCpp compila el .cpp del script a la biblioteca
// compartida que despues carga con dlopen/LoadLibrary.
//
// Vive en un header propio (mismo criterio que SoltarEnCarpeta.h: pocas lineas
// y sin obligar a tocar la lista de fuentes de CMakeLists.txt) para que el
// CONTRATO sea verificable headless. La corrupcion de heap por desajuste de CRT
// no se puede reproducir sin MSVC ni observar sin cargar el .dll en runtime,
// pero si se puede assertar que los flags son los que tienen que ser.
//
// REQUISITO DE ABI, no robustez: la reflexion cruza la frontera del .dll con
// objetos que heap-alocan (std::string, std::vector<DefCampo> y los
// std::function de cada campo). Esa memoria se aloca en un modulo y se libera
// del otro, asi que engine y script tienen que compartir el MISMO runtime de
// C++. Con el CRT estatico (/MT) cada modulo tiene su propio heap y el delete
// del otro lado corrompe el heap.

#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

// Proceso::ejecutarCmdCrudo (solo para el harvest del entorno de vcvars).
#include "../../FileManager/Proceso.h"

namespace CompilacionCpp {

// Familia del compilador: cada una tiene su juego de flags. Se elige por el
// TOOLCHAIN (nombre del ejecutable), no por la plataforma: un build MinGW en
// Windows recibia los flags de MSVC (`/nologo /LD ...`) y `c++.exe` los tomaba
// como nombres de archivo, asi que los scripts C++ no compilaban nunca.
enum class Familia { Msvc, Gcc };

// Compilador con el que ESTE build compila los scripts (lo hornea CMake; mismo
// nombre y default que BackendCpp, para que el header sirva aislado en tests).
#ifndef FUNSHI_CXX_COMPILER
#define FUNSHI_CXX_COMPILER "g++"
#endif

// Runtime dinamico de Visual C++ del build del engine. CMake lo hornea (/MDd en
// Debug, /MD en el resto) para que el script siga la MISMA configuracion que el
// binario que lo carga: un engine en /MDd con un script en /MD tambien seria un
// desajuste (CRT de debug contra CRT de release). El valor puede llegar con las
// comillas externas del literal, como FUNSHI_CXX_COMPILER.
#ifndef FUNSHI_CXX_RUNTIME_FLAG
#define FUNSHI_CXX_RUNTIME_FLAG "/MD"
#endif

namespace detalle {

// Valor sin las comillas externas que agrega el literal del compilador.
inline std::string sinComillas(const std::string& valor) {
    if (valor.size() >= 2 && valor.front() == '"' && valor.back() == '"')
        return valor.substr(1, valor.size() - 2);
    return valor;
}

} // namespace detalle

// Valor normalizado del flag de runtime horneado por CMake.
inline std::string runtimeFlag() {
    return detalle::sinComillas(FUNSHI_CXX_RUNTIME_FLAG);
}

// Nombre del ejecutable del compilador, sin comillas, sin carpeta y sin
// extension, en minusculas: "C:/Program Files/.../cl.exe" -> "cl".
inline std::string nombreEjecutable(const std::string& ruta) {
    std::string nombre = detalle::sinComillas(ruta);
    const std::string::size_type separador = nombre.find_last_of("/\\");
    if (separador != std::string::npos) nombre = nombre.substr(separador + 1);
    const std::string::size_type punto = nombre.find_last_of('.');
    if (punto != std::string::npos && punto > 0) nombre = nombre.substr(0, punto);
    for (char& c : nombre)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return nombre;
}

// Familia por el nombre del ejecutable. Los prefijos de version ("g++-14",
// "clang++-18") cuentan como su familia; un nombre desconocido cae al default
// de la plataforma, que es donde el compilador mas probable vive.
inline Familia familiaDe(const std::string& compiladorRuta) {
    const std::string exe = nombreEjecutable(compiladorRuta);
    if (exe == "cl" || exe == "clang-cl") return Familia::Msvc;
    if (exe == "g++" || exe == "c++" || exe == "gcc" || exe == "clang++" ||
        exe == "clang" || exe.rfind("g++-", 0) == 0 ||
        exe.rfind("gcc-", 0) == 0 || exe.rfind("clang++-", 0) == 0 ||
        exe.rfind("clang-", 0) == 0)
        return Familia::Gcc;
#if defined(_WIN32)
    return Familia::Msvc;
#else
    return Familia::Gcc;
#endif
}

// Familia del toolchain horneado en el build (contrato que verifica el test).
inline Familia familiaCompilador() { return familiaDe(FUNSHI_CXX_COMPILER); }

// Juego de flags de la familia, sin espacios al principio ni al final: quien
// arma la linea de comandos decide los separadores.
inline std::string flagsFamilia(Familia familia, const std::string& nombreClase) {
    if (familia == Familia::Msvc) {
        // /MD   -> CRT dinamico compartido con el engine. Es lo que alinea el
        //          heap entre el .dll del script y el motor.
        // /EHsc -> sin el, la primera excepcion del script llama a
        //          std::terminate en vez de propagarse (cl.exe emite C4530).
        // /link /EXPORT:FUNSHI_CREAR_COMPORTAMIENTO -> la fabrica queda en la
        //          tabla de exportaciones de la .dll aunque el fuente use el
        //          template viejo sin FUNSHI_COMPORTAMIENTO_EXPORT (H-15): en
        //          MSVC un extern "C" pelado no se exporta solo y GetProcAddress
        //          fallaria. Los fuentes con el macro la exportan igual por
        //          dllexport; declarar el export dos veces no da error.
        return "/nologo /LD /std:c++17 /O2 " + runtimeFlag() +
               " /EHsc /DFUNSHI_NOMBRE_CLASE=" + nombreClase +
               " /link /EXPORT:FUNSHI_CREAR_COMPORTAMIENTO";
    }
    // GCC/Clang: Linux, macOS y MinGW (biblioteca compartida con -fPIC).
    return "-std=c++17 -shared -fPIC -O2 -DFUNSHI_NOMBRE_CLASE=" + nombreClase;
}

// Flags del toolchain con el que ESTE build compila los scripts.
inline std::string flagsCompilador(const std::string& nombreClase) {
    return flagsFamilia(familiaCompilador(), nombreClase);
}

// Todo lo que necesita la linea de comandos. Las rutas van SIN citar: el
// argv se arma con los valores crudos y quien cita al lanzar es Proceso
// (CreateProcessW en Windows; en POSIX execvp los recibe tal cual).
struct DatosComando {
    std::string compilador; // ruta al compilador (o el nombre, si esta en PATH)
    std::string nombreClase;
    std::string fuente;
    std::string dirSrc;     // carpeta de cabeceras del motor; vacia si no hay
    std::string dirObjetos; // solo MSVC (/Fo)
    std::string artefacto;  // .so/.dll/.dylib de salida
};

// Argumentos del proceso hijo (H-3 nivel 2): SIN shell, SIN citar y SIN el
// redirect `> log 2>&1` — la salida la redirige Proceso::ejecutar por
// handles/fd. La familia del compilador decide flags, include y salida; la
// plataforma (BackendCpp) decide la extension del artefacto y como se lanza
// el proceso.
inline std::vector<std::string>
argumentosCompilacion(const DatosComando& datos) {
    const Familia familia = familiaDe(datos.compilador);
    std::vector<std::string> argv;
    argv.reserve(10);
    argv.push_back(datos.compilador);

    // Los flags de la familia son tokens literales sin espacios: partirlos
    // por espacio conserva el orden (y los que llevan valor pegado, como
    // /DFUNSHI_NOMBRE_CLASE=X o -std=c++17, son un solo token).
    const std::string flags = flagsFamilia(familia, datos.nombreClase);
    std::string::size_type inicio = 0;
    while (inicio < flags.size()) {
        const std::string::size_type fin = flags.find(' ', inicio);
        const std::string token = flags.substr(
            inicio, fin == std::string::npos ? std::string::npos : fin - inicio);
        if (!token.empty())
            argv.push_back(token);
        if (fin == std::string::npos)
            break;
        inicio = fin + 1;
    }

    if (!datos.dirSrc.empty())
        argv.push_back((familia == Familia::Msvc ? "/I" : "-I") +
                       datos.dirSrc);
    argv.push_back(datos.fuente);
    if (familia == Familia::Msvc) {
        if (!datos.dirObjetos.empty())
            // /Fo necesita la barra final para que cl.exe lo lea como carpeta;
            // Proceso::citar() la duplica al armar la linea para que el hijo
            // no se coma la comilla de cierre.
            argv.push_back("/Fo" + datos.dirObjetos + "\\");
        argv.push_back("/Fe" + datos.artefacto);
    } else {
        argv.push_back("-o");
        argv.push_back(datos.artefacto);
    }
    return argv;
}

// Ruta a vcvars64.bat subiendo desde la carpeta del compilador: el toolset
// MSVC la tiene en <VS>/VC/Auxiliary/Build, arriba de VC/Tools/MSVC/<ver>.
// Devuelve vacia si no es un compilador MSVC (MinGW, FUNSHI_CXX manual, etc.).
//
// Vive en el header para que el test de contrato pueda assertar que TERMINA
// (bug real: en MinGW/libstdc++ `path("C:\\").parent_path()` devuelve
// `C:\\` y nunca vacio, asi que el bucle sobre parent_path() giraba para
// siempre y BackendCpp se colgaba al compilar un script en Windows+MinGW;
// jamas aparecio porque scripts-runtime-tests se saltaba en Windows por
// plataforma). Dos guardas contra eso:
//   1. Si la familia no es MSVC, ni siquiera se recorre: MinGW no usa vcvars.
//   2. El recorte se detiene cuando parent_path() deja de avanzar (raiz).
inline std::string vcvars64Ruta(const std::string& compiladorRuta) {
    if (familiaDe(compiladorRuta) != Familia::Msvc) return std::string();
    std::error_code ec;
    std::filesystem::path p =
        std::filesystem::weakly_canonical(detalle::sinComillas(compiladorRuta),
                                          ec);
    if (ec) p = std::filesystem::path(detalle::sinComillas(compiladorRuta));
    for (std::filesystem::path dir = p.parent_path(); !dir.empty();) {
        std::filesystem::path cand =
            dir / "Auxiliary" / "Build" / "vcvars64.bat";
        if (std::filesystem::exists(cand, ec)) return cand.string();
        const std::filesystem::path arriba = dir.parent_path();
        if (arriba == dir) break; // llego a la raiz: no avanza mas
        dir = arriba;
    }
    return std::string();
}

// --- Entorno de vcvars sin shell (H-3 nivel 2) --------------------------------
//
// cl.exe no arranca sin el entorno del toolset (INCLUDE/LIB/PATH con
// link.exe). Antes eso se lograba encadenando `call vcvars64.bat && cl ...`
// DENTRO de cmd.exe — junto con el comando completo, con todos los datos de
// usuario. Ahora el compilador corre solo (Proceso::ejecutar) y lo unico que
// pasa por cmd es la receta fija de abajo, que no lleva datos de usuario:
// solo la ruta de vcvars, balanceada a mano. Ver PLAN GENERAL DE FIX.md §21.

// Receta cruda para Proceso::ejecutarCmdCrudo. Detalles:
//  - `call`: sin el, cmd entrega el control al .bat y al terminar sale sin
//    ejecutar lo que sigue del `&&`;
//  - `/U`: la salida de `set` en UTF-16LE. Sin /U escribe en la CP OEM
//    (sonda: en espanol, CP850) y una ruta no-ASCII se corromperia al
//    reconvertirla (el round-trip con /U quedo byte-exacto);
//  - `/d`: ignora el AutoRun del registro;
//  - el remainder arranca con `call` (no con comilla), asi que no aplica el
//    strip de primera/ultima comilla de cmd.exe.
inline std::string comandoEntornoVcvars(const std::string& vcvarsRuta) {
    return "/U /d /c call \"" + vcvarsRuta + "\" && set";
}

// Parsea la salida UTF-16 de `cmd /U ... set` y devuelve el bloque de
// entorno (multi-sz: pares clave=valor\0 terminado con un \0 extra),
// ordenado alfabeticamente como pide CreateProcessW. Solo entran lineas con
// nombre de variable valido: el banner de vcvars (sin `=` o con formas que
// no son CLAVE=valor) se descarta. El valor puede contener `=` (se corta en
// el PRIMERO). Devuelve vacio si no hay ni 3 variables: no es una salida de
// `set` utilizable (archivo vacio, cmd no corrio, archivo viejo).
inline std::wstring bloqueDesdeSet(const std::wstring& salida) {
    std::map<std::wstring, std::wstring> variables;
    std::wstring::size_type inicio = 0;
    while (inicio < salida.size()) {
        std::wstring::size_type fin = salida.find(L'\n', inicio);
        if (fin == std::wstring::npos)
            fin = salida.size();
        std::wstring linea = salida.substr(inicio, fin - inicio);
        inicio = fin + 1;
        if (!linea.empty() && linea.back() == L'\r')
            linea.pop_back();

        const std::wstring::size_type igual = linea.find(L'=');
        if (igual == std::wstring::npos || igual == 0)
            continue; // banner u otras lineas sin clave
        bool nombreValido = true;
        for (std::wstring::size_type i = 0; i < igual; ++i) {
            const wchar_t c = linea[i];
            const bool ok = (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') ||
                            (c >= L'0' && c <= L'9') || c == L'_';
            if (!ok || (i == 0 && c >= L'0' && c <= L'9')) {
                nombreValido = false;
                break;
            }
        }
        if (!nombreValido)
            continue;
        variables[linea.substr(0, igual)] = linea.substr(igual + 1);
    }
    if (variables.size() < 3)
        return std::wstring();

    std::wstring bloque;
    for (const auto& par : variables) {
        bloque += par.first;
        bloque += L'=';
        bloque += par.second;
        bloque += L'\0';
    }
    bloque += L'\0';
    return bloque;
}

#if defined(_WIN32)

// Entorno completo de vcvars, ejecutado y parseado UNA vez por proceso
// (cache: el primer script MSVC paga los ~200 ms del .bat). Devuelve el
// bloque multi-sz listo para Proceso::ejecutarConBloque, o VACIO si algo
// fallo (vcvars inexistente, cmd no corrio, salida ilegible): en ese caso
// BackendCpp compila con el entorno heredado y el error exacto de cl queda
// en el log. El archivo se lee como bytes (UTF-16LE) sin windows.h, para
// que el header siga siendo seguro en cualquier TU.
inline const std::wstring& entornoVcvars(const std::string& vcvarsRuta) {
    static std::wstring cache;
    static std::string cacheLlave;
    if (vcvarsRuta == cacheLlave && !cache.empty())
        return cache;
    cacheLlave = vcvarsRuta;
    cache.clear();
    if (vcvarsRuta.empty())
        return cache;

    std::error_code ec;
    const std::filesystem::path tmp =
        std::filesystem::temp_directory_path(ec) /
        ("funshi_vcvars_entorno_" +
         std::to_string(std::chrono::steady_clock::now()
                            .time_since_epoch()
                            .count()) +
         ".txt");
    if (ec)
        return cache;

    Proceso::ejecutarCmdCrudo(comandoEntornoVcvars(vcvarsRuta), tmp.string());

    std::ifstream archivo(tmp, std::ios::binary);
    if (archivo) {
        const std::string bytes((std::istreambuf_iterator<char>(archivo)),
                                std::istreambuf_iterator<char>());
        archivo.close();
        // UTF-16LE: pares de bytes (posible BOM FF FE inicial; los pares
        // incompletos de un archivo truncado se ignoran).
        std::size_t i = 0;
        if (bytes.size() >= 2 &&
            static_cast<unsigned char>(bytes[0]) == 0xFF &&
            static_cast<unsigned char>(bytes[1]) == 0xFE)
            i = 2;
        std::wstring salida;
        salida.reserve((bytes.size() - i) / 2);
        for (; i + 1 < bytes.size(); i += 2)
            salida.push_back(static_cast<wchar_t>(
                static_cast<unsigned char>(bytes[i]) |
                (static_cast<unsigned char>(bytes[i + 1]) << 8)));
        cache = bloqueDesdeSet(salida);
    }
    std::filesystem::remove(tmp, ec);
    return cache;
}

#endif // _WIN32

// Verdadero si `flags` contiene el flag exacto, comparando por tokens: "/MD" no
// tiene que dar positivo sobre "/MDd" ni "-O2" sobre "-O2x". Se usa en el test
// de contrato y deja el chequeo en un solo lugar.
inline bool tieneFlag(const std::string& flags, const std::string& flag) {
    if (flag.empty()) return false;
    std::string::size_type pos = 0;
    while ((pos = flags.find(flag, pos)) != std::string::npos) {
        const bool bordeIzquierdo = pos == 0 || flags[pos - 1] == ' ';
        const std::string::size_type fin = pos + flag.size();
        const bool bordeDerecho = fin >= flags.size() || flags[fin] == ' ';
        if (bordeIzquierdo && bordeDerecho) return true;
        pos = fin;
    }
    return false;
}

} // namespace CompilacionCpp

#endif // COMANDOCOMPILACIONCPP_H
