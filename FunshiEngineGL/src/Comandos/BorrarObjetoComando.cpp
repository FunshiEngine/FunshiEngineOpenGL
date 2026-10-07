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
#include "Scenes/EditorController.h"
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
    if (!sceneRegistry || !editorController || objectId <= 0) return;

    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    if (obj->getParentEntity()) {
        parentId =
            static_cast<GameObject*>(obj->getParentEntity())->getId();
    }
    // Por la puerta del editor: desregistra cuerpos, limpia seleccion/gizmo
    // y extrae el subarbol vivo (para deshacer) en vez de destruirlo.
    objetosEliminados = editorController->extraerSubarbol(obj);
}

void BorrarObjetoComando::deshacer() {
    if (!sceneRegistry || !editorController || objetosEliminados.empty())
        return;

    GameObject* parent = sceneRegistry->getObjectByID(parentId);
    editorController->restaurarSubarbol(std::move(objetosEliminados), parent);
    objetosEliminados.clear();
}

std::string BorrarObjetoComando::descripcion() const {
    std::ostringstream oss;
    oss << "Borrar objeto: " << nombreObjeto;
    return oss.str();
}
