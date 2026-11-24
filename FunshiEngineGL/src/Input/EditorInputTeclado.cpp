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
#include "EditorInputDetalle.h"

void EditorInput::onKey(GLFWwindow* window, int key, int scancode, int action,
                        int mods) {
    (void)scancode;
    (void)mods;
    if (scene) scene->registrarTeclaScript(key, action);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        // Volver al menu de inicio desde el editor: la maquina de estados es
        // la fuente de verdad; main refleja su decision en la fachada MenuGUI.
        if (capturaTeclado()) {
            // Un InputText de ImGui esta activo: Escape revierte el texto en
            // edicion y no corta la edicion de datos del editor.
            trazarTeclaDescartada("Escape",
                                  std::string("la interfaz tiene el teclado (") +
                                      motivoCaptura() + ")");
        } else if (dentroDelEditor()) {
            // La regla vive en el orquestador de estados de GUI: en el editor
            // vuelve al menu y durante el play detiene la simulacion y deja el
            // editor (el menu queda para el Escape siguiente). El reflejo sobre
            // la escena (start y pausa) lo hace main en el bucle, igual que con
            // F5/F6/F7 y con los controles del menu.
            if (orquestador) orquestador->manejarTeclaEscape();
        } else {
            trazarTeclaDescartada("Escape",
                                  "la aplicacion no esta en edicion");
        }
        aplicarModoCursor(window);
        return;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS && dentroDelEditor() &&
        scene && !scene->isEditorActivo() && !modoJuegoActivo() &&
        !capturaTeclado()) {
        cursorOcultoPorC = !cursorOcultoPorC;
        mouseDerechoParaNavegar = false;
        orbitando = false;
        limpiarMovimientoCamara();
        aplicarModoCursor(window);
        return;
    }

    // Ctrl+S: guardado en caliente del proyecto (misma rutina que el guardado
    // al salir, inyectada por main). Se intercepta ANTES de la maquina de
    // movimiento para que el "S" con Ctrl no mueva la camara hacia atras.
    // Guarda SIEMPRE, tambien con un InputText enfocado: el atajo no le quita
    // nada al campo, y con el guard anterior (el mismo que Escape) renombrar un
    // objeto y guardar sin salir del campo perdia el nombre en silencio. La
    // decision vive en AtajosEditor.h, con su test.
    if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        if (accionGuardar &&
            (!orquestador || !orquestador->enModoJuego()) &&
            AtajosEditor::debeGuardar(capturaTeclado()))
            accionGuardar();
        return;
    }

    // Ctrl+Z: deshacer (undo). Se avisa en la barra de estado que cambio tomo
    // el estado (mismo aviso momentaneo que Ctrl+S), o que no hay nada que
    // deshacer, para que el atajo nunca sea silencioso. Con un campo de texto
    // enfocado el editor cede la tecla: ahi Ctrl+Z es el deshacer del campo.
    if (key == GLFW_KEY_Z && (mods & GLFW_MOD_CONTROL) &&
        !(mods & GLFW_MOD_SHIFT) && action == GLFW_PRESS) {
        if (scene && (!orquestador || !orquestador->enModoJuego()) &&
            !AtajosEditor::cedeAlCampoDeTexto(capturaTeclado())) {
            if (auto* ec = scene->getEditorController()) {
                const std::string descripcion = ec->deshacer();
                scene->mostrarMensaje(descripcion.empty()
                                          ? "Nada que deshacer"
                                          : "Deshacer: " + descripcion);
            }
        }
        return;
    }

    // Ctrl+Y: rehacer (redo). Es el UNICO atajo de redo: la "Y" con Ctrl deja de
    // ser el atajo de escala del gizmo (3/U) y no se solapa con ningun otro.
    // La "Y" suelta la tomo la guia de eje (X/Y/Z), que necesita las tres
    // letras libres para dibujar la recta del eje pulsado. Como Ctrl+Z, cede la
    // tecla al campo de texto (que tiene su propio rehacer).
    if (key == GLFW_KEY_Y && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        if (scene && (!orquestador || !orquestador->enModoJuego()) &&
            !AtajosEditor::cedeAlCampoDeTexto(capturaTeclado())) {
            if (auto* ec = scene->getEditorController()) {
                const std::string descripcion = ec->rehacer();
                scene->mostrarMensaje(descripcion.empty()
                                          ? "Nada que rehacer"
                                          : "Rehacer: " + descripcion);
            }
        }
        return;
    }

    // Ctrl+D: borrado rapido del seleccionado, sin dialogo de confirmacion.
    // Va por el gestor (BorrarObjetoComando): Ctrl+Z lo recupera. Como
    // Ctrl+Z/Y, cede ante un campo de texto (ahi la tecla es del campo) y no
    // corre en Juego. La raiz no se borra por diseno.
    if (key == GLFW_KEY_D && (mods & GLFW_MOD_CONTROL) &&
        action == GLFW_PRESS) {
        if (scene && (!orquestador || !orquestador->enModoJuego()) &&
            !AtajosEditor::cedeAlCampoDeTexto(capturaTeclado())) {
            if (auto* ec = scene->getEditorController()) {
                GameObject* seleccionado = ec->getSelectedObject();
                SceneRegistry* registro = ec->getScene();
                if (seleccionado && registro &&
                    seleccionado != registro->getRoot()) {
                    const std::string nombre = seleccionado->inputName;
                    ec->getGestorComandos()->ejecutar(
                        std::make_unique<BorrarObjetoComando>(
                            ec, seleccionado, registro));
                    scene->mostrarMensaje("Eliminar: " + nombre +
                                          " (Ctrl+Z para deshacer)");
                } else {
                    scene->mostrarMensaje("Nada que borrar");
                }
            }
        }
        return;
    }

    // F5/F6/F7: teclas de funcion de la simulacion (Depuracion/Pausa/Stop). La regla
    // por estado vive en el orquestador (funcion de marco de la arquitectura,
    // igual que Escape) y los controles del menu usan las mismas transiciones.
    // Aca solo se reenvia el evento: el reflejo de la decision sobre
    // GameScene::start y la pausa lo hace
    // main en el bucle, para que ninguna puerta escriba ese estado por su cuenta.
    // F5 y F7 salen/entran de nav libre, por eso se recalcula tambien el modo
    // del cursor.
    if ((key == GLFW_KEY_F5 || key == GLFW_KEY_F6 || key == GLFW_KEY_F7) &&
        action == GLFW_PRESS) {
        if (orquestador) {
            const auto tecla = key == GLFW_KEY_F5
                                   ? OrquestadorEstadoGUI::TeclaSimulacion::Depuracion
                                   : key == GLFW_KEY_F6
                                         ? OrquestadorEstadoGUI::TeclaSimulacion::Pausa
                                         : OrquestadorEstadoGUI::TeclaSimulacion::Stop;
            orquestador->manejarTeclaSimulacion(tecla);
            aplicarModoCursor(window);
        }
        return;
    }

    // X/Y/Z: guia de eje sobre el objeto seleccionado. Dibujan la recta del
    // eje pulsado pasando por el objeto (las otras dos coordenadas quedan
    // fijadas a las suyas) para marcar sobre que eje se puede mover, y la guia
    // se apaga con el gizmo mientras esta activa. Como las teclas son de un
    // caracter ("Y" suelta, "Z" suelta), se exigen las mismas guardas que E y
    // los atajos del gizmo: dentro del editor (tambien durante el play), sin
    // editor oculto y con el teclado libre de ImGui. Ctrl queda excluido
    // (Ctrl+Y = rehacer, Ctrl+Z = deshacer) y el estado vive en
    // EditorController, que lo comparte con el gizmo y el renderer.
    if (action == GLFW_PRESS && !(mods & GLFW_MOD_CONTROL) && !capturaTeclado()) {
        int eje = -1;
        if (key == GLFW_KEY_X) eje = 0;
        else if (key == GLFW_KEY_Y) eje = 1;
        else if (key == GLFW_KEY_Z) eje = 2;
        if (eje >= 0) {
            if (dentroDelEditor() && scene &&
                (!orquestador || !orquestador->enModoJuego()) &&
                scene->isEditorActivo() &&
                !orbitando) {
                scene->alternarGuiaEje(eje);
                // Aviso momentaneo en la barra de estado (mismo mecanismo que el
                // guardado y el undo): la guia se ve como una recta que cruza la
                // escena, asi que sin este texto no queda claro si quedo prendida
                // ni sobre que eje se puede mover el objeto.
                static const char* kNombresEje[3] = {"X", "Y", "Z"};
                const bool prendida = scene->hayGuiaEje();
                scene->mostrarMensaje(
                    prendida ? std::string("Guia de eje ") + kNombresEje[eje] +
                                   " activa: arrastra la flecha para mover el "
                                   "objeto en ese eje"
                             : std::string("Guia de eje ") + kNombresEje[eje] +
                                   " apagada");
            }
            return;
        }
    }

    // Maquina de estado de movimiento: las teclas NO mueven la camara aca;
    // solo registran si estan apretadas/sueltas y aplicarMovimiento() las
    // combina por frame. GLFW_REPEAT no cambia el estado.
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_W: teclaAdelante = true; break;
            case GLFW_KEY_S: teclaAtras = true; break;
            case GLFW_KEY_A: teclaIzquierda = true; break;
            case GLFW_KEY_D: teclaDerecha = true; break;
            case GLFW_KEY_SPACE: teclaArriba = true; break;
            case GLFW_KEY_LEFT_SHIFT: teclaAbajo = true; break;
            default: break;
        }
        // Ajustar radio de orbita con W/S durante orbita (editor oculto + clic derecho).
        // W = disminuir radio (acercarse), S = aumentar radio (alejarse).
        if (!modoJuegoActivo() && orbitando &&
            (key == GLFW_KEY_W || key == GLFW_KEY_S)) {
            const float factor = 1.01f;  // ~1% por pulsacion
            float radioAnterior = radioOrbita;
            if (key == GLFW_KEY_W) { // W = disminuir radio (acercarse)
                radioOrbita /= factor;
                if (radioOrbita < CameraComponent::radioMin) radioOrbita = CameraComponent::radioMin;
            } else if (key == GLFW_KEY_S) { // S = aumentar radio (alejarse)
                radioOrbita *= factor;
                if (radioOrbita > CameraComponent::radioMax) radioOrbita = CameraComponent::radioMax;
            }
            // Desplazar camara sobre la recta del radio (mismo logico que onScroll).
            if (CameraComponent* camara = scene ? scene->getActiveCamera() : nullptr) {
                camara->refreshFromTransform();
                const float* origen = origenOrbita;
                const float radX = camara->getYawX() * (3.14159265358979f / 180.f);
                float horizDir[3] = { std::sin(radX), 0.f, -std::cos(radX) };
                float deltaRadio = radioOrbita - radioAnterior;
                const float* pos = camara->getPosition();
                float nuevaPos[3] = {
                    pos[0] - horizDir[0] * deltaRadio,
                    pos[1],
                    pos[2] - horizDir[2] * deltaRadio
                };
                camara->setPosition(nuevaPos);
                camara->escribirATransform();
            }
            return;
        }
    } else if (action == GLFW_REPEAT) {
        // Auto-repeat: ajustar radio de orbita manteniendo W/S (solo durante orbita).
        if (!modoJuegoActivo() && orbitando &&
            (key == GLFW_KEY_W || key == GLFW_KEY_S)) {
            const float factor = 1.01f;  // ~1% por repeticion
            float radioAnterior = radioOrbita;
            if (key == GLFW_KEY_W) { // W = disminuir radio (acercarse)
                radioOrbita /= factor;
                if (radioOrbita < CameraComponent::radioMin) radioOrbita = CameraComponent::radioMin;
            } else if (key == GLFW_KEY_S) { // S = aumentar radio (alejarse)
                radioOrbita *= factor;
                if (radioOrbita > CameraComponent::radioMax) radioOrbita = CameraComponent::radioMax;
            }
            // Desplazar camara sobre la recta del radio (mismo logico que onScroll).
            if (CameraComponent* camara = scene ? scene->getActiveCamera() : nullptr) {
                camara->refreshFromTransform();
                const float* origen = origenOrbita;
                const float radX = camara->getYawX() * (3.14159265358979f / 180.f);
                float horizDir[3] = { std::sin(radX), 0.f, -std::cos(radX) };
                float deltaRadio = radioOrbita - radioAnterior;
                const float* pos = camara->getPosition();
                float nuevaPos[3] = {
                    pos[0] - horizDir[0] * deltaRadio,
                    pos[1],
                    pos[2] - horizDir[2] * deltaRadio
                };
                camara->setPosition(nuevaPos);
                camara->escribirATransform();
            }
            return;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_W: teclaAdelante = false; break;
            case GLFW_KEY_S: teclaAtras = false; break;
            case GLFW_KEY_A: teclaIzquierda = false; break;
            case GLFW_KEY_D: teclaDerecha = false; break;
            case GLFW_KEY_SPACE: teclaArriba = false; break;
            case GLFW_KEY_LEFT_SHIFT: teclaAbajo = false; break;
            default: break;
        }
        return;
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        // Durante orbita: bloquear toggle de editor (E).
        if (orbitando) {
            trazarTeclaDescartada("E", "en orbita");
            return;
        }
        // Dentro del editor o del play: desde el menu de inicio la E no debe
        // "activar el editor" mostrando sus interfaces sobre el menu (fallo de
        // la maquina de estados). Con un InputText de ImGui activo, E tampoco
        // toca las interfaces.
        if (dentroDelEditor() && !capturaTeclado()) {
            if (scene) scene->toggleEditorInterfaces();
            cursorOcultoPorC = false;
            mouseDerechoParaNavegar = false;
            orbitando = false;
            aplicarModoCursor(window);
        } else if (!dentroDelEditor()) {
            trazarTeclaDescartada("E", "la aplicacion no esta en edicion");
        } else {
            trazarTeclaDescartada("E",
                                  std::string("la interfaz tiene el teclado (") +
                                      motivoCaptura() + ")");
        }
    }

    // G: alterna el sistema de coordenadas del gizmo entre LOCAL (ejes que
    // rotan con el objeto) y GLOBAL (ejes del mundo, el gizmo no rota).
    if (key == GLFW_KEY_G && action == GLFW_PRESS &&
        (!orquestador || !orquestador->enModoJuego())) {
        if (scene) scene->setGizmoGlobal(!scene->isGizmoGlobal());
    }

    // Atajos de operacion del gizmo (1/T, 2/R, 3/U): con Ctrl pulsada la tecla
    // pertenece a otro atajo (p. ej. Ctrl+Y = rehacer), asi que no se aplican.
    // La escala quedo en "U" (no en "Y") para dejar las tres letras de los ejes
    // (X/Y/Z) libres de atajos y dedicadas a la guia de eje.
    //
    // Elegir una operacion apaga la guia de eje: el usuario acaba de pedir
    // rotar o escalar, o mover en los tres ejes, asi que el "solo este eje" ya
    // no aplica y dejarlo prendido lo dejaria escribiendo en un unico eje
    // mientras el gizmo muestra el otro (o ninguna flecha, si pidio escalar).
    if (action == GLFW_PRESS && !(mods & GLFW_MOD_CONTROL) &&
        (!orquestador || !orquestador->enModoJuego())) {
        if (key == GLFW_KEY_1 || key == GLFW_KEY_T) {
            if (scene) {
                scene->clearGuiaEje();
                scene->setGizmoOperation(OperacionGizmo::Trasladar);
            }
        } else if (key == GLFW_KEY_2 || key == GLFW_KEY_R) {
            if (scene) {
                scene->clearGuiaEje();
                scene->setGizmoOperation(OperacionGizmo::Rotar);
            }
        } else if (key == GLFW_KEY_3 || key == GLFW_KEY_U) {
            if (scene) {
                scene->clearGuiaEje();
                scene->setGizmoOperation(OperacionGizmo::Escalar);
            }
        }
    }
}
