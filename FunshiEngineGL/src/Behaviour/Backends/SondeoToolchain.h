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
#ifndef SONDEOTOOLCHAIN_H
#define SONDEOTOOLCHAIN_H

// Sondeo de version de una herramienta externa (javac, el compilador C++)
// con el dispositivo nulo de la plataforma.
//
// Vive en un header propio (mismo criterio que ComandoCompilacionCpp.h: pocas
// lineas y sin obligar a tocar la lista de fuentes de CMakeLists.txt) para que
// el CONTRATO sea verificable headless.
//
// El bug que evita (H-14): el sondeo se hacia con `> /dev/null 2>&1`, que es
// un redirect de POSIX. std::system en Windows invoca `cmd.exe /c`, y cmd.exe
// no entiende /dev/null: lo toma como una ruta inexistente y el chequeo
// devuelve error AUNQUE la herramienta este instalada. Por eso
// scripts-java-tests se saltaba en Windows con el JDK perfectamente presente.

#include <string>

namespace SondeoToolchain {

// Dispositivo nulo del shell de la plataforma: `NUL` en cmd.exe (Windows),
// `/dev/null` en los shells de POSIX (Linux/macOS/MSYS2).
inline const char* dispositivoNulo() {
#if defined(_WIN32)
    return "NUL";
#else
    return "/dev/null";
#endif
}

// Comando de sondeo de version: `<herramienta> <flagVersion> > <nulo> 2>&1`.
//
// La herramienta va citada porque puede ser una ruta con espacios (un JDK en
// "Program Files/Eclipse Adoptium/..."). En Windows el comando se envuelve
// ademas en un par extra de comillas: std::system arma `cmd.exe /c <comando>`
// y con comilla inicial cmd se come la primera y la ultima de la linea,
// rompiendola (mismo motivo documentado en BackendJava.cpp para el comando de
// javac). El par extra hace que cmd se coma el envoltorio y el cuerpo llegue
// intacto.
//
// El flag de version no se unifica: javac usa `-version` y las familias de
// C++ usan `--version`, asi que lo decide quien llama.
inline std::string comandoVersion(const std::string& herramienta,
                                  const std::string& flagVersion) {
    std::string cmd = "\"" + herramienta + "\" " + flagVersion + " > " +
                      dispositivoNulo() + " 2>&1";
#if defined(_WIN32)
    cmd = "\"" + cmd + "\"";
#endif
    return cmd;
}

} // namespace SondeoToolchain

#endif // SONDEOTOOLCHAIN_H
