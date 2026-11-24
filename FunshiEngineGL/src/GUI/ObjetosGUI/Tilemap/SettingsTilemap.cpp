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
#include <algorithm>
#include <cstdint>
#include <typeinfo>

#include <imgui.h>
#include <stb_image/stb_image.h>

#include "../../../Assets/Tiles/Tileset.h"
#include "../../../Herramientas/TypeUtils.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Scenes/Editor/EditorController.h"
#include "../../../Scenes/Editor/HerramientaTilemap.h"

SettingsTilemap::SettingsTilemap(GameObject* objeto, EditorController* editor)
    : SettingsComponent(demangle(typeid(Tilemap).name())),
      gameObject_(objeto), editor_(editor) {}

SettingsTilemap::~SettingsTilemap() { liberarAtlas(); }

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
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram),
                               "Tileset: %s", tilemap->getUltimoErrorTileset().c_str());
        return;
    }

    HerramientaTilemap* herramienta = editor_ ? &editor_->getHerramientaTilemap() : nullptr;
    ImGui::SeparatorText("Capas");
    dibujarCapas(tilemap, herramienta);
    if (herramienta) {
        ImGui::SeparatorText("Pintar");
        dibujarHerramienta(*herramienta);
    }
    ImGui::SeparatorText("Paleta");
    dibujarPaleta(tilemap, herramienta);
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

