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

#include "OrquestadorEstadoGUI.h"

#include <cassert>

OrquestadorEstadoGUI::OrquestadorEstadoGUI(ApplicationStateMachine* maquina) noexcept
    : maquina(maquina)
{
    assert(maquina && "El orquestador necesita la maquina de estados");
}

// Centraliza "Escape en el editor vuelve al menu principal". Antes esta
// transicion colgaba en el callback de teclado de main; ahora la regla vive
// aqui. Es inofensivo si ya estamos en el menu (guardia explicita).
void OrquestadorEstadoGUI::manejarTeclaEscape() noexcept
{
    if (maquina->is(ApplicationState::Editing)) {
        maquina->transitionTo(ApplicationState::MainMenu);
    }
}

// Centraliza "Iniciar Estudio": el menu pide cierre y se entra al editor.
// Devuelve true si efectivamente hubo transicion. La solicitud de cierre del
// menu se consume aqui para evitar que el evento quedara colgando: se marca
// como pendiente y se limpia con cerrarMenuPendiente(). Re-entrante: si ya
// estamos en Editing no se transiciona dos veces.
bool OrquestadorEstadoGUI::iniciarEstudio() noexcept
{
    if (!maquina->is(ApplicationState::MainMenu)) {
        return false;
    }
    maquina->transitionTo(ApplicationState::Editing);
    return true;
}

// Funcion de marco de la simulacion: fija el comportamiento del editor ante
// las teclas de funcion (F5/F6/F7) segun el estado activo. Igual que
// manejarTeclaEscape, la REGLA vive aca y el input (EditorInput) solo la
// reenvia; main refleja la decision sobre la escena con setStart().
//
// Reglas:
//  - F5 Play solo arranca desde el editor (Editing -> Playing); en Playing ya
//    esta corriendo y en el menu no tiene sentido. Al arrancar, la pausa
//    nunca queda heredada del play anterior.
//  - F6 Pausa alterna congelar/reanudar SOLO mientras se simula (Playing);
//    fuera del play es inofensivo (no cambia nada).
//  - F7 Stop solo corta desde Playing (Playing -> Editing) y deja la pausa
//    limpia para el proximo play. En el editor/menu es inofensivo.
// En los tres casos el boton "Activar/Detener" del menu de escena y estas
// teclas comparten la misma fuente de verdad: la maquina de estados.
void OrquestadorEstadoGUI::manejarTeclaSimulacion(TeclaSimulacion tecla) noexcept
{
    switch (tecla) {
        case TeclaSimulacion::Play:
            if (maquina->is(ApplicationState::Editing)) {
                simulacionPausada_ = false;
                maquina->transitionTo(ApplicationState::Playing);
            }
            break;
        case TeclaSimulacion::Pausa:
            if (maquina->is(ApplicationState::Playing)) {
                simulacionPausada_ = !simulacionPausada_;
            }
            break;
        case TeclaSimulacion::Stop:
            if (maquina->is(ApplicationState::Playing)) {
                simulacionPausada_ = false;
                maquina->transitionTo(ApplicationState::Editing);
            }
            break;
        case TeclaSimulacion::Ninguna:
            break;
    }
}

bool OrquestadorEstadoGUI::menuDebeEstarVisible() const noexcept
{
    return maquina->is(ApplicationState::MainMenu);
}

bool OrquestadorEstadoGUI::escenaDebeCorrer() const noexcept
{
    // Cualquier estado que no sea el menu de inicio (MainMenu, Editing,
    // Playing y Exiting) implica que la escena corre.
    return !menuDebeEstarVisible();
}

bool OrquestadorEstadoGUI::enSimulacion() const noexcept
{
    return maquina->is(ApplicationState::Playing);
}

bool OrquestadorEstadoGUI::simulacionPausada() const noexcept
{
    return simulacionPausada_;
}

bool OrquestadorEstadoGUI::cerrarMenuPendiente() const noexcept
{
    return false;  // reservado para cuando "Iniciar Estudio" consuma
                   // ConsultarCierre(); en esta fase la transicion es directa.
}

ApplicationState OrquestadorEstadoGUI::getEstado() const noexcept
{
    // La fuente de verdad es la maquina (tambien refleja Playing, que antes
    // se colapsaba a Editing en este getter).
    return maquina->getState();
}
