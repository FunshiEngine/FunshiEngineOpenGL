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
#ifndef CREAR_OBJETO_COMANDO_H
#define CREAR_OBJETO_COMANDO_H

#include "IComando.h"
#include <memory>
#include <string>

class EditorController;
class SceneRegistry;
class GameObject;
class Modelos3D;

class CrearObjetoComando : public IComando {
private:
    EditorController* editorController;
    SceneRegistry* sceneRegistry;
    std::unique_ptr<Modelos3D> modelo;
    std::unique_ptr<GameObject> restaurado;
    GameObject* parent;
    int createdId = -1;
    std::string nombreObjeto;

public:
    CrearObjetoComando(EditorController* ec, SceneRegistry* sr,
                       std::unique_ptr<Modelos3D> m, GameObject* p = nullptr);
    ~CrearObjetoComando() override = default;

    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;
};

#endif
