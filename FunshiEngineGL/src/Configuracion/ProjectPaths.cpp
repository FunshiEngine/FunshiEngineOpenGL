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

// Windows.h ANTES del header propio (que incluye <string> y define _HAS_STD_BYTE),
// para que el define surta efecto antes de que la stdlib defina std::byte.
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "ProjectPaths.h"

#include <cctype>
#include <filesystem>

namespace {

std::string exeDir() {
#ifdef _WIN32
    char exe[4096] = {};
    const DWORD n = GetModuleFileNameA(nullptr, exe, sizeof(exe));
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

bool esReservado(const std::string& nombre) {
    static const char* reservados[] = {
        "Proyects", "Configuraciones", "Exportaciones",
        "Binarios", "Memory", "Interfaces", "Sonidos",
        "Configuracion.json", "imgui.ini"
    };
    for (const char* r : reservados) {
        if (nombre == r) return true;
    }
    if (nombre.rfind("src", 0) == 0 && nombre.size() > 3) return true;
    return false;
}

} // namespace

namespace ProjectPaths {

std::string directorioEjecutable() {
    return exeDir();
}

std::string directorioBase() {
    const std::string d = exeDir();
    return d.empty() ? "MotorGrafico" : d + "MotorGrafico";
}

std::string directorioProyects() {
    return directorioBase() + "/Proyects";
}

std::string directorioConfiguraciones() {
    return directorioBase() + "/Configuraciones";
}

std::string directorioExportaciones() {
    return directorioBase() + "/Exportaciones";
}

std::string directorioProyecto(const std::string& nombreProyecto) {
    const std::string nombre = nombreProyecto.empty() ? "Nuevo Proyecto" : nombreProyecto;
    return directorioProyects() + "/" + nombre;
}

std::string directorioMemory(const std::string& nombreProyecto) {
    return directorioProyecto(nombreProyecto) + "/Memory";
}

std::string directorioBinarios(const std::string& nombreProyecto) {
    return directorioMemory(nombreProyecto) + "/Binarios";
}

std::string directorioInterfaces(const std::string& nombreProyecto) {
    return directorioMemory(nombreProyecto) + "/Interfaces";
}

std::string nombreRaizSrc(const std::string& nombreProyecto) {
    const std::string nombre = nombreProyecto.empty() ? "Nuevo Proyecto" : nombreProyecto;
    return "src" + nombre;
}

std::string directorioSrc(const std::string& nombreProyecto) {
    return directorioProyecto(nombreProyecto) + "/" + nombreRaizSrc(nombreProyecto);
}

std::string directorioSonidos(const std::string& nombreProyecto) {
    return directorioSrc(nombreProyecto) + "/Sonidos";
}

std::string rutaConfiguracionGeneral() {
    return directorioConfiguraciones() + "/Configuracion.json";
}

std::string rutaConfiguracionProyecto(const std::string& nombreProyecto) {
    return directorioMemory(nombreProyecto) + "/ConfiguracionProyecto.json";
}

std::string rutaScenePrefijo(const std::string& nombreProyecto) {
    return directorioBinarios(nombreProyecto) + "/Scene";
}

std::string rutaSceneBBDD(const std::string& nombreProyecto) {
    return directorioBinarios(nombreProyecto) + "/SceneBBDDObjetos.txt";
}

std::string rutaSceneDir(const std::string& nombreProyecto) {
    return directorioBinarios(nombreProyecto) + "/Scene/";
}

std::string rutaImguiIni(const std::string& nombreProyecto) {
    return directorioMemory(nombreProyecto) + "/imgui.ini";
}

std::string directorioExportacion(const std::string& nombreExportacion) {
    return directorioExportaciones() + "/" + nombreExportacion;
}

bool esNombreValido(const std::string& nombre) {
    if (nombre.empty()) return false;
    for (char c : nombre) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|')
            return false;
    }
    return !esReservado(nombre);
}

} // namespace ProjectPaths