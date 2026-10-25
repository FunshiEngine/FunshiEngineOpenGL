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
#include "CrearObjetoComando.h"
#include "Scenes/EditorController.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include "Objetos/ObjetoEscena.h"
#include <sstream>
#include <vector>

CrearObjetoComando::CrearObjetoComando(EditorController* ec, SceneRegistry* sr,
                                       std::unique_ptr<GameObject> o,
                                       GameObject* p)
    : editorController(ec), sceneRegistry(sr),
      objeto(std::move(o)), parentId(p ? p->getId() : 0) {
    if (objeto) {
        nombreObjeto = objeto->inputName;
        if (nombreObjeto.empty()) {
            nombreObjeto = demangle(typeid(*objeto).name());
        }
    }
}

GameObject* CrearObjetoComando::resolverPadre() const {
    if (!sceneRegistry) return nullptr;
    // El id 0 es la raiz de la escena; si el padre ya no existe (se borro
    // entre medias) el objeto vuelve a la raiz en vez de dejar un puntero
    // colgando.
    GameObject* padre = sceneRegistry->getObjectByID(parentId);
    if (!padre) padre = sceneRegistry->getRoot();
    return padre;
}

void CrearObjetoComando::ejecutar() {
    if (!sceneRegistry || !editorController) return;

    if (objeto) {
        GameObject* obj =
            editorController->createGameObject(std::move(objeto), resolverPadre());
        if (obj) {
            createdId = obj->getId();
        }
    } else if (!guardados.empty()) {
        GameObject* obj = editorController->restaurarSubarbol(
            std::move(guardados), resolverPadre());
        guardados.clear();
        if (obj) {
            createdId = obj->getId();
        }
    }
}

void CrearObjetoComando::deshacer() {
    if (!sceneRegistry || !editorController || createdId <= 0) return;
    GameObject* obj = sceneRegistry->getObjectByID(createdId);
    if (obj) {
        guardados = editorController->extraerSubarbol(obj);
    }
    createdId = -1;
}

std::string CrearObjetoComando::descripcion() const {
    std::ostringstream oss;
    oss << "Crear objeto: " << nombreObjeto;
    return oss.str();
}
