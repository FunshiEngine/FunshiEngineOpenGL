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

std::vector<GameExporter::Plataforma> GameExporter::plataformasDisponibles() {
#ifdef _WIN32
    // En Windows el toolchain de MinGW es el nativo: se exporta a Windows.
    return {Plataforma::Windows};
#else
    // En Linux se exporta a Linux y, si se quiere, se cruza a Windows con el
    // toolchain de MinGW.
    return {Plataforma::Linux, Plataforma::Windows};
#endif
}

const char* GameExporter::etiquetaPlataforma(Plataforma plataforma) {
#ifdef _WIN32
    (void)plataforma;
    return "Windows (nativo)";
#else
    return plataforma == Plataforma::Windows ? "Windows (cross-compile MinGW)"
                                             : "Linux (nativo)";
#endif
}

std::string GameExporter::directorioBuildTemporal() {
    // Carpeta temporal del sistema: el directorio de compilacion es de
    // usar y tirar, y ahi no se acumulan restos de exportaciones fallidas.
    std::error_code ec;
    fs::path base = fs::temp_directory_path(ec);
    if (ec || base.empty())
        base = fs::path(".");

    const std::time_t ahora = std::time(nullptr);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &ahora);
#else
    gmtime_r(&ahora, &utc);
#endif
    char sufijo[32] = {0};
    std::strftime(sufijo, sizeof(sufijo), "%Y%m%d_%H%M%S", &utc);
    const std::string nombre = std::string("funshi_export_") + sufijo;

    // El nombre se reserva creando el directorio: la creacion es atomica,
    // asi que dos procesos que exporting a la vez no obtienen el mismo
    // (el que llega segundo prueba el siguiente).
    for (int intento = 0; intento < 1000; ++intento) {
        const fs::path candidata =
            base / (nombre + "_" + std::to_string(intento));
        std::error_code ec2;
        if (fs::create_directory(candidata, ec2))
            return candidata.string();
        if (ec2 && ec2 != std::errc::file_exists)
            break; // la carpeta temporal no es utilizable
    }
    // Sin carpeta temporal utilizable: se cae al directorio del sistema
    // (el nombre lleva el instante, asi que es practicamente unico).
    return (base / (nombre + "_" + std::to_string(std::time(nullptr)))).string();
}

std::string GameExporter::rutaLogExportacion() {
    // Timestamp en UTC, como el log del motor: asi el nombre ordena igual en
    // cualquier huso horario.
    const std::time_t ahora = std::time(nullptr);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &ahora);
#else
    gmtime_r(&ahora, &utc);
