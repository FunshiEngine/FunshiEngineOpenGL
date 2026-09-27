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

#include <string>

namespace CompilacionCpp {

// Familia del compilador: cada una tiene su juego de flags. Hoy se elige por
// plataforma (`flagsCompilador`); la deteccion del toolchain real es el paso
// siguiente (ver `familiaDe` en el commit que la introduce).
enum class Familia { Msvc, Gcc };

// Runtime dinamico de Visual C++ del build del engine. CMake lo hornea (/MDd en
// Debug, /MD en el resto) para que el script siga la MISMA configuracion que el
// binario que lo carga: un engine en /MDd con un script en /MD tambien seria un
// desajuste (CRT de debug contra CRT de release). El valor puede llegar con las
// comillas externas del literal, como FUNSHI_CXX_COMPILER.
#ifndef FUNSHI_CXX_RUNTIME_FLAG
#define FUNSHI_CXX_RUNTIME_FLAG "/MD"
#endif

// Valor normalizado del flag de runtime horneado por CMake.
inline std::string runtimeFlag() {
    std::string flag = FUNSHI_CXX_RUNTIME_FLAG;
    if (flag.size() >= 2 && flag.front() == '"' && flag.back() == '"')
        flag = flag.substr(1, flag.size() - 2);
    return flag;
}

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

// Flags del toolchain con el que ESTE build compila los scripts. En Windows la
// rama es la de MSVC: cuando el compilador configurado es MinGW hay que elegir
// por familia de compilador y no por plataforma (por eso `Familia` es un
// parametro y no un `#if` embebido en `flagsFamilia`).
inline std::string flagsCompilador(const std::string& nombreClase) {
#if defined(_WIN32)
    return flagsFamilia(Familia::Msvc, nombreClase);
#else
    return flagsFamilia(Familia::Gcc, nombreClase);
#endif
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
