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
#include "SettingsSpriteRenderer.h"

#include <typeinfo>

#include <imgui.h>

#include "../../../Assets/Imagenes/StbImageLoader.h"
#include "../../../Herramientas/TypeUtils.h"
#include "../../../Objetos/Componentes/Render/SpriteRenderer.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Localizacion/Localizacion.h"

using Localizacion::Texto;

SettingsSpriteRenderer::SettingsSpriteRenderer(GameObject* objeto)
    : SettingsComponent(demangle(typeid(SpriteRenderer).name())),
      gameObject_(objeto) {}

Component* SettingsSpriteRenderer::getComponent() {
    return objetivo<SpriteRenderer>(gameObject_);
}

void SettingsSpriteRenderer::showDataComponent() {
    SpriteRenderer* sprite =
        objetivo<SpriteRenderer>(gameObject_);
    if (!sprite) return;

    // Ruta de textura: el path (o el placeholder) es el target del drag & drop
    // de archivos. El filtro sondea el formato de imagen antes de asignar la
    // ruta; si no es imagen valida, no se toca el componente.
    const std::string& ruta = sprite->getRutaTextura();
    const char* texto = ruta.empty() ? "[Arrastra textura]" : ruta.c_str();
    const float anchoBoton =
        ImGui::CalcTextSize(texto).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(anchoBoton);
    ImGui::Button(texto);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaNueva =
                static_cast<const char*>(payload->Data);
            int ancho = 0;
            int alto = 0;
            if (StbImageLoader::dimensiones(rutaNueva, ancho, alto))
                sprite->setRutaTextura(rutaNueva);
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    if (ImGui::Button(Localizacion::etiqueta(Texto::Quitar).c_str())) sprite->setRutaTextura(std::string());

    float tamano[2] = {sprite->getTamano()[0], sprite->getTamano()[1]};
    if (ImGui::DragFloat2(Localizacion::etiqueta(Texto::Tamano).c_str(), tamano, 0.05f, 0.0f, 0.0f, "%.3f"))
        sprite->setTamano(tamano[0], tamano[1]);

    float tinte[4] = {sprite->getTint()[0], sprite->getTint()[1],
                      sprite->getTint()[2], sprite->getTint()[3]};
    if (ImGui::ColorEdit4(Localizacion::etiqueta(Texto::Tinte).c_str(), tinte))
        sprite->setTint(tinte[0], tinte[1], tinte[2], tinte[3]);

    float uv[4] = {sprite->getUvRect()[0], sprite->getUvRect()[1],
                   sprite->getUvRect()[2], sprite->getUvRect()[3]};
    if (ImGui::DragFloat4(Localizacion::etiqueta(Texto::RegionUV).c_str(), uv, 0.01f, 0.0f, 1.0f, "%.3f"))
        sprite->setUvRect(uv[0], uv[1], uv[2], uv[3]);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "%s", Localizacion::tr(Texto::U0V0U1V1Recorte));

    ImGui::TextDisabled("%s", Localizacion::tr(Texto::ElOrdenEnPantallaLo));
}