#endif
    char sufijo[32] = {0};
    std::strftime(sufijo, sizeof(sufijo), "%Y%m%d_%H%M%S", &utc);

    const std::string nombreBase = std::string("Exportacion_") + sufijo;
    // La traza vive junto al log del motor (carpeta "logs/"): el usuario
    // encuentra los dos en el mismo sitio.
    const std::vector<std::string> carpetas =
        RutasLog::candidatas(ProjectPaths::directorioBase(), std::string(),
                             std::string());
    for (const std::string& carpeta : carpetas) {
        std::error_code ec;
        fs::create_directories(carpeta, ec);
        if (ec) continue;
        // Dos exportaciones en el mismo segundo no pisan el log anterior:
        // el nombre se reserva creando el archivo.
        std::string ruta =
            (fs::path(carpeta) / (nombreBase + ".log")).string();
        int indice = 2;
        while (fs::exists(ruta, ec)) {
            ruta = (fs::path(carpeta) /
                    (nombreBase + "_" + std::to_string(indice) + ".log"))
                       .string();
            ++indice;
        }
        std::ofstream reserva(ruta, std::ios::out | std::ios::trunc);
        if (!reserva) continue;
        reserva.close();
        return ruta;
    }
    return std::string();
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

    // Que DLL necesita el juego: en Windows se leen de la tabla de
    // importaciones del ejecutable (con su nombre real, version incluida: el
    // runtime de MinGW y las bibliotecas de terceros las Exports).
    const std::string exe = buildDir + "/" + obtenerExtensionEjecutable();

    // De donde se toman: la carpeta del compilador con el que se construyo el
    // motor (ahi viven el runtime y las DLL de las bibliotecas de terceros),
    // las carpetas del PATH y las del build.
    const std::vector<std::string> rutas = directoriosDeDependencias(
        buildDir, cfg_.plataforma == Plataforma::Windows);
    const BuscadorDll buscador = [&rutas](const std::string& dll) {
        return buscarDependencia(rutas, dll);
    };

    // Una DLL puede necesitar otras (assimp necesita zlib y minizip), asi que
    // el cierre de dependencias se calcula sobre los imports de cada DLL
    // copiada, no solo sobre los del ejecutable.
    std::vector<std::pair<std::string, std::string>> cierre;
    if (cfg_.plataforma == Plataforma::Windows) {
        cierre = cierreDeDependencias(exe, buscador);
    } else {
        // En Linux las .so van a lib/ con su nombre tal cual: el binario las
        // encuentra por $ORIGIN/lib (BUILD_RPATH del CMake generado).
        for (const char* so : { "libBulletDynamics.so", "libBulletCollision.so",
                                "libLinearMath.so", "libglfw.so",
                                "libminiaudio.so" }) {
            cierre.emplace_back(so, buscarDependencia(rutas, so));
        }
    }

    for (const auto& par : cierre) {
        const std::string& dll = par.first;
        const std::string& origen = par.second;
        if (origen.empty()) {
            log("La DLL '" + dll + "' no se encontro donde buscarla: el juego "
                "exportado la pedira al arrancar");
            continue;
        }
        fs::copy_file(origen, libDir + "/" + dll,
                      fs::copy_options::overwrite_existing, ec);
        if (ec) {
            log("Error copiando " + dll + ": " + ec.message());
            continue;
        }
        log("Dependencia copiada: " + dll);
    }
    return true;
}

std::vector<std::pair<std::string, std::string>>
GameExporter::cierreDeDependencias(const std::string& rutaEjecutable,
                                   const BuscadorDll& buscador) {
    std::vector<std::pair<std::string, std::string>> cierre;
    std::set<std::string> vistas;
    std::vector<std::string> pendientes = ImportacionesPE::leer(rutaEjecutable);
    while (!pendientes.empty()) {
        const std::string dll = pendientes.back();
        pendientes.pop_back();
        if (dll.empty()) continue;
        if (!vistas.insert(dll).second) continue; // ya mirada
        if (esDllDelSistema(dll)) continue;        // la trae el sistema
        const std::string origen = buscador(dll);
        cierre.emplace_back(dll, origen);
        if (origen.empty()) continue; // no esta: no hay de donde leer lo suyo
        // Lo que esa DLL necesita a su vez.
        for (const std::string& dependiente : ImportacionesPE::leer(origen)) {
            pendientes.push_back(dependiente);
        }
    }
    return cierre;
}

std::vector<std::string> GameExporter::directoriosDeDependencias(
    const std::string& buildDir, bool windows) {
    std::vector<std::string> directorios;

    // La carpeta del compilador con el que se construyo el motor: ahi viven
    // el runtime (libstdc++-6.dll, libgcc_s_seh-1.dll, libwinpthread-1.dll) y
    // las DLL de las bibliotecas con las que enlaza (glfw3, assimp, bullet...).
    const std::string compilador(FUNSHI_CXX_COMPILER);
    const std::size_t separador = compilador.find_last_of("/\\");
    if (separador != std::string::npos) {
        directorios.push_back(compilador.substr(0, separador));
    }

    // Las carpetas del PATH: el toolchain suele estar ahi.
    if (const char* path = std::getenv("PATH")) {
        const std::string separadores = windows ? ";" : ":";
        std::string actual;
        for (const char c : std::string(path)) {
            if (c == separadores[0]) {
                if (!actual.empty()) directorios.push_back(actual);
                actual.clear();
            } else {
                actual.push_back(c);
            }
        }
        if (!actual.empty()) directorios.push_back(actual);
    }

    // Las del build (por si el propio build copio algo junto al binario).
    directorios.push_back(buildDir);
    directorios.push_back(buildDir + "/engine_build");
    directorios.push_back(buildDir + "/engine_build/src");

    // Las de sistema en Linux.
    directorios.push_back("/usr/lib/x86_64-linux-gnu");
    directorios.push_back("/usr/local/lib");
    return directorios;
}

