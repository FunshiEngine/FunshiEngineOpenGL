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
#include "MainDetalle.h"

// Redirige stdout y stderr (cubriendo cout/cerr y printf) a un archivo de log
// en la carpeta "logs/" de la primera candidata que acepte escritura, con
// timestamp por arranque. Asi el editor NO escupe texto a la terminal: se puede
// lanzar con doble clic o desde un .desktop y no queda ninguna consola atras de
// la ventana que moleste.
// La carpeta no es siempre la del ejecutable: instalado en Program Files sin
// elevar no admite escritura, y ahi el log se va a la raiz de datos (que el
// motor ya resuelve a la carpeta del usuario) o, en ultimo caso, a la temporal.
// Devuelve la ruta del archivo de log (vacia si no se pudo crear en ninguna).
static std::string redirigirSalidaALog(char* argv0) {
    namespace fs = std::filesystem;
    const fs::path ejecutable =
        argv0 && argv0[0] != '\0' ? fs::path(argv0).parent_path() : fs::path(".");
    std::error_code ec;
    const fs::path temporal = fs::temp_directory_path(ec);

    const std::time_t ahora = std::time(nullptr);
    // gmtime() del <ctime> estandar: portable entre MSVC, MinGW y Linux
    // (gmtime_s y gmtime_r no existen en todos los compiladores de Windows).
    // No hay threads en este punto del arranque, asi que la zona estatica
    // que devuelve es segura.
    const std::tm tmUtc = *std::gmtime(&ahora);
    char sufijo[32];
    std::strftime(sufijo, sizeof(sufijo), "%Y%m%d_%H%M%S", &tmUtc);
    const std::string nombre = "FunshiEngineGL_" + std::string(sufijo) + ".log";

    for (const std::string& carpeta :
         RutasLog::candidatas(ProjectPaths::directorioBase(),
                              ejecutable.string(), temporal.string())) {
        try {
            fs::create_directories(carpeta);
            const std::string ruta = (fs::path(carpeta) / nombre).string();
            // freopen redirige el FILE* de C (printf, cin/cout via
            // sync_with_stdio y fprintf). Se reabre en modo append por si ya
            // existe.
            g_logSalida = std::freopen(ruta.c_str(), "a", stdout);
            if (!g_logSalida) continue;
            // stderr sin redirigir no es motivo para renunciar: stdout ya lleva
            // el log y el resto de la salida (cout) sigue llegando ahi.
            std::freopen(ruta.c_str(), "a", stderr);
            std::cout << "\n========== Arranque FunshiEngineGL " << sufijo
                      << " ==========\n";
            std::cout << "Log en: " << ruta << "\n";
            return ruta;
        } catch (...) {
            g_logSalida = nullptr;
        }
    }
    return std::string();
}

static void registrarExcepcionNoCapturada() noexcept {
    std::string tipo = "excepcion_no_capturada";
    std::string mensaje = "El proceso termino por una excepcion no capturada";
    std::string traza;
    try {
        const std::exception_ptr pendiente = std::current_exception();
        if (pendiente) {
            try {
                std::rethrow_exception(pendiente);
            } catch (const RuntimeException& excepcion) {
                tipo = excepcion.getTipoExcepcion();
                mensaje = excepcion.what();
                traza = excepcion.getStackTrace();
            } catch (const std::exception& excepcion) {
                tipo = typeid(excepcion).name();
                mensaje = excepcion.what();
            } catch (...) {
                tipo = "excepcion_desconocida";
            }
        }
        EventoDiagnostico evento;
        evento.nivel = NivelDiagnostico::Critico;
        evento.modulo = "main";
        evento.mensaje = mensaje;
        evento.tipoExcepcion = tipo;
        evento.traza = traza;
        RegistroDiagnosticos::global().registrarUnaVez(
            std::move(evento), "ExcepcionesCPP:" + mensaje);
    } catch (...) {
        // La ruta de terminacion no debe lanzar otra excepcion.
    }
    std::fprintf(stderr, "[excepcion critica] %s: %s\n", tipo.c_str(),
                 mensaje.c_str());
    std::fflush(stderr);
    std::abort();
}

