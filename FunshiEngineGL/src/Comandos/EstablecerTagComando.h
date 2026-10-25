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
#ifndef ESTABLECER_TAG_COMANDO_H
#define ESTABLECER_TAG_COMANDO_H

#include "IComando.h"
#include <string>

class EditorController;
class SceneRegistry;
class GameObject;

// Fija la etiqueta (tag) de un objeto y la restaura al deshacer.
class EstablecerTagComando : public IComando {
private:
    EditorController* editorController;
    SceneRegistry* sceneRegistry;
    int objectId;
    std::string etiqueta;
    std::string etiquetaAnterior;
    std::string nombreObjeto;

public:
    EstablecerTagComando(EditorController* ec, SceneRegistry* sr, GameObject* obj,
                          std::string nuevaEtiqueta);
    ~EstablecerTagComando() override = default;

    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;
};

#endif
