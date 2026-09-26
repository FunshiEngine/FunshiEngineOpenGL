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
#include "CreadorDeInterfaces.h"

#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace {
const char* nombreTipo(TipoWidget tipo) {
    switch (tipo) {
        case TipoWidget::Etiqueta: return "Etiqueta";
        case TipoWidget::Boton: return "Boton";
        case TipoWidget::Checkbox: return "Checkbox";
        case TipoWidget::Slider: return "Slider";
        case TipoWidget::EntradaTexto: return "Entrada de texto";
    }
    return "Etiqueta";
}
} // namespace

CreadorDeInterfaces::CreadorDeInterfaces(bool stateGUI)
    : GeneralUserInterface("Creador de Interfaces", stateGUI, ImGuiWindowFlags_None) {}

void CreadorDeInterfaces::refrescarLista() {
    interfaces_.clear();
    std::error_code ec;
    if (directorio_.empty() || !fs::is_directory(directorio_, ec)) return;
    for (const auto& entrada : fs::directory_iterator(directorio_, ec)) {
        if (ec) break;
        if (!entrada.is_regular_file(ec) || entrada.path().extension() != ".json")
            continue;
        interfaces_.push_back(entrada.path().stem().string());
    }
    std::sort(interfaces_.begin(), interfaces_.end());
}

void CreadorDeInterfaces::configurarProyecto(const std::string& directorio) {
    directorio_ = directorio;
    tieneActiva_ = false;
    interfazActiva_ = {};
    nombreActiva_.clear();
    borrador_ = {};
    widgetSeleccionado_ = -1;
    refrescarLista();
}

void CreadorDeInterfaces::setClipNames(std::vector<std::string> nombres) {
    clipNames_ = std::move(nombres);
}

void CreadorDeInterfaces::crearNueva() {
    borrador_ = {};
    borrador_.nombre = "Interfaz " + std::to_string(interfaces_.size() + 1);
    borrador_.titulo = borrador_.nombre;
    widgetSeleccionado_ = -1;
}

UserInterfaceCustom* CreadorDeInterfaces::getInterfazActiva() {
    return tieneActiva_ ? &interfazActiva_ : nullptr;
}

const UserInterfaceCustom* CreadorDeInterfaces::getInterfazActiva() const {
    return tieneActiva_ ? &interfazActiva_ : nullptr;
}

