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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;

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

// ---------------------------------------------------------------------------
// Resolucion de la raiz de datos (vease la nota de ProjectPaths.h)
//
// La raiz MotorGrafico vivio historicamente junto al ejecutable. Con el
// instalador de Windows eso la deja en "C:\Program Files\FunshiEngineGL\", que
// el proceso NO puede escribir sin elevar: el ejecutable no lleva manifest
// requestedExecutionLevel, asi que corre como usuario normal mientras la
// carpeta quedo creada por el instalador (que si es admin) heredando los ACL
// de Program Files. El motor leia bien y escribia mal, en silencio, porque
// ningun llamador comprobaba el exito de las escrituras.
//
// La resolucion prueba de verdad si se puede escribir (no basta con que el
// directorio exista y sea legible) y, si no se puede, cae a una carpeta de
// datos por usuario. Se cachea: es una decision de proceso, no por llamada.
// ---------------------------------------------------------------------------

// Carpeta de datos por usuario: %APPDATA%\FunshiEngineGL en Windows;
// $XDG_DATA_HOME/FunshiEngineGL con respaldo a ~/.local/share en Linux/macOS.
// Vacia si el proceso no tiene carpeta de usuario definida.
std::string directorioDatosUsuario() {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata && *appdata) return std::string(appdata) + "/FunshiEngineGL";
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && *xdg) return std::string(xdg) + "/FunshiEngineGL";
    const char* home = std::getenv("HOME");
    if (home && *home) return std::string(home) + "/.local/share/FunshiEngineGL";
#endif
    return std::string();
}

// Prueba de escritura REAL: crear el directorio no demuestra nada, porque en
// Program Files la carpeta existe y es legible y aun asi el proceso no puede
// crear archivos dentro. La unica comprobacion fiel es abrir un archivo y
// cerrarlo; se borra acto seguido para no dejar rastro.
bool esEscribible(const std::string& dir) {
    std::error_code ec;
    fs::create_directories(fs::path(dir), ec);
    if (ec) return false;

    const fs::path prueba = fs::path(dir) / ".funshi_prueba_escritura";
    {
        std::ofstream salida(prueba, std::ios::binary | std::ios::trunc);
        if (!salida.is_open()) return false;
    }
    fs::remove(prueba, ec);
    return true;
}

// Ubicacion historica, la de siempre: junto al ejecutable. Se calcula aparte
// porque hace falta conocerla aunque NO sea la elegida (para avisar al usuario
// de donde quedan sus datos y para la migracion).
std::string baseOriginal() {
    const std::string d = exeDir();
    return d.empty() ? std::string("MotorGrafico") : d + "MotorGrafico";
}

// Raiz efectiva. Cacheada con un static de ambito de funcion: son las mismas
// rutas para todo el proceso y la comprobacion de escritura implica tocar el
// disco, asi que no puede repetirse en cada llamada.
std::string baseResuelta() {
    static const std::string base = [] {
        const std::string juntoAlExe = baseOriginal();
        if (esEscribible(juntoAlExe)) return juntoAlExe;

        const std::string usuario = directorioDatosUsuario();
        if (!usuario.empty()) {
            const std::string destino = usuario + "/MotorGrafico";
            if (esEscribible(destino)) return destino;
        }
        // Sin carpeta de usuario utilizable: se conserva la ruta historica para
        // no dejar al motor sin rutas. Las escrituras fallaran y se reportan
        // como error en vez de desaparecer en silencio.
        return juntoAlExe;
    }();
    return base;
}

} // namespace

