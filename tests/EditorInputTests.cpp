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
// Pruebas headless de la separacion entre el input del editor y la interfaz.
//
// El modulo de input traduce los eventos de GLFW a acciones del editor y no
// dibuja: el estado de la interfaz (si el teclado o el mouse pertenecen a un
// widget) le llega por una interfaz, y el dibujo de la barra de la orbita vive
// en la GUI. Lo que se comprueba aqui:
//
//  1. La implementacion sobre ImGui devuelve EXACTAMENTE las banderas que el
//     input leia antes de la separacion (WantCaptureKeyboard y
//     WantCaptureMouse): si el valor cambiara, una tecla del editor pasaria a
//     actuar sobre un widget (o al reves).
//  2. Sin contexto de ImGui no hay captura y no hay crash: el motor puede
//     arrancar antes de crear el contexto.
//  3. El motivo de la captura nombra el widget que la sostiene.
//  4. El progreso de la barra de la orbita se acota a su rango.
//  5. La operacion del gizmo que usa el input es la del gizmo (el numero
//     persistido en la configuracion no puede cambiar sin migrar el archivo).

#include <imgui.h>

#include <cmath>
#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/GUI/Estado/EstadoInterfazImGui.h"
#include "../FunshiEngineGL/src/GUI/Navegacion/SidebarOrbitaGUI.h"
#include "../FunshiEngineGL/src/Scenes/OperacionGizmo.h"

#include "ImGuizmo.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

bool cerca(float a, float b, float tol = 1e-5f) {
    return std::fabs(a - b) <= tol;
}
} // namespace

// La operacion del gizmo se persiste como numero: los valores de la
// enumeracion son los del gizmo y no pueden cambiar sin migrar la
// configuracion. La comparacion se fija en tiempo de compilacion.
static_assert(static_cast<int>(OperacionGizmo::Trasladar) ==
                  static_cast<int>(ImGuizmo::TRANSLATE),
              "la operacion de trasladar debe ser la del gizmo");
static_assert(static_cast<int>(OperacionGizmo::Rotar) ==
                  static_cast<int>(ImGuizmo::ROTATE),
              "la operacion de rotar debe ser la del gizmo");
static_assert(static_cast<int>(OperacionGizmo::Escalar) ==
                  static_cast<int>(ImGuizmo::SCALE),
              "la operacion de escalar debe ser la del gizmo");
static_assert(static_cast<int>(OperacionGizmo::Universal) ==
                  static_cast<int>(ImGuizmo::UNIVERSAL),
              "la operacion universal debe ser la del gizmo");
static_assert(static_cast<int>(OperacionGizmo::Ninguna) == 0,
              "sin operacion es cero");

int main() {
    // --- 1. Sin contexto de ImGui: no hay captura y no hay crash ------------
    {
        EstadoInterfazImGui estado;
        CHECK(!estado.capturaTeclado(),
              "sin contexto de ImGui el teclado no esta capturado");
        CHECK(!estado.capturaMouse(),
              "sin contexto de ImGui el mouse no esta capturado");
        CHECK(estado.motivoCapturaTeclado() == "sin captura",
              "sin contexto de ImGui el motivo es 'sin captura'");
    }

    // --- 2. Con contexto: los mismos valores que leia el input ------------
    ImGui::CreateContext();
    {
        EstadoInterfazImGui estado;
        ImGuiIO& io = ImGui::GetIO();

        CHECK(estado.capturaTeclado() == io.WantCaptureKeyboard,
              "la captura de teclado es la misma bandera de ImGui");
        CHECK(estado.capturaMouse() == io.WantCaptureMouse,
              "la captura de mouse es la misma bandera de ImGui");
        CHECK(estado.motivoCapturaTeclado() == "sin captura",
              "sin widget activo el motivo es 'sin captura'");

        // Un campo de texto en edicion es el motivo mas especifico: la
        // bandera de ImGui no dice que widget la sostiene.
        io.WantTextInput = true;
        const std::string motivo = estado.motivoCapturaTeclado();
        CHECK(motivo.find("campo de texto") != std::string::npos,
              "el motivo nombra el campo de texto en edicion");
        io.WantTextInput = false;

        // Con la bandera de ImGui prendida, la interfaz la ve prendida: es
        // lo que hacia que el input cediera la tecla.
        io.WantCaptureKeyboard = true;
        CHECK(estado.capturaTeclado(),
              "con la bandera prendida la interfaz captura el teclado");
        io.WantCaptureKeyboard = false;
        CHECK(!estado.capturaTeclado(),
              "con la bandera apagada la interfaz no captura el teclado");
    }
    ImGui::DestroyContext();

    // --- 3. Destruido el contexto: vuelve al estado sin captura -----------
    {
        EstadoInterfazImGui estado;
        CHECK(!estado.capturaTeclado(),
              "sin contexto el teclado no esta capturado");
        CHECK(!estado.capturaMouse(),
              "sin contexto el mouse no esta capturado");
        CHECK(estado.motivoCapturaTeclado() == "sin captura",
              "sin contexto el motivo es 'sin captura'");
    }

    // --- 4. El progreso de la barra de la orbita se acota ------------------
    CHECK(cerca(SidebarOrbitaGUI::progreso(3.0f, 3.0f, 50.0f), 0.0f),
          "en el radio minimo el progreso es cero");
    CHECK(cerca(SidebarOrbitaGUI::progreso(50.0f, 3.0f, 50.0f), 1.0f),
          "en el radio maximo el progreso es uno");
    CHECK(cerca(SidebarOrbitaGUI::progreso(26.5f, 3.0f, 50.0f), 0.5f),
          "en la mitad del rango el progreso es la mitad");
    CHECK(cerca(SidebarOrbitaGUI::progreso(1.0f, 3.0f, 50.0f), 0.0f),
          "por debajo del radio minimo el progreso no es negativo");
    CHECK(cerca(SidebarOrbitaGUI::progreso(99.0f, 3.0f, 50.0f), 1.0f),
          "por encima del radio maximo el progreso no pasa de uno");
    CHECK(cerca(SidebarOrbitaGUI::progreso(10.0f, 5.0f, 5.0f), 0.0f),
          "sin rango no hay progreso que dibujar");

    // --- 5. La operacion del gizmo se traduce desde el numero persistido ---
    CHECK(operacionDesdeNumero(0) == OperacionGizmo::Ninguna,
          "el numero cero es la ausencia de operacion");
    CHECK(operacionDesdeNumero(7) == OperacionGizmo::Trasladar,
          "el numero de trasladar se traduce a trasladar");
    CHECK(operacionDesdeNumero(120) == OperacionGizmo::Rotar,
          "el numero de rotar se traduce a rotar");
    CHECK(operacionDesdeNumero(896) == OperacionGizmo::Escalar,
          "el numero de escalar se traduce a escalar");
    CHECK(operacionDesdeNumero(14463) == OperacionGizmo::Universal,
          "el numero universal se traduce a universal");
    CHECK(operacionDesdeNumero(999) == OperacionGizmo::Trasladar,
          "un numero desconocido cae en la operacion por defecto");
    CHECK(numeroDeOperacion(OperacionGizmo::Rotar) == 120,
          "el numero de la operacion es el que se persiste");

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
