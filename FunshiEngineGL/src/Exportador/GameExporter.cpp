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

#include "../Configuracion/ProjectPaths.h"
#include "../FileManager/Proceso.h"
#include "ConfigJuegoExportado.h"
#include "RaizEngine.h"
#include "RutasExportacion.h"

#include <thread>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

namespace {

// Raiz de los fuentes del engine para el add_subdirectory del proyecto de
// exportacion: la horneada en el build (FUNSHI_ENGINE_SOURCE_DIR) y, si no
// esta o ya no existe (binario movido de maquina), subiendo desde la carpeta
// del ejecutable hasta un CMakeLists.txt con src/Runtime/main_juego.cpp.
std::string raizDelEngine() {
#ifdef FUNSHI_ENGINE_SOURCE_DIR
    {
        std::string horneada(FUNSHI_ENGINE_SOURCE_DIR);
        std::error_code ec;
        if (fs::exists(fs::path(horneada) / "CMakeLists.txt", ec)) {
            std::replace(horneada.begin(), horneada.end(), '\\', '/');
            return horneada;
        }
    }
#endif
    std::string encontrada = RaizEngine::buscar(
        ProjectPaths::directorioEjecutable(),
        [](const std::string& ruta) {
            std::error_code ec;
            return fs::exists(ruta, ec);
        });
    std::replace(encontrada.begin(), encontrada.end(), '\\', '/');
    return encontrada;
}

} // namespace

// Sustituye TODAS las apariciones de `busqueda` en `texto` (marcadores del
// template de CMake).
void reemplazarTodo(std::string& texto, const std::string& busqueda,
                    const std::string& reemplazo) {
    if (busqueda.empty()) return;
    std::string::size_type pos = 0;
    while ((pos = texto.find(busqueda, pos)) != std::string::npos) {
        texto.replace(pos, busqueda.size(), reemplazo);
        pos += reemplazo.size();
    }
}

GameExporter::GameExporter(const Config& cfg) : cfg_(cfg) {
    // La disposicion del proyecto se resuelve por NOMBRE (RutasExportacion se
    // apoya en ProjectPaths). Si quien arma la configuracion solo paso la ruta,
    // el nombre es el de su carpeta; si no paso ruta pero si nombre, la ruta se
    // deduce de el. Asi ninguna de las dos se deja obligatoria.
    if (cfg_.nombreProyecto.empty() && !cfg_.proyectoOrigen.empty())
        cfg_.nombreProyecto = nombreProyectoDesdeRuta(cfg_.proyectoOrigen);
    if (cfg_.proyectoOrigen.empty())
        cfg_.proyectoOrigen = EditorConfig::directorioProyecto(cfg_.nombreProyecto);
}

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
    progreso(0.55f, "Runtime del motor compilado");

    // Los scripts C++ del proyecto viajan como fuentes: BackendCpp los compila
    // en la primera ejecucion del juego, con la clase que declara la escena
    // (el nombre de clase puede diferir del del archivo, asi que la
    // exportacion no los precompila).
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
    // Raiz de los fuentes del engine: horneada en el build del editor; si el
    // binario no la trae (o el checkout cambio de lugar), se busca subiendo
    // desde la carpeta del ejecutable.
    const std::string engineSrc = raizDelEngine();
    if (engineSrc.empty()) {
        log("No se encontro la raiz de los fuentes del engine "
            "(CMakeLists.txt + src/main.cpp) junto al ejecutable");
        return false;
    }

    // CMakeLists.txt principal del proyecto de exportacion. El binario del
    // juego comparte el punto de entrada del editor (main.cpp): arranca con
    // JuegoExportado.json junto al ejecutable y no pasa por el menu.
    std::string cmakeContent = R"(
