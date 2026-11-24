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
#include "GizmoControllerDetalle.h"

GizmoController::GizmoController(EditorController* controlador,
                                 SceneRegistry* registro,
                                 SceneSelectedInterface* seleccion) noexcept
    : controlador_(controlador), registro_(registro), seleccion_(seleccion) {}

GizmoController::~GizmoController() = default;

bool GizmoController::isCapturingInput() const noexcept {
    return listo_ && (ImGuizmo::IsOver() || ImGuizmo::IsUsing());
}

bool GizmoController::gizmoInUse() const noexcept {
    return listo_ && ImGuizmo::IsUsing();
}

void GizmoController::setGizmoOperation(OperacionGizmo operation) {
    operation_ = operation;
}

void GizmoController::tomarFotoTransform(Transform* t,
                                         EstadoTransform& destino) const {
    if (!t) return;
    if (const float* p = t->getTranslatef()) {
        destino.pos[0] = p[0];
        destino.pos[1] = p[1];
        destino.pos[2] = p[2];
    }
    if (const float* r = t->getRotatef()) {
        destino.rot[0] = r[0];
        destino.rot[1] = r[1];
        destino.rot[2] = r[2];
        destino.rot[3] = r[3];
    }
    if (const float* s = t->getScalef()) {
        destino.esc[0] = s[0];
        destino.esc[1] = s[1];
        destino.esc[2] = s[2];
    }
}

bool GizmoController::transformDistinguible(const EstadoTransform& a,
                                            const EstadoTransform& b) {
    // Tolerancia pequena: descarta el comando cuando el arrastre no movio nada
    // de verdad (un clic sobre el gizmo sin arrastrar no debe llenar el
    // historial).
    constexpr float kEpsilon = 1e-4f;
    for (int i = 0; i < 3; ++i)
        if (std::fabs(a.pos[i] - b.pos[i]) > kEpsilon) return true;
    for (int i = 0; i < 4; ++i)
        if (std::fabs(a.rot[i] - b.rot[i]) > kEpsilon) return true;
    for (int i = 0; i < 3; ++i)
        if (std::fabs(a.esc[i] - b.esc[i]) > kEpsilon) return true;
    return false;
}

void GizmoController::procesarSeleccion(ImGuiIO& io, CameraComponent* camara,
                                        ListaDE<GameObject*>* objetos) {
    const bool ortografica =
        camara &&
        camara->getProjection() == CameraComponent::Projection::Orthographic;
    ImGuizmo::SetOrthographic(ortografica);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::BeginFrame();

    // Seleccion de objetos con clic en la escena 3D: siempre activa, es el
    // disparador que enciende las interfaces de edicion (el mismo sistema que
    // activa el gizmo). Clic en zona vacia deselecciona y vuelve a la
    // navegacion libre (gizmo e interfaces se ocultan).
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!io.WantCaptureMouse && !isCapturingInput()) {
            GameObject* clicked =
                pickObject(io.MousePos.x, io.MousePos.y, camara, objetos);
            if (clicked) {
                if (seleccion_) seleccion_->setReturnableEntity(clicked);
                if (operation_ == OperacionGizmo::Ninguna)
                    operation_ = OperacionGizmo::Trasladar;
            } else {
                if (seleccion_) seleccion_->setReturnableEntity(nullptr);
            }
        }
    }
}
