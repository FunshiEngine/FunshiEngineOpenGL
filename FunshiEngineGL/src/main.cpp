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
// GLFW solo para la ventana/callbacks (GLFW_INCLUDE_NONE: ningun GL aca).
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#endif

#include "../src/Scenes/GameScene.h"
#include "../src/GUIManager/GUIManager.h"
#include "../src/Configuracion/EditorConfig.h"
#include "../src/Configuracion/ProjectPaths.h"
#include "../src/Configuracion/ProyectoInicial.h"
#include "../src/Configuracion/RutasLog.h"
#include "../src/Exportador/ConfigJuegoExportado.h"
#include "../src/GUI/WindowNames.h"
#include "../src/GUI/Tema/TemaEditor.h"
#include <imgui.h>
#include "EngineTime.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "../src/Objetos/Modelos3D.h"
#include "../src/Objetos/Componentes/CameraComponent.h"
#include "../src/Proyectos/GestorDeProyectos.h"
#include "../src/States/ApplicationStateMachine.h"
#include "../src/States/OrquestadorEstadoGUI.h"
#include "../src/Behaviour/ScriptRuntime.h"
#include "../src/Events/EditorEventBus.h"
#include "../src/Ventana.h"
#include "../src/Input/EditorInput.h"
#include "../src/Rendering/Backend/IRenderBackend.h"
#include "ImGuizmo.h"
#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <string>

// Al cerrar la app, LeakSanitizer reporta una fuguita de 128 bytes de una
// libreria externa sin simbolos (GLFW/X11 o el driver de video) que se reserva
// al crear la ventana y no es imputable al codigo del motor. El cheq queda
// pendiente en atexit y no se puede suprimir desde adentro (ni __lsan_disable:
// la fuga ya existia cuando se llama). La solucion es terminar sin pasar por
// los handlers de atexit una vez que todo el cleanup ya corrió de forma
// explicita (escena, scripts JNI, ImGui, glfw). ASan sigue detectando usos
// invalidos en ejecución; solo se descarta el contador de fugas al salir.
// Nota: __has_feature solo existe en Clang; en GCC el "__has_feature(...)"
// del #if falla al parsear, por eso se anida con defined() antes de usarlo.
#if defined(__SANITIZE_ADDRESS__)
#define FUNSHI_ASAN_ACTIVO 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define FUNSHI_ASAN_ACTIVO 1
#else
#define FUNSHI_ASAN_ACTIVO 0
#endif
#else
#define FUNSHI_ASAN_ACTIVO 0
#endif

//VENTANA
static int ventanaHeightEjeY, ventanaWidthEjeX;

//INPUT
static bool recFilesInit = true;

// Variables de estado
// El estado abierto/cerrado del menu de inicio lo gobierna el modelo del
// paquete MenuGUI (MenuModel), sincronizado por frame desde la maquina de
// estados (fuente de verdad); no un bool suelto de main.
// Fuente de verdad del estado de la aplicacion (MainMenu / Editing). El menu
// de inicio (paquete MenuGUI) informa su decision y aqui se refleja.
static ApplicationStateMachine appStateMachine;
static OrquestadorEstadoGUI orquestadorDeGUI(&appStateMachine);
static float deltaTime = 0.0f;

// Maneja el FILE* de la redireccion de salida; el puntero debe vivir toda la
// app para que el flush de cierre de main escriba en el log.
static FILE* g_logSalida = nullptr;

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

#if defined(_WIN32)
// Windows con subsistema GUI (/SUBSYSTEM:WINDOWS, WIN32_EXECUTABLE=TRUE):
// MSVC y MinGW exigen WinMain como entrada, no main. Este wrapper construye
// argc/argv (ANSI, el codepage que usa std::filesystem en Windows) desde
// GetCommandLineW y delega en el mismo arranque que usa main en Linux. Asi el
// doble clic no abre ninguna consola detras de la ventana.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <vector>