cmake_minimum_required(VERSION 3.15)
project(ExportedGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sin build type la raiz del engine cae en Debug (compilacion lenta): la
# exportacion se compila en Release salvo que se pida otra cosa.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Mismo CRT que el build del editor: el juego y los scripts C++ que compila en
# runtime tienen que compartir el heap (/MD), como en el propio motor.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Runtime-only: la raiz del engine construye solo su libreria (sin ejecutable
# de editor ni pruebas). FUNSHI_JAVA ON incluye soporte de scripts Java via JNI; USE_ASSIMP
# queda ON porque el motor incluye AssimpMeshLoader sin guards.
set(BUILD_RUNTIME ON CACHE BOOL "" FORCE)
set(FUNSHI_JAVA ON CACHE BOOL "" FORCE)

add_subdirectory("@ENGINE@" engine_build)

# El juego usa su propio punto de entrada (main_juego.cpp): NO incluye codigo
# del editor (GUIManager, MenuGUI, paneles, atajos de editor).
# JuegoExportado.json junto al binario fija proyecto y modo juego.
add_executable(@EXE@ "@ENGINE@/src/Runtime/main_juego.cpp")
target_link_libraries(@EXE@ PRIVATE funshi_engine)
if(WIN32)
    set_target_properties(@EXE@ PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(@EXE@ PRIVATE shell32)
endif()
# El empaquetado deja las librerias de sistema en lib/ junto al binario; en
# Linux el binario las resuelve via $ORIGIN/lib (BUILD_RPATH).
set_target_properties(@EXE@ PROPERTIES BUILD_RPATH "$ORIGIN/lib")
)";
    reemplazarTodo(cmakeContent, "@ENGINE@", engineSrc);
    reemplazarTodo(cmakeContent, "@EXE@", cfg_.nombreEjecutable);

    // Guardar CMakeLists.txt principal
    std::ofstream cmakeFile(buildDir + "/CMakeLists.txt");
    if (!cmakeFile) return false;
    cmakeFile << cmakeContent;
    cmakeFile.close();

    // Cross-compile a Windows: el toolchain va en un archivo aparte que se
    // pasa con -DCMAKE_TOOLCHAIN_FILE en el configure. Declarar
    // CMAKE_SYSTEM_NAME dentro del CMakeLists (como hacia antes este
    // exportador) llega despues de project() y CMake no entra en modo cross.
    if (cfg_.plataforma == Plataforma::Windows) {
        static const char* kToolchain = R"(
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX ".dll.a")
set(CMAKE_FIND_LIBRARY_CUSTOM_PATH_SUFFIXES "/x86_64-w64-mingw32")
)";
        std::ofstream toolchain(buildDir + "/toolchain-mingw.cmake");
        if (!toolchain) return false;
        toolchain << kToolchain;
        toolchain.close();
    }

    // Los scripts C++ no llevan subproyecto de compilacion: BackendCpp los
    // compila en la primera ejecucion del juego con la clase que declara la
    // escena, y las cabeceras del motor viajan a <salida>/include (ver
    // empaquetarDistribucion).
    return true;
}

bool GameExporter::compilarEngineRuntime(const std::string& buildDir) {
    // Compilar solo la libreria del motor. Sin shell (H-3 nivel 2): el
    // `cd X && ...` viejo pasaba la linea entera por cmd.exe/make; ahora el
    // directorio de trabajo es el parametro cwd del runner y cada token va
    // como argv. El log vacio = el hijo hereda la salida del motor (igual
    // que con std::system: la progresion del export queda en el log propio).
    // `cmake .` (fuente = el propio buildDir, donde el generador dejo el
    // CMakeLists): el viejo `cmake ..` apuntaba al padre del buildDir, que no
    // contiene CMakeLists.txt y el configure fallaba siempre. El toolchain
    // MinGW (si la plataforma objetivo es Windows) se pasa en este configure:
    // es lo unico que hace que CMake cruce compilers.
    std::vector<std::string> argvConfigure = {"cmake", "."};
    if (cfg_.plataforma == Plataforma::Windows)
        argvConfigure.push_back("-DCMAKE_TOOLCHAIN_FILE=" + buildDir +
                                "/toolchain-mingw.cmake");
    if (Proceso::ejecutar(argvConfigure, std::string(), buildDir) != 0)
        return false;
    return Proceso::ejecutar(
               {"cmake", "--build", ".", "--target", "funshi_engine", "-j4"},
               std::string(), buildDir) == 0;
}

bool GameExporter::compilarJuego(const std::string& buildDir) {
    // El objetivo es el binario con el nombre del ejecutable que pidio el
    // usuario (el CMake generado lo crea asi); antes era "ExportedGame", el
    // nombre del project() temporal.
    return Proceso::ejecutar(
               {"cmake", "--build", ".", "--target", cfg_.nombreEjecutable,
                "-j4"},
               std::string(), buildDir) == 0;
}

