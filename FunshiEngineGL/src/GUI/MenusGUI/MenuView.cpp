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

#include <algorithm>
#include <cstring>
#include <string>

#include "../../Configuracion/Apariencia.h"

namespace {
// Constantes de layout del menu: tamanio de boton y desvios verticales desde
// el centro de la ventana para cada fila del menu principal.
constexpr float kBotonAncho = 200.0f;
constexpr float kBotonAlto = 50.0f;
constexpr float kSeparacionVertical = 60.0f;
// Vista ConfigProyecto: barra izquierda de navegacion fija + panel derecho de
// la seccion activa.
constexpr float kMargen = 40.0f;
constexpr float kBarraAncho = 220.0f;
// Tarjetas de la seccion "Crear proyecto": area reservada para la imagen de
// ejemplo + titulo + descripcion accesible.
constexpr float kTarjetaAncho = 260.0f;
constexpr float kTarjetaAlto = 280.0f;
constexpr float kImagenAlto = 96.0f;
constexpr float kEspacioTarjetas = 24.0f;

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
        model->traducir(dimension == DimensionProyecto::DosD ? "dimension_2d"
                                                             : "dimension_3d");
    const std::string descripcion =
        model->traducir(dimension == DimensionProyecto::DosD ? "descripcion_2d"
                                                             : "descripcion_3d");
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
} // namespace

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
    // (o se confirme un proyecto en Config Proyect). Obliga a elegir
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