// Fase previa a la ventana: subproceso/arranque del motor web, argumentos
// (--proyecto, --juego, .fnep), redireccion del log y diagnostico de inicio.
// Devuelve >= 0 si el proceso debe terminar con ese codigo (subproceso CEF) y
// -1 si el editor sigue arrancando.
int prepararArranque(int argc, char* argv[], ArgumentosEditor& a) {
    std::string& proyectoCLI = a.proyectoCLI;
    std::string& rutaFnepCLI = a.rutaFnepCLI;
    bool& modoJuegoExportado = a.modoJuegoExportado;
    std::string& errorFnep = a.errorFnep;
    std::string& rutaLog = a.rutaLog;
#ifdef FUNSHI_CEF
    // Rol de subproceso CEF (renderer/gpu/utility/zygote): CEF relanza este
    // mismo binario con --type=... y CefExecuteProcess ejecuta ese rol. Va
    // ANTES de todo (log, ventana, GL): el subproceso no arranca el motor.
    // Devuelve >=0 si este proceso ES un subproceso; -1 si debe seguir.
    {
        const int codigoSubproceso = WebUIRuntime::ejecutarSubproceso(argc, argv);
        if (codigoSubproceso >= 0) return codigoSubproceso;
    }
#endif
    // Motor web de interfaces (CEF): se arranca UNA vez por proceso, ANTES de
    // cualquier inicializacion del motor (ventana, GL, log). Con
    // windowless_rendering_enabled no necesita la ventana GLFW para
    // inicializarse; el contexto GL solo se necesita despues, en render().
    // Si no puede arrancar, el runtime cae al backend nulo y el motor sigue.
#ifdef FUNSHI_CEF
    if (!WebUIRuntime::inicializar(argc, argv)) {
        std::cout << "[webui] Sin backend web real: las interfaces HTML "
                     "quedan sin textura (backend nulo)." << std::endl;
    } else {
        std::cout << "[webui] Backend activo: " << WebUIRuntime::nombreBackendActivo()
                  << std::endl;
    }
#endif
    std::set_terminate(registrarExcepcionNoCapturada);
    // El hilo principal del motor queda registrado: el contexto de OpenGL,
    // ImGui, CEF y la JVM solo se pueden usar desde aqui (ver Hilos/
    // HiloActual.h).
    Hilos::fijarHiloPrincipal();
    // Parseo simple de --proyecto <nombre> para arrancar directo en editor
    // (skip menu). Usado para ejecutar un juego exportado: FunshiEngineGL
    // --proyecto MiJuego. --juego fuerza el modo juego, que es como arranca
    // una exportacion (tambien via JuegoExportado.json, ver mas abajo).
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--proyecto" && i + 1 < argc) {
            proyectoCLI = argv[++i];
        } else if (arg.rfind("--proyecto=", 0) == 0) {
            proyectoCLI = arg.substr(11);
        } else if (arg == "--juego") {
            modoJuegoExportado = true;
        } else if (arg == "--abrir" && i + 1 < argc) {
            rutaFnepCLI = argv[++i];
        } else if (arg.size() > 5 && arg.compare(arg.size() - 5, 5, ".fnep") == 0) {
            // Doble clic sobre un .fnep: el sistema pasa la ruta como argumento.
            rutaFnepCLI = arg;
        }
    }

    // Manifiesto de arranque de una exportacion: si JuegoExportado.json existe
    // junto al binario, el juego arranca directo en modo juego sin argumentos
    // (doble clic). Un --proyecto explicito tiene prioridad sobre el nombre.
    {
        std::string proyectoManifiesto;
        bool modoManifiesto = false;
        if (ConfigJuegoExportado::leer(
                ConfigJuegoExportado::rutaPorDefecto(
                    ProjectPaths::directorioEjecutable()),
                proyectoManifiesto, modoManifiesto)) {
            if (proyectoCLI.empty()) proyectoCLI = proyectoManifiesto;
            modoJuegoExportado = modoJuegoExportado || modoManifiesto;
        }
    }

    // .fnep: abre el proyecto al que pertenece (vive en la carpeta del
    // proyecto). Si no se puede, el editor arranca en el menu y lo avisa.
    if (!rutaFnepCLI.empty()) {
        // Con doble clic el sistema arranca el editor en otra carpeta (la del
        // .fnep, o la del usuario) y los recursos del editor se resuelven
        // contra la carpeta de trabajo: se vuelve a la del ejecutable.
        std::error_code ec;
        rutaFnepCLI = std::filesystem::absolute(rutaFnepCLI, ec).string();
        const std::string exe = ProjectPaths::directorioEjecutable();
        if (!exe.empty()) std::filesystem::current_path(exe, ec);
    }
    if (!rutaFnepCLI.empty() && proyectoCLI.empty()) {
        if (auto nombre = ArchivoProyecto::resolverApertura(rutaFnepCLI, errorFnep))
            proyectoCLI = *nombre;
    }

    // La redireccion va PRIMERO: el diagnostico de GPU de initVentana() y los
    // [diag] de render ya escriben al log, no a la terminal.
    rutaLog = redirigirSalidaALog(argc > 0 ? argv[0] : nullptr);
    (void)rutaLog;

    EventoDiagnostico inicio;
    inicio.nivel = NivelDiagnostico::Informacion;
    inicio.modulo = "main";
    inicio.proyecto = proyectoCLI;
    inicio.mensaje = "Editor iniciado";
    RegistroDiagnosticos::global().registrar(std::move(inicio));
    if (!errorFnep.empty()) {
        std::cerr << "[proyecto] No se abre " << rutaFnepCLI << ": " << errorFnep << '\n';
        EventoDiagnostico aviso;
        aviso.nivel = NivelDiagnostico::Advertencia;
        aviso.modulo = "proyecto";
        aviso.mensaje = "No se pudo abrir " + rutaFnepCLI + ": " + errorFnep;
        RegistroDiagnosticos::global().registrar(std::move(aviso));
    }
    return -1;
}
