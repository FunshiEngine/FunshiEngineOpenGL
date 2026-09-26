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

// Windows.h ANTES del header propio y de la stdlib, para que
// _HAS_STD_BYTE=0 surta efecto antes de que la stdlib defina std::byte.
#if defined(_WIN32)
#define _HAS_STD_BYTE 0
#include <windows.h>
#define FUNSHI_DLOPEN(name) LoadLibraryA((name).c_str())
#define FUNSHI_DLSYM(handle, symbol) GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol)
#define FUNSHI_DLOPENCERRAR(handle) FreeLibrary(reinterpret_cast<HMODULE>(handle))
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "dll"
#elif defined(__APPLE__)
#include <dlfcn.h>
#define FUNSHI_DLOPEN(name) dlopen((name).c_str(), RTLD_NOW | RTLD_LOCAL)
#define FUNSHI_DLSYM(handle, symbol) dlsym(handle, symbol)
#define FUNSHI_DLOPENCERRAR(handle) dlclose(handle)
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "dylib"
#else
#include <dlfcn.h>
#include <unistd.h>
#define FUNSHI_DLOPEN(name) dlopen((name).c_str(), RTLD_NOW | RTLD_LOCAL)
#define FUNSHI_DLSYM(handle, symbol) dlsym(handle, symbol)
#define FUNSHI_DLOPENCERRAR(handle) dlclose(handle)
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "so"
#endif

#include "BackendCpp.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "../ScriptGameObject.h"
#include "../IScriptBehaviour.h"

#ifndef FUNSHI_CXX_COMPILER
#define FUNSHI_CXX_COMPILER "g++"
#endif
#ifndef FUNSHI_SRC_DIR
#define FUNSHI_SRC_DIR ""
#endif

namespace {
const char* nombreFabrica() { return FUNSHI_SYM_CREAR; }

std::string compilador() {
    const char* env = std::getenv("FUNSHI_CXX");
    if (env && *env) return env;
    return FUNSHI_CXX_COMPILER;
}

// Escapa una ruta para pasarla como argumento de linea de comandos.
std::string escapar(const std::string& ruta) {
    std::string resultado;
    for (char c : ruta) {
        if (c == '"') {
            resultado += "\\\"";
        } else if (c == '\\') {
            resultado += "\\\\";
        } else {
            resultado += c;
        }
    }
    return resultado;
}

// Directorio con las cabeceras del motor para que el script pueda incluir
// ScriptGameObject.h. Prioridad: variable de entorno FUNSHI_SRC_DIR; si no,
// la ruta horneada en el build (el repo en un build dev, o la carpeta de
// instalacion en un build de CI/instalador). Vacia si no hay cabeceras.
std::string directorioSrcMotor() {
    const char* env = std::getenv("FUNSHI_SRC_DIR");
    if (env && *env) return env;
    return FUNSHI_SRC_DIR;
}

std::string directorioCache() {
    std::error_code ec;
    std::filesystem::path base;
#if defined(_WIN32)
    const char* tmp = std::getenv("TEMP");
    if (tmp && *tmp) base = tmp;
    else base = ".";
#else
    const char* tmp = std::getenv("TMPDIR");
    if (tmp && *tmp) base = tmp;
    else base = "/tmp";
#endif
    return (base / "funshi_scripts").string();
}

std::string mtimeDe(const std::string& ruta) {
    std::error_code ec;
    auto t = std::filesystem::last_write_time(ruta, ec);
    if (ec) return "";
    return std::to_string(t.time_since_epoch().count());
}
} // namespace

const char* BackendCpp::lenguaje() const { return "cpp"; }

std::string BackendCpp::compiladorRuta() {
    const char* env = std::getenv("FUNSHI_CXX");
    return (env && *env) ? env : FUNSHI_CXX_COMPILER;
}

std::string BackendCpp::cacheDir() { return directorioCache(); }

std::string BackendCpp::artefacto(const std::string& fuente) {
    std::size_t hash =
        std::hash<std::string>{}(std::filesystem::weakly_canonical(fuente).string());
    return (std::filesystem::path(directorioCache()) /
            ("script_" + std::to_string(hash) + "." + FUNSHI_ARTEFACTO_EXT))
        .string();
}