void MenuView::renderizarOpciones() {
    // Columna centrada con scroll.
    const ImVec2 win = ImGui::GetWindowSize();
    constexpr float kAncho = 460.0f;
    constexpr float kAltoInferior = 90.0f; // deja lugar al boton "Volver"

    ImGui::SetCursorPos(ImVec2((win.x - kAncho) * 0.5f, 50.0f));
    ImGui::BeginChild("##opciones", ImVec2(kAncho, win.y - 50.0f - kAltoInferior),
                      false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::TextUnformatted(model->traducir("opciones").c_str());
    ImGui::Separator();

    // Idioma: afecta a todas las pestañas, por eso vive fuera de ellas.
    {
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
    }

    ImGui::Spacing();

    if (ImGui::BeginTabBar("##pestanasOpciones")) {
        // ---- Audio: volumen maestro de efectos de sonido. ----
        if (ImGui::BeginTabItem(model->traducir("audio").c_str())) {
            float volumen = model->getVolumenMaestro();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##volumenMaestro", &volumen, 0.0f, 1.0f,
                                   "%.2f")) {
                model->setVolumenMaestro(volumen);
            }
            ImGui::TextDisabled(
                "Afecta a todos los efectos de sonido de la escena.");
            ImGui::EndTabItem();
        }

        // ---- Apariencia: tema, B/N, acento y fondos de cielo. ----
        if (ImGui::BeginTabItem(model->traducir("apariencia").c_str())) {
            ImGui::TextUnformatted(model->traducir("radio_difuminado").c_str());
            // Radio del circulo-horizonte de la grilla y de la guia de eje,
            // en unidades de mundo: por debajo del slider el piso se ve mas
            // cerca y con menos lineas (mas performant); por encima llega mas
            // lejos. Se edita sobre una copia y se delega al modelo UNA vez,
            // como el resto de apariencia.
            {
                Apariencia apRadio = model->getApariencia();
                float radio = apRadio.radioDifuminado;
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderFloat("##radioDifuminado", &radio,
                                       AparienciaUtil::kRadioDifuminadoMinimo,
                                       AparienciaUtil::kRadioDifuminadoMaximo,
                                       "%.0f")) {
                    apRadio.radioDifuminado = radio;
                    model->setApariencia(apRadio);
                }
            }

            // Se edita una copia y se delega al modelo UNA vez si hubo
            // cambios; asi el modelo sigue siendo la unica fuente de verdad
            // (patron MVP).
            Apariencia ap = model->getApariencia();
            bool cambio = false;

            cambio |= ImGui::Checkbox(etiqueta(model, "tema_claro").c_str(),
                                      &ap.temaClaro);
            cambio |= ImGui::Checkbox(etiqueta(model, "modo_bn").c_str(),
                                      &ap.blancoYNegro);
            ImGui::TextUnformatted(model->traducir("color_acento").c_str());
            // Acento solo RGB (ColorEdit3 = ColorEdit4 + NoAlpha): el alpha
            // del perfil no participa del tema (cada rol aporta su propia
            // transparencia, ver TemaEditor::acento), asi que no se expone la
            // barra de alpha que podia apagar los checks, los grabs y los
            // enlaces de la interfaz. ColorEdit3 preserva acento[3] tal como
            // estaba guardado.
            cambio |= ImGui::ColorEdit3("##acento", ap.acento);
            ImGui::TextUnformatted(model->traducir("color_cielo_superior").c_str());
            cambio |= ImGui::ColorEdit3("##cieloSuperior", ap.fondoSuperior);
            ImGui::TextUnformatted(model->traducir("color_cielo_inferior").c_str());
            cambio |= ImGui::ColorEdit3("##cieloInferior", ap.fondoInferior);
            ImGui::TextDisabled("%s", model->traducir("ayuda_bn").c_str());

            if (ImGui::Button(etiqueta(model, "restablecer_apariencia").c_str(),
                              ImVec2(-1.0f, 0.0f))) {
                ap.restablecer();
                cambio = true;
            }

            if (cambio) model->setApariencia(ap);
            ImGui::EndTabItem();
        }

        // ---- Controles: sensibilidad de camara y de movimiento. ----
        if (ImGui::BeginTabItem(model->traducir("controles").c_str())) {
            ImGui::TextUnformatted(model->traducir("sensibilidad_camara").c_str());
            float sensibilidad = model->getSensibilidadCamara();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##sensibilidad", &sensibilidad, 0.02f,
                                   5.0f, "%.2f")) {
                model->setSensibilidadCamara(sensibilidad);
            }

            ImGui::TextUnformatted(model->traducir("sensibilidad_movimiento").c_str());
            float movimiento = model->getSensibilidadMovimientoCamara();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##sensibilidadMovimiento", &movimiento,
                                   0.1f, 5.0f, "%.2f")) {
                model->setSensibilidadMovimientoCamara(movimiento);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

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
    const float altoContenido = win.y - 2.0f * kMargen - 20.0f;

    // Barra izquierda fija: navegacion entre las secciones. El pie lleva el
    // boton "Volver" que regresa al menu principal.
    ImGui::SetCursorPos(ImVec2(kMargen, kMargen));
    ImGui::BeginChild("##barraSecciones", ImVec2(kBarraAncho, altoContenido),
                      true);
    ImGui::TextUnformatted(model->traducir("config_proyecto").c_str());
    ImGui::Separator();

    const MenuModel::SeccionConfig activa = model->getSeccionConfig();
    if (ImGui::Selectable(etiqueta(model, "crear_proyecto").c_str(),
                          activa == MenuModel::SeccionConfig::CrearProyecto)) {
        model->setSeccionConfig(MenuModel::SeccionConfig::CrearProyecto);
    }
    if (ImGui::Selectable(etiqueta(model, "proyectos").c_str(),
                          activa == MenuModel::SeccionConfig::Proyectos)) {
        model->setSeccionConfig(MenuModel::SeccionConfig::Proyectos);
    }
    ImGui::BeginDisabled();
    ImGui::Selectable(etiqueta(model, "importar_proyecto").c_str(),
                      activa == MenuModel::SeccionConfig::Importar);
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button(etiqueta(model, "volver").c_str(), ImVec2(-1.0f, 0.0f))) {
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
    ImGui::TextUnformatted(model->traducir("dimension").c_str());
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
    ImGui::TextWrapped(model->traducir("aviso_no_revertir").c_str());

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
    ImGui::TextWrapped("%s: %s", model->traducir("proyecto_actual").c_str(),
                       model->getNombreProyecto().c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Los proyectos son las carpetas de <directorioBase>/FunshiEngine-DATA. Elegir
    // una carpeta la vuelve el proyecto activo con la intencion Abrir (el
    // gestor de proyectos decide crear vs abrir mirando la existencia real de
    // la carpeta; la intencion solo redacta los mensajes).
    const std::vector<std::string>& proyectos = model->getProyectosDisponibles();
    if (proyectos.empty()) {
        ImGui::TextDisabled("%s", model->traducir("sin_proyectos").c_str());
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

void MenuView::renderizarSeccionImportar() {
    ImGui::TextUnformatted(model->traducir("importar_proyecto").c_str());
    ImGui::Separator();
    ImGui::Spacing();
    // Visible pero deshabilitada: la seccion comunica que la opcion existe y
    // por que no se puede usar todavia.
    ImGui::TextWrapped(model->traducir("importar_no_disponible").c_str());
}

void MenuView::renderizarModalCrearProyecto() {
    if (abrirModalCrearProyecto) {
        ImGui::OpenPopup("CrearProyecto");
        abrirModalCrearProyecto = false;
    }
    if (!ImGui::BeginPopupModal("CrearProyecto", NULL,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::Text("%s", model->traducir("nombre").c_str());
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
        ImGui::TextDisabled("%s", model->traducir("nombre_vacio").c_str());
    } else if (repetido) {
        ImGui::TextDisabled("%s", model->traducir("nombre_repetido").c_str());
    } else {
        ImGui::Spacing();
    }

    if (!esValido) ImGui::BeginDisabled();
    const bool confirmado =
        ImGui::Button(etiqueta(model, "crear").c_str(), ImVec2(140, 0)) ||
        (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
    if (!esValido) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(etiqueta(model, "cancelar").c_str(), ImVec2(140, 0))) {
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