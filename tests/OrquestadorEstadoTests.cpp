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
// Pruebas headless del orquestador de estados de GUI (la "funcion de marco"
// que fija el comportamiento del editor ante F5/F6/F7, el boton
// Activar/Detener y las teclas). Cubre las reglas por estado de la simulacion
// (Play/Pausa/Stop), la condicion compartida por las teclas del editor (editor
// o play), Escape por estado y "Iniciar Estudio" -> editor. Sin pila grafica:
// solo ApplicationStateMachine + OrquestadorEstadoGUI, que son pura logica de
// estados.

#include <iostream>

#include "../FunshiEngineGL/src/Input/AtajosEditor.h"
#include "../FunshiEngineGL/src/States/ApplicationStateMachine.h"
#include "../FunshiEngineGL/src/States/OrquestadorEstadoGUI.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"  \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

using Tecla = OrquestadorEstadoGUI::TeclaSimulacion;

void estadoInicial() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    CHECK(orquestador.getEstado() == ApplicationState::MainMenu,
          "arranca en MainMenu");
    CHECK(orquestador.menuDebeEstarVisible(), "menu visible al iniciar");
    CHECK(!orquestador.escenaDebeCorrer(), "escena parada en el menu");
    CHECK(!orquestador.enSimulacion(), "sin simulacion al iniciar");
    CHECK(!orquestador.simulacionPausada(), "sin pausa al iniciar");
}

void iniciarEstudio() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    CHECK(orquestador.iniciarEstudio(), "Iniciar Estudio desde el menu transiciona");
    CHECK(orquestador.getEstado() == ApplicationState::Editing,
          "Iniciar Estudio -> Editing");
    CHECK(orquestador.menuDebeEstarVisible() == false,
          "tras Iniciar Estudio el menu no se muestra");
    CHECK(orquestador.escenaDebeCorrer(), "la escena corre en Editing");
    CHECK(!orquestador.iniciarEstudio(),
          "Iniciar Estudio desde Editing no repite la transicion");
}

void escapePorEstado() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    // En el menu, Escape es inofensivo.
    orquestador.manejarTeclaEscape();
    CHECK(orquestador.getEstado() == ApplicationState::MainMenu,
          "Escape en el menu no cambia nada");
    // En el editor, Escape vuelve al menu.
    orquestador.iniciarEstudio();
    orquestador.manejarTeclaEscape();
    CHECK(orquestador.getEstado() == ApplicationState::MainMenu,
          "Escape en el editor vuelve al menu");
    // Durante el play, Escape deja la simulacion como F7 (detiene y vuelve al
    // editor) en vez de saltar al menu: la simulacion se abandona con la misma
    // tecla que la detiene, y el menu queda a un Escape mas.
    orquestador.iniciarEstudio();
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    orquestador.manejarTeclaEscape();
    CHECK(orquestador.getEstado() == ApplicationState::Editing,
          "Escape en play detiene y vuelve al editor");
    CHECK(!orquestador.enSimulacion(), "Escape en play corta la simulacion");
    CHECK(!orquestador.simulacionPausada(),
          "Escape en play deja la pausa limpia para el proximo play");
    // Segundo Escape, ya en el editor: al menu.
    orquestador.manejarTeclaEscape();
    CHECK(orquestador.getEstado() == ApplicationState::MainMenu,
          "el segundo Escape, en el editor, vuelve al menu");
    // Con la simulacion pausada pasa lo mismo, y la pausa no queda pegada.
    orquestador.iniciarEstudio();
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    orquestador.manejarTeclaEscape();
    CHECK(orquestador.getEstado() == ApplicationState::Editing,
          "Escape con la simulacion pausada vuelve al editor");
    CHECK(!orquestador.simulacionPausada(),
          "Escape con la simulacion pausada limpia la pausa");
}

// Condicion compartida por las teclas del editor que tambien tienen sentido
// con la simulacion en marcha (E, WASD, guia de eje, modo del cursor): el
// play es el editor con las interfaces ocultas, no un estado aparte.
void dentroDelEditorSegunEstado() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    CHECK(!orquestador.dentroDelEditor(), "en el menu no es el editor");
    orquestador.iniciarEstudio();
    CHECK(orquestador.dentroDelEditor(), "en edicion es el editor");
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    CHECK(orquestador.dentroDelEditor(),
          "en play tambien: el editor sigue vivo, solo se ocultan sus interfaces");
    orquestador.manejarTeclaSimulacion(Tecla::Stop);
    CHECK(orquestador.dentroDelEditor(),
          "al volver a edicion sigue siendo el editor");
}

// El boton Activar/Detener del menu de escena no decide por su cuenta: pide el
// mismo cambio que F5/F7, asi que el play tiene un solo dueno y el boton no
// puede dejar la maquina desfasada respecto de lo que se ve.
void elBotonAlternaLaSimulacion() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    orquestador.alternarSimulacion();
    CHECK(orquestador.getEstado() == ApplicationState::MainMenu,
          "el boton en el menu de inicio no arranca la simulacion");
    orquestador.iniciarEstudio();
    orquestador.alternarSimulacion();
    CHECK(orquestador.getEstado() == ApplicationState::Playing,
          "el boton en el editor arranca la simulacion");
    CHECK(orquestador.enSimulacion(), "el boton deja la simulacion corriendo");
    orquestador.alternarSimulacion();
    CHECK(orquestador.getEstado() == ApplicationState::Editing,
          "el boton en play la detiene y vuelve al editor");
    CHECK(!orquestador.enSimulacion() && !orquestador.simulacionPausada(),
          "el boton deja la simulacion cortada y sin pausa heredada");
    // Cortar una simulacion pausada tambien limpia la pausa.
    orquestador.alternarSimulacion();
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    orquestador.alternarSimulacion();
    CHECK(orquestador.getEstado() == ApplicationState::Editing &&
              !orquestador.simulacionPausada(),
          "el boton corta una simulacion pausada y limpia la pausa");
    // Y arrancar de nuevo no hereda esa pausa.
    orquestador.alternarSimulacion();
    CHECK(orquestador.enSimulacion() && !orquestador.simulacionPausada(),
          "arrancar con el boton no hereda la pausa");
}

