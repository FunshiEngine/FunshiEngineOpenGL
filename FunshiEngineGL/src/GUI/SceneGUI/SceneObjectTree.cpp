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
#include "SceneObjectTreeDetalle.h"
#include "../../Localizacion/Localizacion.h"

using Localizacion::Texto;

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
                if (objetoSeleccionPendiente == event.object)
                    objetoSeleccionPendiente = nullptr;
                if (objetoBajoMouseAlSoltar_ == event.object)
                    objetoBajoMouseAlSoltar_ = nullptr;
                // El portapapeles observa: si se borra afuera, se invalida.
                if (objetoCopiado == event.object) {
                    objetoCopiado = nullptr;
                    cortePendiente = false;
                }
                if (objetoAPegar == event.object) {
                    objetoAPegar = nullptr;
                    padreDePegado = nullptr;
                }
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
    objetoSeleccionPendiente = nullptr;
    objetoBajoMouseAlSoltar_ = nullptr;
    objetoAEliminar = nullptr;
    objetoAReParentar = nullptr;
    objetoPadreNuevo = nullptr;
    objetoADesanidar = nullptr;
    objetoCopiado = nullptr;
    cortePendiente = false;
    objetoAPegar = nullptr;
    padreDePegado = nullptr;
}

void SceneObjectTree::draw() {
    if (!scene) return;
    auto* tree = scene->getEntitysTree();
    if (!tree || tree->isEmpty()) return;
    objetoBajoMouseAlSoltar_ = nullptr;
    // El recorrido generico gestiona PushID, colapso y recursion.
    TreeIG::drawTree(tree, tree->rootOfTree(), openNodes,
                     [this](GameObject* element, bool wasOpen) {
                         return drawRow(element, wasOpen);
                     });
    if (objetoSeleccionPendiente) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const float deltaX = mouse.x - posicionInicioSeleccion.x;
        const float deltaY = mouse.y - posicionInicioSeleccion.y;
        constexpr float umbralArrastre = 6.0f;
        if (deltaX * deltaX + deltaY * deltaY >=
            umbralArrastre * umbralArrastre) {
            objetoSeleccionPendiente = nullptr;
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (objetoBajoMouseAlSoltar_ == objetoSeleccionPendiente && editor)
                editor->selectObject(objetoSeleccionPendiente);
            objetoSeleccionPendiente = nullptr;
        }
    }
    applyDeferredOperations();
    manejarAtajos();
    // Clic en el vacio de la ventana deselecciona (para pegar a la raiz o
    // para soltar la seleccion). Solo al presionar, con la ventana enfocada
    // y sin nada bajo el mouse: no pelea con el arrastre ni con los popups.
    if (editor && ImGui::IsWindowHovered() &&
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
        editor->clearSelection();
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

    const bool hovered = ImGui::IsItemHovered();
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        objetoSeleccionPendiente = object;
        posicionInicioSeleccion = ImGui::GetIO().MousePos;
    }
    if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        objetoBajoMouseAlSoltar_ = object;
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && editor)
        editor->selectObject(object);
    if (hovered)
        ImGui::SetTooltip(Localizacion::tr(Texto::ID), object->getId());

// Menu contextual con 5 opciones: Cambiar ID, Renombrar, Desanidar a raiz, Crear Prefab, Eliminar
        if (ImGui::BeginPopupContextItem("MenuObjeto")) {
            ImGui::Text("%s", etiqueta.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem(Localizacion::etiqueta(Texto::CambiarID).c_str())) {
                dialogoActivo = DialogoTipo::CambiarID;
                objetoEnDialogo = object;
                std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%d", object->getId());
                dialogoRecienAbierto = true;
            }
            if (ImGui::MenuItem(Localizacion::etiqueta(Texto::Renombrar).c_str())) {
                dialogoActivo = DialogoTipo::Renombrar;
                objetoEnDialogo = object;
                std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%s", object->inputName);
                dialogoRecienAbierto = true;
            }
            // Desanidar a raiz: solo si cuelga de un padre intermedio (un hijo
            // directo de la raiz ya esta al nivel superior).
            if (esCandidatoADesanidar(object, scene ? scene->getRoot() : nullptr)) {
                if (ImGui::MenuItem(Localizacion::etiqueta(Texto::DesanidarARaiz).c_str())) {
                    // Diferido: mutar el arbol tras el recorrido para no invalidar
                    // iteradores (patron igual que objetoAReParentar).
                    objetoADesanidar = object;
                }
            }
            if (ImGui::MenuItem(Localizacion::etiqueta(Texto::CrearPrefab).c_str())) {
                dialogoActivo = DialogoTipo::CrearPrefab;
                objetoEnDialogo = object;
                std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%s", object->inputName);
                dialogoRecienAbierto = true;
            }
            if (ImGui::MenuItem(Localizacion::etiqueta(Texto::Eliminar).c_str())) {
                dialogoActivo = DialogoTipo::Eliminar;
                objetoEnDialogo = object;
                dialogoRecienAbierto = true;
            }
            ImGui::EndPopup();
        }

    // Drag & drop...
    if (ImGui::BeginDragDropSource()) {
        GameObject* draggable = object;
        ImGui::SetDragDropPayload("ENTITY_NODE", &draggable,
                                  sizeof(draggable));
        ImGui::Text(Localizacion::tr(Texto::Moviendo), etiqueta.c_str());
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
        // Archivo soltado desde el explorador sobre la fila: se aplica a ese
        // objeto (un prefab se instancia como hijo, un modelo/imagen/audio
        // reemplaza o agrega el componente que corresponde).
        if (const ImGuiPayload* archivo = ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaSoltada = static_cast<const char*>(archivo->Data);
            if (rutaSoltada && *rutaSoltada && editor) {
                archivoSoltado_ = rutaSoltada;
                objetoDestinoArchivo_ = object;
            }
        }
        ImGui::EndDragDropTarget();
    }

    return {nodeOpen, toggled};
}
