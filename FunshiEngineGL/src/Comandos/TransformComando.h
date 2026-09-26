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
#ifndef TRANSFORM_COMANDO_H
#define TRANSFORM_COMANDO_H

#include "IComando.h"
#include <string>

class EditorController;
class SceneRegistry;
class GameObject;

class TransformComando : public IComando {
private:
    EditorController* editorController;
    SceneRegistry* sceneRegistry;
    int objectId = -1;
    float oldPosX = 0, oldPosY = 0, oldPosZ = 0;
    float oldAngle = 0, oldRotX = 0, oldRotY = 0, oldRotZ = 0;
    float oldScaleX = 1, oldScaleY = 1, oldScaleZ = 1;
    float newPosX = 0, newPosY = 0, newPosZ = 0;
    float newAngle = 0, newRotX = 0, newRotY = 0, newRotZ = 0;
    float newScaleX = 1, newScaleY = 1, newScaleZ = 1;
    std::string nombreObjeto;
    bool guardado = false;

public:
    TransformComando(EditorController* ec, GameObject* obj, SceneRegistry* sr);
    ~TransformComando() override = default;

    void setNuevoEstado(float px, float py, float pz, float angle,
                        float rx, float ry, float rz,
                        float sx, float sy, float sz);
    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;
};

#endif
