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

using Localizacion::Texto;

namespace {

// Dibuja una tarjeta de dimension dentro de la seccion "Crear proyecto": area
// reservada para la imagen (con la forma de ejemplo dibujada: cuadrado 2D /
// cubo 3D), el titulo y la descripcion accesible. El clic se registra sobre
// TODA la tarjeta (Selectable a tam completo); el contenido se dibuja encima
// con el draw list sin robarle la interaccion. Devuelve true si el usuario
// hizo clic en la tarjeta.
bool tarjetaDimension(const MenuModel* model, const char* id,
                      DimensionProyecto dimension, bool seleccionada,
                      ImVec2 tam) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clic = ImGui::Selectable(("##" + std::string(id)).c_str(),
                                        seleccionada, 0, tam);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 borde = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 forma = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 fondoImagen = ImGui::GetColorU32(ImGuiCol_WindowBg);

    constexpr float kPad = 16.0f;
    const ImVec2 imgIni(p.x + kPad, p.y + kPad);
    const ImVec2 imgFin(imgIni.x + tam.x - 2.0f * kPad,
                        imgIni.y + kImagenAlto);

    // Lugar reservado para la imagen del tipo de proyecto. Por ahora se dibuja
    // la forma de ejemplo (cuadrado 2D / cubo 3D) en ese lugar; la imagen real
    // reemplazara la forma. No se indica que falta la imagen: el usuario la
    // pondra luego, no se marca como pendiente ninguna carga.
    dl->AddRectFilled(imgIni, imgFin, fondoImagen);
    dl->AddRect(imgIni, imgFin, borde);
    const ImVec2 c((imgIni.x + imgFin.x) * 0.5f, (imgIni.y + imgFin.y) * 0.5f);
    const float lado =
        std::min(imgFin.x - imgIni.x, imgFin.y - imgIni.y) * 0.32f;
    if (dimension == DimensionProyecto::DosD) {
        // Cuadrado: la forma que describe "plano" para la tarjeta 2D.
        const ImVec2 a(c.x - lado, c.y - lado);
        const ImVec2 b(c.x + lado, c.y + lado);
        dl->AddRectFilled(a, b, forma);
        dl->AddRect(a, b, borde);
    } else {
        // Cubo alambrico: cara trasera desplazada en diagonal, aristas que
        // unen las dos caras y cara frontal.
        const float s = lado * 0.6f;
        const float dx = lado * 0.4f;
        const float dy = -lado * 0.32f;
        const ImVec2 f0(c.x - s, c.y - s);
        const ImVec2 f1(c.x + s, c.y + s);
        const ImVec2 t0(f0.x + dx, f0.y + dy);
        const ImVec2 t1(f1.x + dx, f1.y + dy);
        dl->AddRect(t0, t1, forma);
        dl->AddLine(t0, f0, forma);
        dl->AddLine(ImVec2(t0.x, t1.y), ImVec2(f0.x, f1.y), forma);
        dl->AddLine(ImVec2(t1.x, t0.y), ImVec2(f1.x, f0.y), forma);
        dl->AddLine(t1, f1, forma);
        dl->AddRect(f0, f1, forma);
    }

    // Titulo y descripcion dentro de la tarjeta. Se dibujan con el draw list y
    // NO con widgets de layout: un Text mueve el cursor y deja la linea actual
    // sobre el texto interno, con lo que SameLine (que alinea la segunda
    // tarjeta) tomaba esa linea en vez del tope de la tarjeta. Dibujados aparte,
    // el unico item de layout es el Selectable, asi que la tarjeta se comporta
    // como un bloque y las dos quedan alineadas por arriba.
    const std::string titulo =
        model->traducir(dimension == DimensionProyecto::DosD ? Texto::Dimension2D
                                                             : Texto::Dimension3D);
    const std::string descripcion =
        model->traducir(dimension == DimensionProyecto::DosD ? Texto::Descripcion2D
                                                             : Texto::Descripcion3D);
    const ImU32 colorTexto = ImGui::GetColorU32(ImGuiCol_Text);
    ImFont* fuente = ImGui::GetFont();
    const float tamFuente = ImGui::GetFontSize();
    const float textoX = p.x + kPad;
    const float textoAncho = tam.x - 2.0f * kPad;
    float textoY = p.y + kPad + kImagenAlto + 12.0f;
    dl->AddText(fuente, tamFuente, ImVec2(textoX, textoY), colorTexto,
                titulo.c_str());
    textoY += ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
    dl->AddText(fuente, tamFuente, ImVec2(textoX, textoY), colorTexto,
                descripcion.c_str(), nullptr, textoAncho);
    return clic;
}

}  // namespace

