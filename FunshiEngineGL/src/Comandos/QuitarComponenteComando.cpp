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
#include "QuitarComponenteComando.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include "Objetos/Componentes/Component.h"
#include <sstream>

QuitarComponenteComando::QuitarComponenteComando(EditorController* ec,
                                                 GameObject* obj,
                                                 const std::string& nombreComp,
                                                 SceneRegistry* sr)
    : editorController(ec), sceneRegistry(sr),
      nombreComponente(nombreComp) {
    if (obj) {
        objectId = obj->getId();
        nombreObjeto = obj->inputName;
    }
}

void QuitarComponenteComando::ejecutar() {
    if (!sceneRegistry || objectId <= 0 || componenteEliminado) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    Component* comp = obj->getComponentByName(nombreComponente);
    if (comp) {
        componenteEliminado = obj->extractComponent(comp);
    }
}

void QuitarComponenteComando::deshacer() {
    if (!sceneRegistry || objectId <= 0 || !componenteEliminado) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    obj->addComponent(std::move(componenteEliminado));
}

std::string QuitarComponenteComando::descripcion() const {
    std::ostringstream oss;
    oss << "Quitar componente: " << nombreComponente << " de " << nombreObjeto;
    return oss.str();
}
