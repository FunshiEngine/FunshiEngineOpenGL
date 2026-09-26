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
#include "SceneSelectedInterface.h"

#include "../../Objetos/GameObject.h"
#include "../../Objetos/GameObjectFactory.h"
#include "../../Scenes/EditorController.h"
#include "../../Scenes/SceneRegistry.h"
#include "../../Events/EventBus.h"
#include "../../Herramientas/IconosGUI/IconosGUI.h"
#include <memory>

SceneSelectedInterface::SceneSelectedInterface(bool state)
    : GeneralUserInterface("SelectedObjects", state, ImGuiWindowFlags_MenuBar) {}

void SceneSelectedInterface::bindScene(SceneRegistry* value,
                                       EditorController* controller,
                                       EventBus* bus) {
    scene = value;
    editor = controller;
    sceneTree.bindScene(value, controller, bus);
    // Al cambiar de escena la seleccion anterior queda fuera de contexto.
    if (editor) editor->clearSelection();
}

ArbolEnlazado<GameObject*>* SceneSelectedInterface::getEntitysTree() {
    return scene ? scene->getEntitysTree() : nullptr;
}
void SceneSelectedInterface::setEntitys(ListaDE<GameObject*>* value) {
    // Kept for source compatibility. The GUI no longer imports or copies
    // scene state from an external list.
    (void)value;
}
GameObject* SceneSelectedInterface::getReturnableEntity() {
    // La seleccion vive en el EditorController: unica fuente de verdad.
    return editor ? editor->getSelectedObject() : nullptr;
}
void SceneSelectedInterface::setReturnableEntity(GameObject* object) {
    if (editor) editor->selectObject(object);
}

void SceneSelectedInterface::initGUI() {
    ImGui::Begin(getNameGui().c_str(), &stateGUI, getFlagGui());
    ImGui::PushID(this);
}

void SceneSelectedInterface::contentGUI() {
	auto* entitys = getEntitysTree();
    if (!entitys) return;

    // Sin deseleccion por clic en area vacia: la interface se conserva al
    // navegar con clics (mismo comportamiento que el explorador de archivos).
    if (!entitys->isEmpty()) sceneTree.draw();

    if (ImGui::BeginPopupContextWindow(
            "SelectedEntitysPopup",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("New GameObject")) {
            if (editor) {
                GameObject* created = editor->createGameObject(
                    GameObjectFactory::createSimpleObject(),
                    scene ? scene->getRoot() : nullptr);
                if (created) setReturnableEntity(created);
            }
        }
        ImGui::EndPopup();
    }
}

void SceneSelectedInterface::createGameObject(GameObject* object) {
    if (!object) return;
    std::unique_ptr<GameObject> owned(object);
    createGameObject(std::move(owned));
}

void SceneSelectedInterface::createGameObject(
    std::unique_ptr<GameObject> object) {
    if (!object) return;
    if (!editor) return;
    editor->createGameObject(std::move(object),
                             scene ? scene->getRoot() : nullptr);
}

void SceneSelectedInterface::addChildGameObject(GameObject* parent, GameObject* object) {
    if (!object) return;
    std::unique_ptr<GameObject> owned(object);
    if (!parent || !editor) return;
    editor->createGameObject(std::move(owned), parent);
}

void SceneSelectedInterface::replaceRootGameObject(GameObject* newRoot) {
    if (!newRoot) return;
    std::unique_ptr<GameObject> owned(newRoot);
    if (!scene) return;
    if (editor) editor->clearScene();
    scene->replaceRoot(std::move(owned));
}

void SceneSelectedInterface::clearGameObjects() {
    if (editor) editor->clearScene();
    else if (scene) scene->clear();
}

void SceneSelectedInterface::refreshGameObjectView() {
    if (scene) scene->refreshGameObjectView();
}

bool SceneSelectedInterface::deleteObjectByID(int id) {
    const bool deleted = editor && editor->deleteObjectByID(id);
    return deleted;
}

ListaDE<GameObject*>* SceneSelectedInterface::getGameObjects() {
    return scene ? scene->getGameObjects() : nullptr;
}
void SceneSelectedInterface::endGUI() { ImGui::PopID(); ImGui::End(); }
void SceneSelectedInterface::printGUI() {
    if (stateGUI) { initGUI(); contentGUI(); endGUI(); }
}