namespace ProjectPaths {

std::string directorioEjecutable() {
    return exeDir();
}

std::string directorioBase() {
    return baseResuelta();
}

std::string directorioBaseOriginal() {
    return baseOriginal();
}

bool datosEnRutaDeUsuario() {
    return baseResuelta() != baseOriginal();
}

// Copia el arbol `origen` dentro de `destino`, que debe estar vacio. Se separa
// de la migracion para poder testearla en CI sin depender de que la carpeta del
// ejecutable sea no escribible, cosa que en un runner nunca ocurre. Devuelve
// false dejando `error` vacio cuando no habia nada que hacer (origen que no es
// directorio, o destino ya poblado), y con el motivo en `error` cuando la copia
// fallo de verdad.
bool copiarArbolSiDestinoVacio(const std::string& origen,
                               const std::string& destino,
                               std::string& error) {
    error.clear();

    const fs::path desde(origen);
    const fs::path hacia(destino);

    std::error_code ec;
    if (!fs::is_directory(desde, ec)) {
        error = "El origen no es un directorio: " + origen;
        return false;
    }
    // Destino ya poblado: no se toca. Es la garantia de que la migracion nunca
    // pisa datos que el usuario ya tenga en la carpeta de destino.
    if (fs::is_directory(hacia, ec) && !fs::is_empty(hacia, ec)) {
        error.clear();
        return false;
    }

    // ec puede venir sucio del is_directory de arriba (el destino todavia no
    // existe, que es el caso normal), y create_directories no debe heredar ese
    // error: se limpia antes de operar.
    ec.clear();
    fs::create_directories(hacia, ec);
    if (ec) {
        error = "No se pudo preparar la carpeta de destino: " + destino;
        return false;
    }

    // `recursive` y `copy_symlinks`, sin `overwrite_existing`. El flag sobra por
    // semantica: hace un rato se garantizo que el destino esta vacio, y
    // sobrescribir es justo lo que esta operacion no debe hacer nunca. Ademas no
    // es inofensivo: en libstdc++, combinar `overwrite_existing` con
    // `recursive` hace que `copy` falle con "Invalid argument" y no copie nada
    // cuando el directorio destino todavia NO existe. Hoy no se llega a ese
    // caso porque se crea arriba, asi que quitarlo es lo que hace que esta
    // funcion no dependa del orden create->copy.
    const fs::copy_options opciones = fs::copy_options::recursive |
                                      fs::copy_options::copy_symlinks;
    ec.clear();
    fs::copy(desde, hacia, opciones, ec);
    if (ec) {
        error = "Se copiaron los datos desde '" + origen + "' con errores: " +
                ec.message();
        return false;
    }
    return true;
}

bool migrarDatosDesdeRutaOriginal(std::string& mensaje) {
    // Copia unica de los datos que hubieran quedado en la ruta historica (por
    // ejemplo de una ejecucion elevada del motor instalado) a la carpeta de
    // datos por usuario, para que no se pierdan al aplicar el fallback. Solo
    // actsua si la carpeta destino esta VACIA: nunca pisa datos ya existentes.
    //
    // Las rutas de assets de la escena se guardan RELATIVAS a la raiz de assets
    // del proyecto (EditorConfig::relativizarRuta) y se vuelven absolutas al
    // cargar, contra la raiz que se recalcula en cada arranque, asi que los
    // proyectos copiados siguen resolviendo sus assets sin tocar nada. La
    // excepcion son las escenas en formato legacy, que guardaban la ruta
    // absoluta intacta: si hay de esas, habra que reasignar sus assets a mano.
    if (!datosEnRutaDeUsuario()) {
        mensaje.clear();
        return false;
    }

    const std::string origen = baseOriginal();
    const std::string destino = baseResuelta();

    // La copia vive en copiarArbolSiDestinoVacio, incluida su garantia de no
    // pisar un destino poblado. `error` queda vacio cuando simplemente no
    // habia nada que migrar, que no es un fallo y no se le avisa al usuario.
    std::string error;
    if (!copiarArbolSiDestinoVacio(origen, destino, error)) {
        mensaje = error;
        return false;
    }

    std::cout << "ProjectPaths: datos migrados desde '" << origen
              << "' a '" << destino << "'. Las rutas relativas de los "
                 "assets se resuelven solas; si un proyecto usa escenas en formato "
                 "legacy (con rutas absolutas), reasigna sus assets a mano."
              << std::endl;

    mensaje = "Se migraron los datos de '" + origen + "' a '" + destino +
              "'. Tus proyectos siguen ahí; si alguno no encuentra sus assets, "
              "reasígnalos desde el explorador.";
    return true;
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