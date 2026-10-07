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
#ifndef DUPLICAR_OBJETO_COMANDO_H
#define DUPLICAR_OBJETO_COMANDO_H

#include "IComando.h"
#include <memory>
#include <string>
#include <vector>

class EditorController;
class SceneRegistry;
class GameObject;

// Duplica un objeto y su subarbol (Ctrl+C/V de la jerarquia) por la puerta
// del editor: registra los cuerpos clonados y publica los eventos. Todo por
// ids (nunca se retienen punteros): deshacer borra las copias por id y
// rehacer vuelve a duplicar el original.
class DuplicarObjetoComando : public IComando {
private:
    EditorController* editorController;
    SceneRegistry* sceneRegistry;
    int originalId = -1;
    int parentId = -1;
    std::vector<int> creadosIds;
    std::string nombreObjeto;

public:
    DuplicarObjetoComando(EditorController* ec, GameObject* obj,
                          GameObject* padre, SceneRegistry* sr);
    ~DuplicarObjetoComando() override = default;

    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;
};

#endif
