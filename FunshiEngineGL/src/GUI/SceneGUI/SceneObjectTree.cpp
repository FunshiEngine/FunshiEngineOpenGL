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
#include "JerarquiaArbol.h"

#include "../../Objetos/GameObject.h"
#include "../../Scenes/EditorController.h"
#include "../../Scenes/SceneRegistry.h"
#include "../../Comandos/BorrarObjetoComando.h"
#include "../../Comandos/ReparentarComando.h"
#include "../../Comandos/DuplicarObjetoComando.h"
#include "../../Comandos/GestorComandos.h"
#include "../../Objetos/PrefabLibrary.h"
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
        ImGui::SetTooltip("ID: %d", object->getId());

// Menu contextual con 5 opciones: Cambiar ID, Renombrar, Desanidar a raiz, Crear Prefab, Eliminar
        if (ImGui::BeginPopupContextItem("MenuObjeto")) {
            ImGui::Text("%s", etiqueta.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Cambiar ID")) {
                dialogoActivo = DialogoTipo::CambiarID;
                objetoEnDialogo = object;
                std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%d", object->getId());
                dialogoRecienAbierto = true;
            }
            if (ImGui::MenuItem("Renombrar")) {
                dialogoActivo = DialogoTipo::Renombrar;
                objetoEnDialogo = object;
                std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%s", object->inputName);
                dialogoRecienAbierto = true;
            }
            // Desanidar a raiz: solo si cuelga de un padre intermedio (un hijo
            // directo de la raiz ya esta al nivel superior).
            if (esCandidatoADesanidar(object, scene ? scene->getRoot() : nullptr)) {
                if (ImGui::MenuItem("Desanidar a raiz")) {
                    // Diferido: mutar el arbol tras el recorrido para no invalidar
                    // iteradores (patron igual que objetoAReParentar).
                    objetoADesanidar = object;
                }
            }
            if (ImGui::MenuItem("Crear Prefab")) {
                dialogoActivo = DialogoTipo::CrearPrefab;
                objetoEnDialogo = object;
                std::snprintf(bufferDialogo, sizeof(bufferDialogo), "%s", object->inputName);
                dialogoRecienAbierto = true;
            }
            if (ImGui::MenuItem("Eliminar")) {
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
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("DialogoCambiarID");
            dialogoRecienAbierto = false;
        }
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
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("DialogoRenombrar");
        }
        if (ImGui::BeginPopupModal("DialogoRenombrar", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (dialogoRecienAbierto) {
                ImGui::SetKeyboardFocusHere();
                dialogoRecienAbierto = false;
            }
            ImGui::Text("Nuevo nombre:");
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
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("DialogoEliminar");
            dialogoRecienAbierto = false;
        }
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

    // Dialogo Crear Prefab
    if (dialogoActivo == DialogoTipo::CrearPrefab) {
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("DialogoCrearPrefab");
            dialogoRecienAbierto = false;
        }
        if (ImGui::BeginPopupModal("DialogoCrearPrefab", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nombre del prefab:");
            ImGui::InputText("##NombrePrefab", bufferDialogo, sizeof(bufferDialogo),
                             ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Separator();
            if (ImGui::Button("Crear", ImVec2(120, 0))) {
                if (bufferDialogo[0] != '\0' && editor && editor->getPrefabLibrary()) {
                    editor->getPrefabLibrary()->crearPrefab(bufferDialogo, objetoEnDialogo, editor);
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
}

void SceneObjectTree::applyDeferredOperations() {
    GestorComandos* comandos =
        editor ? editor->getGestorComandos() : nullptr;
    if (objetoAReParentar && objetoPadreNuevo && editor && comandos)
        comandos->ejecutar(std::make_unique<ReparentarComando>(
            editor, objetoAReParentar, objetoPadreNuevo, scene));
    objetoAReParentar = nullptr;
    objetoPadreNuevo = nullptr;

    if (objetoADesanidar) {
        GameObject* raiz = scene ? scene->getRoot() : nullptr;
        if (editor && comandos && raiz)
            comandos->ejecutar(std::make_unique<ReparentarComando>(
                editor, objetoADesanidar, raiz, scene));
        objetoADesanidar = nullptr;
    }

    if (objetoAEliminar) {
        GameObject* doomed = objetoAEliminar;
        objetoAEliminar = nullptr;
        openNodes.erase(static_cast<const void*>(doomed));
        if (renombrando == doomed) renombrando = nullptr;
        if (editor && comandos)
            comandos->ejecutar(
                std::make_unique<BorrarObjetoComando>(editor, doomed, scene));
    }

    if (objetoAPegar) {
        GameObject* fuente = objetoAPegar;
        objetoAPegar = nullptr;
        GameObject* destino = padreDePegado;
        padreDePegado = nullptr;
        if (editor && scene && scene->contains(fuente)) {
            if (!destino || !scene->contains(destino)) destino = nullptr;
            // Pegar sobre si mismo o sobre un descendiente colgaria el
            // duplicado de su propio subarbol: en ese caso va a la raiz.
            if (destino && (destino == fuente || esDescendiente(destino, fuente)))
                destino = nullptr;
            // Sin tocar la seleccion: el clon no se selecciona para que el
            // proximo Ctrl+V caiga en el mismo destino y nunca se anide.
            // Por el gestor: Ctrl+Z deshace el pegado.
            if (GestorComandos* comandos = editor->getGestorComandos())
                comandos->ejecutar(std::make_unique<DuplicarObjetoComando>(
                    editor, fuente, destino, scene));
            // Cortar es mover: tras pegar se borra el original y se vacia.
            // Tambien por el gestor: el corte completo se deshace en dos
            // Ctrl+Z (borrado y duplicado).
            if (cortePendiente) {
                openNodes.erase(static_cast<const void*>(fuente));
                if (renombrando == fuente) renombrando = nullptr;
                if (GestorComandos* comandos = editor->getGestorComandos())
                    comandos->ejecutar(std::make_unique<BorrarObjetoComando>(
                        editor, fuente, scene));
                objetoCopiado = nullptr;
                cortePendiente = false;
            }
        }
    }
}

bool SceneObjectTree::esDescendiente(GameObject* nodo, GameObject* ancestro) {
    for (Entity* p = nodo ? nodo->getParentEntity() : nullptr; p;
         p = p->getParentEntity()) {
        if (p == ancestro) return true;
    }
    return false;
}

void SceneObjectTree::manejarAtajos() {
    if (!editor || !scene) return;
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) return;
    // Con un campo de texto activo (renombrado), Ctrl+C/V son del texto.
    if (ImGui::IsAnyItemActive()) return;
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_C) ||
        ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_X)) {
        GameObject* seleccionado = editor->getSelectedObject();
        if (!seleccionado || seleccionado == scene->getRoot()) return;
        objetoCopiado = seleccionado;
        cortePendiente =
            ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_X);
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_V)) {
        if (!objetoCopiado || !scene->contains(objetoCopiado)) {
            objetoCopiado = nullptr;
            cortePendiente = false;
            return;
        }
        // Pegar con un objeto seleccionado entra como su hijo; sin seleccion
        // (clic en el vacio deselecciona) entra a la raiz. Diferido como el
        // resto: mutar el arbol aca invalidaria el recorrido.
        objetoAPegar = objetoCopiado;
        padreDePegado = editor->getSelectedObject();
    }
}