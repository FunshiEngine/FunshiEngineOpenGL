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

// ImGui (contexto, estilo, backends) y las suscripciones al bus de la GUI;
// carga la escena del proyecto activo.
void configurarGUI(ContextoEditor& c) {
    // Estado del editor (vive en EjecutarMotor durante toda la ejecucion): los
    // nombres son referencias a sus campos, asi el codigo y las lambdas que
    // lo capturan siguen refiriendo a los mismos objetos.
    Ventana*& auxVentana = c.auxVentana;
    GLFWwindow*& window = c.window;
    GUIManager*& managerOfGUI = c.managerOfGUI;
    GameScene*& scene = c.scene;
    MenuGUI*& mainMenu = c.mainMenu;
    TreeFilesInterface*& treeFilesInterface = c.treeFilesInterface;
    ContentFolderInterface*& contentFolderInterface = c.contentFolderInterface;
    EditorInput*& input = c.input;
    EditorConfig& editorConfig = *c.editorConfig;
    GestorDeProyectos& gestor = *c.gestor;
    std::string& proyectoActual = *c.proyectoActual;
    const ProyectoInicial::Resolucion& arranque = *c.arranque;
    std::vector<ModCargado>& modsCargados = *c.modsCargados;
    bool& menuReflejadoEnFachada = *c.menuReflejadoEnFachada;
    float& FPS = c.FPS;
    bool& imguiGl3Listo = c.imguiGl3Listo;
    const std::string& proyectoCLI = c.argumentos->proyectoCLI;
    const bool& modoJuegoExportado = c.argumentos->modoJuegoExportado;
    (void)auxVentana; (void)window; (void)managerOfGUI; (void)scene; (void)mainMenu; (void)treeFilesInterface; (void)contentFolderInterface; (void)input; (void)editorConfig; (void)gestor; (void)proyectoActual; (void)arranque; (void)modsCargados; (void)menuReflejadoEnFachada; (void)FPS; (void)imguiGl3Listo; (void)proyectoCLI; (void)modoJuegoExportado;
    FPS = 60.0;    //LIMITE DE FPS
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // El contexto de ImGui se lo entrega el motor a los mods (opaco): lo
    // dibujan con su propia copia de ImGui, asi que necesitan el puntero al
    // contexto del motor para poder fijar el suyo.
    FunshiMods::fijarContextoImGui(ImGui::GetCurrentContext());

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
            Localizacion::fijarIdiomaActual(ev.idioma);
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
    imguiGl3Listo = ImGui_ImplOpenGL3_Init("#version 330 core");
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
        // Volumen maestro de efectos de sonido: se aplica al bus de audio y
        // se persiste en la configuracion general.
        eventosGUI->subscribe([scene, &editorConfig](const EditorEvent& ev) {
            if (ev.type != EditorEventType::VolumenCambio) return;
            scene->getAudioEngine()->setVolumenMaestro(ev.volumen);
            editorConfig.datos().volumenMaestro = ev.volumen;
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
                mainMenu->setVolumenMaestro(cfg.volumenMaestro);
                mainMenu->setApariencia(cfg.apariencia);
                scene->setVentanaCamarasAbierta(cfg.ventanaCamarasAbierta);
                scene->setGizmoOperation(operacionDesdeNumero(cfg.gizmoOperacion));
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
    // La dimension del proyecto se fija ANTES de cargar la escena: la siembra
    // de la grilla y la posicion de la camara inicial dependen de ella.
    scene->setDimension(editorConfig.datos().dimension);
    scene->loadScene(sceneBBDD, sceneDir);

     // Restaura la camara activa elegida con "Usar" (persistida por id). Si el
     // id ya no existe, GameScene se queda en modo automatico.
     scene->setActiveCameraById(editorConfig.datos().camaraActivaId);

    // Guardado en caliente (Ctrl+S) = misma rutina que el guardado al salir
    // (escena + manifiesto + config del proyecto). Lo delega el gestor de
    // proyectos; se inyecta en EditorInput, asi vale para toda la sesion.
    input->setAccionGuardar([&gestor]() { gestor.guardarProyectoCompleto(); });
}
