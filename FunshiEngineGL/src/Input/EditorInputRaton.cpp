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

void EditorInput::onMouseButton(GLFWwindow* window, int button, int action,
                                int mods) {
    (void)mods;
    if (button != GLFW_MOUSE_BUTTON_RIGHT) return;
    if (modoJuegoActivo()) {
        mouseDerechoParaNavegar = false;
        orbitando = false;
        aplicarModoCursor(window);
        return;
    }
    if (action == GLFW_PRESS) {
        const bool gizmoCapturing = scene && scene->isGizmoCapturingInput();
        const bool editorActivo = scene && scene->isEditorActivo();
        const bool cursorYaCapturado =
            glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
        const bool puedeUsarEscena =
            (!editorActivo || !capturaMouse()) && !gizmoCapturing;

        // En 2D la camara esta fija mirando el plano: la orbita (que rota la
        // camara) no aplica, asi que el clic derecho no la inicia.
        const bool es2D =
            scene && scene->getDimensionProyecto() == DimensionProyecto::DosD;
        if (!editorActivo && cursorOcultoPorC && cursorYaCapturado &&
            puedeUsarEscena && !es2D) {
            // Con la GUI oculta y el cursor capturado por C, el clic derecho
            // inicia orbita alrededor del punto al que mira la camara.
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
        } else if (puedeUsarEscena) {
            // Si el cursor estaba visible, el clic derecho lo captura mientras
            // se navega, sin iniciar orbita.
            mouseDerechoParaNavegar = true;
        }
    } else {
        mouseDerechoParaNavegar = false;
        orbitando = false;
    }
    aplicarModoCursor(window);
}

void EditorInput::onMouse(GLFWwindow* window, double xpos, double ypos) {
    if (modoJuegoActivo() ||
        (window &&
         glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)) {
        // El HUD nativo se queda el raton cuando el puntero esta encima de un
        // boton: si el movimiento siguiera llegando al script, la camara
        // giraria mientras el jugador pulsa. Con el cursor capturado no hay
        // puntero visible, asi que el HUD no captura nada.
        const bool cursorVisible =
            window && glfwGetInputMode(window, GLFW_CURSOR) !=
                          GLFW_CURSOR_DISABLED;
        const bool sobreInterfaz =
            cursorVisible && scene && scene->ratonSobreInterfazNativa();
        if (!sobreInterfaz && scene) scene->registrarMouseScript(xpos, ypos);
    }
    if (modoJuegoActivo()) {
        firstTimeMouseX = true;
        firstTimeMouseY = true;
        return;
    }
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

    const bool gizmoCapturing = scene && scene->isGizmoCapturingInput();
    const bool editorActivo = scene && scene->isEditorActivo();
    // El mouse solo rota la camara cuando el cursor esta capturado o mientras
    // se sostiene clic derecho. Con la GUI oculta, C captura el cursor para
    // mirada libre; el clic derecho en ese estado activa orbita. Si el cursor
    // estaba visible, el clic derecho solo permite mirar mientras se sostiene.
    const bool navegandoLibre =
        !editorActivo && cursorOcultoPorC &&
        (!editorActivo || !capturaMouse()) && !gizmoCapturing &&
        !orbitando;
    const bool navegandoConDerecho =
        mouseDerechoParaNavegar && !gizmoCapturing && !orbitando;
    // En 2D la camara mira el plano y no rota: ni orbita ni yaw/pitch, para que
    // los sprites queden siempre de frente. El desplazamiento (WASD) sigue vivo.
    const bool es2D =
        scene && scene->getDimensionProyecto() == DimensionProyecto::DosD;

    if (orbitando && !gizmoCapturing && !es2D) {
        if (CameraComponent* camara = scene ? scene->getActiveCamera() : nullptr) {
            const float sensibilidad =
                scene ? scene->getSensibilidadCamara() : 1.0f;
            camara->orbitAround(origenOrbita, radioOrbita,
                                dx * sensibilidad, dy * sensibilidad);
        }
    } else if ((navegandoLibre || navegandoConDerecho) && !es2D) {
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
    if (modoJuegoActivo()) return;
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

// El input no dibuja: entrega el estado de la orbita y la GUI lo presenta.
EstadoOrbita EditorInput::estadoOrbita() const {
    EstadoOrbita estado;
    estado.activa = orbitando;
    estado.radio = radioOrbita;
    estado.radioMinimo = CameraComponent::radioMin;
    estado.radioMaximo = CameraComponent::radioMax;
    return estado;
}