void MenuView::renderizarConfigProyecto() {
    const ImVec2 win = ImGui::GetWindowSize();
    const float altoContenido = win.y - 2.0f * kMargen - 20.0f;

    // Barra izquierda fija: navegacion entre las secciones. El pie lleva el
    // boton "Volver" que regresa al menu principal.
    ImGui::SetCursorPos(ImVec2(kMargen, kMargen));
    ImGui::BeginChild("##barraSecciones", ImVec2(kBarraAncho, altoContenido),
                      true);
    ImGui::TextUnformatted(model->traducir(Texto::ConfigProyecto));
    ImGui::Separator();

    const MenuModel::SeccionConfig activa = model->getSeccionConfig();
    if (ImGui::Selectable(etiqueta(model, Texto::CrearProyecto).c_str(),
                          activa == MenuModel::SeccionConfig::CrearProyecto)) {
        model->setSeccionConfig(MenuModel::SeccionConfig::CrearProyecto);
    }
    if (ImGui::Selectable(etiqueta(model, Texto::Proyectos).c_str(),
                          activa == MenuModel::SeccionConfig::Proyectos)) {
        model->setSeccionConfig(MenuModel::SeccionConfig::Proyectos);
    }
    ImGui::BeginDisabled();
    ImGui::Selectable(etiqueta(model, Texto::ImportarProyecto).c_str(),
                      activa == MenuModel::SeccionConfig::Importar);
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button(etiqueta(model, Texto::Volver).c_str(), ImVec2(-1.0f, 0.0f))) {
        model->volver();
    }
    ImGui::EndChild();

    // Panel derecho: la seccion activa de la barra.
    const float panelX = kMargen + kBarraAncho + kMargen;
    const float panelAncho = std::max(win.x - panelX - kMargen, 1.0f);
    ImGui::SetCursorPos(ImVec2(panelX, kMargen));
    ImGui::BeginChild("##panelSeccion", ImVec2(panelAncho, altoContenido), false,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
    switch (activa) {
    case MenuModel::SeccionConfig::CrearProyecto:
        renderizarSeccionCrearProyecto();
        break;
    case MenuModel::SeccionConfig::Proyectos:
        renderizarSeccionProyectos();
        break;
    case MenuModel::SeccionConfig::Importar:
        renderizarSeccionImportar();
        break;
    }
    ImGui::EndChild();

    renderizarModalCrearProyecto();
}

void MenuView::renderizarSeccionCrearProyecto() {
    ImGui::TextUnformatted(model->traducir(Texto::Dimension));
    ImGui::Separator();
    ImGui::Spacing();

    // Tarjetas 2D y 3D: elegir una abre el dialogo de creacion con esa
    // dimension. Las dos quedan visibles una al lado de la otra, alineadas por
    // su borde superior. Cada tarjeta es un unico item de layout (su contenido
    // va al draw list), asi que SameLine las alinea por el tope.
    const DimensionProyecto actual = model->getDimensionProyecto();
    const bool clic2D = tarjetaDimension(
        model, "2d", DimensionProyecto::DosD,
        actual == DimensionProyecto::DosD,
        ImVec2(kTarjetaAncho, kTarjetaAlto));
    ImGui::SameLine(0.0f, kEspacioTarjetas);
    const bool clic3D = tarjetaDimension(
        model, "3d", DimensionProyecto::TresD,
        actual == DimensionProyecto::TresD,
        ImVec2(kTarjetaAncho, kTarjetaAlto));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    // Advertencia de que el tipo de proyecto no se puede revertir: se muestra
    // debajo de las opciones, como pide el diseno.
    ImGui::TextWrapped(model->traducir(Texto::AvisoNoRevertir));

    if (clic2D || clic3D) {
        model->setDimensionProyecto(clic2D ? DimensionProyecto::DosD
                                           : DimensionProyecto::TresD);
        // El dialogo pide solo el nombre y su campo arranca vacio en cada
        // apertura (respuesta cerrada de diseno).
        nombreCrearBuffer[0] = '\0';
        abrirModalCrearProyecto = true;
    }
}

