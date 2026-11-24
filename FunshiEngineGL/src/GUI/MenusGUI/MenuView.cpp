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
#include "MenuViewDetalle.h"
#include "../../Localizacion/Localizacion.h"

namespace {

// Posiciona el cursor para el boton de la fila `fila` (0 = "Iniciar Estudio",
// 1 = "Configurar proyecto", 2 = "Opciones", 3 = "Salir"), centrado horizontalmente.
void cursorFila(int fila) {
    ImVec2 c = ImGui::GetWindowSize();
    c.x *= 0.5f;
    c.y *= 0.5f;
    ImGui::SetCursorPos(ImVec2(c.x - kBotonAncho * 0.5f,
                               c.y - 2.0f * kSeparacionVertical
                                   + fila * kSeparacionVertical));
}

// Alinea verticalmente una etiqueta a la derecha del boton recien dibujado.
void etiquetaDerecha(ImVec2 posBoton, const char* texto) {
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(
        posBoton.x + kBotonAncho + 10.0f,
        posBoton.y + (kBotonAlto - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::Text("%s", texto);
}

}  // namespace

MenuView::MenuView(MenuModel* model, StartMenuPresenter* presenter, GLFWwindow* window)
    : GeneralUserInterface("Menu", true, ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoDocking),
      model(model),
      presenter(presenter),
      window(window) {}

void MenuView::initGUI() {
    // Pantalla completa sin decoracion, fondo opaco propio del menu. El color
    // se deriva del tema activo (TemaEditor) en lugar de un azul fijo, asi el
    // menu acompana al modo claro/oscuro y al modo blanco y negro.
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImVec4 fondoMenu = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    fondoMenu.w = 1.0f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, fondoMenu);
    ImGui::Begin(getNameGui().c_str(), &stateGUI, getFlagGui());
}

void MenuView::contentGUI() {
    // Un solo Begin por frame: switch sobre la vista activa del modelo.
    switch (model->getVista()) {
    case MenuModel::Vista::Principal:
        renderizarPrincipal();
        break;
    case MenuModel::Vista::Opciones:
        renderizarOpciones();
        break;
    case MenuModel::Vista::ConfigProyecto:
        renderizarConfigProyecto();
        break;
    case MenuModel::Vista::Ninguna:
        break;
    }
}

void MenuView::endGUI() {
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void MenuView::printGUI() {
    // El menu (todas sus vistas) se dibuja solo si el modelo lo declara visible.
    if (!model->estaVisible()) return;
    initGUI();
    contentGUI();
    endGUI();
}

void MenuView::renderizarPrincipal() {
    // Sin proyecto elegido no se puede entrar al editor: "Iniciar Estudio"
    // queda deshabilitado hasta que se seleccione una carpeta de la lista
    // (o se confirme un proyecto en Configurar proyecto). Obliga a elegir
    // proyecto antes de abrir el estudio y evita crear "Nuevo Proyecto" solo.
    const bool sinProyectoElegido = model->getNombreProyecto().empty();
    if (sinProyectoElegido) {
        ImGui::BeginDisabled();
    }
    cursorFila(0);
    if (ImGui::Button(etiqueta(model, Texto::IniciarEstudio).c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        // Cerrar el menu afecta al resto del motor: se notifica por el
        // presenter (DebeCerrar -> ConsultarCierre en main), que hace la
        // transicion real. La vista solo informa la intencion.
        presenter->PedirCierre();
    }
    if (sinProyectoElegido) {
        ImGui::EndDisabled();
    }

    cursorFila(1);
    ImVec2 posBoton = ImGui::GetCursorScreenPos();
    if (ImGui::Button(etiqueta(model, Texto::ConfigProyecto).c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        model->abrirConfigProyecto();
    }
    etiquetaDerecha(posBoton, model->getNombreProyecto().c_str());

    cursorFila(2);
    posBoton = ImGui::GetCursorScreenPos();
    if (ImGui::Button(etiqueta(model, Texto::Opciones).c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        model->abrirOpciones();
    }
    etiquetaDerecha(posBoton, Localizacion::nombreIdioma(model->getIdioma()).c_str());

    cursorFila(3);
    if (ImGui::Button(etiqueta(model, Texto::Salir).c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}
