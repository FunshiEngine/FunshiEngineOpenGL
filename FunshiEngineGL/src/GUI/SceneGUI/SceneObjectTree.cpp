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
#include "SceneObjectTree.h"

#include "../../Objetos/GameObject.h"
#include "../../Scenes/EditorController.h"
#include "../../Scenes/SceneRegistry.h"
#include "../../Events/EventBus.h"
#include "../../Herramientas/TypeUtils.h"
#include "../../Herramientas/IconosGUI/IconosGUI.h"
#include <cstring>
#include <imgui.h>
#include <typeinfo>

SceneObjectTree::~SceneObjectTree() { unbind(); }

void SceneObjectTree::unbind() {
    if (events && eventSubscription) events->unsubscribe(eventSubscription);
    eventSubscription = 0;
}

void SceneObjectTree::bindScene(SceneRegistry* value, EditorController* controller,
                                EventBus* bus) {
    unbind();
    scene = value;
    editor = controller;
    events = bus;
    resetState();
    if (events) {
        // Mantener el estado interno coherente con la vida de los objetos.
        eventSubscription = events->subscribe([this](const SceneEvent& event) {
            if (event.type == SceneEventType::ObjectDeleted) {
                if (event.object)
                    openNodes.erase(static_cast<const void*>(event.object));
                if (renombrando == event.object) renombrando = nullptr;
            } else if (event.type == SceneEventType::SceneCleared) {
                resetState();
            }
        });
    }
}

void SceneObjectTree::setIconosGUI(IconosGUI* iconos) { iconosGUI = iconos; }

void SceneObjectTree::resetState() {
    openNodes.clear();
    renombrando = nullptr;
    objetoAEliminar = nullptr;
    objetoAReParentar = nullptr;
    objetoPadreNuevo = nullptr;
}

void SceneObjectTree::draw() {
    if (!scene) return;
    auto* tree = scene->getEntitysTree();
    if (!tree || tree->isEmpty()) return;
    // El recorrido generico gestiona PushID, colapso y recursion.
    TreeIG::drawTree(tree, tree->rootOfTree(), openNodes,
                     [this](GameObject* element, bool wasOpen) {
                         return drawRow(element, wasOpen);
                     });
    applyDeferredOperations();
    // Los dialogos modales deben dibujarse cada frame, fuera del recorrido
    // del arbol, para que ImGui los mantenga abiertos.
    dibujarDialogosModales();
}

TreeIG::RowResult SceneObjectTree::drawRow(GameObject* object, bool wasOpen) {
    if (!object) return {};

    std::string etiqueta = object->inputName;
    if (etiqueta.empty()) etiqueta = demangle(typeid(*object).name());

    // Mostrar ID a la izquierda del nombre: "123 Nombre"
    std::string labelConID = std::to_string(object->getId()) + " " + etiqueta;

    if (renombrando == object) {
        // Renombrado en linea (doble click o menu Renombrar): Enter commitea,
        // Escape cancela. No se dibujan los hijos mientras se edita.
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        const bool commit = ImGui::InputText(
            "##renombrar", object->inputName, IM_ARRAYSIZE(object->inputName),
            ImGuiInputTextFlags_EnterReturnsTrue);
        const bool cancelado =
            ImGui::IsKeyPressed(ImGuiKey_Escape) && ImGui::IsItemActive();
        if (commit || cancelado || ImGui::IsItemDeactivatedAfterEdit()) {
            renombrando = nullptr;
            if (commit && events)
                events->publish({SceneEventType::ComponentChanged, object,
                                 nullptr});
        }
        return {};
    }

    if (iconosGUI && iconosGUI->getIconoGameObject() != ImTextureID_Invalid) {
        ImGui::Image(iconosGUI->getIconoGameObject(), ImVec2(22, 22));
        ImGui::SameLine();
    }

    const bool selected = editor && editor->getSelectedObject() == object;
    ImGuiTreeNodeFlags nodeFlags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selected) nodeFlags |= ImGuiTreeNodeFlags_Selected;
    if (wasOpen) nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

    const bool nodeOpen = ImGui::TreeNodeEx(labelConID.c_str(), nodeFlags, "%s",
                                            labelConID.c_str());
    const bool toggled = ImGui::IsItemToggledOpen();

    // Doble click izquierdo -> iniciar renombrado inline
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && editor) {
        editor->selectObject(object);
        renombrando = object;
        ImGui::SetKeyboardFocusHere(-1);
    }

    if ((ImGui::IsItemClicked(ImGuiMouseButton_Left) ||
         ImGui::IsItemClicked(ImGuiMouseButton_Right)) &&
        editor)
        editor->selectObject(object);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("ID: %d", object->getId());

    // Menu contextual con 3 opciones: Cambiar ID, Renombrar, Eliminar
    if (ImGui::BeginPopupContextItem("MenuObjeto")) {
        ImGui::Text("%s", etiqueta.c_str());
        ImGui::Separator();
        if (ImGui::MenuItem("Cambiar ID")) {
            dialogoActivo = DialogoTipo::CambiarID;
            objetoEnDialogo = object;
            std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%d", object->getId());
            ImGui::OpenPopup("DialogoCambiarID");
        }
        if (ImGui::MenuItem("Renombrar")) {
            dialogoActivo = DialogoTipo::Renombrar;
            objetoEnDialogo = object;
            std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%s", object->inputName);
            ImGui::OpenPopup("DialogoRenombrar");
        }
        if (ImGui::MenuItem("Eliminar")) {
            dialogoActivo = DialogoTipo::Eliminar;
            objetoEnDialogo = object;
            ImGui::OpenPopup("DialogoEliminar");
        }
        ImGui::EndPopup();
    }

    // Drag & drop...
    if (ImGui::BeginDragDropSource()) {
        GameObject* draggable = object;
        ImGui::SetDragDropPayload("ENTITY_NODE", &draggable,
                                  sizeof(draggable));
        ImGui::Text("Moviendo %s", etiqueta.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ENTITY_NODE")) {
            GameObject* arrastrado = nullptr;
            std::memcpy(&arrastrado, payload->Data, sizeof(arrastrado));
            if (arrastrado && arrastrado != object) {
                // Diferido: reparent muta el arbol durante el recorrido.
                objetoAReParentar = arrastrado;
                objetoPadreNuevo = object;
            }
        }
        ImGui::EndDragDropTarget();
    }

    return {nodeOpen, toggled};
}

