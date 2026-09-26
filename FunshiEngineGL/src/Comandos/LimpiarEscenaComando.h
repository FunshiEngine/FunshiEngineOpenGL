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
#ifndef LIMPIAR_ESCENA_COMANDO_H
#define LIMPIAR_ESCENA_COMANDO_H

#include "IComando.h"
#include <memory>
#include <string>
#include <vector>

class EditorController;
class SceneRegistry;
class GameObject;

class LimpiarEscenaComando : public IComando {
private:
    EditorController* editorController;
    SceneRegistry* sceneRegistry;
    std::vector<std::unique_ptr<GameObject>> objetosEliminados;

public:
    LimpiarEscenaComando(EditorController* ec, SceneRegistry* sr);
    ~LimpiarEscenaComando() override = default;

    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;
};

#endif
