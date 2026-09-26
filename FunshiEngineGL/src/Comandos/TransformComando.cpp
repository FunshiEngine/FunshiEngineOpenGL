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
#include "TransformComando.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include "Objetos/Componentes/Transform.h"
#include <sstream>

TransformComando::TransformComando(EditorController* ec, GameObject* obj,
                                   SceneRegistry* sr)
    : editorController(ec), sceneRegistry(sr) {
    if (obj) {
        objectId = obj->getId();
        nombreObjeto = obj->inputName;
        if (auto* t = obj->getComponent<Transform>()) {
            float* p = t->getTranslatef();
            oldPosX = p[0]; oldPosY = p[1]; oldPosZ = p[2];
            float* r = t->getRotatef();
            oldAngle = r[0]; oldRotX = r[1]; oldRotY = r[2]; oldRotZ = r[3];
            float* s = t->getScalef();
            oldScaleX = s[0]; oldScaleY = s[1]; oldScaleZ = s[2];
        }
    }
}

void TransformComando::setNuevoEstado(float px, float py, float pz,
                                      float angle, float rx, float ry, float rz,
                                      float sx, float sy, float sz) {
    newPosX = px; newPosY = py; newPosZ = pz;
    newAngle = angle; newRotX = rx; newRotY = ry; newRotZ = rz;
    newScaleX = sx; newScaleY = sy; newScaleZ = sz;
    guardado = true;
}

void TransformComando::ejecutar() {
    if (!sceneRegistry || objectId <= 0 || !guardado) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    Transform* t = obj->getComponent<Transform>();
    if (!t) return;
    t->setTranslatef(newPosX, newPosY, newPosZ);
    t->setRotatef(newAngle, newRotX, newRotY, newRotZ);
    t->setScalef(newScaleX, newScaleY, newScaleZ);
}

void TransformComando::deshacer() {
    if (!sceneRegistry || objectId <= 0) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    Transform* t = obj->getComponent<Transform>();
    if (!t) return;
    t->setTranslatef(oldPosX, oldPosY, oldPosZ);
    t->setRotatef(oldAngle, oldRotX, oldRotY, oldRotZ);
    t->setScalef(oldScaleX, oldScaleY, oldScaleZ);
}

std::string TransformComando::descripcion() const {
    std::ostringstream oss;
    oss << "Transformar: " << nombreObjeto;
    return oss.str();
}