void CreadorDeInterfaces::contentGUI() {
    if (directorio_.empty()) {
        ImGui::TextUnformatted(
            "Configura primero un proyecto: Memory/Interfaces quedara listo.");
        return;
    }

    if (ImGui::BeginChild("ListaInterfaces", ImVec2(0.f, 90.f), true)) {
        for (const std::string& nombre : interfaces_) {
            const bool activa = nombreActiva_ == nombre;
            ImGui::PushID(nombre.c_str());
            if (ImGui::Selectable(nombre.c_str(), activa)) {
                UserInterfaceCustom ui;
                ui.nombre = nombre;
                if (ui.cargar(directorio_)) {
                    borrador_ = ui;
                    widgetSeleccionado_ = -1;
                }
            }
            if (activa && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Es la interfaz activa en el canvas");
            }
            ImGui::PopID();
        }
        if (interfaces_.empty())
            ImGui::TextDisabled("(sin interfaces: crea una y guardala)");
    }
    ImGui::EndChild();

    if (ImGui::Button("Nueva interfaz")) crearNueva();
    ImGui::SameLine();
    const bool guardarOK =
        ImGui::Button("Guardar") && !borrador_.nombre.empty();
    if (guardarOK) {
        borrador_.guardar(directorio_);
        refrescarLista();
    }
    ImGui::SameLine();
    if (ImGui::Button("Activar en canvas") && !borrador_.nombre.empty()) {
        borrador_.guardar(directorio_);
        interfazActiva_ = borrador_;
        nombreActiva_ = borrador_.nombre;
        tieneActiva_ = true;
        refrescarLista();
    }
    ImGui::SameLine();
    if (ImGui::Button("Quitar del canvas")) {
        tieneActiva_ = false;
        nombreActiva_.clear();
        interfazActiva_ = {};
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Edicion:");

    {
        char nombre[256] = {};
        std::snprintf(nombre, sizeof(nombre), "%s", borrador_.nombre.c_str());
        if (ImGui::InputText("Nombre", nombre, sizeof(nombre)))
            borrador_.nombre = nombre;
        if (borrador_.nombre.empty())
            ImGui::TextDisabled("(nombre vacio: no se puede guardar)");
    }
    {
        char titulo[256] = {};
        std::snprintf(titulo, sizeof(titulo), "%s", borrador_.titulo.c_str());
        if (ImGui::InputText("Titulo", titulo, sizeof(titulo)))
            borrador_.titulo = titulo;
    }
    ImGui::SliderFloat("Ancho", &borrador_.ancho, 120.f, 1200.f, "%.0f");
    ImGui::SliderFloat("Alto", &borrador_.alto, 100.f, 800.f, "%.0f");

    ImGui::SeparatorText("Widgets");
    for (std::size_t i = 0; i < borrador_.widgets.size(); ++i) {
        WidgetUI& widget = borrador_.widgets[i];
        ImGui::PushID(static_cast<int>(i));
        std::string etiquetaVista = widget.etiqueta;
        if (etiquetaVista.empty()) etiquetaVista = widget.nombre;
        if (etiquetaVista.empty()) etiquetaVista = nombreTipo(widget.tipo);
        const bool seleccion = widgetSeleccionado_ == static_cast<int>(i);
        if (ImGui::Selectable(etiquetaVista.c_str(), seleccion)) {
            widgetSeleccionado_ = static_cast<int>(i);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Eliminar")) {
            borrador_.widgets.erase(
                borrador_.widgets.begin() + static_cast<std::ptrdiff_t>(i));
            widgetSeleccionado_ = -1;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
    if (ImGui::Button("Agregar widget")) {
        WidgetUI w;
        w.tipo = TipoWidget::Boton;
        w.sonido = kSinSonido;
        w.etiqueta = "Boton " + std::to_string(borrador_.widgets.size() + 1);
        w.nombre = "widget" + std::to_string(borrador_.widgets.size() + 1);
        borrador_.widgets.push_back(w);
        widgetSeleccionado_ = static_cast<int>(borrador_.widgets.size()) - 1;
    }

    if (widgetSeleccionado_ >= 0 &&
        widgetSeleccionado_ < static_cast<int>(borrador_.widgets.size())) {
        WidgetUI& w = borrador_.widgets[widgetSeleccionado_];
        ImGui::SeparatorText("Widget seleccionado");

        const char* tipos[] = {"Etiqueta", "Boton", "Checkbox",
                               "Slider", "Entrada de texto"};
        int actual = static_cast<int>(w.tipo);
        if (ImGui::Combo("Tipo", &actual, tipos, 5))
            w.tipo = static_cast<TipoWidget>(actual);

        char nombre[256] = {};
        std::snprintf(nombre, sizeof(nombre), "%s", w.nombre.c_str());
        if (ImGui::InputText("Nombre", nombre, sizeof(nombre)))
            w.nombre = nombre;

        char etiqueta[256] = {};
        std::snprintf(etiqueta, sizeof(etiqueta), "%s", w.etiqueta.c_str());
        if (ImGui::InputText("Etiqueta", etiqueta, sizeof(etiqueta)))
            w.etiqueta = etiqueta;

        // Dropdown de sonido: los clips del proyecto (Sonidos/).
        std::vector<const char*> opciones;
        opciones.push_back(kSinSonido);
        for (const std::string& c : clipNames_) opciones.push_back(c.c_str());
        int actualSonido = 0;
        if (!w.sonido.empty()) {
            // Comparar por contenido (std::find con const char* compararia
            // punteros y nunca encontraria el clip seleccionado).
            const auto it = std::find_if(
                opciones.begin(), opciones.end(),
                [&w](const char* op) { return w.sonido == op; });
            actualSonido = static_cast<int>(it - opciones.begin());
        }
        if (ImGui::Combo("Sonido", &actualSonido, opciones.data(),
                         static_cast<int>(opciones.size())))
            w.sonido = opciones[static_cast<std::size_t>(actualSonido)];

        switch (w.tipo) {
            case TipoWidget::Slider:
                ImGui::SliderFloat("Valor", &w.valor, w.minimo, w.maximo,
                                   "%.2f");
                ImGui::DragFloat("Minimo", &w.minimo, 0.01f);
                ImGui::DragFloat("Maximo", &w.maximo, 0.01f);
                break;
            case TipoWidget::Checkbox:
                ImGui::Checkbox("Activado", &w.activado);
                break;
            case TipoWidget::EntradaTexto: {
                char txt[512] = {};
                std::snprintf(txt, sizeof(txt), "%s", w.texto.c_str());
                if (ImGui::InputText("Texto", txt, sizeof(txt))) w.texto = txt;
                break;
            }
            default:
                break;
        }
    }
}

// Activa por nombre la interfaz que usara el juego (modo play). Carga el
// asset de disco si no es la activa actual; nullptr si no existe.
UserInterfaceCustom* CreadorDeInterfaces::activarInterfaz(
    const std::string& nombre) {
    if (nombre.empty() || directorio_.empty()) {
        tieneActiva_ = false;
        interfazActiva_ = {};
        nombreActiva_.clear();
        return nullptr;
    }
    // Si ya es la activa, reutiliza la instancia (conserva estado editado en
    // play: checkbox, slider, texto).
    if (tieneActiva_ && nombreActiva_ == nombre) return &interfazActiva_;
    UserInterfaceCustom ui;
    ui.nombre = nombre;
    if (!ui.cargar(directorio_)) {
        tieneActiva_ = false;
        interfazActiva_ = {};
        nombreActiva_.clear();
        return nullptr;
    }
    interfazActiva_ = ui;
    nombreActiva_ = nombre;
    tieneActiva_ = true;
    return &interfazActiva_;
}

void CreadorDeInterfaces::printGUI() {
    if (!stateGUI) return;
    initGUI();
    contentGUI();
    endGUI();
}