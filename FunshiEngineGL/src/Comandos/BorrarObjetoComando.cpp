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
#include "BorrarObjetoComando.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include <sstream>
#include <vector>

BorrarObjetoComando::BorrarObjetoComando(EditorController* ec, GameObject* obj,
                                         SceneRegistry* sr)
    : editorController(ec), sceneRegistry(sr) {
    if (obj) {
        objectId = obj->getId();
        nombreObjeto = obj->inputName;
         if (obj->getParentEntity()) {
            parentId = static_cast<GameObject*>(obj->getParentEntity())->getId();
        }
    }
}

void BorrarObjetoComando::ejecutar() {
    if (!sceneRegistry || objectId <= 0) return;

    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;

    if (obj == sceneRegistry->getRoot()) {
        objetoEliminado = sceneRegistry->takeObject(obj);
        return;
    }

    objetoEliminado = sceneRegistry->takeObject(obj);
}

void BorrarObjetoComando::deshacer() {
    if (!sceneRegistry || !objetoEliminado) return;

    GameObject* parent = sceneRegistry->getObjectByID(parentId);
    if (!parent) parent = sceneRegistry->getRoot();
    if (!parent) return;

    std::vector<std::unique_ptr<GameObject>> objs;
    objs.push_back(std::move(objetoEliminado));
    sceneRegistry->restoreSubtree(std::move(objs), parent);
    objetoEliminado = nullptr;
}

std::string BorrarObjetoComando::descripcion() const {
    std::ostringstream oss;
    oss << "Borrar objeto: " << nombreObjeto;
    return oss.str();
}
