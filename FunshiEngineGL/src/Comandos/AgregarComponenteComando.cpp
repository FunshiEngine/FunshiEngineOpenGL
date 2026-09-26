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
#include "AgregarComponenteComando.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include "Objetos/Componentes/Component.h"
#include "Herramientas/TypeUtils.h"
#include <sstream>

AgregarComponenteComando::AgregarComponenteComando(EditorController* ec,
                                                   GameObject* obj,
                                                   std::unique_ptr<Component> comp,
                                                   SceneRegistry* sr)
    : editorController(ec), sceneRegistry(sr),
      componente(std::move(comp)) {
    if (obj) {
        objectId = obj->getId();
        nombreObjeto = obj->inputName;
    }
    if (componente) {
        nombreComponente = demangle(typeid(*componente).name());
    }
}

void AgregarComponenteComando::ejecutar() {
    if (!sceneRegistry || objectId <= 0 || !componente) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    obj->addComponent(std::move(componente));
}

void AgregarComponenteComando::deshacer() {
    if (!sceneRegistry || objectId <= 0 || nombreComponente.empty()) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    Component* comp = obj->getComponentByName(nombreComponente);
    if (comp) {
        componente = obj->extractComponent(comp);
    }
}

std::string AgregarComponenteComando::descripcion() const {
    std::ostringstream oss;
    oss << "Agregar componente: " << nombreComponente << " a " << nombreObjeto;
    return oss.str();
}