void SceneObjectTree::dibujarDialogosModales() {
    if (dialogoActivo == DialogoTipo::Ninguno || !objetoEnDialogo) return;

    // Dialogo Cambiar ID
    if (dialogoActivo == DialogoTipo::CambiarID) {
        if (ImGui::BeginPopupModal("DialogoCambiarID", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nuevo ID para '%s':", objetoEnDialogo->inputName);
            ImGui::InputText("##id", bufferDialogo, sizeof(bufferDialogo),
                             ImGuiInputTextFlags_CharsDecimal);
            ImGui::Separator();
            if (ImGui::Button("Aceptar", ImVec2(120, 0))) {
                int nuevoId = std::atoi(bufferDialogo);
                const bool esRaiz = objetoEnDialogo->getParentEntity() == nullptr;
                if (nuevoId > 0 || (esRaiz && nuevoId >= 0)) {
                    objetoEnDialogo->setId(nuevoId);
                    if (events)
                        events->publish({SceneEventType::ComponentChanged,
                                         objetoEnDialogo, nullptr});
                }
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // Dialogo Renombrar
    if (dialogoActivo == DialogoTipo::Renombrar) {
        if (ImGui::BeginPopupModal("DialogoRenombrar", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nuevo nombre:");
            ImGui::SetKeyboardFocusHere();
            bool enterPresionado = ImGui::InputText("##nombre", bufferDialogo, sizeof(bufferDialogo),
                                                     ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Separator();
            bool confirmado = ImGui::Button("Aceptar", ImVec2(120, 0));
            ImGui::SameLine();
            bool cancelado = ImGui::Button("Cancelar", ImVec2(120, 0));
            if (confirmado || cancelado || enterPresionado) {
                if ((confirmado || enterPresionado) && bufferDialogo[0] != '\0') {
                    std::snprintf(objetoEnDialogo->inputName,
                                  sizeof(objetoEnDialogo->inputName), "%s",
                                  bufferDialogo);
                    if (events)
                        events->publish({SceneEventType::ComponentChanged,
                                         objetoEnDialogo, nullptr});
                }
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // Dialogo Eliminar
    if (dialogoActivo == DialogoTipo::Eliminar) {
        if (ImGui::BeginPopupModal("DialogoEliminar", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Eliminar '%s' (ID: %d)?",
                        objetoEnDialogo->inputName, objetoEnDialogo->getId());
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                               "Esta accion no se puede deshacer.");
            ImGui::Separator();
            if (ImGui::Button("Eliminar", ImVec2(120, 0))) {
                if (editor) editor->clearSelection();
                renombrando = nullptr;
                objetoAEliminar = objetoEnDialogo;
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void SceneObjectTree::applyDeferredOperations() {
    if (objetoAReParentar && objetoPadreNuevo && editor)
        editor->reparentGameObject(objetoAReParentar, objetoPadreNuevo);
    objetoAReParentar = nullptr;
    objetoPadreNuevo = nullptr;

    if (objetoAEliminar) {
        GameObject* doomed = objetoAEliminar;
        objetoAEliminar = nullptr;
        openNodes.erase(static_cast<const void*>(doomed));
        if (renombrando == doomed) renombrando = nullptr;
        if (editor) editor->deleteGameObject(doomed);
    }
}