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
#include "MenuView.h"

#include <cstring>
#include <string>

namespace {
// Constantes de layout del menu (solo lectura): tamanio de boton y desvios
// verticales desde el centro de la ventana para cada fila.
constexpr float kBotonAncho = 200.0f;
constexpr float kBotonAlto = 50.0f;
constexpr float kSeparacionVertical = 60.0f;

// Posiciona el cursor para el boton de la fila `fila` (0 = "Iniciar Estudio",
// 1 = "Config Proyect", 2 = "Opciones", 3 = "Exit"), centrado horizontalmente.
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

// Etiqueta traducida con ID estable. ImGui deriva el ID de un widget de su
// texto, asi que pasar la traduccion directa hacia que el ID cambiara al
// cambiar de idioma (se perdia el foco/estado del widget en ese frame y los
// IDs no eran predecibles). Se agrega el sufijo oculto "##<clave>": el usuario
// ve solo el texto traducido y el ID queda atado a la clave, no al idioma.
std::string etiqueta(const MenuModel* model, const char* clave) {
    return model->traducir(clave) + "##" + clave;
}
} // namespace

MenuView::MenuView(MenuModel* model, StartMenuPresenter* presenter, GLFWwindow* window)
    : GeneralUserInterface("Menu", true, ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoDocking),
      model(model),
      presenter(presenter),
      window(window),
      nombreProyectoBuffer{0} {}

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
        renderizarListaProyectos();
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
    // (o se tipee/confirme un nombre en Config Proyect). Obliga a elegir
    // proyecto antes de abrir el estudio y evita crear "Nuevo Proyecto" solo.
    const bool sinProyectoElegido = model->getNombreProyecto().empty();
    if (sinProyectoElegido) {
        ImGui::BeginDisabled();
    }
    cursorFila(0);
    if (ImGui::Button(etiqueta(model, "iniciar_estudio").c_str(),
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
    if (ImGui::Button(etiqueta(model, "config_proyecto").c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        model->abrirConfigProyecto();
    }
    etiquetaDerecha(posBoton, model->getNombreProyecto().c_str());

    cursorFila(2);
    posBoton = ImGui::GetCursorScreenPos();
    if (ImGui::Button(etiqueta(model, "opciones").c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        model->abrirOpciones();
    }
    etiquetaDerecha(posBoton, model->getIdioma().c_str());

    cursorFila(3);
    if (ImGui::Button(etiqueta(model, "salir").c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void MenuView::renderizarListaProyectos() {
    // Lista de proyectos a la izquierda: los proyectos son las carpetas de
    // <directorioBase>/MotorGrafico. Elegir una carpeta la vuelve el proyecto
    // activo (setNombreProyecto); main sincroniza el FileManager al detectar
    // el cambio de nombre. La lista se rellena desde la fachada por frame.
    // El layout es: lista fija a la izquierda (ancho 300), formulario de
    // edicion a la derecha (ver renderizarConfigProyecto).
    const ImVec2 win = ImGui::GetWindowSize();
    ImGui::SetCursorPos(ImVec2(40.0f, 60.0f));
    ImGui::BeginChild("##listaProyectos", ImVec2(300.0f, win.y - 140.0f), true);
    ImGui::TextUnformatted(model->traducir("proyectos").c_str());
    ImGui::Separator();
    const std::vector<std::string>& proyectos = model->getProyectosDisponibles();
    if (proyectos.empty()) {
        ImGui::TextDisabled("%s", model->traducir("sin_proyectos").c_str());
    } else {
        for (size_t i = 0; i < proyectos.size(); i++) {
            const std::string& nombre = proyectos[i];
            const bool seleccionado = (nombre == model->getNombreProyecto());
            // ID unico por fila (incluye el indice) para que ningun Selectable
            // colisione, incluso si dos carpetas tienen el mismo nombre.
            if (ImGui::Selectable((nombre + "###proyecto" + std::to_string(i)).c_str(),
                                  seleccionado)) {
                // Cambiar de proyecto: actualiza el modelo Y sincroniza el
                // buffer de edicion para que el InputText muestre el proyecto
                // recien elegido (sin marcarlo como pendiente).
                model->setNombreProyecto(nombre);
                model->limpiarProyectoARenombrar();
                std::strncpy(nombreProyectoBuffer, nombre.c_str(),
                             sizeof(nombreProyectoBuffer) - 1);
                nombreProyectoBuffer[sizeof(nombreProyectoBuffer) - 1] = '\0';
                nombreProyectoPendiente = false;
            }
            // Popup sin ID explicito: usa el ID del item anterior (unico por
            // fila). "Editar nombre" abre el modal de renombre: una ventana
            // con InputText para el nuevo nombre + botones Renombrar/Cancelar
            // (igual que ContentFolderInterface). El renombre se registra en
            // el modelo y lo ejecuta main en disco.
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem(etiqueta(model, "editar_nombre").c_str())) {
                    proyectoRenombrando = nombre;
                    std::memset(nombreRenombrarBuffer, 0,
                                sizeof(nombreRenombrarBuffer));
                    std::strncpy(nombreRenombrarBuffer, nombre.c_str(),
                                  sizeof(nombreRenombrarBuffer) - 1);
                    abrirModalRenombrar = true;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem(
                        etiqueta(model, "eliminar_proyecto").c_str())) {
                    proyectoEliminando = nombre;
                    abrirModalEliminar = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::EndChild();

    if (abrirModalRenombrar) {
        ImGui::OpenPopup("RenombrarProyecto");
        abrirModalRenombrar = false;
    }
    if (!proyectoRenombrando.empty() &&
        ImGui::BeginPopupModal("RenombrarProyecto", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", model->traducir("nombre").c_str());
        ImGui::InputText("##renombrarProyecto", nombreRenombrarBuffer,
                         sizeof(nombreRenombrarBuffer));
        const bool esValido = nombreRenombrarBuffer[0] != '\0' &&
                              proyectoRenombrando != nombreRenombrarBuffer;
        if (!esValido) {
            ImGui::BeginDisabled();
        }
        const bool confirmado =
            ImGui::Button(etiqueta(model, "confirmar").c_str(),
                          ImVec2(140, 0)) ||
            (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
        if (!esValido) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button(etiqueta(model, "volver").c_str(), ImVec2(140, 0))) {
            proyectoRenombrando.clear();
            std::memset(nombreRenombrarBuffer, 0,
                        sizeof(nombreRenombrarBuffer));
            ImGui::CloseCurrentPopup();
        }
        if (confirmado && esValido) {
            // Confirmar renombra YA: se fija el nuevo nombre en el modelo y se
            // registra la carpeta original; main aplica el rename en disco el
            // proximo frame al detectar el cambio (mismo mecanismo que elegir
            // un proyecto de la lista). El formulario lateral se sincroniza
            // con el nombre confirmado (sin marcarlo como pendiente).
            model->setProyectoARenombrar(proyectoRenombrando);
            model->setNombreProyecto(nombreRenombrarBuffer);
            std::strncpy(nombreProyectoBuffer, nombreRenombrarBuffer,
                          sizeof(nombreProyectoBuffer) - 1);
            nombreProyectoBuffer[sizeof(nombreProyectoBuffer) - 1] = '\0';
            nombreProyectoPendiente = false;
            proyectoRenombrando.clear();
            std::memset(nombreRenombrarBuffer, 0,
                        sizeof(nombreRenombrarBuffer));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (abrirModalEliminar) {
        ImGui::OpenPopup("EliminarProyecto");
        abrirModalEliminar = false;
    }
    if (!proyectoEliminando.empty() &&
        ImGui::BeginPopupModal("EliminarProyecto", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", model->traducir("aviso_eliminar").c_str());
        ImGui::TextUnformatted(proyectoEliminando.c_str());
        ImGui::Separator();
        // Confirmar registra en el modelo la carpeta a eliminar; main la borra
        // de disco el proximo frame al detectar el cambio (mismo mecanismo que
        // el renombre por click derecho).
        const bool borrarConfirmado =
            ImGui::Button(etiqueta(model, "eliminar").c_str(), ImVec2(140, 0)) ||
            (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
        ImGui::SameLine();
        if (ImGui::Button(etiqueta(model, "volver").c_str(), ImVec2(140, 0))) {
            proyectoEliminando.clear();
            ImGui::CloseCurrentPopup();
        }
        if (borrarConfirmado) {
            model->setProyectoAEliminar(proyectoEliminando);
            proyectoEliminando.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MenuView::renderizarOpciones() {
    // Columna centrada con scroll: la vista crecio con las opciones de
    // apariencia y ya no entra en las filas fijas del menu principal.
    const ImVec2 win = ImGui::GetWindowSize();
    constexpr float kAncho = 460.0f;
    constexpr float kAltoInferior = 90.0f; // deja lugar al boton "Volver"

    ImGui::SetCursorPos(ImVec2((win.x - kAncho) * 0.5f, 50.0f));
    ImGui::BeginChild("##opciones", ImVec2(kAncho, win.y - 50.0f - kAltoInferior),
                      false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::TextUnformatted(model->traducir("opciones").c_str());
    ImGui::Separator();

    ImGui::SeparatorText(model->traducir("juego").c_str());
    ImGui::TextUnformatted(model->traducir("idioma").c_str());
    const std::vector<std::string>& idiomas = model->getIdiomas();
    const std::string& actual = model->getIdioma();
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##idioma", actual.c_str())) {
        for (const std::string& opcion : idiomas) {
            const bool seleccionada = (opcion == actual);
            if (ImGui::Selectable(opcion.c_str(), seleccionada)) {
                model->setIdioma(opcion);
            }
            if (seleccionada) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::TextUnformatted(model->traducir("sensibilidad_movimiento").c_str());
    float movimiento = model->getSensibilidadMovimientoCamara();
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderFloat("##sensibilidadMovimiento", &movimiento, 0.1f,
                           5.0f, "%.2f")) {
        model->setSensibilidadMovimientoCamara(movimiento);
    }

    ImGui::SeparatorText(model->traducir("apariencia").c_str());
    // Se edita una copia y se delega al modelo UNA vez si hubo cambios; asi
    // el modelo sigue siendo la unica fuente de verdad (patron MVP).
    Apariencia ap = model->getApariencia();
    bool cambio = false;

    cambio |= ImGui::Checkbox(etiqueta(model, "tema_claro").c_str(),
                              &ap.temaClaro);
    cambio |= ImGui::Checkbox(etiqueta(model, "modo_bn").c_str(),
                              &ap.blancoYNegro);
    ImGui::TextUnformatted(model->traducir("color_acento").c_str());
    // Acento solo RGB (ColorEdit3 = ColorEdit4 + NoAlpha): el alpha del perfil
    // no participa del tema (cada rol aporta su propia transparencia, ver
    // TemaEditor::acento), asi que no se expone la barra de alpha que podia
    // apagar los checks, los grabs y los enlaces de la interfaz. ColorEdit3
    // preserva acento[3] tal como estaba guardado.
    cambio |= ImGui::ColorEdit3("##acento", ap.acento);
    ImGui::TextUnformatted(model->traducir("color_fondo").c_str());
    cambio |= ImGui::ColorEdit3("##fondo", ap.fondo);
    ImGui::TextDisabled("%s", model->traducir("ayuda_bn").c_str());

    if (ImGui::Button(etiqueta(model, "restablecer_apariencia").c_str(),
                      ImVec2(-1.0f, 0.0f))) {
        ap.restablecer();
        cambio = true;
    }

    if (cambio) model->setApariencia(ap);

    ImGui::SeparatorText(model->traducir("configuracion").c_str());
    if (ImGui::Button(etiqueta(model, "restablecer_configuracion").c_str(),
                      ImVec2(-1.0f, 0.0f))) {
        model->restablecerConfiguracion();
    }
    ImGui::TextDisabled("%s", model->traducir("ayuda_reset").c_str());

    ImGui::EndChild();

    // Boton "Volver" fijo abajo, centrado, fuera del area con scroll.
    ImGui::SetCursorPos(ImVec2((win.x - kBotonAncho) * 0.5f,
                               win.y - kAltoInferior + 20.0f));
    if (ImGui::Button(etiqueta(model, "volver").c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        model->volver();
    }
}

void MenuView::renderizarConfigProyecto() {
    const ImVec2 win = ImGui::GetWindowSize();
    constexpr float kListaAncho = 300.0f;
    constexpr float kListaX = 40.0f;
    constexpr float kFormularioX = kListaX + kListaAncho + 40.0f;
    constexpr float kFormularioAncho = 360.0f;

    const float formularioRight = kFormularioX + kFormularioAncho;
    if (formularioRight > win.x - 40.0f) {
        (void)win;
        return;
    }

    ImGui::SetCursorPos(ImVec2(kFormularioX, 60.0f));
    ImGui::BeginChild("##configProyectoForm", ImVec2(kFormularioAncho, win.y - 140.0f), false);

    ImGui::SeparatorText(model->traducir("config_proyecto").c_str());
    ImGui::Spacing();

    // El buffer se rellena desde el modelo la primera vez que se abre la
    // vista (o cuando no hay cambios pendientes). Mientras el usuario escribe,
    // solo se actualiza el buffer local; el modelo no se toca hasta Confirmar.
    if (nombreProyectoBuffer[0] == '\0' && !nombreProyectoPendiente) {
        std::strncpy(nombreProyectoBuffer,
                     model->getNombreProyecto().c_str(),
                     sizeof(nombreProyectoBuffer) - 1);
        nombreProyectoBuffer[sizeof(nombreProyectoBuffer) - 1] = '\0';
    }

    // Campo: Nombre del proyecto
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", model->traducir("nombre").c_str());
    ImGui::SameLine(kFormularioAncho * 0.4f);
    ImGui::SetNextItemWidth(kFormularioAncho * 0.55f);
    if (ImGui::InputText("##nombreProyecto", nombreProyectoBuffer,
                         sizeof(nombreProyectoBuffer))) {
        nombreProyectoPendiente = true;
    }

    if (nombreProyectoPendiente) {
        ImGui::SameLine();
        ImGui::TextDisabled("(%s: %s)", model->traducir("actual").c_str(),
                            model->getNombreProyecto().c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Botones centrados
    const float btnWidth = 140.0f;
    const float btnSpacing = 20.0f;
    const float totalBtnWidth = btnWidth * 2 + btnSpacing;
    const float btnStartX = (kFormularioAncho - totalBtnWidth) * 0.5f;

    ImGui::SetCursorPosX(btnStartX);

    // Confirmar
    const bool confirmarDeshabilitado =
        !nombreProyectoPendiente || nombreProyectoBuffer[0] == '\0';
    if (confirmarDeshabilitado) ImGui::BeginDisabled();
    if (ImGui::Button(etiqueta(model, "confirmar").c_str(),
                      ImVec2(btnWidth, 0))) {
        model->setNombreProyecto(nombreProyectoBuffer);
        nombreProyectoPendiente = false;
    }
    if (confirmarDeshabilitado) ImGui::EndDisabled();

    ImGui::SameLine(0, btnSpacing);

    // Volver
    if (ImGui::Button(etiqueta(model, "volver").c_str(),
                      ImVec2(btnWidth, 0))) {
        if (nombreProyectoPendiente) {
            nombreProyectoBuffer[0] = '\0';
            nombreProyectoPendiente = false;
        }
        model->limpiarProyectoARenombrar();
        model->volver();
    }

    ImGui::EndChild();
    (void)win;
}