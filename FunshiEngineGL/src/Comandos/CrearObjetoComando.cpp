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
#include "Scenes/SceneRegistry.h"
#include "Objetos/Modelos3D.h"
#include "Objetos/GameObject.h"
#include "Objetos/ObjetoEscena.h"
#include <sstream>
#include <vector>

CrearObjetoComando::CrearObjetoComando(EditorController* ec, SceneRegistry* sr,
                                       std::unique_ptr<Modelos3D> m,
                                       GameObject* p)
    : editorController(ec), sceneRegistry(sr),
      modelo(std::move(m)), parent(p) {
    if (modelo) {
        nombreObjeto = modelo->inputName;
        if (nombreObjeto.empty()) {
            nombreObjeto = demangle(typeid(*modelo).name());
        }
    }
}

void CrearObjetoComando::ejecutar() {
    if (!sceneRegistry) return;

    if (modelo) {
        std::unique_ptr<GameObject> objPtr(modelo.release());
        GameObject* obj = sceneRegistry->createObject(std::move(objPtr), parent);
        if (obj) {
            createdId = obj->getId();
        }
    } else if (restaurado) {
        std::vector<std::unique_ptr<GameObject>> objs;
        objs.push_back(std::move(restaurado));
        GameObject* obj = sceneRegistry->restoreSubtree(std::move(objs), parent);
        if (obj) {
            createdId = obj->getId();
        }
    }
}

void CrearObjetoComando::deshacer() {
    if (!sceneRegistry || createdId <= 0) return;
    GameObject* obj = sceneRegistry->getObjectByID(createdId);
    if (obj) {
        restaurado = sceneRegistry->takeObject(obj);
    }
    createdId = -1;
}

std::string CrearObjetoComando::descripcion() const {
    std::ostringstream oss;
    oss << "Crear objeto: " << nombreObjeto;
    return oss.str();
}
