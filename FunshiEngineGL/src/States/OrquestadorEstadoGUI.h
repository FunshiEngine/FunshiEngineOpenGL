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
#ifndef ORQUESTADOR_ESTADO_GUI_H
#define ORQUESTADOR_ESTADO_GUI_H

#include "ApplicationStateMachine.h"

// Orquestador de estados de GUI: centraliza las transiciones que gobiernan
// "que GUI es visible este frame" (menu de inicio vs editor) y la simulacion
// (F5/F6/F7). Encapsula las REGLAS (no los if sueltos de main): Escape en
// editor -> volver al menu, "Iniciar Estudio" -> entrar al editor, F5 -> play,
// F6 -> pausa, F7 -> stop. Ni la escena ni las fachadas de GUI conocen esta
// clase; main le pregunta por frame y ella es la unica fuente de verdad sobre
// cuando hay que mostrar menu, correr la escena o simular.
//
// Es una orquestacion PURA de estado (sin ImGui ni OpenGL): recibe un
// puntero no propietario a la maquina de estados y solo la transiciona.
// Por eso se puede probar headless igual que EditorConfig.
class OrquestadorEstadoGUI {
public:
    // Teclas de funcion de la simulacion (F5/F6/F7): el comportamiento de
    // CADA tecla por estado vive aqui (funcion de marco de la arquitectura).
    // "y las teclas" del editor (E, WASD, gizmo, Escape) siguen el mismo
    // esquema: el input (EditorInput) reenvia el evento y la regla por estado
    // se decide aca, nunca en main.
    enum class TeclaSimulacion {
        Ninguna,
        Play,    // F5: arrancar (Editing -> Playing)
        Pausa,   // F6: congelar/reanudar (solo en Playing; fuera no hace nada)
        Stop     // F7: cortar y volver (Playing -> Editing)
    };

public:
    explicit OrquestadorEstadoGUI(ApplicationStateMachine* maquina) noexcept;

    // Centraliza "Escape en editor vuelve al menu principal" (antes colgaba en
    // el callback de teclado de main). Inofensivo si ya estamos en el menu.
    void manejarTeclaEscape() noexcept;

    // Centraliza "Iniciar Estudio" -> el menu pide cierre y se entra al
    // editor. Devuelve true si efectivamente hubo transicion.
    bool iniciarEstudio() noexcept;

    // Funcion de marco de la simulacion: fija el comportamiento del editor
    // ante F5/F6/F7 aplicando las reglas por estado (ver TeclaSimulacion).
    // main solo refleja la decision: setStart(estaEnSimulacion()).
    void manejarTeclaSimulacion(TeclaSimulacion tecla) noexcept;

    // Reflejos por frame (lo que main pregunta en el bucle):
    bool menuDebeEstarVisible() const noexcept;   // MainMenu activo
    bool escenaDebeCorrer() const noexcept;       // cualquier estado no-menu
    bool cerrarMenuPendiente() const noexcept;    // quedo solicitud sin aplicar
    bool enSimulacion() const noexcept;           // estado Playing
    bool simulacionPausada() const noexcept;      // F6 dentro de Playing

    ApplicationState getEstado() const noexcept;

private:
    ApplicationStateMachine* maquina;  // no propietario; vive en la app
    // Guardia de cambio double-consumo: la solicitud de cierre se marca al
    // llamar iniciarEstudio() y se limpia al consultar (patron usado por la
    // fachada del menu para no repetir la transicion el siguiente frame).
    bool solicitudCierrePendiente = false;
    // Pausa de la simulacion (F6): solo tiene efecto mientras esta en Playing;
    // se limpia al entrar/salir de play para que nunca quede heredada.
    bool simulacionPausada_ = false;
};

#endif // ORQUESTADOR_ESTADO_GUI_H
