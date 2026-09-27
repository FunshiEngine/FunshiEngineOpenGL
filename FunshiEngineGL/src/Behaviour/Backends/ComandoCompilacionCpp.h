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
#include <string>

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
        return "/nologo /LD /std:c++17 /O2 " + runtimeFlag() +
               " /EHsc /DFUNSHI_NOMBRE_CLASE=" + nombreClase;
    }
    // GCC/Clang: Linux, macOS y MinGW (biblioteca compartida con -fPIC).
    return "-std=c++17 -shared -fPIC -O2 -DFUNSHI_NOMBRE_CLASE=" + nombreClase;
}

// Flags del toolchain con el que ESTE build compila los scripts.
inline std::string flagsCompilador(const std::string& nombreClase) {
    return flagsFamilia(familiaCompilador(), nombreClase);
}

// Cita un argumento entre comillas y deja el interior tal como lo espera el
// parser de la linea de comandos del proceso hijo.
inline std::string citar(const std::string& ruta) {
    std::string resultado = "\"";
    for (char c : ruta) {
        if (c == '"') {
            resultado += "\\\"";
        } else if (c == '\\') {
            resultado += "\\\\";
        } else {
            resultado += c;
        }
    }
    resultado += "\"";
    return resultado;
}

// Todo lo que necesita la linea de comandos. Las rutas van SIN citar: el armado
// de abajo las pasa por `citar()`.
struct DatosComando {
    std::string compilador; // ruta al compilador (o el nombre, si esta en PATH)
    std::string nombreClase;
    std::string fuente;
    std::string dirSrc;     // carpeta de cabeceras del motor; vacia si no hay
    std::string dirObjetos; // solo MSVC (/Fo)
    std::string artefacto;  // .so/.dll/.dylib de salida
    std::string log;        // archivo que recibe stdout+stderr
};

// Linea de comandos completa. La familia del compilador decide flags, include y
// salida; la plataforma (BackendCpp) decide la extension del artefacto y como se
// lanza el proceso.
inline std::string comandoCompilacion(const DatosComando& datos) {
    const Familia familia = familiaDe(datos.compilador);
    std::string cmd =
        citar(datos.compilador) + " " + flagsFamilia(familia, datos.nombreClase);
    if (!datos.dirSrc.empty())
        cmd += (familia == Familia::Msvc ? " /I" : " -I") + citar(datos.dirSrc);
    cmd += " " + citar(datos.fuente);
    if (familia == Familia::Msvc) {
        if (!datos.dirObjetos.empty())
            // El backslash final va doblado A PROPOSITO: con /Fo"dir\" el parser
            // lee \" como comilla escapada, se traga el argumento siguiente y
            // falla con C1083 sobre el archivo generado.
            cmd += " /Fo\"" + datos.dirObjetos + "\\\\\"";
        cmd += " /Fe" + citar(datos.artefacto);
    } else {
        cmd += " -o " + citar(datos.artefacto);
    }
    cmd += " > " + citar(datos.log) + " 2>&1";
    return cmd;
}

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
