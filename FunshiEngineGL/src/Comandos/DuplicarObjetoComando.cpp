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
#include "DuplicarObjetoComando.h"
#include "Scenes/EditorController.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include "Entity/Entity.h"
#include <functional>
#include <sstream>

DuplicarObjetoComando::DuplicarObjetoComando(EditorController* ec,
                                             GameObject* obj,
                                             GameObject* padre,
                                             SceneRegistry* sr)
    : editorController(ec), sceneRegistry(sr) {
    if (obj) {
        originalId = obj->getId();
        nombreObjeto = obj->inputName;
    }
    if (padre) parentId = padre->getId();
}

void DuplicarObjetoComando::ejecutar() {
    if (!sceneRegistry || !editorController || originalId <= 0) return;

    GameObject* original = sceneRegistry->getObjectByID(originalId);
    if (!original) return;
    GameObject* padre = sceneRegistry->getObjectByID(parentId);
    GameObject* cima = editorController->duplicarObjeto(original, padre);
    if (!cima) return;
    std::function<void(GameObject*)> recolectar = [&](GameObject* nodo) {
        if (!nodo) return;
        creadosIds.push_back(nodo->getId());
        for (Entity* hijo : nodo->getChildEntities()) {
            if (GameObject* hijoObjeto = dynamic_cast<GameObject*>(hijo))
                recolectar(hijoObjeto);
        }
    };
    recolectar(cima);
}

void DuplicarObjetoComando::deshacer() {
    if (!sceneRegistry || !editorController || creadosIds.empty()) return;
    for (auto it = creadosIds.rbegin(); it != creadosIds.rend(); ++it) {
        if (GameObject* creado = sceneRegistry->getObjectByID(*it))
            editorController->deleteGameObject(creado);
    }
    creadosIds.clear();
}

std::string DuplicarObjetoComando::descripcion() const {
    std::ostringstream oss;
    oss << "Duplicar objeto: " << nombreObjeto;
    return oss.str();
}
