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
}

TreeIG::RowResult SceneObjectTree::drawRow(GameObject* object, bool wasOpen) {
    if (!object) return {};

    std::string etiqueta = object->inputName;
    if (etiqueta.empty()) etiqueta = demangle(typeid(*object).name());

    if (renombrando == object) {
        // Renombrado en linea: Enter commitea, Escape cancela. No se dibujan
        // los hijos mientras se edita (evita TreePop sin TreeNode).
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

    // El label se pasa tambien como formato para que ni "##" ni "#" del
    // nombre alteren la construccion del ID interno.
    const bool nodeOpen = ImGui::TreeNodeEx(etiqueta.c_str(), nodeFlags, "%s",
                                            etiqueta.c_str());
    // Leer "toggled" justo despues del TreeNodeEx: el menu contextual y el
    // drag&drop sobreescriben el "last item" de ImGui.
    const bool toggled = ImGui::IsItemToggledOpen();

    if ((ImGui::IsItemClicked(ImGuiMouseButton_Left) ||
         ImGui::IsItemClicked(ImGuiMouseButton_Right)) &&
        editor)
        editor->selectObject(object);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("ID: %d", object->getId());

    if (ImGui::BeginPopupContextItem("MenuObjeto")) {
        ImGui::Text("%s", etiqueta.c_str());
        ImGui::Separator();
        if (ImGui::MenuItem("Renombrar")) renombrando = object;
        if (ImGui::MenuItem("Eliminar")) {
            // Diferido: borrar durante el recorrido invalidaria iteradores.
            if (editor) editor->clearSelection();
            renombrando = nullptr;
            objetoAEliminar = object;
        }
        ImGui::EndPopup();
    }

    // Drag & drop con la identidad del objeto (GameObject*), no con el nodo
    // interno del arbol: el puntero del objeto sobrevive a una reconstruccion
    // del arbol durante el arrastre.
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