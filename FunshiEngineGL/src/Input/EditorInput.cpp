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
#define GLFW_INCLUDE_NONE
#include "EditorInput.h"

#include <cmath>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "../Scenes/GameScene.h"
#include "../Objetos/Componentes/CameraComponent.h"
#include "../States/ApplicationStateMachine.h"
#include "../States/OrquestadorEstadoGUI.h"
#include "ImGuizmo.h"
#include "../Scenes/EditorController.h"

EditorInput* EditorInput::instancia = nullptr;

EditorInput::EditorInput(GameScene* escena, ApplicationStateMachine* maquina,
                         OrquestadorEstadoGUI* orquestador)
    : scene(escena), appState(maquina), orquestador(orquestador) {
    instancia = this;
}

void EditorInput::registrarCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, EditorInput::teclado_callback);
    glfwSetCursorPosCallback(window, EditorInput::mouse_callback);
    glfwSetMouseButtonCallback(window, EditorInput::mouse_button_callback);
    glfwSetScrollCallback(window, EditorInput::scroll_callback);
}

void EditorInput::teclado_callback(GLFWwindow* window, int key, int scancode,
                                   int action, int mods) {
    if (instancia) instancia->onKey(window, key, scancode, action, mods);
}

void EditorInput::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (instancia) instancia->onMouse(window, xpos, ypos);
}

void EditorInput::mouse_button_callback(GLFWwindow* window, int button,
                                        int action, int mods) {
    if (instancia) instancia->onMouseButton(window, button, action, mods);
}

void EditorInput::scroll_callback(GLFWwindow* window, double xoffset,
                                  double yoffset) {
    if (instancia) instancia->onScroll(window, xoffset, yoffset);
}

void EditorInput::aplicarModoCursor(GLFWwindow* window) {
    if (!window || !scene || !appState) return;
    const bool enEditor = appState->is(ApplicationState::Editing);
    const bool ocultar =
        enEditor && (mouseDerechoParaNavegar || !scene->isEditorActivo());
    const int modo = ocultar ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    if (glfwGetInputMode(window, GLFW_CURSOR) != modo) {
        glfwSetInputMode(window, GLFW_CURSOR, modo);
        // Al atrapar/soltar el cursor (GLFW_CURSOR_DISABLED mueve el cursor
        // virtual al centro pero el OS queda donde estaba) se descarta el
        // delta acumulado del look.
        firstTimeMouseX = true;
        firstTimeMouseY = true;
    }
}

void EditorInput::descartarDeltaLook() {
    firstTimeMouseX = true;
    firstTimeMouseY = true;
}

void EditorInput::setAccionGuardar(std::function<void()> accion) {
    accionGuardar = std::move(accion);
}

void EditorInput::aplicarMovimiento(float deltaTime) {
    // La camara del editor solo se mueve con WASD/Espacio/Shift dentro del
    // editor (ni en el menu ni cuando ImGui esta capturando el teclado, p. ej.
    // mientras se edita un InputText).
    if (!scene || !appState || !appState->is(ApplicationState::Editing)) return;
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    // Durante orbita (editor oculto + clic derecho): bloquear traslacion WASD.
    if (orbitando) return;
    // Misma regla que la mirada (EditorInput::onMouse): con las interfaces del
    // editor visibles el WASD no traslada la camara; hace falta ocultarlas (E)
    // o navegar con el clic derecho sostenido sobre el viewport. Sin esto se
    // podia volar por la escena sin "esconder" el editor.
    if (scene->isEditorActivo() && !mouseDerechoParaNavegar) return;

    CameraComponent* camara = scene->getActiveCamera();
    if (!camara) return;

    // Vector de direccion combinado [derecha, arriba, adelante] a partir de la
    // maquina de estado de teclas. Las diagonales (W+A, W+D, etc.) suman ejes
    // y el vector se NORMALIZA cuando el modulo supera 1: mover dos ejes a la
    // vez queda a la misma velocidad que un eje solo (antes, W y D eran cada
    // una una callback que escribia el Transform -> diagonal a sqrt(2) y con
    // reinicios de transform).
    float x = 0.f, y = 0.f, z = 0.f;
    if (teclaDerecha) x += 1.0f;
    if (teclaIzquierda) x -= 1.0f;
    if (teclaArriba) y += 1.0f;
    if (teclaAbajo) y -= 1.0f;
    if (teclaAdelante) z += 1.0f;
    if (teclaAtras) z -= 1.0f;
    if (x == 0.f && y == 0.f && z == 0.f) return;

    const float modulo =
        std::sqrt(x * x + y * y + z * z);
    const float factor = modulo > 1.0f ? 1.0f / modulo : 1.0f;
    const float direccion[3] = {x * factor, y * factor, z * factor};

    const float dt = deltaTime * scene->getSensibilidadMovimientoCamara();
    camara->moverDireccion(direccion, dt);
}

