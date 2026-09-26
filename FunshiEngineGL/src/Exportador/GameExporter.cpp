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
#include "GameExporter.h"

#include <thread>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

GameExporter::GameExporter(const Config& cfg) : cfg_(cfg) {}

GameExporter::~GameExporter() {
    if (hiloExportacion_.joinable()) hiloExportacion_.join();
}

void GameExporter::iniciar() {
    terminado_ = false;
    hiloExportacion_ = std::thread(&GameExporter::runExportacion, this);
}

bool GameExporter::haTerminado() const noexcept {
    return terminado_;
}

void GameExporter::runExportacion() {
    log("Iniciando exportación de: " + cfg_.nombreProyectoExportado);
    progreso(0.05f, "Preparando directorio de build");

    // Directorio temporal de build
    std::string buildDir = cfg_.directorioSalida + "_build";
    std::error_code ec;
    fs::remove_all(buildDir, ec);
    fs::create_directories(buildDir, ec);

    if (!generarProyectoCMake(buildDir)) {
        finalizar(false, "Error generando CMake");
        return;
    }
    progreso(0.2f, "CMake generado");

    if (!compilarEngineRuntime(buildDir)) {
        finalizar(false, "Error compilando engine runtime");
        return;
    }
    progreso(0.5f, "Engine runtime compilado");

    if (!compilarScriptsUsuario(buildDir)) {
        finalizar(false, "Error compilando scripts de usuario");
        return;
    }
    progreso(0.7f, "Scripts compilados");

    if (!compilarJuego(buildDir)) {
        finalizar(false, "Error compilando el juego");
        return;
    }
    progreso(0.85f, "Juego compilado");

    if (!copiarAssetsYDependencias(buildDir)) {
        finalizar(false, "Error copiando assets/dependencias");
        return;
    }
    progreso(0.95f, "Assets copiados");

    if (!empaquetarDistribucion(buildDir)) {
        finalizar(false, "Error empaquetando distribución");
        return;
    }

    // Limpiar build temporal
    fs::remove_all(buildDir, ec);

    finalizar(true, "Exportación completada en: " + cfg_.directorioSalida);
}

bool GameExporter::generarProyectoCMake(const std::string& buildDir) {
    // Obtener ruta al source del engine
    std::string engineSrc = fs::absolute(fs::path(cfg_.proyectoOrigen).parent_path().parent_path()).string();

    // CMakeLists.txt principal del proyecto de exportación
    std::string cmakeContent = R"(
cmake_minimum_required(VERSION 3.15)
project(ExportedGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build engine as runtime-only (no editor/ImGui)
set(BUILD_RUNTIME ON CACHE BOOL "" FORCE)
set(FUNSHI_JAVA OFF CACHE BOOL "" FORCE)
set(USE_ASSIMP OFF CACHE BOOL "" FORCE)

# Include engine as subdirectory
add_subdirectory(")" + engineSrc + R"(" engine_build)

# User scripts
add_subdirectory(${CMAKE_SOURCE_DIR}/scripts)

# Game executable - entry point provided by user or generated
add_executable(${PROJECT_NAME} "")
target_link_libraries(${PROJECT_NAME} PRIVATE funshi_runtime)

# Install to distribution directory
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION .
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
)";

    // Toolchain para cross-compile Windows
    if (cfg_.plataforma == Plataforma::Windows) {
        cmakeContent += R"(
# Toolchain MinGW para Windows
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX ".dll.a")
set(CMAKE_FIND_LIBRARY_CUSTOM_PATH_SUFFIXES "/x86_64-w64-mingw32")
)";
    }

    // Guardar CMakeLists.txt principal
    std::ofstream cmakeFile(buildDir + "/CMakeLists.txt");
    if (!cmakeFile) return false;
    cmakeFile << cmakeContent;
    cmakeFile.close();

    // Generar CMakeLists.txt para scripts de usuario
    std::string scriptsDir = cfg_.proyectoOrigen + "/scripts";
    if (fs::exists(scriptsDir)) {
        std::string scriptsCMake = "cmake_minimum_required(VERSION 3.15)\n";
        scriptsCMake += "project(UserScripts LANGUAGES CXX)\n";
        scriptsCMake += "set(CMAKE_CXX_STANDARD 17)\n";
        scriptsCMake += "file(GLOB SCRIPT_SOURCES *.cpp)\n";
        scriptsCMake += "foreach(script ${SCRIPT_SOURCES})\n";
        scriptsCMake += "  get_filename_component(name ${script} NAME_WE)\n";
        scriptsCMake += "  add_library(${name} MODULE ${script})\n";
        scriptsCMake += "  target_include_directories(${name} PRIVATE " + engineSrc + "/src)\n";
        scriptsCMake += "  target_link_libraries(${name} PRIVATE funshi_runtime)\n";
        scriptsCMake += "endforeach()\n";
        std::error_code ec;
        fs::create_directories(buildDir + "/scripts", ec);
        std::ofstream scm(buildDir + "/scripts/CMakeLists.txt");
        scm << scriptsCMake;
        scm.close();
    }

    return true;
}

