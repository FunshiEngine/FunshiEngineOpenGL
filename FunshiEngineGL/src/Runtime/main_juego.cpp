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

// Punto de entrada DEDICADO para el JUEGO EXPORTADO.
// NO incluye codigo del editor (GUIManager, MenuGUI, paneles, atajos de editor).
// Solo lo que corre en modo "Play": escena, fisica, scripts, render, input de juego.

// Windows.h ANTES del header propio y de la stdlib, para que _HAS_STD_BYTE=0
// surta efecto antes de que la stdlib defina std::byte (mismo baile que
// Proceso.cpp y TerminalSession.cpp). Ademas, windows.h declara un "byte" global
// que, con el "using namespace std;" de las estructuras del motor, choca con
// std::byte: si se incluye despues, el archivo no compila.
#if defined(_WIN32)
#define _HAS_STD_BYTE 0
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "../Scenes/GameScene.h"
#include "../Configuracion/EditorConfig.h"
#include "../Configuracion/ProjectPaths.h"
#include "../Configuracion/RutasLog.h"
#include "../Exportador/ConfigJuegoExportado.h"
#include "../EngineTime.h"
#include "../Ventana.h"
#include "../Input/EditorInput.h"
#include "../GUI/Estado/EstadoInterfazImGui.h"
#include "../Hilos/HiloActual.h"
#include "../Rendering/Backend/IRenderBackend.h"
#include "../Rendering/FondoEscena.h"
#include "../Behaviour/ScriptRuntime.h"
#include "../States/ApplicationStateMachine.h"
#include "../States/OrquestadorEstadoGUI.h"
#ifdef FUNSHI_CEF
#include "../WebUI/WebUIRuntime.h"
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

static FILE* g_logSalida = nullptr;
static int ventanaHeightEjeY, ventanaWidthEjeX;

// Copiado de main.cpp: redirige stdout/stderr a log con timestamp UTC
static std::string redirigirSalidaALog(char* argv0) {
    const fs::path ejecutable =
        argv0 && argv0[0] != '\0' ? fs::path(argv0).parent_path() : fs::path(".");
    std::error_code ec;
    const fs::path temporal = fs::temp_directory_path(ec);

    const std::time_t ahora = std::time(nullptr);
    const std::tm tmUtc = *std::gmtime(&ahora);
    char sufijo[32];
    std::strftime(sufijo, sizeof(sufijo), "%Y%m%d_%H%M%S", &tmUtc);
    const std::string nombre = "JuegoExportado_" + std::string(sufijo) + ".log";

    for (const std::string& carpeta :
         RutasLog::candidatas(ProjectPaths::directorioBase(),
                              ejecutable.string(), temporal.string())) {
        try {
            fs::create_directories(carpeta);
            const std::string ruta = (fs::path(carpeta) / nombre).string();
            g_logSalida = std::freopen(ruta.c_str(), "a", stdout);
            if (!g_logSalida) continue;
            // stderr va al mismo archivo. Si su redireccion falla, el log
            // sigue saliendo por stdout: no es motivo para no arrancar.
            (void)std::freopen(ruta.c_str(), "a", stderr);
            std::cout << "\n========== Arranque JuegoExportado " << sufijo
                      << " ==========\n";
            std::cout << "Log en: " << ruta << "\n";
            return ruta;
        } catch (...) {
            g_logSalida = nullptr;
        }
    }
    return std::string();
}

// Windows GUI entry point (sin consola)
#ifdef _WIN32
static int EjecutarJuego(int argc, char* argv[]);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // argc lo cuenta CommandLineToArgvW: el cuarto parametro de la firma de
    // WinMain es como se muestra la ventana, no el numero de argumentos.
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<char*> argvAnsi;
    if (wargv) {
        for (int i = 0; i < argc; ++i) {
            int largo = WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
            if (largo > 0) {
                std::string s(largo, '\0');
                WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, &s[0], largo, nullptr, nullptr);
                s.pop_back();
                argvAnsi.push_back(strdup(s.c_str()));
            }
        }
        LocalFree(wargv);
    }
    int ret = EjecutarJuego(static_cast<int>(argvAnsi.size()), argvAnsi.data());
    for (auto p : argvAnsi) free(p);
    return ret;
}
#endif