bool BackendCpp::compilarYCargar(const std::string& fuente,
                                 const std::string& nombreClase,
                                 ComportamientoCargado& salida,
                                 std::string& error) {
    std::error_code ec;
    if (!std::filesystem::exists(fuente, ec)) {
        error = "El fuente del script no existe:\n" + fuente;
        return false;
    }

    try {
        std::filesystem::create_directories(directorioCache(), ec);
    } catch (...) {
    }

    const std::string artefactoPath = artefacto(fuente);
    const std::string mtime = mtimeDe(fuente);

    // Compilar solo si cambio el fuente (hot reload; recompila en play mode).
    bool hayQueRecompilar = !std::filesystem::exists(artefactoPath, ec) ||
                            salida.mtimeFuente != mtime;
    if (hayQueRecompilar) {
        // -I al dir de cabeceras del motor + fuente a compilar, ambos
        // entrecomillados (rutas con espacios). En Windows ademas del /I no
        // existia el include path, asi que el script nunca encontraba las
        // cabeceras del SDK: se arregla aqui.
        std::string logic;
        const std::string dirSrc = directorioSrcMotor();
        if (!dirSrc.empty()) {
#if defined(_WIN32)
            logic = "/I\"" + escapar(dirSrc) + "\" ";
#else
            logic = "-I\"" + escapar(dirSrc) + "\" ";
#endif
        }
        logic += escapar(fuente);
        const std::string logPath =
            (std::filesystem::path(directorioCache()) / "compilar.log")
                .string();
        std::string cmd;
#if defined(_WIN32)
        cmd = compilador() +
              " /nologo /LD /std:c++17 /O2 /DFUNSHI_NOMBRE_CLASE=" +
              nombreClase + " " + logic + " /Fo\"" + directorioCache() +
              "\\\" /Fe" + escapar(artefactoPath) + " > \"" + logPath +
              "\" 2>&1";
#else
        cmd = compilador() +
              " -std=c++17 -shared -fPIC -O2 -DFUNSHI_NOMBRE_CLASE=" +
              nombreClase + " " + logic + " -o " + escapar(artefactoPath) +
              " > " + logPath + " 2>&1";
#endif
        int rc = std::system(cmd.c_str());
        if (rc != 0) {
            std::ifstream log(logPath);
            std::string contenido((std::istreambuf_iterator<char>(log)),
                                  std::istreambuf_iterator<char>());
            error = "Error al compilar el script C++:\n" + contenido;
            return false;
        }
    }

    void* manejador = FUNSHI_DLOPEN(artefactoPath);
    if (!manejador) {
        std::string detalle;
#if !defined(_WIN32)
        const char* d = dlerror();
        if (d) detalle = d;
#endif
        error = "No se pudo cargar el artefacto del script:\n" + artefactoPath;
        if (!detalle.empty()) error += "\n" + detalle;
        return false;
    }

    using Fabrica =
        IScriptBehaviour* (*)(const MotorScript::ApiScriptGameObject*);
    Fabrica fabrica =
        reinterpret_cast<Fabrica>(FUNSHI_DLSYM(manejador, nombreFabrica()));
    if (!fabrica) {
        error = "El .so no exporta 'FUNSHI_CREAR_COMPORTAMIENTO'. ¿El fuente "
                "deriva de IScriptBehaviour y usa el template del motor?";
        FUNSHI_DLOPENCERRAR(manejador);
        return false;
    }

    IScriptBehaviour* instancia = fabrica(MotorScript::tablaApi());
    if (!instancia) {
        error = "La fabrica devolvio un comportamiento nulo.";
        FUNSHI_DLOPENCERRAR(manejador);
        return false;
    }

    // La fábrica recibe la tabla pero el template no la guarda: el motor la
    // inyecta acá para que `this->api` quede siempre disponible en el script.
    instancia->conectarApi(MotorScript::tablaApi());

    salida.fuente = fuente;
    salida.lenguaje = "cpp";
    salida.artefacto = artefactoPath;
    salida.manejador = manejador;
    salida.instancia = instancia;
    salida.campos = instancia->camposReflejados();
    salida.mtimeFuente = mtime;
    salida.cargado = true;
    error.clear();
    return true;
}

void BackendCpp::descargar(ComportamientoCargado& comportamiento) {
    descargar(comportamiento, nullptr);
}

void BackendCpp::descargar(ComportamientoCargado& comportamiento,
                           GameObject* owner) {
    if (!comportamiento.cargado) return;
    if (owner && comportamiento.instancia) {
        reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)
            ->onStop(owner);
    }
    if (comportamiento.instancia) {
        delete reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia);
    }
    void* manejador = comportamiento.manejador;
    // IMPORTANTE: destruir el arbol de campos (std::function con target dentro
    // del .so) ANTES de dlclose; si no, los destructores saltan a codigo
    // descargado y segfaultean.
    comportamiento = ComportamientoCargado{};
    FUNSHI_DLOPENCERRAR(manejador);
}

void BackendCpp::llamarInicio(ComportamientoCargado& comportamiento,
                              GameObject* owner) {
    if (!comportamiento.valido()) return;
    reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)->onStart(owner);
}

void BackendCpp::llamarActualizar(ComportamientoCargado& comportamiento,
                                  GameObject* owner, float deltaTime) {
    if (!comportamiento.valido()) return;
    reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)
        ->onUpdate(owner, deltaTime);
}

void BackendCpp::llamarDetener(ComportamientoCargado& comportamiento,
                               GameObject* owner) {
    if (!comportamiento.valido()) return;
    reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)->onStop(owner);
}