bool GameExporter::compilarEngineRuntime(const std::string& buildDir) {
    // Compilar solo el target funshi_runtime
    std::string cmd = "cd " + buildDir + " && cmake .. && cmake --build . --target funshi_runtime -j4";
    int result = std::system(cmd.c_str());
    return result == 0;
}

bool GameExporter::compilarScriptsUsuario(const std::string& buildDir) {
    std::string scriptsDir = buildDir + "/scripts";
    if (!fs::exists(scriptsDir)) return true; // sin scripts

    // Configurar y compilar scripts
    std::string cmd = "cd " + scriptsDir + " && cmake .. && make -j4";
    int result = std::system(cmd.c_str());
    return result == 0;
}

bool GameExporter::compilarJuego(const std::string& buildDir) {
    std::string cmd = "cd " + buildDir + " && cmake --build . --target ExportedGame -j4";
    int result = std::system(cmd.c_str());
    return result == 0;
}

bool GameExporter::copiarAssetsYDependencias(const std::string& buildDir) {
    // Copiar Memory/, Sonidos/, ConfiguracionProyecto.json
    std::vector<std::pair<std::string, std::string>> copiar = {
        {cfg_.proyectoOrigen + "/Memory", cfg_.directorioSalida + "/Data/Memory"},
        {cfg_.proyectoOrigen + "/Sonidos", cfg_.directorioSalida + "/Data/Sonidos"},
        {cfg_.proyectoOrigen + "/ConfiguracionProyecto.json", cfg_.directorioSalida + "/Data/ConfiguracionProyecto.json"}
    };

    for (auto& [src, dst] : copiar) {
        if (fs::exists(src)) {
            std::error_code ec;
            fs::create_directories(fs::path(dst).parent_path(), ec);
            fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
            if (ec) return false;
        }
    }

    // Copiar dependencias runtime (.so/.dll) al directorio lib/
    copiarDependenciasRuntime(buildDir);

    return true;
}

bool GameExporter::copiarDependenciasRuntime(const std::string& buildDir) {
    std::string libDir = cfg_.directorioSalida + "/lib";
    std::error_code ec;
    fs::create_directories(libDir, ec);

    // Lista de librerías a buscar y copiar
    std::vector<std::string> libs;
    if (cfg_.plataforma == Plataforma::Windows) {
        libs = {
            "libBulletDynamics.dll", "libBulletCollision.dll", "libLinearMath.dll",
            "glfw3.dll", "miniaudio.dll", "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll"
        };
    } else {
        libs = {
            "libBulletDynamics.so", "libBulletCollision.so", "libLinearMath.so",
            "libglfw.so", "libminiaudio.so"
        };
    }

    // Buscar en rutas típicas del sistema y build
    std::vector<std::string> searchPaths = {
        "/usr/lib/x86_64-linux-gnu/",
        "/usr/local/lib/",
        buildDir + "/engine_build/",
        buildDir + "/engine_build/src/",
        buildDir + "/"
    };

    for (const auto& lib : libs) {
        for (const auto& path : searchPaths) {
            std::string src = path + lib;
            if (fs::exists(src)) {
                fs::copy_file(src, libDir + "/" + lib, fs::copy_options::overwrite_existing, ec);
                break;
            }
        }
    }
    return true;
}

bool GameExporter::empaquetarDistribucion(const std::string& buildDir) {
    std::string exeName = obtenerExtensionEjecutable();
    std::string exeSrc = buildDir + "/" + exeName;
    std::string exeDst = cfg_.directorioSalida + "/" + exeName;

    std::error_code ec;
    fs::create_directories(cfg_.directorioSalida, ec);
    if (!fs::exists(exeSrc)) {
        log("Ejecutable no encontrado en: " + exeSrc);
        return false;
    }
    fs::copy_file(exeSrc, exeDst, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

std::string GameExporter::obtenerExtensionEjecutable() const {
    if (cfg_.plataforma == Plataforma::Windows) return cfg_.nombreEjecutable + ".exe";
    return cfg_.nombreEjecutable;
}

std::string GameExporter::obtenerToolchainCMake() const {
    return "";
}

void GameExporter::log(const std::string& msg) {
    if (cfg_.onLog) cfg_.onLog(msg);
    std::cout << "[Exporter] " << msg << std::endl;
}

void GameExporter::progreso(float p, const std::string& etapa) {
    if (cfg_.onProgreso) cfg_.onProgreso(p, etapa);
}

void GameExporter::finalizar(bool ok, const std::string& msg) {
    terminado_ = true;
    exito_ = ok;
    mensajeFinal_ = msg;
    if (cfg_.onFinalizado) cfg_.onFinalizado(ok, msg);
}