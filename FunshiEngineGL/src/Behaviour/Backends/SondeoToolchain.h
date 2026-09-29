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
// ejecutandola directamente y tirando stdout+stderr al dispositivo nulo de
// la plataforma.
//
// Vive en un header propio (mismo criterio que ComandoCompilacionCpp.h: pocas
// lineas y sin obligar a tocar la lista de fuentes de CMakeLists.txt) para que
// el CONTRATO sea verificable headless.
//
// Historia de los dos bugs que va dejando atras:
//  - H-14: el sondeo original redirigia con `> /dev/null 2>&1` via
//    std::system. En Windows eso es `cmd.exe /c ...`, que no entiende
//    /dev/null: lo toma como una ruta inexistente y el chequeo devolvia
//    error AUNQUE la herramienta este instalada (por eso
//    scripts-java-tests se saltaba en Windows con el JDK presente).
//  - H-3 nivel 2: el envoltorio sobre `comandoVersion()` (que era un string
//    de shell, con su cita y su redirect) y el std::system desaparecen:
//    Proceso::ejecutar lanza la herramienta sin shell y el log es el
//    dispositivo nulo AHI (abierto por el runner: NUL en Windows, /dev/null
//    en POSIX). No hay redirect que un shell pueda no entender.
//    Ver PLAN GENERAL DE FIX.md §21.

#include <string>

#include "../../FileManager/Proceso.h"

namespace SondeoToolchain {

// Dispositivo nulo del shell de la plataforma: `NUL` en Windows,
// `/dev/null` en POSIX. Se usa como destino del log de Proceso::ejecutar.
inline const char* dispositivoNulo() {
#if defined(_WIN32)
    return "NUL";
#else
    return "/dev/null";
#endif
}

// Ejecuta `herramienta flagVersion` sin shell y devuelve true si arranco y
// salio con 0. El output se descarta (dispositivo nulo); lo que se prueba es
// que el proceso CORRE de verdad (existe, arranca, no requiere un shell que
// no esta).
inline bool sondear(const std::string& herramienta,
                    const std::string& flagVersion) {
    if (herramienta.empty())
        return false;
    return Proceso::ejecutar({herramienta, flagVersion},
                              dispositivoNulo()) == 0;
}

} // namespace SondeoToolchain

#endif // SONDEOTOOLCHAIN_H