void SettingsTilemap::dibujarCapas(Tilemap* tilemap, HerramientaTilemap* herramienta) {
    const int total = tilemap->cantidadCapas();
    if (herramienta) capaActiva_ = herramienta->capa();
    if (capaActiva_ >= total) capaActiva_ = total - 1;
    if (capaActiva_ < 0) capaActiva_ = 0;

    for (int i = 0; i < total; ++i) {
        const CapaTilemap* c = tilemap->capa(i);
        if (!c) continue;
        ImGui::PushID(i);
        char etiqueta[96];
        std::snprintf(etiqueta, sizeof(etiqueta), "%s (%dx%d)", c->nombre.c_str(),
                      c->ancho, c->alto);
        if (ImGui::Selectable(etiqueta, i == capaActiva_, 0, ImVec2(160.0f, 0.0f)))
            capaActiva_ = i;
        ImGui::SameLine();
        bool visible = c->visible;
        if (ImGui::Checkbox("Vis", &visible)) tilemap->setCapaVisible(i, visible);
        ImGui::SameLine();
        bool colision = c->colision;
        if (ImGui::Checkbox("Col", &colision)) tilemap->setCapaColision(i, colision);
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

    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputInt("##ancho", &anchoCapaNueva_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputInt("##alto", &altoCapaNueva_);
    if (anchoCapaNueva_ < 1) anchoCapaNueva_ = 1;
    if (altoCapaNueva_ < 1) altoCapaNueva_ = 1;
    ImGui::SameLine();
    if (ImGui::Button("Agregar capa")) {
        const int indice =
            tilemap->agregarCapa("Capa", anchoCapaNueva_, altoCapaNueva_, false);
        if (indice >= 0) capaActiva_ = indice;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Ancho x alto (celdas) de la capa nueva.");
    if (herramienta) herramienta->setCapa(capaActiva_);
}

void SettingsTilemap::dibujarHerramienta(HerramientaTilemap& herramienta) {
    bool activa = herramienta.activa();
    if (ImGui::Checkbox("Pintar en escena", &activa)) herramienta.setActiva(activa);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Clic en la escena pinta la capa activa; Shift + clic borra.\n"
                          "Mientras esta activo, el clic no cambia la seleccion.");
    int modo = static_cast<int>(herramienta.modo());
    ImGui::RadioButton("Pincel", &modo, static_cast<int>(HerramientaTilemap::Modo::Pincel));
    ImGui::SameLine();
    ImGui::RadioButton("Borrador", &modo, static_cast<int>(HerramientaTilemap::Modo::Borrador));
    ImGui::SameLine();
    ImGui::RadioButton("Relleno", &modo, static_cast<int>(HerramientaTilemap::Modo::Relleno));
    ImGui::SameLine();
    ImGui::RadioButton("Rectangulo", &modo,
                       static_cast<int>(HerramientaTilemap::Modo::Rectangulo));
    herramienta.setModo(static_cast<HerramientaTilemap::Modo>(modo));
}

void SettingsTilemap::liberarAtlas() {
    if (atlas_ != Rendering::Backend::kInvalidHandle)
        Rendering::Backend::activeBackend().destroyTexture2D(atlas_);
    atlas_ = Rendering::Backend::kInvalidHandle;
    rutaAtlas_.clear();
}

void SettingsTilemap::asegurarAtlas(Tilemap* tilemap) {
    const std::string ruta = tilemap->rutaImagenTileset();
    if (ruta == rutaAtlas_) return;
    liberarAtlas();
    rutaAtlas_ = ruta;
    int ancho = 0, alto = 0, canales = 0;
    // Sin volteo: ImGui muestrea con el origen arriba a la izquierda.
    unsigned char* pixeles = stbi_load(ruta.c_str(), &ancho, &alto, &canales, 4);
    if (!pixeles) return;
    Rendering::Backend::Image2D imagen;
    imagen.width = ancho;
    imagen.height = alto;
    imagen.pixels = pixeles;
    atlas_ = Rendering::Backend::activeBackend().createTexture2D(imagen);
    stbi_image_free(pixeles);
}

void SettingsTilemap::dibujarPaleta(Tilemap* tilemap, HerramientaTilemap* herramienta) {
    const Tileset& ts = tilemap->getTileset();
    asegurarAtlas(tilemap);
    std::uint32_t& elegido = tileLocal_;
    if (herramienta) elegido = herramienta->tile();

    const void* textura = atlas_ != Rendering::Backend::kInvalidHandle
                              ? Rendering::Backend::activeBackend().imguiTextureId(atlas_)
                              : nullptr;
    const float lado = 32.0f;
    const float disponible = ImGui::GetContentRegionAvail().x;
    const int porFila =
        std::max(1, static_cast<int>(disponible / (lado + ImGui::GetStyle().ItemSpacing.x + 8.0f)));
    ImGui::BeginChild("paleta", ImVec2(0, 180), true);
    // El id 0 es la celda vacia: la paleta arranca en 1.
    for (int id = 1; id < ts.tileCount; ++id) {
        ImGui::PushID(id);
        const bool activo = elegido == static_cast<std::uint32_t>(id);
        if (activo)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        bool pulsado = false;
        if (textura) {
            float uv[4];
            ts.uvDeTile(static_cast<std::uint32_t>(id), uv);
            // uvDeTile da coordenadas para la imagen volteada (como la sube el
            // motor); el atlas de la paleta no esta volteado.
            pulsado = ImGui::ImageButton(
                "##tile",
                static_cast<ImTextureID>(reinterpret_cast<std::intptr_t>(textura)),
                ImVec2(lado, lado), ImVec2(uv[0], 1.0f - uv[3]), ImVec2(uv[2], 1.0f - uv[1]));
        } else {
            char etiqueta[16];
            std::snprintf(etiqueta, sizeof(etiqueta), "%d", id);
            pulsado = ImGui::Button(etiqueta, ImVec2(lado, lado));
        }
        if (activo) ImGui::PopStyleColor();
        if (pulsado) {
            elegido = static_cast<std::uint32_t>(id);
            if (herramienta) {
                herramienta->setTile(elegido);
                if (herramienta->modo() == HerramientaTilemap::Modo::Borrador)
                    herramienta->setModo(HerramientaTilemap::Modo::Pincel);
            }
        }
        const bool solido = id < static_cast<int>(ts.solido.size()) &&
                            ts.solido[static_cast<std::size_t>(id)] != 0;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Tile %d%s", id, solido ? " (solido)" : "");
        ImGui::PopID();
        if (id % porFila != 0) ImGui::SameLine();
    }
    ImGui::EndChild();
}
