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
#include "GameExporterDetalle.h"

namespace {

// La maquina donde corre el motor es Windows: el toolchain de MinGW es el
// nativo (no hace falta cruzar compilers) y el binario exportado lleva sus
// DLL al lado, no en lib/.
bool hostEsWindows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

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

}  // namespace

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
    if (hiloExportacion_.joinable()) {
        hiloExportacion_.join();
    } else if (motorHilos_) {
        // La exportacion corre en el pool: se espera a que termine antes de
        // destruir el exportador (si no, la tarea usaria un objeto ya
        // destruido).
        std::unique_lock<std::mutex> bloqueo(mutexEstado_);
        cvEstado_.wait(bloqueo, [this] {
            return terminado_.load(std::memory_order_acquire);
        });
    }
}

void GameExporter::iniciar() {
    // Un exportador no puede tener dos exportaciones vivas: si la anterior
    // sigue en curso, se espera a que termine. Asignar a un std::thread que ya
    // es joinable termina el proceso.
    if (hiloExportacion_.joinable()) hiloExportacion_.join();
    terminado_.store(false, std::memory_order_release);
    exito_.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> bloqueo(mutexEstado_);
        mensajeFinal_.clear();
    }
    if (motorHilos_) {
        // El trabajo pesado va al pool de hilos del motor; el editor sigue
        // en su hilo y se entera por los callbacks.
        if (!motorHilos_->encolar([this] { runExportacion(); }))
            finalizar(false, "El motor de hilos no aceptó la exportación");
        return;
    }
    hiloExportacion_ = std::thread(&GameExporter::runExportacion, this);
}

bool GameExporter::haTerminado() const noexcept {
    return terminado_.load(std::memory_order_acquire);
}

std::string GameExporter::mensajeFinal() const {
    std::lock_guard<std::mutex> bloqueo(mutexEstado_);
    return mensajeFinal_;
}

void GameExporter::runExportacion() {
    try {
        runExportacionInterna();
    } catch (const std::exception& excepcion) {
        // La excepcion no puede escapar al hilo: dejaria el exportador sin
        // marcar como terminado y quien espera (el dialogo) no sabria
        // cuando acabar.
        finalizar(false, std::string("Error inesperado en la exportación: ") +
                             excepcion.what());
    } catch (...) {
        finalizar(false, "Error inesperado en la exportación");
    }
}

void GameExporter::runExportacionInterna() {
    // Traza propia de esta exportacion: la salida de la compilacion se
    // escribe ahi, para poder revisarla cuando algo salga raro.
    rutaLog_ = rutaLogExportacion();
    if (!rutaLog_.empty()) {
        std::ofstream cabecera(rutaLog_, std::ios::app);
        if (cabecera) {
            cabecera << "=== Exportacion de " << cfg_.nombreProyectoExportado
                     << " (proyecto: " << cfg_.nombreProyecto << ") ===\n";
        }
    }
    // La traza dice de que proyecto se exporta: es el dato que el dialogo
    // puede perder (o no tener) y el que hace falta para saber desde el log
    // que se exporto lo que el usuario pedia.
    log("Iniciando exportación de: " + cfg_.nombreProyectoExportado +
        " (proyecto: " + cfg_.nombreProyecto + ")");
    progreso(0.05f, "Preparando directorio de build");

    // Directorio temporal de build: uno propio por exportacion, en la carpeta
    // temporal del sistema. Con un nombre fijo, dos exportaciones (del mismo
    // proyecto o de distintos) compartian el directorio de compilacion: la
    // segunda lo vaciaba mientras la primera compilaba, y esta fallaba con
    // errores de "opening dependency file ... No such file or directory".
    std::string buildDir = directorioBuildTemporal();
    std::error_code ec;
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
    // juego tiene su propio punto de entrada (main_juego.cpp): arranca con
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

# Los sanitizadores (ASan/UBSan) son para el build de trabajo del motor: el
# toolchain de la exportacion (MinGW) no trae sus runtimes, asi que activarlos
# rompe el enlace del binario del juego. La exportacion se compila sin ellos.
set(ENABLE_ASAN OFF CACHE BOOL "" FORCE)

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
    // Solo cuando la maquina NO es Windows: aqui el toolchain de MinGW ya es
    // el nativo y el archivo de toolchain (que busca el compilador cruzado
    // x86_64-w64-mingw32) no estaria disponible.
    if (cfg_.plataforma == Plataforma::Windows && !hostEsWindows()) {
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
    // como argv. La salida del compilador va al archivo de traza de esta
    // exportacion (agregandose al final, para que las etapas se acumulen en
    // el mismo archivo); si no hay traza, el hijo hereda la salida del motor
    // (igual que con std::system).
    // `cmake .` (fuente = el propio buildDir, donde el generador dejo el
    // CMakeLists): el viejo `cmake ..` apuntaba al padre del buildDir, que no
    // contiene CMakeLists.txt y el configure fallaba siempre. El toolchain
    // MinGW (si la plataforma objetivo es Windows y la maquina no lo es) se
    // pasa en este configure: es lo unico que hace que CMake cruce compilers.
    std::vector<std::string> argvConfigure = {"cmake", "."};
    if (cfg_.plataforma == Plataforma::Windows && !hostEsWindows())
        argvConfigure.push_back("-DCMAKE_TOOLCHAIN_FILE=" + buildDir +
                                "/toolchain-mingw.cmake");
    if (Proceso::ejecutar(argvConfigure, rutaLog_, buildDir, {}, false) != 0)
        return false;
    return Proceso::ejecutar(
               {"cmake", "--build", ".", "--target", "funshi_engine", "-j4"},
               rutaLog_, buildDir, {}, false) == 0;
}

bool GameExporter::compilarJuego(const std::string& buildDir) {
    // El objetivo es el binario con el nombre del ejecutable que pidio el
    // usuario (el CMake generado lo crea asi); antes era "ExportedGame", el
    // nombre del project() temporal.
    return Proceso::ejecutar(
               {"cmake", "--build", ".", "--target", cfg_.nombreEjecutable,
                "-j4"},
               rutaLog_, buildDir, {}, false) == 0;
}

bool GameExporter::copiarAssetsYDependencias(const std::string& buildDir) {
    // En lugar de copiar todo el proyecto y luego limpiar, copiamos SOLO lo
    // necesario para el juego: escenas, binarios compilados de scripts y assets.
    // NO se copia la estructura FunshiEngine-DATA/Proyects: el juego exportado tiene
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
    // La ruta del proyecto viene con barra final (".../Scene/"): con ella, la
    // copia falla con "no such file or directory", asi que se normaliza.
    std::string sceneDirOrigen = EditorConfig::rutaSceneDir(cfg_.nombreProyecto);
    while (!sceneDirOrigen.empty() &&
           (sceneDirOrigen.back() == '/' || sceneDirOrigen.back() == '\\')) {
        sceneDirOrigen.pop_back();
    }
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
