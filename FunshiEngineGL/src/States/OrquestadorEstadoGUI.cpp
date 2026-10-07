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

// Centraliza "Escape en el editor vuelve al menu principal" y "Escape durante
// el play detiene la simulacion". Antes esta transicion colgaba en el callback
// de teclado de main; ahora la regla vive aqui. Es inofensivo si ya estamos en
// el menu (guardia explicita).
//
// Reglas por estado:
//  - MainMenu: no hay nada que abandonar.
//  - Editing: vuelve al menu principal.
//  - Debugging y Playing: termina la simulacion y vuelve al editor. El cierre
//    conserva la escena en Depuracion y restaura el baseline en Juego.
void OrquestadorEstadoGUI::manejarTeclaEscape() noexcept
{
    if (enSimulacion()) {
        // Misma regla que F7: cortar la simulacion y dejar la pausa limpia
        // para el proximo play.
        simulacionPausada_ = false;
        maquina->transitionTo(ApplicationState::Editing);
        return;
    }
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
//  - F5 Depuracion solo arranca desde el editor (Editing -> Debugging).
//  - Juego arranca desde el editor (Editing -> Playing).
//  - F6 Pausa alterna congelar/reanudar SOLO mientras se simula
//    (Debugging o Playing);
//    fuera del play es inofensivo (no cambia nada).
//  - F7 Stop corta desde cualquiera de los modos de simulacion y deja la pausa
//    limpia para el proximo inicio. En el editor/menu es inofensivo.
// Los botones y las teclas comparten la misma fuente de verdad: la maquina
// de estados.
void OrquestadorEstadoGUI::manejarTeclaSimulacion(TeclaSimulacion tecla) noexcept
{
    switch (tecla) {
        case TeclaSimulacion::Depuracion:
            if (maquina->is(ApplicationState::Editing)) {
                simulacionPausada_ = false;
                maquina->transitionTo(ApplicationState::Debugging);
            }
            break;
        case TeclaSimulacion::Juego:
            if (maquina->is(ApplicationState::Editing)) {
                simulacionPausada_ = false;
                maquina->transitionTo(ApplicationState::Playing);
            }
            break;
        case TeclaSimulacion::Pausa:
            if (enSimulacion()) {
                simulacionPausada_ = !simulacionPausada_;
            }
            break;
        case TeclaSimulacion::Stop:
            if (enSimulacion()) {
                simulacionPausada_ = false;
                maquina->transitionTo(ApplicationState::Editing);
            }
            break;
        case TeclaSimulacion::Reset:
            if (enSimulacion()) solicitudReset_ = true;
            break;
        case TeclaSimulacion::Ninguna:
            break;
    }
}

// El boton de inicio de Depuracion comparte la regla de F5 y el de terminar
// comparte F7; ambos caminos pasan por la maquina de estados.
void OrquestadorEstadoGUI::alternarSimulacion() noexcept
{
    manejarTeclaSimulacion(enSimulacion() ? TeclaSimulacion::Stop
                                          : TeclaSimulacion::Depuracion);
}

void OrquestadorEstadoGUI::iniciarJuego() noexcept
{
    manejarTeclaSimulacion(TeclaSimulacion::Juego);
}

void OrquestadorEstadoGUI::solicitarReset() noexcept
{
    manejarTeclaSimulacion(TeclaSimulacion::Reset);
}

bool OrquestadorEstadoGUI::consumirSolicitudReset() noexcept
{
    const bool solicitada = solicitudReset_;
    solicitudReset_ = false;
    return solicitada;
}

bool OrquestadorEstadoGUI::menuDebeEstarVisible() const noexcept
{
    return maquina->is(ApplicationState::MainMenu);
}

// "Dentro del editor" = editor o play. Es la condicion que comparten las teclas
// del editor que tienen sentido mientras se edita Y mientras se simula (E,
// WASD, guia de eje, modo del cursor): con la simulacion en marcha el editor no
// desaparece, solo deja de dibujar sus interfaces, asi que el teclado sigue
// siendo del editor. Solo el menu de inicio queda afuera.
bool OrquestadorEstadoGUI::dentroDelEditor() const noexcept
{
    return maquina->is(ApplicationState::Editing) ||
           maquina->is(ApplicationState::Debugging) ||
           maquina->is(ApplicationState::Playing);
}

bool OrquestadorEstadoGUI::escenaDebeCorrer() const noexcept
{
    // Cualquier estado que no sea el menu de inicio (MainMenu, Editing,
    // Playing y Exiting) implica que la escena corre.
    return !menuDebeEstarVisible();
}

bool OrquestadorEstadoGUI::enSimulacion() const noexcept
{
    return enDepuracion() || enModoJuego();
}

bool OrquestadorEstadoGUI::enDepuracion() const noexcept
{
    return maquina->is(ApplicationState::Debugging);
}

bool OrquestadorEstadoGUI::enModoJuego() const noexcept
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