bool GameExporter::copiarAssetsYDependencias(const std::string& buildDir) {
    // En lugar de copiar todo el proyecto y luego limpiar, copiamos SOLO lo
    // necesario para el juego: escenas, binarios compilados de scripts y assets.
    // NO se copia la estructura MotorGrafico/Proyects: el juego exportado tiene
    // su propia estructura simplificada.

    std::error_code ec;

    // 1. Copiar la base de datos de escenas (.db con GameObjects, componentes, jerarquía)
    const std::string sceneBBDDOrigen = EditorConfig::rutaSceneBBDD(cfg_.nombreProyecto);
    const std::string sceneBBDDDestino = cfg_.directorioSalida + "/scenes/Scene.db";
    
    if (!fs::exists(sceneBBDDOrigen, ec)) {
        log("No existe la base de datos de escenas: " + sceneBBDDOrigen);
        return false;
    }
    
    fs::create_directories(fs::path(sceneBBDDDestino).parent_path(), ec);
    fs::copy_file(sceneBBDDOrigen, sceneBBDDDestino, 
                  fs::copy_options::overwrite_existing, ec);
    if (ec) {
        log("Error copiando Scene.db: " + ec.message());
        return false;
    }

    // 2. Copiar el directorio de escenas (assets de las escenas: modelos, texturas, etc.)
    const std::string sceneDirOrigen = EditorConfig::rutaSceneDir(cfg_.nombreProyecto);
    const std::string sceneDirDestino = cfg_.directorioSalida + "/scenes/SceneDir";
    
    if (fs::exists(sceneDirOrigen, ec)) {
        fs::create_directories(fs::path(sceneDirDestino).parent_path(), ec);
        fs::copy(sceneDirOrigen, sceneDirDestino,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            log("Error copiando SceneDir: " + ec.message());
            return false;
        }
    }

    // 3. Copiar los scripts del usuario (C++ y Java) - se compilan en runtime
    const std::string scriptsOrigen = EditorConfig::directorioScripts(cfg_.nombreProyecto);
    const std::string scriptsDestino = cfg_.directorioSalida + "/scripts";
    
    if (fs::exists(scriptsOrigen, ec)) {
        fs::create_directories(scriptsDestino, ec);
        fs::copy(scriptsOrigen, scriptsDestino,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            log("Error copiando scripts: " + ec.message());
            return false;
        }
    }

    // 4. Copiar binarios compilados de scripts (si existen) - cachés de BackendCpp/BackendJava
    const std::string binariosOrigen = EditorConfig::directorioMemory(cfg_.nombreProyecto) + "/Binarios";
    const std::string binariosDestino = cfg_.directorioSalida + "/cache/scripts";
    
    if (fs::exists(binariosOrigen, ec)) {
        fs::create_directories(binariosDestino, ec);
        fs::copy(binariosOrigen, binariosDestino,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            log("Error copiando binarios de scripts: " + ec.message());
            // No fatal: los scripts se recompilan en la primera ejecución
        }
    }

    // 5. Copiar contenido de assets del proyecto (srcTest/Assets/ -> raíz del juego)
    // Las rutas en Scene.db son relativas a Assets/ (ej: "Assets/Modelos/Cube.obj")
    // Para que funcionen en el juego exportado, Assets/ debe estar en la raíz
    const std::string srcProyecto = EditorConfig::directorioSrc(cfg_.nombreProyecto);
    
    if (fs::exists(srcProyecto, ec)) {
        // Copiar el contenido completo de srcTest/ a la raíz del juego exportado
        // Esto incluye Assets/ y cualquier otra carpeta que tenga el proyecto
        for (const auto& entry : fs::directory_iterator(srcProyecto, ec)) {
            if (ec) continue;
            const std::string destino = cfg_.directorioSalida + "/" + entry.path().filename().string();
            fs::copy(entry.path(), destino,
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                     ec);
            if (ec) {
                log("Error copiando " + entry.path().string() + ": " + ec.message());
                return false;
            }
        }
        log("Assets del proyecto copiados a la raíz");
    }

    // 6. Dependencias de sistema (.so/.dll) que el binario necesita para arrancar
    copiarDependenciasRuntime(buildDir);

    log("Assets y dependencias copiados exitosamente");
    return true;
}

bool GameExporter::copiarDependenciasRuntime(const std::string& buildDir) {
    // Windows resuelve las DLL junto al .exe (el loader no busca en lib/);
    // Linux lleva las .so a lib/ y el binario las encuentra via $ORIGIN/lib
    // (BUILD_RPATH del CMake generado).
    const std::string libDir = (cfg_.plataforma == Plataforma::Windows)
                                   ? cfg_.directorioSalida
                                   : cfg_.directorioSalida + "/lib";
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
    std::error_code ec;
    fs::create_directories(cfg_.directorioSalida, ec);

    // Ejecutable del juego.
    const std::string exeName = obtenerExtensionEjecutable();
    const std::string exeSrc = buildDir + "/" + exeName;
    const std::string exeDst = cfg_.directorioSalida + "/" + exeName;
    if (!fs::exists(exeSrc, ec)) {
        log("Ejecutable no encontrado en: " + exeSrc);
        return false;
    }
    fs::copy_file(exeSrc, exeDst, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        log("Error copiando el ejecutable: " + ec.message());
        return false;
    }

    // Cabeceras que BackendCpp necesita al compilar los scripts C++ en la
    // primera ejecucion del juego: funshi_cabeceras_script las deja en
    // <build>/include y el valor horneado FUNSHI_SRC_DIR="include" las busca
    // junto al binario.
    const std::string includeOrigen = buildDir + "/include";
    if (fs::exists(includeOrigen, ec)) {
        fs::copy(includeOrigen, cfg_.directorioSalida + "/include",
                 fs::copy_options::recursive |
                     fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            log("Error copiando las cabeceras de script: " + ec.message());
            return false;
        }
    }

    // Manifiesto de arranque: main_juego.cpp lo lee junto al binario y arranca
    // directo en modo juego (sin editor) sobre el proyecto exportado.
    if (!ConfigJuegoExportado::escribir(
            ConfigJuegoExportado::rutaPorDefecto(cfg_.directorioSalida),
            cfg_.nombreProyecto, true)) {
        log("No se pudo escribir JuegoExportado.json");
        return false;
    }
    return true;
}

std::string GameExporter::obtenerExtensionEjecutable() const {
    if (cfg_.plataforma == Plataforma::Windows) return cfg_.nombreEjecutable + ".exe";
    return cfg_.nombreEjecutable;
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