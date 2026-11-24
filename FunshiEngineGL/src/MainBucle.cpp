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

// Bucle principal del editor.
void ejecutarBucle(ContextoEditor& c) {
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
    while (!glfwWindowShouldClose(window)) //BUCLE PRINCIPAL
    {
        // Se despachan los eventos primero: las callbacks (teclado/mouse, el
        // toggle de E/clic derecho, etc.) se ejecutan AL INICIO del frame en
        // vez de esperar el sleep de limitFPS, que antes iba primero y sumaba
        // hasta un frame (~16ms) de latencia a la reaccion del editor.
        glfwPollEvents();

#ifdef FUNSHI_CEF
        // Bucle de mensajes del motor web (external_message_pump): una vez por
        // frame, en el hilo principal. Sin CEF es no-op.
        WebUIRuntime::bombear();
#endif

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

        // Actualizacion de los mods cargados: una vez por frame, cada mod que
        // implemente update() recibe el delta del frame. El nombre del
        // proyecto se refresca aqui porque puede cambiar desde el menu de
        // inicio (los mods lo leen de la tabla de servicios).
        CargadorMods::actualizar(modsCargados, deltaTime);
        FunshiMods::fijarNombreProyecto(gestor.proyectoActual().c_str());

        // Limpieza del framebuffer de la ventana con el color de fondo vigente
        // (setClearColor). El backend lo limpia y queda el default framebuffer
        // listo para ImGui y la pasada de la escena. El color sale del perfil
        // de apariencia de la escena, el mismo que usa el juego exportado.
        FondoEscena::limpiar(Rendering::Backend::activeBackend(),
                             scene->getApariencia());

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

            // Importacion pedida desde Configurar proyecto (copia en disco).
            gestor.importarProyectoDesdeMenu();

            if (mainMenu->ConsultarMenu()) {
                // Refresca el listado de proyectos (carpetas de FunshiEngine-DATA):
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
                // Paneles que aportan los mods cargados: cada uno es una
                // ventana propia que el motor dibuja alrededor del callback
                // que entrego el mod.
                managerOfGUI->getPanelesModsGUI()->printGUI();
                managerOfGUI->getDockSpaceGUI()->repararVentanasFlotantes();
            }
            // El estado abierto/cerrado del menu de inicio lo gobierna el
            // modelo del paquete MenuGUI (MenuModel), no un bool suelto de main.

        // Sidebar de radio de orbita (editor oculto + clic derecho): es una
        // ayuda de navegacion del editor, no una superposicion del Juego. La
        // dibuja la GUI con el estado que entrega el input.
        if (!scene->isModoJuego())
            managerOfGUI->getSidebarOrbitaGUI()->dibujar(input->estadoOrbita());

        ImGui::Render();
        // Mismo guardia que NewFrame: sin backend, RenderDrawData tambien
        // desreferencia el puntero nulo del backend data.
        if (imguiGl3Listo)
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);    
    }
}

// Cierre ordenado: guardado, scripts, motor web, mods, ImGui y GLFW.
int cerrarEditor(ContextoEditor& c) {
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

    // Apagado del motor web (CefShutdown): despues de descargar las vistas
    // (las libera GameScene) y antes de romper ImGui/GL. Sin CEF es no-op.
#ifdef FUNSHI_CEF
    WebUIRuntime::apagar();
#endif

    // Apagado de los mods: cada uno recibe su shutdown() y despues se cierra
    // su binario, en orden inverso al de carga. Va antes de romper ImGui: un
    // mod puede soltar recursos de GUI en su shutdown.
    CargadorMods::descargar(modsCargados);
    // El contexto de ImGui deja de ser valido: se retira de la tabla de
    // servicios antes de destruirlo.
    FunshiMods::fijarContextoImGui(nullptr);

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