// Entry point principal (Linux y fallback Windows)
int EjecutarJuego(int argc, char* argv[]) {
#ifdef FUNSHI_CEF
    // Igual que en el editor: si CEF relanzo este binario como subproceso, se
    // ejecuta ese rol y no se arranca el juego.
    {
        const int codigoSubproceso = WebUIRuntime::ejecutarSubproceso(argc, argv);
        if (codigoSubproceso >= 0) return codigoSubproceso;
    }
#else
    (void)argc; (void)argv; // En exportacion no se usan argumentos CLI
#endif

    // 1. Redirigir salida al log
    std::string rutaLog = redirigirSalidaALog(argc > 0 ? argv[0] : nullptr);
    (void)rutaLog;

    // El hilo principal del motor queda registrado: el contexto de OpenGL,
    // ImGui, CEF y la JVM solo se pueden usar desde aqui (ver Hilos/
    // HiloActual.h).
    Hilos::fijarHiloPrincipal();

    // 2. Registrar el callback de errores de GLFW antes de crear la ventana.
    // Ventana::initVentana llama a glfwInit(); no se duplica aqui.
    glfwSetErrorCallback([](int error, const char* desc) {
        std::cerr << "[GLFW] error " << error << ": " << desc << '\n';
    });

    // 3. Crear ventana (Ventana llama a glfwInit internamente, crea la ventana,
    // inicializa el contexto 3.3 core y carga las funciones de OpenGL).
    Ventana* auxVentana = new Ventana();
    // Sin ventana no hay contexto de OpenGL ni entrada: seguir adelante
    // dejaria el motor operando sobre una ventana nula.
    if (auxVentana->initVentana() != 0) {
        std::cerr << "No se pudo crear la ventana de OpenGL 3.3; el juego "
                     "no puede arrancar\n";
        delete auxVentana;
        return -1;
    }
    GLFWwindow* window = auxVentana->getWindow();

    // 4. ImGui init (contexto básico, SIN GUIManager del editor)
    // En juego exportado NO se crean GUIManager, MenuGUI, TreeFilesInterface, etc.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr; // No guardar imgui.ini en juego exportado

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // El contexto OpenGL es 3.3 core (ver Ventana::initVentana); el GLSL
    // version string debe coincidir. "#version 460" provocaba que ImGui no
    // pudiera compilar sus shaders internos y el frame quedaba en negro.
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // 5. GameScene (nucleo: render, fisica, scripts, input)
    // El constructor que NO requiere GUIManager - solo para runtime. El backend
    // de render se le pasa igual: sin el, la escena no tiene con que dibujar.
    GameScene* scene = new GameScene(nullptr,
                                     Rendering::Backend::activeBackend());

    // 6. Leer manifiesto JuegoExportado.json junto al binario para obtener metadatos
    // El juego exportado NO replica la estructura FunshiEngine-DATA/Proyects: las escenas
    // y assets están directamente en scenes/ y assets/ junto al ejecutable.
    std::string dirEjecutable = ProjectPaths::directorioEjecutable();
    std::string nombreProyecto;
    bool modoJuego = true;
    if (!ConfigJuegoExportado::leer(ConfigJuegoExportado::rutaPorDefecto(dirEjecutable), nombreProyecto, modoJuego)) {
        std::cerr << "Advertencia: no se encontro JuegoExportado.json, usando configuracion por defecto\n";
        nombreProyecto = "JuegoExportado";
    }

    // 7. Crear directorios de trabajo si no existen (scripts y cache)
    const std::string dirEjecutableConBarra = dirEjecutable.back() == '/' || dirEjecutable.back() == '\\' 
        ? dirEjecutable : dirEjecutable + "/";
    const std::string scriptsDir = dirEjecutableConBarra + "scripts";
    const std::string cacheDir = dirEjecutableConBarra + "cache";
    
    std::error_code ec;
    fs::create_directories(scriptsDir, ec);
    fs::create_directories(cacheDir, ec);

    // 8. Cargar escena desde la estructura simplificada del juego exportado
    // Las escenas están en scenes/ junto al ejecutable, NO en FunshiEngine-DATA/Proyects.
    // Las rutas de assets de la escena se guardan RELATIVAS a la raiz del
    // juego (p. ej. "Assets/Modelos/Cube.obj"): se fija esa raiz ANTES de
    // cargar, para que se resuelvan contra el directorio del juego y no
    // contra el cwd desde donde se lanzo (que dejaria las mallas sin
    // encontrar y la pantalla en negro).
    EditorConfig::fijarRaizAssets(dirEjecutable);
    const std::string sceneBBDD = dirEjecutableConBarra + "scenes/Scene.db";
    const std::string sceneDir = dirEjecutableConBarra + "scenes/SceneDir";
    
    if (fs::exists(sceneBBDD)) {
        scene->loadScene(sceneBBDD, sceneDir);
        std::cout << "[juego] Escena cargada desde: " << sceneBBDD << '\n';
    } else {
        std::cerr << "[juego] ADVERTENCIA: No se encontro Scene.db en: " << sceneBBDD << '\n';
        std::cerr << "[juego] Iniciando con escena vacia (el juego seguira ejecutandose)\n";
        // No cargar escena - GameScene arranca con una escena vacía por defecto
    }

    // configurarProyecto() asegura la estructura de carpetas del proyecto
    // (Memory/ y src<nombre>/). Debe llamarse DESPUES de loadScene para que la
    // escena ya exista.
    scene->configurarProyecto(nombreProyecto);

    // 9. Maquina de estados y orquestador
    ApplicationStateMachine appStateMachine;
    OrquestadorEstadoGUI orquestadorDeGUI(&appStateMachine);

    // Estado base del pipeline (test de profundidad, multisample): el editor
    // lo deja igual al arrancar la ventana.
    Rendering::Backend::activeBackend().applyBaseState();

    // 10. Entrar DIRECTAMENTE en modo Playing (Juego)
    appStateMachine.transitionTo(ApplicationState::Playing);
    scene->setModoJuego(true);

    // 11. Input para juego (EditorInput maneja input de juego SIN GUI de editor).
    // El estado de la interfaz se inyecta: el juego exportado si tiene contexto
    // de ImGui (para el HUD web), asi que el input puede preguntar si el
    // teclado es de un widget.
    EstadoInterfazImGui estadoInterfaz;
    EditorInput* input = new EditorInput(scene, &appStateMachine,
                                          &orquestadorDeGUI, &estadoInterfaz);

    // 12. Registrar callbacks de input (antes del loop)
    input->registrarCallbacks(window);
    input->aplicarModoCursor(window);

    // 13. Bucle principal del juego
    Time::start();

#ifdef FUNSHI_CEF
    // Motor web: mismo ciclo que el editor (init una vez, pump por frame,
    // apagado al salir). Sin runtime CEF cae al nulo y el juego sigue.
    if (!WebUIRuntime::inicializar(argc, argv)) {
        std::cout << "[webui] Sin backend web real en el juego exportado."
                  << std::endl;
    }
#endif

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
#ifdef FUNSHI_CEF
        WebUIRuntime::bombear();
#endif
        Time::update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // El juego exportado siempre corre en modo Playing: setStart(true)
        // habilita el avance de la fisica y los scripts cada frame. En
        // main.cpp este valor lo refleja el orquestador de estados; aqui se
        // fija directamente porque el estado nunca cambia a Editing.
        scene->setStart(true);
        scene->setModoJuego(true);

        // update() avanza la fisica, los scripts y las animaciones.
        // gameScene() dibuja el frame con el estado ya actualizado.
        scene->update(Time::getDeltaTime());

        // El frame arranca limpio: el color de fondo del perfil y el buffer de
        // profundidad a cero. Sin esta limpieza el test de profundidad
        // compara contra el valor inicial del buffer (que no es 1.0 en todos
        // los drivers) y ni el cielo ni la geometria se dibujan: la pantalla
        // queda en negro. El editor hace lo mismo desde el arranque del
        // frame; el juego exportado lo hace aqui.
        FondoEscena::limpiar(Rendering::Backend::activeBackend(),
                             scene->getApariencia());

        scene->gameScene();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 14. Cleanup ordenado (igual que main.cpp, sin GUIManager)
    scene->cerrarSimulacionAntesDeGuardar();
    scene->descargarScripts();
    ScriptRuntime::apagarScripts();

#ifdef FUNSHI_CEF
    WebUIRuntime::apagar();
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete input;
    delete scene;
    delete auxVentana;
    glfwTerminate();
    return 0;
}

#ifndef _WIN32
int main(int argc, char* argv[]) { return EjecutarJuego(argc, argv); }
#endif
