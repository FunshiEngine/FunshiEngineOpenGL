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

// Estado global de la aplicacion (declarado en MainDetalle.h).
int ventanaHeightEjeY, ventanaWidthEjeX;
bool recFilesInit = true;
ApplicationStateMachine appStateMachine;
OrquestadorEstadoGUI orquestadorDeGUI(&appStateMachine);
float deltaTime = 0.0f;
FILE* g_logSalida = nullptr;

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
    ArgumentosEditor argumentos;
    const int codigoArranque = prepararArranque(argc, argv, argumentos);
    if (codigoArranque >= 0) return codigoArranque;
    const std::string& proyectoCLI = argumentos.proyectoCLI;
    const bool modoJuegoExportado = argumentos.modoJuegoExportado;

    Ventana* auxVentana = new Ventana();
    // Sin ventana no hay contexto de OpenGL ni entrada: seguir adelante
    // dejaria el motor callingando sobre una ventana nula. Se corta el
    // arranque con un mensaje claro.
    if (auxVentana->initVentana() != 0) {
        std::cerr << "No se pudo crear la ventana de OpenGL 3.3; el motor "
                     "no puede arrancar\n";
        delete auxVentana;
        return -1;
    }
    GLFWwindow* window = auxVentana->getWindow();
    GUIManager* managerOfGUI = new GUIManager(window);
    GameScene* scene = new GameScene(
        managerOfGUI, Rendering::Backend::activeBackend());
    MenuGUI* mainMenu = managerOfGUI->getMenuGUI();
    TreeFilesInterface* treeFilesInterface = managerOfGUI->getTreeFilesGUI();
    ContentFolderInterface* contentFolderInterface = managerOfGUI->getContentFolderGUI();
    // Callbacks del editor (teclado/mouse) + navegacion de la camara: su
    // propio modulo (Input/EditorInput) las traduce a acciones y mantiene la
    // maquina de estado de movimiento (diagonales WASD normalizadas). El
    // estado de la interfaz se le inyecta: el input no sabe que hay una GUI
    // debajo, solo pregunta si el teclado es de un widget.
    EditorInput* input =
        new EditorInput(scene, &appStateMachine, &orquestadorDeGUI,
                        managerOfGUI->getEstadoInterfaz());
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

    // Descubrimiento y carga de mods: se leen los .fmod de la carpeta global
    // del motor y, si hay proyecto abierto, los de su carpeta. El resultado se
    // reporta al registro de diagnosticos (panel de estado/log). Los paquetes
    // validos se cargan de verdad: su binario se extrae a la cache y se abre,
    // y el mod recibe su inicializacion. Las carpetas se crean en
    // asegurarEstructuraBase / al crear el proyecto.
    //
    // La lista de mods cargados vive hasta el final del proceso: se descargan
    // en orden inverso al salir (cada mod apaga sus recursos antes de que su
    // binario se cierre).
    std::vector<ModCargado> modsCargados;
    {
        std::vector<CarpetaMods> carpetasMods = {
            {ProjectPaths::directorioMods(), AmbitoMod::Motor}};
        if (gestor.hayProyecto()) {
            carpetasMods.push_back({ProjectPaths::directorioModsProyecto(
                                        gestor.proyectoActual()),
                                    AmbitoMod::Proyecto});
        }
        const GestorMods::Resultado modsDescubiertos =
            GestorMods::descubrir(carpetasMods);
        GestorMods::reportar(modsDescubiertos);

        const CargadorMods::ResultadoCarga modsCargadosResultado =
            CargadorMods::cargar(modsDescubiertos);
        CargadorMods::reportar(modsCargadosResultado);
        modsCargados = modsCargadosResultado.modsCargados;
    }

    // Los mods consultan los datos de la escena (objetos, componentes) a
    // traves de la tabla de servicios: se conecta la fuente de datos real y
    // el nombre del proyecto abierto.
    FunshiMods::inyectarEscenaMods(scene);
    FunshiMods::inyectarDatosEscena(FunshiMods::fuenteDatosEscenaMods());
    FunshiMods::fijarNombreProyecto(gestor.proyectoActual().c_str());

    if (gestor.hayProyecto())
        gestor.prepararProyectoAlArrancar();

    mainMenu->setNombreProyecto(proyectoActual);
    Localizacion::fijarIdiomaActual(editorConfig.datos().idioma);
    mainMenu->setIdioma(editorConfig.datos().idioma);
    mainMenu->setSensibilidadCamara(editorConfig.datos().sensibilidadCamara);
    mainMenu->setSensibilidadMovimientoCamara(
        editorConfig.datos().sensibilidadMovimientoCamara);
    mainMenu->setVolumenMaestro(editorConfig.datos().volumenMaestro);

    mainMenu->setApariencia(editorConfig.datos().apariencia);
    scene->setVentanaCamarasAbierta(editorConfig.datos().ventanaCamarasAbierta);
    scene->setSensibilidadMovimientoCamara(
        editorConfig.datos().sensibilidadMovimientoCamara);
    scene->setGizmoOperation(
        operacionDesdeNumero(editorConfig.datos().gizmoOperacion));
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

    ContextoEditor c;
    c.auxVentana = auxVentana;
    c.window = window;
    c.managerOfGUI = managerOfGUI;
    c.scene = scene;
    c.mainMenu = mainMenu;
    c.treeFilesInterface = treeFilesInterface;
    c.contentFolderInterface = contentFolderInterface;
    c.input = input;
    c.editorConfig = &editorConfig;
    c.gestor = &gestor;
    c.proyectoActual = &proyectoActual;
    c.arranque = &arranque;
    c.modsCargados = &modsCargados;
    c.menuReflejadoEnFachada = &menuReflejadoEnFachada;
    c.argumentos = &argumentos;

    configurarGUI(c);
    ejecutarBucle(c);
    return cerrarEditor(c);
}

// Entrada portable (Linux y consola en Windows). En Windows con subsistema
// GUI la CPU llama a WinMain (arriba); aqui main queda solo como fallback.
int main(int argc, char* argv[]) { return EjecutarMotor(argc, argv); }
