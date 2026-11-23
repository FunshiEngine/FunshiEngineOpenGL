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
#include "SettingsTilemap.h"

#include <cstdio>
#include <typeinfo>

#include <imgui.h>

#include "../../../Assets/Tiles/Tileset.h"
#include "../../../Herramientas/TypeUtils.h"
#include "../../../Objetos/GameObject.h"

SettingsTilemap::SettingsTilemap(GameObject* objeto)
    : SettingsComponent(demangle(typeid(Tilemap).name())),
      gameObject_(objeto) {}

Component* SettingsTilemap::getComponent() {
    return gameObject_ ? gameObject_->getComponent<Tilemap>() : nullptr;
}

void SettingsTilemap::showDataComponent() {
    Tilemap* tilemap =
        gameObject_ ? gameObject_->getComponent<Tilemap>() : nullptr;
    if (!tilemap) return;

    dibujarTileset(tilemap);

    if (!tilemap->tilesetValido()) {
        if (tilemap->getRutaTileset().empty())
            ImGui::TextDisabled("Asigna un tileset (JSON de Tiled).");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Tileset: %s",
                               tilemap->getUltimoErrorTileset().c_str());
        return;
    }

    ImGui::Separator();
    dibujarCapas(tilemap);
    ImGui::Separator();
    dibujarPaleta(tilemap);
    ImGui::Separator();
    dibujarGrilla(tilemap);
}

void SettingsTilemap::dibujarTileset(Tilemap* tilemap) {
    const std::string& ruta = tilemap->getRutaTileset();
    const char* texto =
        ruta.empty() ? "[Arrastra el tileset .json]" : ruta.c_str();
    const float anchoBoton =
        ImGui::CalcTextSize(texto).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(anchoBoton);
    ImGui::Button(texto);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaNueva = static_cast<const char*>(payload->Data);
            Tileset validacion;
            std::string error;
            if (cargarTilesetDesdeJson(rutaNueva, validacion, error))
                tilemap->setRutaTileset(rutaNueva);
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    if (ImGui::Button("Recargar")) tilemap->recargarTileset();
    ImGui::SameLine();
    if (ImGui::Button("Quitar")) tilemap->setRutaTileset(std::string());

    if (tilemap->tilesetValido()) {
        const Tileset& ts = tilemap->getTileset();
        ImGui::Text("Tileset: %dx%d tiles (%dx%d px), %d tipos", ts.columns,
                    ts.rows, ts.tileWidth, ts.tileHeight, ts.tileCount);
    }
}

void SettingsTilemap::dibujarCapas(Tilemap* tilemap) {
    const int total = tilemap->cantidadCapas();
    if (capaActiva_ >= total) capaActiva_ = total - 1;
    if (capaActiva_ < 0) capaActiva_ = 0;

    for (int i = 0; i < total; ++i) {
        const CapaTilemap* c = tilemap->capa(i);
        if (!c) continue;
        ImGui::PushID(i);
        if (ImGui::Selectable(c->nombre.c_str(), i == capaActiva_,
                              ImGuiSelectableFlags_AllowDoubleClick))
            capaActiva_ = i;

        bool visible = c->visible;
        if (ImGui::Checkbox("Vis", &visible))
            tilemap->setCapaVisible(i, visible);
        ImGui::SameLine();
        bool colision = c->colision;
        if (ImGui::Checkbox("Col", &colision))
            tilemap->setCapaColision(i, colision);
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            tilemap->eliminarCapa(i);
            if (capaActiva_ >= tilemap->cantidadCapas())
                capaActiva_ = tilemap->cantidadCapas() - 1;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }

    static int anchoNueva = 16;
    static int altoNueva = 16;
    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputInt("##ancho", &anchoNueva);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputInt("##alto", &altoNueva);
    if (anchoNueva < 1) anchoNueva = 1;
    if (altoNueva < 1) altoNueva = 1;
    ImGui::SameLine();
    if (ImGui::Button("Agregar capa")) {
        const int indice =
            tilemap->agregarCapa("Capa", anchoNueva, altoNueva, false);
        if (indice >= 0) capaActiva_ = indice;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Alto x ancho (celdas) de la capa nueva.");
}

void SettingsTilemap::dibujarPaleta(Tilemap* tilemap) {
    const Tileset& ts = tilemap->getTileset();
    ImGui::Text("Paleta (%d tiles)", ts.tileCount);
    ImGui::BeginChild("paleta", ImVec2(0, 120), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    const float celda = 28.0f;
    for (int id = 0; id < ts.tileCount; ++id) {
        ImGui::PushID(id);
        const bool solido = id < static_cast<int>(ts.solido.size()) &&
                            ts.solido[static_cast<std::size_t>(id)] != 0;
        const bool activo = (pincel_ == static_cast<std::uint32_t>(id));
        if (activo)
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        if (solido)
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        char etiqueta[16];
        std::snprintf(etiqueta, sizeof(etiqueta), "%d", id);
        if (ImGui::Button(etiqueta, ImVec2(celda, celda)))
            pincel_ = static_cast<std::uint32_t>(id);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Tile %d%s", id, solido ? " (solido)" : "");
        if (solido) ImGui::PopStyleColor();
        if (activo) ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::EndChild();
}

void SettingsTilemap::dibujarGrilla(Tilemap* tilemap) {
    const CapaTilemap* capa = tilemap->capa(capaActiva_);
    if (!capa) {
        ImGui::TextDisabled("Sin capa activa.");
        return;
    }

    ImGui::Text("Pincel: id %d | clic izq pinta, clic der borra",
                static_cast<int>(pincel_));
    const float celda = 20.0f;
    const Tileset& ts = tilemap->getTileset();
    ImGui::BeginChild("grilla", ImVec2(0, 260), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
    for (int y = 0; y < capa->alto; ++y) {
        for (int x = 0; x < capa->ancho; ++x) {
            ImGui::PushID(y * capa->ancho + x);
            const std::uint32_t id = tilemap->tileEn(capaActiva_, x, y);
            const bool solido =
                id > 0 && id < ts.solido.size() && ts.solido[id] != 0;
            char etiqueta[16];
            if (id == 0)
                std::snprintf(etiqueta, sizeof(etiqueta), "-");
            else
                std::snprintf(etiqueta, sizeof(etiqueta), "%u",
                              static_cast<unsigned>(id));
            if (solido)
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::Button(etiqueta, ImVec2(celda, celda));
            if (solido) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
                tilemap->pintar(capaActiva_, x, y, pincel_);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1))
                tilemap->pintar(capaActiva_, x, y, 0);
            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}
