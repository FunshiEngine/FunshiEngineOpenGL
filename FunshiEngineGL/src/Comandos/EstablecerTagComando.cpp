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
#include "EstablecerTagComando.h"
#include "Objetos/GameObject.h"
#include "Scenes/SceneRegistry.h"
#include <sstream>

EstablecerTagComando::EstablecerTagComando(EditorController* ec, SceneRegistry* sr,
                                           GameObject* obj, std::string nuevaEtiqueta)
    : editorController(ec), sceneRegistry(sr), objectId(-1),
      etiqueta(std::move(nuevaEtiqueta)) {
    if (obj) {
        objectId = obj->getId();
        nombreObjeto = obj->inputName;
        etiquetaAnterior = obj->getTag();
    }
}

void EstablecerTagComando::ejecutar() {
    if (!sceneRegistry || objectId <= 0) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    obj->setTag(etiqueta);
}

void EstablecerTagComando::deshacer() {
    if (!sceneRegistry || objectId <= 0) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    obj->setTag(etiquetaAnterior);
}

std::string EstablecerTagComando::descripcion() const {
    std::ostringstream oss;
    oss << "Etiquetar " << nombreObjeto << " como " << etiqueta;
    return oss.str();
}
