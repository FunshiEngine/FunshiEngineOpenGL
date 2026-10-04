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
#ifndef RESOLUCION_JDK_H
#define RESOLUCION_JDK_H

#include <filesystem>
#include <string>
#include <vector>

// Emparejamiento de las dos herramientas del toolchain Java: la biblioteca de la
// JVM que ejecuta los scripts y el compilador que los produce. Tiene que salir
// de la MISMA raiz de JDK.
//
// Antes cada una recorria la lista de raices por su cuenta y se quedaba con el
// primer acierto, asi que una raiz con solo la JVM podia ganar para libjvm y otra
// con javac ganar para el compilador. El sintoma de esa mezcla es invisible: un
// javac moderno genera bytecode que la JVM antigua rechaza con
// UnsupportedClassVersionError y el motor informa "no se encontro la clase", que
// es justo el mensaje de "el .class no existe".
//
// Se separa de BackendJava para poder probar el emparejamiento sin disco ni JVM:
// los dos resolutores se inyectan, de modo que un arbol falso de JDK suffit.
namespace ResolucionJdk {

struct Herramientas {
    std::string libjvm;  // vacio si no hay ninguna JVM
    std::string javac;   // vacio si la raiz elegida no trae compilador
    std::string raiz;    // raiz de JDK de la que salen ambas, para el diagnostico
};

// Primer acierto, pero con las dos herramientas de la misma raiz: si la raiz
// que tiene la JVM no trae javac, el resultado NO busca javac en otra raiz,
// porque esa seria justo la mezcla que rompe. El llamador decide entonces si
// avisa de que hace falta un JDK y no un JRE, o si acepta el .class ya
// compilado.
template <typename ResLibjvm, typename ResJavac>
inline Herramientas desdeRaices(const std::vector<std::string>& raices,
                                ResLibjvm libjvmEn,
                                ResJavac javacEn) {
    for (const std::string& raiz : raices) {
        const std::string libjvm = libjvmEn(raiz);
        if (libjvm.empty()) continue;
        return Herramientas{libjvm, javacEn(raiz), raiz};
    }
    return Herramientas{};
}

// Raiz de JDK a la que pertenece una biblioteca de la JVM, deducida del propio
// archivo: <jdk>/lib/server/libjvm.so, <jdk>/bin/server/jvm.dll y el
// <jdk>/lib/jvm.lib que entrega FindJNI en Windows. Sirve para el caso de que la
// biblioteca venga dada por una ruta suelta (FUNSHI_LIBJVM o el valor horneado
// por CMake), donde no hay una "raiz" que recorrer y aun asi hay que buscar el
// javac de ese mismo JDK.
//
// Si la ruta no lleva "jvm" en el nombre, NO es una biblioteca sino la propia
// raiz de un JDK (alguien apunta FUNSHI_LIBJVM a /opt/jdk): se toma tal cual,
// porque quedarse con su carpeta padre apuntaria a /opt y el javac de ese JDK no
// se encontraria nunca.
inline std::string raizDesdeLibjvm(const std::string& libjvm) {
    if (libjvm.empty()) return {};
    const std::filesystem::path ruta(libjvm);
    if (ruta.filename().string().find("jvm") == std::string::npos)
        return libjvm;

    std::filesystem::path dir = ruta.parent_path();
    // "server" cuelga de "lib" (POSIX) o de "bin" (Windows): en los tres
    // layouts la raiz es el ancestro de la carpeta que precede a lib/bin.
    if (dir.filename().string() == "server") dir = dir.parent_path();
    const std::string carpeta = dir.filename().string();
    if (carpeta == "lib" || carpeta == "bin") dir = dir.parent_path();
    return dir.empty() ? std::string() : dir.string();
}

// Si el compilador se puede ejecutar desde donde se esta. Un nombre suelto
// ("javac") lo resuelve el PATH de la maquina y no se puede comprobar sin
// arrancarlo; una ruta absoluta o relativa con separadores, en cambio, tiene que
// existir en disco: si no existe, el fallo de Proceso::ejecutar es un "no se pudo
// ejecutar" mudo, cuando lo que falta de verdad es un JDK con bin/javac.
inline bool compiladorEjecutable(const std::string& javac) {
    if (javac.empty()) return false;
    std::error_code ec;
    if (std::filesystem::path(javac).filename().string() == javac)
        return true;  // lo buscara el PATH
    return std::filesystem::is_regular_file(javac, ec);
}

}  // namespace ResolucionJdk

#endif