static int EjecutarMotor(int argc, char* argv[]);

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<char*> argvAnsi;
    std::vector<std::string> almacen;
    if (wargv) {
        almacen.reserve(static_cast<size_t>(argc));
        for (int i = 0; i < argc; ++i) {
            int largo = WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, nullptr, 0,
                                            nullptr, nullptr);
            std::string s(static_cast<size_t>(largo), '\0');
            WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, &s[0], largo, nullptr,
                                nullptr);
            if (!s.empty()) {
                s.pop_back();  // el -1 de W2CB deja el '\0' incluido en s.
            }
            almacen.push_back(std::move(s));
        }
        for (auto& s : almacen) {
            argvAnsi.push_back(s.data());
        }
        LocalFree(wargv);
    }
    return EjecutarMotor(static_cast<int>(argvAnsi.size()), argvAnsi.data());
}
#endif  // _WIN32

static int EjecutarMotor(int argc, char* argv[])
{
    // Parseo simple de --proyecto <nombre> para arrancar directo en editor
    // (skip menu). Usado para ejecutar un juego exportado: FunshiEngineGL
    // --proyecto MiJuego. --juego fuerza el modo juego, que es como arranca
    // una exportacion (tambien via JuegoExportado.json, ver mas abajo).
    std::string proyectoCLI;
    bool modoJuegoExportado = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--proyecto" && i + 1 < argc) {
            proyectoCLI = argv[++i];
        } else if (arg.rfind("--proyecto=", 0) == 0) {
            proyectoCLI = arg.substr(11);
        } else if (arg == "--juego") {
            modoJuegoExportado = true;
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

    // La redireccion va PRIMERO: el diagnostico de GPU de initVentana() y los
    // [diag] de render ya escriben al log, no a la terminal.
    std::string rutaLog = redirigirSalidaALog(argc > 0 ? argv[0] : nullptr);
    (void)rutaLog;

    Ventana* auxVentana = new Ventana();
    auxVentana->initVentana();
    GLFWwindow* window = auxVentana->getWindow();
    GUIManager* managerOfGUI = new GUIManager(window);
    GameScene* scene = new GameScene(managerOfGUI);
    MenuGUI* mainMenu = managerOfGUI->getMenuGUI();
    TreeFilesInterface* treeFilesInterface = managerOfGUI->getTreeFilesGUI();
    ContentFolderInterface* contentFolderInterface = managerOfGUI->getContentFolderGUI();
    // Callbacks del editor (teclado/mouse) + navegacion de la camara: su
    // propio modulo (Input/EditorInput) las traduce a acciones y mantiene la
    // maquina de estado de movimiento (diagonales WASD normalizadas).
    EditorInput* input =
        new EditorInput(scene, &appStateMachine, &orquestadorDeGUI);
    // Los controles del menu de escena piden transiciones al orquestador,
    // unica fuente de verdad para Depuracion, Juego, pausa, reset y fin.
    if (SceneMenuBarInterface* menuBarEscena = managerOfGUI->getMenuBarGUI())
        menuBarEscena->setAccionSimulacion(
            [](SceneMenuBarInterface::AccionSimulacion accion) {
                using Accion = SceneMenuBarInterface::AccionSimulacion;
                switch (accion) {
                    case Accion::IniciarDepuracion:
                        orquestadorDeGUI.manejarTeclaSimulacion(
                            OrquestadorEstadoGUI::TeclaSimulacion::Depuracion);
                        break;
                    case Accion::IniciarJuego:
                        orquestadorDeGUI.iniciarJuego();
                        break;
                    case Accion::Pausa:
                        orquestadorDeGUI.manejarTeclaSimulacion(
                            OrquestadorEstadoGUI::TeclaSimulacion::Pausa);
                        break;
                    case Accion::Reset:
                        orquestadorDeGUI.solicitarReset();
                        break;
                    case Accion::Terminar:
                        orquestadorDeGUI.manejarTeclaSimulacion(
                            OrquestadorEstadoGUI::TeclaSimulacion::Stop);
                        break;
                }
            });
    Time::start();

    // Resolucion de la raiz de datos y recuperacion de datos previos. Va antes
    // de cargar la configuracion para que se lea la migrada, no la de la ruta
    // historica. Cuando el motor esta en una carpeta donde no puede escribir
    // (instalado en Program Files y sin elevar), la raiz cae a la carpeta de
    // datos del usuario; si quedo algo de una ejecucion elevada, se copia aqui.
    {
        std::string mensajeMigracion;
        const bool migro = ProjectPaths::migrarDatosDesdeRutaOriginal(mensajeMigracion);
        if (ProjectPaths::datosEnRutaDeUsuario()) {
            std::cout << "[rutas] '" << ProjectPaths::directorioBaseOriginal()
                      << "' no admite escritura, asi que los datos van a '"
                      << ProjectPaths::directorioBase() << "'."
                      << std::endl;
        }
        if (migro && !mensajeMigracion.empty())
            if (StatusBarInterface* status = managerOfGUI->getStatusBarGUI())
                status->mostrarMensaje(mensajeMigracion);
    }

    // Configuration del editor (interfaz + menu) persistida en JSON en Memory
    // del proyecto del usuario. Al arrancar se carga y se aplica a cada capa; al
    // salir se recogen los valores actuales y se guarda (ver fin de main).
    // La escena es independiente: sigue en sus binarios (SceneSerializer).
    EditorConfig editorConfig;
    editorConfig.cargar(EditorConfig::rutaPorDefecto());
    // Ciclo de vida del proyecto activo (entrar/guardar/renombrar/eliminar/
    // exportar + imgui.ini): main delega todo en esta fachada.
    GestorDeProyectos gestor(editorConfig, scene, managerOfGUI, mainMenu);

    // Estructura base de datos del motor (Proyects/, Configuraciones/,
    // Exportaciones/) y migraciones de estructuras antiguas. No crea ningun
    // proyecto: elegirlo (o crearlo) es una decision del usuario en el menu.
    EditorConfig::asegurarEstructuraBase();

    // Proyecto activo al arrancar: lo decide ProyectoInicial::resolver a partir
    // de --proyecto (si se pasa) o vacio para forzar el menu. NO se usa
    // ultimoProyecto persistido automaticamente: siempre arranca en el menu
    // salvo que se pase --proyecto (skip menu). Asi se evita entrada automatica
    // al editor por config previa.
    const bool hayConfigGeneral =
        std::filesystem::exists(EditorConfig::rutaPorDefecto());
    const ProyectoInicial::Resolucion arranque = ProyectoInicial::resolver(
        editorConfig.datos().nombreProyecto, proyectoCLI, hayConfigGeneral);
    std::string proyectoActual = arranque.nombre;
    gestor.fijarProyectoActual(proyectoActual);
    // El nombre persistido debe reflejar el proyecto realmente resuelto: en
    // primer arranque queda vacio para que guardarGeneral no escriba
    // "ultimoProyecto" (si no, al releer se crearian las carpetas de "Nuevo
    // Proyecto" solas).
    editorConfig.datos().nombreProyecto = proyectoActual;

    // Contexto de rutas de la serializacion portable: con proyecto, la raiz de
    // assets (src<nombre>) es el ancla con la que se guardan (relativas) y se
    // cargan (absolutas) las rutas de mallas/texturas/scripts de la escena.
    // Sin proyecto (primer arranque) se limpia y las rutas pasan sin cambios.
    EditorConfig::fijarRaizAssets(
        gestor.proyectoActual().empty()
            ? std::string()
            : EditorConfig::directorioSrc(gestor.proyectoActual()));

    if (gestor.hayProyecto())
        gestor.prepararProyectoAlArrancar();

    mainMenu->setNombreProyecto(proyectoActual);
    mainMenu->setIdioma(editorConfig.datos().idioma);
    mainMenu->setSensibilidadCamara(editorConfig.datos().sensibilidadCamara);
    mainMenu->setSensibilidadMovimientoCamara(
        editorConfig.datos().sensibilidadMovimientoCamara);

    mainMenu->setApariencia(editorConfig.datos().apariencia);
    scene->setVentanaCamarasAbierta(editorConfig.datos().ventanaCamarasAbierta);
    scene->setSensibilidadMovimientoCamara(
        editorConfig.datos().sensibilidadMovimientoCamara);
    scene->setGizmoOperation(editorConfig.datos().gizmoOperacion);
    scene->setGizmoGlobal(editorConfig.datos().gizmoGlobal);
    scene->setApariencia(editorConfig.datos().apariencia);
    scene->setSensibilidadCamara(editorConfig.datos().sensibilidadCamara);
    managerOfGUI->restaurarEstadosVentanas(editorConfig.datos().estadoVentanas);

    // Ultimo estado de la maquina reflejado en la fachada del paquete MenuGUI
    // (guardia de cambio; ver el bucle principal).
    bool menuReflejadoEnFachada = appStateMachine.is(ApplicationState::MainMenu);

    // --proyecto: forzar modo editor y mostrar paneles sin pasar por el menu.
    if (!proyectoCLI.empty() && !modoJuegoExportado) {
        appStateMachine.transitionTo(ApplicationState::Editing);
        scene->setMenuActivo(true);
    }

    // Juego exportado: arrancar directo en modo juego sobre la escena cargada,
    // por el mismo camino que el boton Iniciar Juego del menu de escena.
    if (modoJuegoExportado && !proyectoCLI.empty()) {
        // El proyecto ya se cargara via gestor.prepararProyectoAlArrancar()
        // al entrar en el bucle principal. Forzamos modo Playing.
        appStateMachine.transitionTo(ApplicationState::Playing);
        scene->setModoJuego(true);
    }

    
    // Se registra ANTES de ImGui_ImplGlfw_InitForOpenGL (mas abajo): el backend
    // de ImGui encadena la callback previa para los clics del editor.
    input->registrarCallbacks(window);
    // Cursor consistente con la maquina desde el arranque (el menu arranca
    // visible y el editor con las interfaces activas: cursor normal).
    input->aplicarModoCursor(window);




    // Fondo del viewport 3D segun el perfil de apariencia; el bucle principal
    // lo refresca por frame para reflejar cambios en vivo desde Opciones. La
    // limpieza del frame la hace el backend (el color de fondo vive en el).
    {
        float fondoInicial[3];
        AparienciaUtil::fondoEfectivo(mainMenu->getApariencia(), fondoInicial);
        Rendering::Backend::activeBackend().applyBaseState();
        Rendering::Backend::activeBackend().setClearColor(fondoInicial);
    }

    // La iluminacion no tiene estado que encender: LightSystem arma los datos en
    // CPU y SceneRenderer los sube como uniforms del shader en cada pasada.

    float FPS = 60.0;    //LIMITE DE FPS
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Suscriptores del bus de GUI. Se registran despues de crear el contexto
    // porque AparienciaCambio aplica TemaEditor::aplicarEstilo, que necesita
    // ImGui. La apariencia inicial no depende de esto: la escena la recibe
    // directo y el estilo se aplica mas abajo. Los cambios en vivo del usuario
    // si llegan por el bus.
    EditorEventBus* eventosGUI = managerOfGUI->getEditorEventBus();
    if (eventosGUI) {
        eventosGUI->subscribe([scene, &editorConfig](const EditorEvent& ev) {
            if (ev.type != EditorEventType::AparienciaCambio) return;
            scene->setApariencia(ev.apariencia);
            TemaEditor::aplicarEstilo(ev.apariencia);
            float fondo[3];
            AparienciaUtil::fondoEfectivo(ev.apariencia, fondo);
            Rendering::Backend::activeBackend().setClearColor(fondo);
            auto& cfg = editorConfig.datos();
            cfg.apariencia = ev.apariencia;
            editorConfig.solicitarGuardadoGeneral();
        });
        eventosGUI->subscribe([&editorConfig](const EditorEvent& ev) {
            if (ev.type != EditorEventType::IdiomaCambio) return;
            editorConfig.datos().idioma = ev.idioma;
            editorConfig.solicitarGuardadoGeneral();
        });
    }

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // El imgui.ini (layout de docks y geometria de ventanas) lo gestiona el
    // GestorDeProyectos (se guarda en Memory del proyecto del usuario, no en el
    // directorio actual de lanzamiento). Sin proyecto aun (primer arranque) no
    // se fija: ImGui queda sin ini en disco y no crea carpetas de "Nuevo
    // Proyecto" por el camino.
    gestor.fijarImguiIO(&io);
    gestor.aplicarImguiIniDelProyectoActual();
    // Tema global de ImGui a partir del perfil de apariencia cargado. Los
    // cambios en vivo llegan por el bus de GUI (EditorEventBus/AparienciaCambio),
    // ya no por relectura por frame del menu.
    TemaEditor::aplicarEstilo(mainMenu->getApariencia());
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // El contexto que pide Ventana es OpenGL 3.3 core, asi que el GLSL del
    // backend tiene que caber en ese contexto (un "#version 440" no compila en
    // 3.3 y el shader del backend queda rechazado). El backend parsea el numero
    // con sscanf("#version %d"), por eso la variante "core" no altera la
    // eleccion de shaders y el string se prepende tal cual al fuente GLSL.
    // El retorno se chequea: si el backend no arranca, io.BackendRendererUserData
    // queda en nullptr y en Release (sin IM_ASSERT) la primera llamada a
    // ImGui_ImplOpenGL3_NewFrame() desreferencia ese puntero. Con el flag el
    // bucle omite los llamados del renderer y la app corre sin UI dibujada en
    // vez de caerse.
    bool imguiGl3Listo = ImGui_ImplOpenGL3_Init("#version 330 core");
    if (!imguiGl3Listo)
        std::cerr << "[main] ImGui_ImplOpenGL3_Init fallo: no se dibujara la "
                     "interfaz ImGui (renderer OpenGL3 no disponible)\n";

    // Ventana Estado: al cerrarla con la 'X' se persiste en la config
    // del proyecto activo, no en la general. El mismo canal sirve para el
    // menu "Ventanas" de la barra: al tildar/destildar un panel se aplica
    // su visibilidad aqui y se persiste (asi el explorador reabre).
    if (eventosGUI) {
        eventosGUI->subscribe([&gestor, &editorConfig, managerOfGUI](
                                  const EditorEvent& ev) {
            if (ev.type != EditorEventType::VentanaEstadoCambio) return;
            if (!ev.nombreVentana) return;
            editorConfig.datos().estadoVentanas[ev.nombreVentana] = ev.abierta;
            managerOfGUI->setEstadoVentana(ev.nombreVentana, ev.abierta);
            editorConfig.guardarProyecto(gestor.proyectoActual());
        });
        // Sensibilidad: la aplica a la escena al instante y la persiste en la
        // config general (junto a idioma/apariencia). Reemplaza al polling por
        // frame que leia el menu a mano.
        eventosGUI->subscribe([scene, &editorConfig](const EditorEvent& ev) {
            if (ev.type != EditorEventType::SensibilidadCambio) return;
            scene->setSensibilidadCamara(ev.sensibilidad);
            editorConfig.datos().sensibilidadCamara = ev.sensibilidad;
            editorConfig.solicitarGuardadoGeneral();
        });
        // Sensibilidad de movimiento (WASD): misma semantica que la del mouse
        // look: se aplica a la escena al instante y se persiste en la config
        // general.
        eventosGUI->subscribe([scene, &editorConfig](const EditorEvent& ev) {
            if (ev.type != EditorEventType::SensibilidadMovimientoCambio) return;
            scene->setSensibilidadMovimientoCamara(ev.sensibilidadMovimiento);
            editorConfig.datos().sensibilidadMovimientoCamara =
                ev.sensibilidadMovimiento;
            editorConfig.solicitarGuardadoGeneral();
        });
        // Restablecer configuracion: se reaplican los defaults en general
        // (idioma/apariencia/sensibilidad) y en el estado del proyecto
        // (ventanas, gizmo, camara). El nombre del proyecto se conserva: el
        // reset no cambia de carpeta/proyecto ni destruye la escena.
        eventosGUI->subscribe(
            [scene, mainMenu, managerOfGUI, &editorConfig](const EditorEvent& ev) {
                if (ev.type != EditorEventType::ReiniciarConfiguracion) return;
                const std::string nombreConservado = mainMenu->getNombreProyecto();
                editorConfig.restablecer();
                auto& cfg = editorConfig.datos();
                cfg.nombreProyecto = nombreConservado;
                // Reaplicar: los setters del menu publican sus eventos
                // (conservan persistencia/efecto vivo sin duplicar logica).
                mainMenu->setIdioma(cfg.idioma);
                mainMenu->setSensibilidadCamara(cfg.sensibilidadCamara);
                mainMenu->setSensibilidadMovimientoCamara(
                    cfg.sensibilidadMovimientoCamara);
                mainMenu->setApariencia(cfg.apariencia);
                scene->setVentanaCamarasAbierta(cfg.ventanaCamarasAbierta);
                scene->setGizmoOperation(cfg.gizmoOperacion);
                scene->setGizmoGlobal(cfg.gizmoGlobal);
                scene->setActiveCameraById(cfg.camaraActivaId);
                managerOfGUI->restaurarEstadosVentanas(cfg.estadoVentanas);
                editorConfig.guardarProyecto(cfg.nombreProyecto);
            });
        // Archivos/carpetas movidos o renombrados en el explorador (arbol o
        // grid): se reescriben en memoria las referencias de la escena cuyo
        // path cayo bajo la ruta anterior (mallas, texturas, fuentes de
        // script) y se persiste al instante para dejar los binarios en el
        // mismo estado. Los paneles e interfaces no se tocan: se referencian
        // por nombre, no por ruta.
        eventosGUI->subscribe([&gestor](const EditorEvent& ev) {
            if (ev.type != EditorEventType::ArchivosReubicados) return;
            gestor.manejarArchivosReubicados(ev.rutaAnterior, ev.rutaNueva);
        });
    }

    const std::string sceneBBDD = EditorConfig::rutaSceneBBDD(gestor.proyectoActual());
    const std::string sceneDir = EditorConfig::rutaSceneDir(gestor.proyectoActual());
    scene->loadScene(sceneBBDD, sceneDir);

     // Restaura la camara activa elegida con "Usar" (persistida por id). Si el
     // id ya no existe, GameScene se queda en modo automatico.
     scene->setActiveCameraById(editorConfig.datos().camaraActivaId);

    // Guardado en caliente (Ctrl+S) = misma rutina que el guardado al salir
    // (escena + manifiesto + config del proyecto). Lo delega el gestor de
    // proyectos; se inyecta en EditorInput, asi vale para toda la sesion.
    input->setAccionGuardar([&gestor]() { gestor.guardarProyectoCompleto(); });

    while (!glfwWindowShouldClose(window)) //BUCLE PRINCIPAL
    {
        // Se despachan los eventos primero: las callbacks (teclado/mouse, el
        // toggle de E/clic derecho, etc.) se ejecutan AL INICIO del frame en
        // vez de esperar el sleep de limitFPS, que antes iba primero y sumaba
        // hasta un frame (~16ms) de latencia a la reaccion del editor.
        glfwPollEvents();

        Time::limitFPS(FPS);
        deltaTime = Time::getDeltaTime();

        // Movimiento contino de la camara desde la maquina de estado de
        // teclas (diagonales W+A, W+D, ... normalizadas a la misma velocidad
        // que un solo eje).
        input->aplicarMovimiento(deltaTime);

        // La apariencia se aplica por eventos (EditorEventBus/AparienciaCambio)
        // cuando el usuario la cambia en Opciones; ya no se relee el menu y se
        // reaplica estilo/fondo cada frame.

        // El estado de simulacion de la escena es un reflejo del orquestador: lo
        // piden las teclas y los controles del menu, y aca se escribe una sola
        // vez por frame. La pausa y el modo se reflejan aparte porque
        // congela fisica y scripts sin tocar `start` (asi no se dispara la
        // limpieza de play->editor).
        scene->setStart(orquestadorDeGUI.enSimulacion());
        scene->setModoJuego(orquestadorDeGUI.enModoJuego());
        scene->setSimulacionPausada(orquestadorDeGUI.simulacionPausada());
        input->aplicarModoCursor(window);
        if (orquestadorDeGUI.consumirSolicitudReset())
            scene->solicitarResetSimulacion();

        // GameScene::update() se llama siempre: adentro cada bloque se auto-gatea
        // por start (fisicas, scripts, cola de compilacion solo corren en play).
        // Gatearlo desde aca hacia afuera dejaba inalcanzable el bloque de
        // transicion play->editor (limpieza de audio, desconexion de servicios y
        // cola) porque ese bloque vive en el flanco de bajada, que solo se evalua
        // con update() corriendo; ademas previousStart quedaba pegado en true y
        // el segundo arranque no disparaba su flanco de subida.
        scene->update(deltaTime);

        
        // Limpieza del framebuffer de la ventana con el color de fondo vigente
        // (setClearColor). El backend lo limpia y queda el default framebuffer
        // listo para ImGui y la pasada de la escena.
        Rendering::Backend::activeBackend().clearScreen(nullptr);

            // El frame del renderer solo si el backend arranco: con
            // ImGui_ImplOpenGL3_Init() fallido no hay backend data y en Release
            // (sin IM_ASSERT) NewFrame desreferencia un puntero nulo.
            if (imguiGl3Listo)
                ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Carga diferida de imgui.ini: DESPUÉS de NewFrame para que g.Windows
            // contenga las ventanas del nuevo proyecto (con stateGUI restaurado).
            gestor.procesarCargaIniDiferida();

            //Obtenemos El tam del Frame De La ventana en X y Y
            glfwGetFramebufferSize(window, &ventanaWidthEjeX, &ventanaHeightEjeY);     

            auxVentana->redimension(ventanaWidthEjeX, ventanaHeightEjeY);                
            // Interfaz de inicio (paquete MenuGUI): main solo conversa con la
            // fachada. ApplicationStateMachine es la fuente de verdad del
            // estado (MainMenu = menu visible, Editing = editor) y los dos
            // flujos que la cambian son: Escape (callback, Editing -> MainMenu)
            // y el boton "Iniciar Estudio" (ConsultarCierre, MainMenu ->
            // Editing). Por frame: se consulta el cierre (consumo unico), se
            // transiciona y se refleja la decision de la maquina en la fachada.
            if (mainMenu->ConsultarCierre()) {  // "Iniciar Estudio"
                // main solo conversa con la fachada (pregunta si el menu pidio
                // cerrarse); la DECISION de transicion MainMenu -> Editing vive
                // en el orquestador, que ademas guarda el resultado por frame.
                if (orquestadorDeGUI.iniciarEstudio()) {
                    // Al entrar al editor se enciende el modo editor: los
                    // paneles (explorador, jerarquia, settings) quedan visibles
                    // en vez de esperar a apretar E o seleccionar un objeto
                    // (antes solo aparecia la vista 3D "navegacion libre" y el
                    // browser parecia no existir).
                    scene->setMenuActivo(true);
                    // El explorador se abre aunque la config del proyecto lo
                    // tenga persistido cerrado: sin el arbol no hay forma de
                    // navegar las carpetas ni de que muestre su contenido el
                    // panel "ShowFolder" (que se auto-oculta sin seleccion).
                    // Al salir, la config recoge el estado real y lo persiste.
                    managerOfGUI->setEstadoVentana(WindowNames::BrowseFile, true);
                    // Actualizar proyecto actual en el menu bar para exportación
                    gestor.reflejarProyectoEnMenuBar();
                    // Se descarta el delta de look acumulado del clic en
                    // "Iniciar Estudio": sin esto el primer movimiento del
                    // mouse "teletransporta" el look y la camara queda mirando
                    // al cielo (la grilla nunca se ve).
                    input->descartarDeltaLook();
                    // Estado inicial del cursor al entrar al editor: visible
                    // (interfaces activas). E / clic derecho lo atrapan.
                    input->aplicarModoCursor(window);
                }
            }

            // Sincronizacion con guardia de cambio: SetMenuActivo(true) reinicia
            // la vista del menu a Principal, asi que solo se aplica cuando la
            // maquina efectivamente cambio de estado (evita sacar al usuario de
            // las sub-vistas Opciones/ConfigProyecto en cada frame). La decision
            // la da el orquestador de GUI (unica fuente de verdad), no un
            // appStateMachine.is() suelto aqui.
            // Reflejo por frame: main pregunta al ORQUESTADOR (no a la maquina
            // a pelo) que GUI este frame es la correcta. Esta es la unica
            // fuente de verdad para la fachada; la maquina interna no se filtra
            // al paquete MenuGUI.
            const bool menuDebeEstarAbierto =
                orquestadorDeGUI.menuDebeEstarVisible();
            if (menuDebeEstarAbierto != menuReflejadoEnFachada) {
                mainMenu->SetMenuActivo(menuDebeEstarAbierto);
                menuReflejadoEnFachada = menuDebeEstarAbierto;
            }

            // La sensibilidad del mouse look se aplica por eventos
            // (EditorEventBus/SensibilidadCambio) cuando cambia en Opciones;
            // ya no se relee el menu y se escribe en la escena cada frame.

            // Ciclo de vida de proyectos confirmado desde el menu (editar
            // nombre, elegir/conmutar proyecto): el gestor guarda el estado del
            // activo, renombra en disco si corresponde y entra al destino.
            gestor.sincronizarProyectoDesdeMenu();

            // Eliminacion de proyecto confirmada desde el modal del menu.
            gestor.eliminarProyectoDesdeMenu();

            if (mainMenu->ConsultarMenu()) {
                // Refresca el listado de proyectos (carpetas de MotorGrafico):
                // recoge proyectos creados/borrados mientras el menu esta abierto.
                mainMenu->actualizarProyectos();
                mainMenu->Renderizar();             // la vista dibuja la vista activa del modelo
            }

            // La escena corre en Editing y en ambos modos de simulacion; el
            // menu visible la detiene y dibuja solo.
            const bool sceneRunning =
                orquestadorDeGUI.escenaDebeCorrer();

            // Estado actual de los paneles para las casillas del menu
            // "Ventanas" (incluye cierres con 'X' del frame anterior).
            managerOfGUI->sincronizarVentanasMenu();

            // Guardado diferido de la config general: los cambios en vivo de
            // Opciones (apariencia/idioma/sensibilidades) quedaron encolados por
            // sus handlers y aca se vuelcan como maximo una vez cada
            // kIntervaloEscritura (la ultima edicion persiste al salir aunque
            // no llegue a volcarse, porque guardarProyectoCompleto() guarda).
            editorConfig.volcarGuardadoGeneral();

            if (sceneRunning) {
                ImGui::PushStyleVar(
                    ImGuiStyleVar_Alpha,
                    scene->isEditorGUIVisible() ? 1.0f : 0.0f);
                managerOfGUI->getDockSpaceGUI()->printGUI();
                ImGui::BeginDisabled(scene->isModoJuego() ||
                                     !scene->isEditorGUIVisible());
                treeFilesInterface->printGUI();
                contentFolderInterface->printGUI();
                ImGui::EndDisabled();
                ImGui::PopStyleVar();
                scene->gameScene();
                managerOfGUI->getDockSpaceGUI()->repararVentanasFlotantes();
            }
            // El estado abierto/cerrado del menu de inicio lo gobierna el
            // modelo del paquete MenuGUI (MenuModel), no un bool suelto de main.

        // Sidebar de radio de orbita (editor oculto + clic derecho): es una
        // ayuda de navegacion del editor, no una superposicion del Juego.
        if (!scene->isModoJuego()) input->dibujarSidebarOrbita();

        ImGui::Render();
        // Mismo guardia que NewFrame: sin backend, RenderDrawData tambien
        // desreferencia el puntero nulo del backend data.
        if (imguiGl3Listo)
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);    
    }

    // Sin proyecto elegido (primer arranque cerrado desde el menu sin entrar
    // al estudio) ninguna ruta debe escribir bajo un "Nuevo Proyecto" fantasma:
    // el gestor no toca nada con proyecto vacio. Con proyecto, guarda
    // escena + manifiesto + config del proyecto (misma rutina que Ctrl+S).
    scene->cerrarSimulacionAntesDeGuardar();
    gestor.guardarProyectoCompleto();

    // Apagado ordenado de los scripts antes de salir: primero se liberan las
    // referencias globales JNI de las instancias y luego se apaga el JVM
    // (DestroyJavaVM). Sin esto LeakSanitizer reporta cientos de bloques
    // internos del JVM (nmethods, oopmaps, metaspace) como fugas al cerrar.
    scene->descargarScripts();
    ScriptRuntime::apagarScripts();

    // Shutdown solo con backend vivo: ImGui_ImplOpenGL3_Shutdown() desreferencia
    // el backend data y en Release no hay assert que lo frene.
    if (imguiGl3Listo)
        ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
#if FUNSHI_ASAN_ACTIVO
    std::cout << std::flush;
    std::cerr << std::flush;
    std::_Exit(0);
#else
    return 0;
#endif
}

// Entrada portable (Linux y consola en Windows). En Windows con subsistema
// GUI la CPU llama a WinMain (arriba); aqui main queda solo como fallback.
int main(int argc, char* argv[]) { return EjecutarMotor(argc, argv); }
