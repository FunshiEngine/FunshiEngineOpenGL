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
#include "ReparentarComando.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include <sstream>

ReparentarComando::ReparentarComando(EditorController* ec, GameObject* obj,
                                     GameObject* nuevoPadre, SceneRegistry* sr)
    : editorController(ec), sceneRegistry(sr) {
    if (obj) {
        objectId = obj->getId();
        nombreObjeto = obj->inputName;
         if (obj->getParentEntity()) {
            oldParentId = static_cast<GameObject*>(obj->getParentEntity())->getId();
        }
    }
    if (nuevoPadre) {
        newParentId = nuevoPadre->getId();
    }
}

void ReparentarComando::ejecutar() {
    if (!sceneRegistry || objectId <= 0 || newParentId < 0) return;

    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    GameObject* newParent = sceneRegistry->getObjectByID(newParentId);
    if (!obj || !newParent) return;

    sceneRegistry->reparent(obj, newParent);
}

void ReparentarComando::deshacer() {
    if (!sceneRegistry || objectId <= 0 || oldParentId < 0) return;

    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    GameObject* oldParent = sceneRegistry->getObjectByID(oldParentId);
    if (!obj || !oldParent) return;

    sceneRegistry->reparent(obj, oldParent);
}

std::string ReparentarComando::descripcion() const {
    std::ostringstream oss;
    oss << "Reparentar: " << nombreObjeto;
    return oss.str();
}
