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

SettingsAnimador2D::SettingsAnimador2D(GameObject* objeto)
    : SettingsComponent(demangle(typeid(Animador2D).name())),
      gameObject_(objeto) {}

Component* SettingsAnimador2D::getComponent() {
    return gameObject_ ? gameObject_->getComponent<Animador2D>() : nullptr;
}

void SettingsAnimador2D::showDataComponent() {
    Animador2D* anim =
        gameObject_ ? gameObject_->getComponent<Animador2D>() : nullptr;
    if (!anim) return;

    if (gameObject_->getComponent<SpriteRenderer>() == nullptr) {
        ImGui::TextDisabled("Requiere un SpriteRenderer en el mismo objeto.");
        return;
    }

    int columnas = anim->getColumnas();
    if (ImGui::DragInt("Columnas", &columnas, 0.1f, 1, 256))
        anim->setColumnas(columnas);
    int filas = anim->getFilas();
    if (ImGui::DragInt("Filas", &filas, 0.1f, 1, 256))
        anim->setFilas(filas);

    float fps = anim->getFps();
    if (ImGui::DragFloat("FPS", &fps, 0.1f, 0.1f, 240.0f, "%.1f"))
        anim->setFps(fps);

    int primer = anim->getPrimerFrame();
    if (ImGui::DragInt("Primer frame", &primer, 0.1f, 0, 65535))
        anim->setPrimerFrame(primer);
    int ultimo = anim->getUltimoFrame();
    if (ImGui::DragInt("Ultimo frame", &ultimo, 0.1f, -1, 65535)) {
        if (ultimo >= 0 && ultimo < primer) ultimo = primer;
        anim->setUltimoFrame(ultimo);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Ultimo frame < 0 usa hasta el final de la grilla.");

    bool reproduciendo = anim->getReproduciendo();
    if (ImGui::Checkbox("Reproduciendo", &reproduciendo))
        anim->setReproduciendo(reproduciendo);
    ImGui::SameLine();
    bool bucle = anim->getBucle();
    if (ImGui::Checkbox("Bucle", &bucle)) anim->setBucle(bucle);

    ImGui::Text("Frame actual: %d / %d", anim->getFrameActual(),
                anim->getTotalFrames());
    ImGui::SameLine();
    if (ImGui::Button("Reiniciar")) anim->reiniciar();

    ImGui::TextDisabled("Recorta la textura del SpriteRenderer en uvRect.");
}
