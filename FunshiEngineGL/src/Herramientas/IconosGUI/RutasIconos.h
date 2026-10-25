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
#ifndef RUTASICONOS_H
#define RUTASICONOS_H

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

// Resolucion de la carpeta de imagenes del motor.
//
// Vive aparte de IconosGUI para que el orden de busqueda sea una pieza
// testeable: el editor carga los iconos y las pruebas comprueban que cada
// icono declarado exista de verdad en el disco, sin necesitar una ventana.
namespace RutasIconos {

// Directorio del ejecutable (sin el nombre del binario, con separador final).
// Refleja donde quedo el build y NO depende del directorio desde el que se
// lance el motor: los iconos se resuelven 100% relativos al ejecutable.
inline std::string directorioEjecutable() {
#ifdef _WIN32
    char exe[4096] = {};
    DWORD n = GetModuleFileNameA(nullptr, exe, sizeof(exe));
    if (n == 0 || n >= sizeof(exe)) return "";
    const std::string path(exe, static_cast<std::size_t>(n));
    const std::size_t sep = path.find_last_of("\\/");
    return (sep == std::string::npos) ? "" : path.substr(0, sep + 1);
#else
    char link[4096] = {};
    const ssize_t n = readlink("/proc/self/exe", link, sizeof(link) - 1);
    if (n <= 0) return "";
    link[n] = '\0';
    const std::string path(link);
    const std::size_t sep = path.find_last_of('/');
    return (sep == std::string::npos) ? "" : path.substr(0, sep + 1);
#endif
}

// Carpetas donde se buscan los assets de imagen, en orden de prioridad:
//  1) Relativas al directorio del ejecutable (determinista): los vectores de
//     build suelen quedar junto al binario o un nivel arriba (build/ ->
//     ../Imagenes, build-java/ -> ../FunshiEngineGL/Imagenes) y en la CI el
//     build vive en la raiz del repositorio con las fuentes un nivel mas
//     abajo (build/Release/ -> ../../FunshiEngineGL/Imagenes).
//  2) Relativas al directorio de trabajo (fallback historico).
inline std::vector<std::string> carpetasImagenes() {
    std::vector<std::string> carpetas;
    const std::string exeDir = directorioEjecutable();
    if (!exeDir.empty()) {
        carpetas.push_back(exeDir + "Imagenes/");
        carpetas.push_back(exeDir + "../Imagenes/");
        carpetas.push_back(exeDir + "../FunshiEngineGL/Imagenes/");
        carpetas.push_back(exeDir + "../../Imagenes/");
        carpetas.push_back(exeDir + "../../FunshiEngineGL/Imagenes/");
        carpetas.push_back(exeDir + "../../../Imagenes/");
        carpetas.push_back(exeDir + "../../../FunshiEngineGL/Imagenes/");
    }
    carpetas.emplace_back("Imagenes/");
    carpetas.emplace_back("../Imagenes/");
    carpetas.emplace_back("../FunshiEngineGL/Imagenes/");
    carpetas.emplace_back("../../Imagenes/");
    carpetas.emplace_back("../../FunshiEngineGL/Imagenes/");
    carpetas.emplace_back("../../../Imagenes/");
    carpetas.emplace_back("../../../FunshiEngineGL/Imagenes/");
    return carpetas;
}

// Ruta real del archivo (ruta relativa a la carpeta de imagenes) o cadena
// vacia si no existe en ninguna de las carpetas candidatas.
inline std::string resolverRuta(const std::string& nombreRelativo) {
    for (const std::string& carpeta : carpetasImagenes()) {
        const std::string ruta = carpeta + nombreRelativo;
        std::ifstream archivo(ruta.c_str(), std::ios::binary);
        if (archivo.good()) return ruta;
    }
    return std::string();
}

// true si el archivo existe en alguna de las carpetas candidatas.
inline bool existe(const std::string& nombreRelativo) {
    return !resolverRuta(nombreRelativo).empty();
}

} // namespace RutasIconos

#endif // RUTASICONOS_H