void EditorInput::onKey(GLFWwindow* window, int key, int scancode, int action,
                        int mods) {
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        // Volver al menu de inicio desde el editor: la maquina de estados es
        // la fuente de verdad; main refleja su decision en la fachada MenuGUI.
        if (ImGui::GetIO().WantCaptureKeyboard) {
            // Un InputText de ImGui esta activo: Escape revierte el texto en
            // edicion y no corta la edicion de datos del editor.
        } else if (appState && appState->is(ApplicationState::Editing)) {
            // La regla vive en el orquestador de estados de GUI.
            orquestador->manejarTeclaEscape();
        }
        aplicarModoCursor(window);
        return;
    }

    // Ctrl+S: guardado en caliente del proyecto (misma rutina que el guardado
    // al salir, inyectada por main). Se intercepta ANTES de la maquina de
    // movimiento para que el "S" con Ctrl no mueva la camara hacia atras, y
    // con el mismo guard que Escape: si un campo de texto de ImGui esta
    // capturando el teclado, la combinacion es del editor de texto.
    if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        if (accionGuardar && !ImGui::GetIO().WantCaptureKeyboard) accionGuardar();
        return;
    }

    // Ctrl+Z: deshacer (undo). Se avisa en la barra de estado que cambio tomo
    // el estado (mismo aviso momentaneo que Ctrl+S), o que no hay nada que
    // deshacer, para que el atajo nunca sea silencioso.
    if (key == GLFW_KEY_Z && (mods & GLFW_MOD_CONTROL) &&
        !(mods & GLFW_MOD_SHIFT) && action == GLFW_PRESS) {
        if (scene && !ImGui::GetIO().WantCaptureKeyboard) {
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
    // letras libres para dibujar la recta del eje pulsado.
    if (key == GLFW_KEY_Y && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        if (scene && !ImGui::GetIO().WantCaptureKeyboard) {
            if (auto* ec = scene->getEditorController()) {
                const std::string descripcion = ec->rehacer();
                scene->mostrarMensaje(descripcion.empty()
                                          ? "Nada que rehacer"
                                          : "Rehacer: " + descripcion);
            }
        }
        return;
    }

    // F5/F6/F7: teclas de funcion de la simulacion (Play/Pausa/Stop). La regla
    // por estado vive en el orquestador (funcion de marco de la arquitectura,
    // igual que Escape); aca solo se reenvia el evento y se refleja la decision
    // sobre GameScene::start (el mismo flag que maneja el boton Activar/Detener
    // del menu de escena, asi ambas puertas comparten estado). F5 y F7 salen
    // entran de nav libre, por eso se recalcula tambien el modo del cursor.
    if ((key == GLFW_KEY_F5 || key == GLFW_KEY_F6 || key == GLFW_KEY_F7) &&
        action == GLFW_PRESS) {
        if (orquestador) {
            const auto tecla = key == GLFW_KEY_F5
                                   ? OrquestadorEstadoGUI::TeclaSimulacion::Play
                                   : key == GLFW_KEY_F6
                                         ? OrquestadorEstadoGUI::TeclaSimulacion::Pausa
                                         : OrquestadorEstadoGUI::TeclaSimulacion::Stop;
            orquestador->manejarTeclaSimulacion(tecla);
            // Refleja Playing/Editing sobre la simulacion de la escena (F5
            // arranca, F7 corta; F5 con la maquina ya en Playing re-asegura el
            // arranque despues de un "Detener" con el boton del menu de escena).
            if (scene) scene->setStart(orquestador->enSimulacion());
            // La pausa (F6) tambien se refleja: congela fisica/scripts sin
            // tocar `start` (asi no se dispara la limpieza de play->editor).
            if (scene)
                scene->setSimulacionPausada(orquestador->simulacionPausada());
            aplicarModoCursor(window);
        }
        return;
    }

    // X/Y/Z: guia de eje sobre el objeto seleccionado. Dibujan la recta del
    // eje pulsado pasando por el objeto (las otras dos coordenadas quedan
    // fijadas a las suyas) para marcar sobre que eje se puede mover, y la guia
    // se apaga con el gizmo mientras esta activa. Como las teclas son de un
    // caracter ("Y" suelta, "Z" suelta), se exigen las mismas guardas que E y
    // los atajos del gizmo: solo en edicion, sin editor oculto y con el
    // teclado libre de ImGui. Ctrl queda excluido (Ctrl+Y = rehacer,
    // Ctrl+Z = deshacer) y el estado vive en EditorController, que lo comparte
    // con el gizmo y el renderer.
    if (action == GLFW_PRESS && !(mods & GLFW_MOD_CONTROL) && !ImGui::GetIO().WantCaptureKeyboard) {
        int eje = -1;
        if (key == GLFW_KEY_X) eje = 0;
        else if (key == GLFW_KEY_Y) eje = 1;
        else if (key == GLFW_KEY_Z) eje = 2;
        if (eje >= 0) {
            if (appState && appState->is(ApplicationState::Editing) && scene &&
                scene->isEditorActivo() && !orbitando) {
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
        if (orbitando) return;
        // Solo en el estado de edicion: desde el menu de inicio la E no debe
        // "activar el editor" mostrando sus interfaces sobre el menu (fallo de
        // la maquina de estados). Con un InputText de ImGui activo, E tampoco
        // toca las interfaces.
        if (appState && appState->is(ApplicationState::Editing) &&
            !ImGui::GetIO().WantCaptureKeyboard) {
            if (scene) scene->toggleEditorInterfaces();
            // Entrar/salir de navegacion libre: captura y oculta el cursor.
            aplicarModoCursor(window);
        }
    }

    // G: alterna el sistema de coordenadas del gizmo entre LOCAL (ejes que
    // rotan con el objeto) y GLOBAL (ejes del mundo, el gizmo no rota).
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
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
    if (action == GLFW_PRESS && !(mods & GLFW_MOD_CONTROL)) {
        if (key == GLFW_KEY_1 || key == GLFW_KEY_T) {
            if (scene) {
                scene->clearGuiaEje();
                scene->setGizmoOperation(ImGuizmo::TRANSLATE);
            }
        } else if (key == GLFW_KEY_2 || key == GLFW_KEY_R) {
            if (scene) {
                scene->clearGuiaEje();
                scene->setGizmoOperation(ImGuizmo::ROTATE);
            }
        } else if (key == GLFW_KEY_3 || key == GLFW_KEY_U) {
            if (scene) {
                scene->clearGuiaEje();
                scene->setGizmoOperation(ImGuizmo::SCALE);
            }
        }
    }
}

void EditorInput::onMouseButton(GLFWwindow* window, int button, int action,
                                int mods) {
    (void)mods;
    if (button != GLFW_MOUSE_BUTTON_RIGHT) return;
    if (action == GLFW_PRESS) {
        ImGuiIO& io = ImGui::GetIO();
        const bool gizmoCapturing = scene && scene->isGizmoCapturingInput();
        const bool editorActivo = scene && scene->isEditorActivo();

        if (!editorActivo && !io.WantCaptureMouse && !gizmoCapturing) {
            // Editor oculto (E presionado) + clic derecho sobre la escena:
            // entrar en modo orbita. El origen esta en la recta de la direccion
            // de la camara; el radio es la distancia horizontal actual camara-origen.
            if (CameraComponent* camara = scene ? scene->getActiveCamera() : nullptr) {
                camara->refreshFromTransform();
                const float* pos = camara->getPosition();
                const float* dir = camara->getDirection();
                // origen = pos + dir * radio. La camara queda sobre la
                // circunferencia y el origen sobre la recta en direccion y
                // sentido a donde mira la camara.
                float radio = 10.0f;
                origenOrbita[0] = pos[0] + dir[0] * radio;
                origenOrbita[1] = pos[1] + dir[1] * radio;
                origenOrbita[2] = pos[2] + dir[2] * radio;
                // Calcular radio horizontal real (distancia XZ camara <-> origen).
                float dx = pos[0] - origenOrbita[0];
                float dz = pos[2] - origenOrbita[2];
                radioOrbita = std::sqrt(dx * dx + dz * dz);
                orbitando = true;
            }
        } else {
            mouseDerechoParaNavegar = !io.WantCaptureMouse && !gizmoCapturing;
        }
    } else {
        mouseDerechoParaNavegar = false;
        orbitando = false;
    }
    aplicarModoCursor(window);
}

void EditorInput::onMouse(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    float dx;
    float dy;
    if (firstTimeMouseX) {
        dx = 0;
        dy = 0;
        lastMousePosX = xpos;
        firstTimeMouseX = false;
    }
    if (firstTimeMouseY) {
        dx = 0;
        dy = 0;
        lastMousePosY = ypos;
        firstTimeMouseY = false;
    }

    dx = static_cast<float>(xpos - lastMousePosX);
    dy = static_cast<float>(ypos - lastMousePosY);

    lastMousePosX = xpos;
    lastMousePosY = ypos;

    ImGuiIO& io = ImGui::GetIO();
    const bool gizmoCapturing = scene && scene->isGizmoCapturingInput();
    const bool editorActivo = scene && scene->isEditorActivo();
    // Tres caminos para rotar la camara:
    //  (1) modo editor libre (sin E y sin objeto seleccionado), mirada con el
    //      mouse suelto y sin ImGui capturando la escena;
    //  (2) CLIC DERECHO sostenido sobre la escena desde el editor: permite
    //      mira-se (y con WASD trasladarse) sin apretar E y SIN esconder las
    //      interfaces. Una vez enganchado, se mantiene aunque el cursor pase
    //      sobre un panel (los popups de ImGui necesitan clic nuevo).
    //  (3) ORBITA: editor oculto (E) + clic derecho sostenido sobre la escena.
    //      La camara gira alrededor de un punto origen en la direccion de mirada,
    //      manteniendo radio fijo y mirando siempre hacia el origen.
    const bool navegandoLibre =
        !editorActivo && !io.WantCaptureMouse && !gizmoCapturing && !orbitando;
    const bool navegandoConDerecho =
        mouseDerechoParaNavegar && !gizmoCapturing && !orbitando;

    if (orbitando && !gizmoCapturing) {
        if (CameraComponent* camara = scene ? scene->getActiveCamera() : nullptr) {
            const float sensibilidad =
                scene ? scene->getSensibilidadCamara() : 1.0f;
            camara->orbitAround(origenOrbita, radioOrbita,
                                dx * sensibilidad, dy * sensibilidad);
        }
    } else if (navegandoLibre || navegandoConDerecho) {
        if (CameraComponent* camara = scene ? scene->getActiveCamera() : nullptr) {
            const float sensibilidad =
                scene ? scene->getSensibilidadCamara() : 1.0f;
            camara->updateYaw(dx * sensibilidad, dy * sensibilidad);
        }
    }
}

void EditorInput::onScroll(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    // Ajustar radio de orbita con la rueda del mouse solo durante orbita.
    // Invertido: rueda arriba (yoffset>0) = decrementa, abajo = incrementa.
    if (!orbitando) return;
    const float factor = 1.01f;  // ~1% por notch
    float radioAnterior = radioOrbita;
    if (yoffset > 0) {
        radioOrbita /= factor;
        if (radioOrbita < CameraComponent::radioMin) radioOrbita = CameraComponent::radioMin;
    } else if (yoffset < 0) {
        radioOrbita *= factor;
        if (radioOrbita > CameraComponent::radioMax) radioOrbita = CameraComponent::radioMax;
    }
    // Desplazar la camara suavemente sobre la recta del radio por el delta.
    // La nueva posicion = origen - horizDir * radioOrbita.
    // Delta = radioOrbita - radioAnterior. Si radio disminuye (rueda arriba),
    // delta < 0 -> camara se acerca al origen (avanza sobre -horizDir).
    // Si radio aumenta (rueda abajo), delta > 0 -> camara se aleja (retrocede).
    if (CameraComponent* camara = scene ? scene->getActiveCamera() : nullptr) {
        camara->refreshFromTransform();
        const float* origen = origenOrbita;
        const float radX = camara->getYawX() * (3.14159265358979f / 180.f);
        float horizDir[3] = { std::sin(radX), 0.f, -std::cos(radX) };
        float deltaRadio = radioOrbita - radioAnterior;
        const float* pos = camara->getPosition();
        // Mover posicion a lo largo de la recta del radio (suave, tiempo real).
        // origen - horizDir * (radioAnterior + deltaRadio) = pos - horizDir * deltaRadio.
        float nuevaPos[3] = {
            pos[0] - horizDir[0] * deltaRadio,
            pos[1],
            pos[2] - horizDir[2] * deltaRadio
        };
        camara->setPosition(nuevaPos);
        camara->escribirATransform();
    }
}

void EditorInput::dibujarSidebarOrbita() {
    if (!orbitando) return;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Orbita", nullptr, flags)) {
        ImGui::Text("Radio Orbita");
        ImGui::Separator();
        // Barra vertical: altura fija, progreso = (radio - min) / (max - min).
        float progreso = (radioOrbita - CameraComponent::radioMin) /
                         (CameraComponent::radioMax - CameraComponent::radioMin);
        if (progreso < 0.f) progreso = 0.f;
        if (progreso > 1.f) progreso = 1.f;
        // ProgressBar vertical custom: usar DrawList para barra vertical.
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size(30, 200);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
        // Relleno con el acento del tema (mismo criterio que la barra de
        // StatusBarInterface): el amarillo de fabrica de ImGuiCol_PlotHistogram
        // quedaba fuera del color elegido en Opciones.
        ImU32 fgCol = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
        // Fondo
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgCol);
        // Relleno (de abajo hacia arriba)
        float fillH = size.y * progreso;
        draw->AddRectFilled(
            ImVec2(pos.x, pos.y + size.y - fillH),
            ImVec2(pos.x + size.x, pos.y + size.y),
            fgCol);
        // Borde
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      ImGui::GetColorU32(ImGuiCol_Border));
        // Avanzar cursor por el tamaño de la barra
        ImGui::Dummy(size);
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%.1f", radioOrbita);
        ImGui::Text("min: %.1f", CameraComponent::radioMin);
        ImGui::Text("max: %.1f", CameraComponent::radioMax);
        ImGui::EndGroup();
        ImGui::End();
    }
}