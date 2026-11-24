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

namespace {

// Trazas de captura de teclado del editor.
//
// Las teclas del editor (E, Escape, el movimiento WASD y la guia de eje) se
// ceden mientras la interfaz tenga el teclado, y ese flag se prende por tres
// motivos distintos: un campo de texto en edicion, cualquier otro widget con
// el foco o un popup modal abierto. Solo el primero es una cesion intencional,
// pero los tres apagan las mismas teclas. El log registra cual de ellos sostiene
// la captura cada vez que cambia, y por que una tecla del editor quedo sin
// efecto: es el unico lado del descarte que no se ve en la pantalla.
// Estado de la maquina de aplicacion junto a la captura: varias teclas del
// editor se ceden segun el estado (menu, edicion o play), asi que la cronologia
// del log sirve poco si no dice en que estado estaba la aplicacion cuando algo
// se descarto.
std::string nombreDeEstado(const ApplicationStateMachine* appState) {
    if (!appState) return "desconocido";
    if (appState->is(ApplicationState::MainMenu)) return "menu";
    if (appState->is(ApplicationState::Editing)) return "edicion";
    if (appState->is(ApplicationState::Debugging)) return "depuracion";
    if (appState->is(ApplicationState::Playing)) return "juego";
    return "salida";
}

}  // namespace

EditorInput* EditorInput::instancia = nullptr;

EditorInput::EditorInput(GameScene* escena, ApplicationStateMachine* maquina,
                         OrquestadorEstadoGUI* orquestador,
                         IEstadoInterfaz* estadoInterfaz)
    : scene(escena), appState(maquina), orquestador(orquestador),
      interfaz(estadoInterfaz) {
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

// La regla "editor o play" la decide el orquestador; aca solo se consulta para
// que ninguna puerta de teclado repita la condicion por su cuenta. Sin
// orquestador (construccion incompleta) se cae al estado de edicion.
bool EditorInput::dentroDelEditor() const noexcept {
    if (orquestador) return orquestador->dentroDelEditor();
    return appState && appState->is(ApplicationState::Editing);
}

bool EditorInput::modoJuegoActivo() const noexcept {
    return orquestador && orquestador->enModoJuego();
}

// La interfaz es la que sabe si el teclado o el mouse pertenecen a un widget.
// Sin interfaz (el juego exportado puede no tenerla) no hay captura: las
// teclas del editor funcionan siempre.
bool EditorInput::capturaTeclado() const noexcept {
    return interfaz && interfaz->capturaTeclado();
}

bool EditorInput::capturaMouse() const noexcept {
    return interfaz && interfaz->capturaMouse();
}

std::string EditorInput::motivoCaptura() const {
    if (!interfaz) return "sin captura";
    return interfaz->motivoCapturaTeclado();
}

// Traza del estado de captura del teclado: el log ordena el momento en que la
// captura se prendio, tambien en los estados donde el movimiento ni se evalua.
void EditorInput::trazarCapturaDeTeclado() const {
    static std::string firmaAnterior;
    static bool capturaAnterior = false;
    static double ultimoRegistro = -1.0;

    const bool captura = capturaTeclado();
    const std::string firma = std::string(captura ? "prendida" : "apagada") +
                              "; motivo: " + motivoCaptura() +
                              "; estado: " + nombreDeEstado(appState);

    const bool cambioDeCaptura = captura != capturaAnterior;
    if (firma == firmaAnterior && !cambioDeCaptura) return;

    // Con la bandera prendida, un motivo que solo parpadea (un widget que entra
    // y sale cada frame) llenaria el log: ese cambio se registra como maximo
    // una vez por cuarto de segundo. El encendido/apagado de la bandera en si
    // siempre se registra, que es el evento que ordena la cronologia.
    const double ahora = glfwGetTime();
    if (!cambioDeCaptura && ultimoRegistro >= 0.0 &&
        ahora - ultimoRegistro < 0.25) {
        return;
    }

    capturaAnterior = captura;
    ultimoRegistro = ahora;
    firmaAnterior = firma;
    std::cout << "[input] captura de teclado " << firma << std::endl;
}

void EditorInput::aplicarModoCursor(GLFWwindow* window) {
    if (!window || !scene || !appState) return;
    const bool juego = modoJuegoActivo();
    const bool enEditor = dentroDelEditor();
    if (juego || !enEditor) {
        mouseDerechoParaNavegar = false;
        orbitando = false;
        cursorOcultoPorC = false;
        limpiarMovimientoCamara();
    }
    const bool interfazVisible = scene->isEditorActivo();
    const bool ocultar = !juego && enEditor &&
                         (mouseDerechoParaNavegar ||
                          (!interfazVisible && cursorOcultoPorC));
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

void EditorInput::limpiarMovimientoCamara() noexcept {
    teclaAdelante = false;
    teclaAtras = false;
    teclaIzquierda = false;
    teclaDerecha = false;
    teclaArriba = false;
    teclaAbajo = false;
}

void EditorInput::descartarDeltaLook() {
    firstTimeMouseX = true;
    firstTimeMouseY = true;
}

void EditorInput::setAccionGuardar(std::function<void()> accion) {
    accionGuardar = std::move(accion);
}

void EditorInput::aplicarMovimiento(float deltaTime) {
    // Traza del estado de captura: corre antes de cualquier guard para que el
    // log ordene el momento en que la captura se prendio, tambien en los
    // estados donde el movimiento ni se evalua.
    trazarCapturaDeTeclado();

    // La camara libre pertenece al editor; Juego no debe modificar la escena
    // mediante los controles de navegacion del editor.
    if (!scene || !appState || !dentroDelEditor() || modoJuegoActivo()) return;
    static bool avisoDeCaptura = false;
    if (capturaTeclado()) {
        // Un aviso por episodio de captura, no por frame: el registro que
        // sostiene la traza es el de encendido/apagado de la bandera.
        const bool hayMovimiento = teclaAdelante || teclaAtras ||
                                   teclaIzquierda || teclaDerecha ||
                                   teclaArriba || teclaAbajo;
        if (hayMovimiento && !avisoDeCaptura) {
            avisoDeCaptura = true;
            trazarTeclaDescartada("WASD",
                                  std::string("la interfaz tiene el teclado (") +
                                      motivoCaptura() + ")");
        }
        return;
    }
    avisoDeCaptura = false;
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