void MenuView::renderizarSeccionProyectos() {
    // Encabezado de la seccion: cual proyecto esta activo ahora mismo.
    ImGui::TextWrapped("%s: %s", model->traducir(Texto::ProyectoActual),
                       model->getNombreProyecto().c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Los proyectos son las carpetas de <directorioBase>/FunshiEngine-DATA. Elegir
    // una carpeta la vuelve el proyecto activo con la intencion Abrir (el
    // gestor de proyectos decide crear vs abrir mirando la existencia real de
    // la carpeta; la intencion solo redacta los mensajes).
    const std::vector<std::string>& proyectos = model->getProyectosDisponibles();
    if (proyectos.empty()) {
        ImGui::TextDisabled("%s", model->traducir(Texto::SinProyectos));
    } else {
        for (size_t i = 0; i < proyectos.size(); i++) {
            const std::string& nombre = proyectos[i];
            const bool seleccionado = (nombre == model->getNombreProyecto());
            // ID unico por fila (incluye el indice) para que ningun Selectable
            // colisione, incluso si dos carpetas tienen el mismo nombre.
            if (ImGui::Selectable((nombre + "###proyecto" +
                                   std::to_string(i))
                                      .c_str(),
                                  seleccionado)) {
                if (nombre != model->getNombreProyecto()) {
                    model->setNombreProyecto(nombre);
                    model->setAccionProyecto(MenuModel::AccionProyecto::Abrir);
                }
                model->limpiarProyectoARenombrar();
            }
            // Popup sin ID explicito: usa el ID del item anterior (unico por
            // fila). "Editar nombre" abre el modal de renombre: una ventana
            // con InputText para el nuevo nombre + botones Renombrar/Cancelar
            // (igual que ContentFolderInterface). El renombre se registra en
            // el modelo y lo ejecuta main en disco.
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem(etiqueta(model, Texto::EditarNombre).c_str())) {
                    proyectoRenombrando = nombre;
                    std::memset(nombreRenombrarBuffer, 0,
                                sizeof(nombreRenombrarBuffer));
                    std::strncpy(nombreRenombrarBuffer, nombre.c_str(),
                                 sizeof(nombreRenombrarBuffer) - 1);
                    abrirModalRenombrar = true;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem(
                        etiqueta(model, Texto::EliminarProyecto).c_str())) {
                    proyectoEliminando = nombre;
                    abrirModalEliminar = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    if (abrirModalRenombrar) {
        ImGui::OpenPopup("###RenombrarProyecto");
        abrirModalRenombrar = false;
    }
    if (!proyectoRenombrando.empty() &&
        ImGui::BeginPopupModal(
            Localizacion::etiqueta(Texto::TituloRenombrarProyecto, "###RenombrarProyecto").c_str(), NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", model->traducir(Texto::Nombre));
        ImGui::InputText("##renombrarProyecto", nombreRenombrarBuffer,
                         sizeof(nombreRenombrarBuffer));
        const bool esValido = nombreRenombrarBuffer[0] != '\0' &&
                              proyectoRenombrando != nombreRenombrarBuffer;
        if (!esValido) {
            ImGui::BeginDisabled();
        }
        const bool confirmado =
            ImGui::Button(etiqueta(model, Texto::Confirmar).c_str(),
                          ImVec2(140, 0)) ||
            (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
        if (!esValido) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button(etiqueta(model, Texto::Volver).c_str(), ImVec2(140, 0))) {
            proyectoRenombrando.clear();
            std::memset(nombreRenombrarBuffer, 0,
                        sizeof(nombreRenombrarBuffer));
            ImGui::CloseCurrentPopup();
        }
        if (confirmado && esValido) {
            // Confirmar renombra YA: se fija el nuevo nombre en el modelo y se
            // registra la carpeta original; main aplica el rename en disco el
            // proximo frame al detectar el cambio (mismo mecanismo que elegir
            // un proyecto de la lista).
            model->setProyectoARenombrar(proyectoRenombrando);
            model->setNombreProyecto(nombreRenombrarBuffer);
            proyectoRenombrando.clear();
            std::memset(nombreRenombrarBuffer, 0,
                        sizeof(nombreRenombrarBuffer));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (abrirModalEliminar) {
        ImGui::OpenPopup("###EliminarProyecto");
        abrirModalEliminar = false;
    }
    if (!proyectoEliminando.empty() &&
        ImGui::BeginPopupModal(
            Localizacion::etiqueta(Texto::TituloEliminarProyecto, "###EliminarProyecto").c_str(), NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", model->traducir(Texto::AvisoEliminar));
        ImGui::TextUnformatted(proyectoEliminando.c_str());
        ImGui::Separator();
        // Confirmar registra en el modelo la carpeta a eliminar; main la borra
        // de disco el proximo frame al detectar el cambio (mismo mecanismo que
        // el renombre por click derecho).
        const bool borrarConfirmado =
            ImGui::Button(etiqueta(model, Texto::Eliminar).c_str(), ImVec2(140, 0)) ||
            (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
        ImGui::SameLine();
        if (ImGui::Button(etiqueta(model, Texto::Volver).c_str(), ImVec2(140, 0))) {
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

void MenuView::renderizarSeccionImportar() {
    ImGui::TextUnformatted(model->traducir(Texto::ImportarProyecto));
    ImGui::Separator();
    ImGui::Spacing();
    // Visible pero deshabilitada: la seccion comunica que la opcion existe y
    // por que no se puede usar todavia.
    ImGui::TextWrapped(model->traducir(Texto::ImportarNoDisponible));
}

void MenuView::renderizarModalCrearProyecto() {
    if (abrirModalCrearProyecto) {
        ImGui::OpenPopup("###CrearProyecto");
        abrirModalCrearProyecto = false;
    }
    if (!ImGui::BeginPopupModal(
            Localizacion::etiqueta(Texto::TituloCrearProyecto, "###CrearProyecto").c_str(), NULL,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::Text("%s", model->traducir(Texto::Nombre));
    ImGui::InputText("##nombreCrearProyecto", nombreCrearBuffer,
                     sizeof(nombreCrearBuffer));

    // Validacion del dialogo: no vacio y no repetido. Mientras no sea valido
    // se avisa al usuario y se impide confirmar/crear (respuesta cerrada de
    // diseno de F1).
    const std::string nombre = nombreCrearBuffer;
    const bool vacio = nombre.empty();
    const bool repetido = model->existeProyecto(nombre);
    const bool esValido = !vacio && !repetido;
    if (vacio) {
        ImGui::TextDisabled("%s", model->traducir(Texto::NombreVacio));
    } else if (repetido) {
        ImGui::TextDisabled("%s", model->traducir(Texto::NombreRepetido));
    } else {
        ImGui::Spacing();
    }

    if (!esValido) ImGui::BeginDisabled();
    const bool confirmado =
        ImGui::Button(etiqueta(model, Texto::Crear).c_str(), ImVec2(140, 0)) ||
        (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
    if (!esValido) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(etiqueta(model, Texto::Cancelar).c_str(), ImVec2(140, 0))) {
        nombreCrearBuffer[0] = '\0';
        ImGui::CloseCurrentPopup();
    }
    if (confirmado && esValido) {
        // La dimension ya esta en el modelo (se fijo al elegir la tarjeta). El
        // confirmar registra el nombre nuevo y la intencion Crear; main la
        // consume al sincronizar proyectos (que decide crear vs abrir segun
        // exista la carpeta).
        model->setNombreProyecto(nombre);
        model->setAccionProyecto(MenuModel::AccionProyecto::Crear);
        nombreCrearBuffer[0] = '\0';
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