bool GameExporter::esDllDelSistema(const std::string& dll) {
    // Las que Windows ya trae: no hacen falta en la carpeta del juego.
    // opengl32.dll y dbghelp.dll tambien: la del toolchain es la version de
    // Mesa (render por software), ycopiarla taparia la del sistema.
    static const char* const kDelSistema[] = {
        "kernel32.dll", "user32.dll", "gdi32.dll", "advapi32.dll",
        "shell32.dll", "ole32.dll", "oleaut32.dll", "uuid.dll",
        "comdlg32.dll", "ws2_32.dll", "winmm.dll", "msvcrt.dll",
        "ntdll.dll", "imm32.dll", "setupapi.dll", "shlwapi.dll",
        "psapi.dll", "bcrypt.dll", "crypt32.dll", "iphlpapi.dll",
        "dwmapi.dll", "uxtheme.dll", "comctl32.dll", "mpr.dll",
        "netapi32.dll", "userenv.dll", "version.dll", "oleacc.dll",
        "msvcp_win.dll", "vcruntime140.dll", "vcruntime140_1.dll",
        "ucrtbase.dll", "msvcp140.dll", "opengl32.dll", "dbghelp.dll"
    };
    std::string nombre = dll;
    for (char& c : nombre) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    for (const char* delSistema : kDelSistema) {
        if (nombre == delSistema) return true;
    }
    // Los conjuntos de API de Windows (api-ms-win-*) tambien son del sistema.
    if (nombre.rfind("api-ms-win-", 0) == 0) return true;
    return false;
}

std::string GameExporter::buscarDependencia(
    const std::vector<std::string>& directorios, const std::string& dll) {
    std::error_code ec;
    for (const std::string& directorio : directorios) {
        if (directorio.empty()) continue;
        const std::string ruta = directorio + "/" + dll;
        if (fs::exists(ruta, ec) && fs::is_regular_file(ruta, ec)) return ruta;
    }
    return "";
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
    // La traza tambien queda en el archivo de la exportacion: lo que se ve
    // en la terminal (compilacion, avisos del enlazador) se puede revisar
    // despues sin bucear en el log general del motor.
    if (!rutaLog_.empty()) {
        std::ofstream archivo(rutaLog_, std::ios::app);
        if (archivo) archivo << msg << "\n";
    }
}

void GameExporter::progreso(float p, const std::string& etapa) {
    if (cfg_.onProgreso) cfg_.onProgreso(p, etapa);
}

void GameExporter::finalizar(bool ok, const std::string& msg) {
    // El mensaje final dice donde esta la traza: sin eso, lo que se ve en la
    // terminal al compilar se pierde al cerrar la ventana.
    const std::string detalle =
        rutaLog_.empty() ? msg : (msg + " (traza: " + rutaLog_ + ")");
    exito_.store(ok, std::memory_order_release);
    {
        std::lock_guard<std::mutex> bloqueo(mutexEstado_);
        mensajeFinal_ = detalle;
    }
    // El mensaje se escribe antes del indicador: quien vea que la
    // exportacion termino ve tambien el mensaje final.
    terminado_.store(true, std::memory_order_release);
    cvEstado_.notify_all();
    if (cfg_.onFinalizado) cfg_.onFinalizado(ok, detalle);
}