void f5NoOpFueraDelEditor() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    CHECK(orquestador.getEstado() == ApplicationState::MainMenu,
          "F5 en el menu de inicio no activa la simulacion");
    CHECK(orquestador.enSimulacion() == false, "F5 en el menu no simula");
}

void f5ArrancaDesdeEditor() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    orquestador.iniciarEstudio();
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    CHECK(orquestador.getEstado() == ApplicationState::Playing,
          "F5 en el editor entra a Playing");
    CHECK(orquestador.enSimulacion(), "F5 pide que la simulacion corra");
    CHECK(!orquestador.simulacionPausada(),
          "F5 arranca sin pausa heredada del play anterior");
    // F5 repetido ya en play no reinicia nada.
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    CHECK(orquestador.getEstado() == ApplicationState::Playing,
          "F5 repetido en play es inofensivo");
}

void f6SoloEnPlay() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    CHECK(!orquestador.simulacionPausada(),
          "F6 en el menu no pausa nada");
    orquestador.iniciarEstudio();
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    CHECK(!orquestador.simulacionPausada(),
          "F6 en el editor (sin play) no pausa nada");
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    CHECK(orquestador.simulacionPausada(), "F6 pausa la simulacion en play");
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    CHECK(!orquestador.simulacionPausada(), "F6 reanuda la simulacion en play");
}

void f7ParaElPlay() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    orquestador.iniciarEstudio();
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    orquestador.manejarTeclaSimulacion(Tecla::Stop);
    CHECK(orquestador.getEstado() == ApplicationState::Editing,
          "F7 desde play vuelve a editar");
    CHECK(!orquestador.enSimulacion(), "F7 corta la simulacion");
    CHECK(!orquestador.simulacionPausada(),
          "F7 deja la pausa limpia para el proximo play");
    // F7 fuera del play: inofensivo.
    orquestador.manejarTeclaSimulacion(Tecla::Stop);
    CHECK(orquestador.getEstado() == ApplicationState::Editing,
          "F7 en el editor no hace nada");
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    CHECK(orquestador.enSimulacion(),
          "F7 no quedo 'pegada' despues de volver a editar");
    orquestador.manejarTeclaSimulacion(Tecla::Stop);
    orquestador.manejarTeclaSimulacion(Tecla::Ninguna);
    CHECK(orquestador.enSimulacion() == false, "tecla Ninguna no hace nada");
}

void reflejosPorEstado() {
    ApplicationStateMachine maquina;
    OrquestadorEstadoGUI orquestador(&maquina);
    CHECK(orquestador.escenaDebeCorrer() == false, "menu: escena no corre");
    orquestador.iniciarEstudio();
    CHECK(orquestador.escenaDebeCorrer(), "editor: escena corre");
    orquestador.manejarTeclaSimulacion(Tecla::Play);
    CHECK(orquestador.escenaDebeCorrer(), "play: escena corre (y simula)");
    orquestador.manejarTeclaSimulacion(Tecla::Pausa);
    CHECK(orquestador.escenaDebeCorrer() && orquestador.simulacionPausada(),
          "pausa: la escena sigue pero la simulacion congelada");
    orquestador.manejarTeclaSimulacion(Tecla::Stop);
    orquestador.manejarTeclaEscape();
    CHECK(orquestador.menuDebeEstarVisible(), "stop + Escape vuelven al menu");
}

// --- Atajos del editor frente al teclado de ImGui ----------------------------
// Ctrl+S guarda SIEMPRE: un InputText enfocado no hace nada con esa combinacion,
// y con el guard viejo (el mismo que Escape) renombrar un objeto y guardar sin
// salir del campo perdia el nombre en silencio. Ctrl+Z y Ctrl+Y si significan
// otra cosa dentro de un campo de texto (su propio deshacer/rehacer), asi que
// ahi el editor cede la tecla.
void atajosFrenteAlCampoDeTexto() {
    CHECK(AtajosEditor::debeGuardar(true),
          "Ctrl+S guarda aunque haya un campo de texto enfocado");
    CHECK(AtajosEditor::debeGuardar(false),
          "Ctrl+S guarda cuando no hay campo de texto enfocado");
    CHECK(AtajosEditor::cedeAlCampoDeTexto(true),
          "Ctrl+Z/Ctrl+Y le dejan el deshacer al campo de texto");
    CHECK(!AtajosEditor::cedeAlCampoDeTexto(false),
          "Ctrl+Z/Ctrl+Y son del editor cuando no hay campo enfocado");
}

} // namespace

int main() {
    estadoInicial();
    iniciarEstudio();
    escapePorEstado();
    dentroDelEditorSegunEstado();
    elBotonAlternaLaSimulacion();
    f5NoOpFueraDelEditor();
    f5ArrancaDesdeEditor();
    f6SoloEnPlay();
    f7ParaElPlay();
    reflejosPorEstado();
    atajosFrenteAlCampoDeTexto();

    std::cout << "orquestador-estado-tests: " << total << " comprobaciones, "
              << fallos << " fallos." << std::endl;
    return fallos == 0 ? 0 : 1;
}