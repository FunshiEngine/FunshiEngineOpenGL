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
#include <vector>

class EditorController;
class SceneRegistry;
class GameObject;

// Crea un objeto de cualquier tipo (un modelo, un objeto simple...) y lo
// mete en la escena.
//
// El objeto llega ya construido: la fabrica que lo crea decide su tipo y su
// nombre (ver GameObjectFactory). Todo se guarda por id (nunca por puntero):
// el padre se resuelve por id en cada ejecucion, para que un padre borrado
// entre medias no deje un puntero colgando.
class CrearObjetoComando : public IComando {
private:
    EditorController* editorController;
    SceneRegistry* sceneRegistry;
    std::unique_ptr<GameObject> objeto;
    std::vector<std::unique_ptr<GameObject>> guardados;
    int parentId = 0;
    int createdId = -1;
    std::string nombreObjeto;

    GameObject* resolverPadre() const;

public:
    CrearObjetoComando(EditorController* ec, SceneRegistry* sr,
                       std::unique_ptr<GameObject> o, GameObject* p = nullptr);
    ~CrearObjetoComando() override = default;

    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;

    // Id del objeto creado en la ultima ejecucion (-1 si todavia no se
    // ejecuto). Permite al quien lo encola conocer el objeto resultante.
    int idCreado() const noexcept { return createdId; }
};

#endif
