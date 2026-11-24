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
#include "SettingsAnimador2D.h"

#include <imgui.h>

#include "../../../Herramientas/TypeUtils.h"
#include "../../../Objetos/Componentes/Juego2D/Animador2D.h"
#include "../../../Objetos/Componentes/Render/SpriteRenderer.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Localizacion/Localizacion.h"

using Localizacion::Texto;

SettingsAnimador2D::SettingsAnimador2D(GameObject* objeto)
    : SettingsComponent(demangle(typeid(Animador2D).name())),
      gameObject_(objeto) {}

Component* SettingsAnimador2D::getComponent() {
    return objetivo<Animador2D>(gameObject_);
}

void SettingsAnimador2D::showDataComponent() {
    Animador2D* anim =
        objetivo<Animador2D>(gameObject_);
    if (!anim) return;

    if (gameObject_->getComponent<SpriteRenderer>() == nullptr) {
        ImGui::TextDisabled("%s", Localizacion::tr(Texto::RequiereUnSpriteRendererEnEl));
        return;
    }

    int columnas = anim->getColumnas();
    if (ImGui::DragInt(Localizacion::etiqueta(Texto::Columnas).c_str(), &columnas, 0.1f, 1, 256))
        anim->setColumnas(columnas);
    int filas = anim->getFilas();
    if (ImGui::DragInt(Localizacion::etiqueta(Texto::Filas).c_str(), &filas, 0.1f, 1, 256))
        anim->setFilas(filas);

    float fps = anim->getFps();
    if (ImGui::DragFloat("FPS", &fps, 0.1f, 0.1f, 240.0f, "%.1f"))
        anim->setFps(fps);

    int primer = anim->getPrimerFrame();
    if (ImGui::DragInt(Localizacion::etiqueta(Texto::PrimerFrame).c_str(), &primer, 0.1f, 0, 65535))
        anim->setPrimerFrame(primer);
    int ultimo = anim->getUltimoFrame();
    if (ImGui::DragInt(Localizacion::etiqueta(Texto::UltimoFrame).c_str(), &ultimo, 0.1f, -1, 65535)) {
        if (ultimo >= 0 && ultimo < primer) ultimo = primer;
        anim->setUltimoFrame(ultimo);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localizacion::tr(Texto::UltimoFrame0UsaHasta));

    bool reproduciendo = anim->getReproduciendo();
    if (ImGui::Checkbox(Localizacion::etiqueta(Texto::Reproduciendo).c_str(), &reproduciendo))
        anim->setReproduciendo(reproduciendo);
    ImGui::SameLine();
    bool bucle = anim->getBucle();
    if (ImGui::Checkbox(Localizacion::etiqueta(Texto::Bucle).c_str(), &bucle)) anim->setBucle(bucle);

    ImGui::Text(Localizacion::tr(Texto::FrameActual), anim->getFrameActual(),
                anim->getTotalFrames());
    ImGui::SameLine();
    if (ImGui::Button(Localizacion::etiqueta(Texto::Reiniciar).c_str())) anim->reiniciar();

    ImGui::TextDisabled("%s", Localizacion::tr(Texto::RecortaLaTexturaDelSpriteRenderer));
}
