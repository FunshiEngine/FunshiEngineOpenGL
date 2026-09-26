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
#ifndef EDITOR_INPUT_H
#define EDITOR_INPUT_H
#include <functional>

struct GLFWwindow;

class GameScene;
class ApplicationStateMachine;
class OrquestadorEstadoGUI;

// Callbacks del editor (teclado/mouse de GLFW) + navegacion de la camara,
// extraidas de main.cpp a su propio modulo para que el bucle principal quede
// solo con el orquestado (estados, ImGui, render). Vive una sola instancia
// creada por main; las callbacks estaticas redireccionan via instancia.
//
// Dos responsabilidades:
//  1. Traduccion de eventos GLFW a acciones del editor (E, G, gizmo, Escape,
//     clic derecho = navegar, cursor capturado).
//  2. Maquina de estado de movimiento: onKey() SOLO registra las teclas
//     WASD/Espacio/Shift (PRESS/RELEASE) y main llama aplicarMovimiento(dt)
//     por frame; el desplazamiento se computa con un vector de direccion
//     combinado y normalizado, de modo que las diagonales se permiten y se
//     mueven a la misma velocidad que un eje solo (antes cada tecla era una
//     callback separada: la diagonal salia a sqrt(2) y con reinicios de
//     transform por cada eje).
class EditorInput {
public:
    EditorInput(GameScene* escena, ApplicationStateMachine* maquina,
                OrquestadorEstadoGUI* orquestador);

    // Registra las callbacks de GLFW. OJO: se debe llamar ANTES de
    // ImGui_ImplGlfw_InitForOpenGL(window, true): el backend de ImGui guarda
    // estas como "previas" y las encadena, manteniendo el orden actual.
    void registrarCallbacks(GLFWwindow* window);

    // Movimiento continuo por frame (maquina de estado de teclas): combina las
    // direcciones activas, normaliza (diagonal == eje solo) y traslada la
    // camara activa multiplicando por la sensibilidad de movimiento.
    void aplicarMovimiento(float deltaTime);

    // Devuelve el cursor a su estado segun la maquina de estados (atrapado en
    // navegacion libre o clic derecho; normal en menu/editor). main la llama
    // al arrancar y al transicionar estados.
    void aplicarModoCursor(GLFWwindow* window);

    // Descarta el delta de look acumulado (transicion Iniciar Estudio): sin
    // esto el primer movimiento del mouse "teletransporta" la mirada.
    void descartarDeltaLook();

    // Accion que se dispara con Ctrl+S (guardado en caliente del proyecto).
    // Se inyecta desde main (patron del EditorEventBus): EditorInput no
    // conoce proyectos ni configuracion.
    void setAccionGuardar(std::function<void()> accion);

    // Dibuja barra lateral de radio de orbita (vertical) si orbitando.
    void dibujarSidebarOrbita();

    static void teclado_callback(GLFWwindow* window, int key, int scancode,
                                 int action, int mods);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback(GLFWwindow* window, int button,
                                      int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset,
                                double yoffset);

private:
    void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    void onMouseButton(GLFWwindow* window, int button, int action, int mods);
    void onMouse(GLFWwindow* window, double xpos, double ypos);
    void onScroll(GLFWwindow* window, double xoffset, double yoffset);

    GameScene* scene;
    ApplicationStateMachine* appState;
    OrquestadorEstadoGUI* orquestador;
    // Guardado en caliente (Ctrl+S): inyectado por main con la misma rutina
    // que el guardado al salir.
    std::function<void()> accionGuardar;

    // Maquina de estado del movimiento: que teclas estan APRETADAS ahora
    // (onKey solo las setea con PRESS/RELEASE; aplicarMovimiento las combina).
    bool teclaAdelante = false;   // W
    bool teclaAtras = false;      // S
    bool teclaIzquierda = false;  // A
    bool teclaDerecha = false;    // D
    bool teclaArriba = false;     // Space
    bool teclaAbajo = false;      // Left Shift

    // Clic derecho sostenido sobre la escena 3D: navegacion desde el editor sin
    // apretar E (mira-se mueve) manteniendo visibles las interfaces.
    bool mouseDerechoParaNavegar = false;

    // Orbita con clic derecho + editor oculto (E): la camara rota alrededor de
    // un punto origen en la direccion de mirada, manteniendo radio y mirando
    // hacia el origen. La rueda del mouse ajusta el radio durante la orbita.
    bool orbitando = false;
    float origenOrbita[3] = {0.f, 0.f, 0.f};
    float radioOrbita = 10.0f;

    double lastMousePosX = 0.0;
    double lastMousePosY = 0.0;
    // Primero evento de mouse tras atrapar/soltar el cursor: se descarta el
    // delta para que el look no se "teletransporte".
    bool firstTimeMouseX = true;
    bool firstTimeMouseY = true;

    static EditorInput* instancia;
};

#endif // EDITOR_INPUT_H