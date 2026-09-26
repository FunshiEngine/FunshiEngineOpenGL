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
#ifndef OBJETOESCENA_H
#define OBJETOESCENA_H

#include "SimpleObject.h"

#include <cstdio>

// GameObject "Scene": la raiz estructural de cada escena. Es un contenedor
// sin geometria (SimpleObject) que agrupa como hijos a TODAS las entidades de
// la escena: cada objeto 3D y las camaras cuelgan de el en el arbol. La
// escena tiene exactamente una raiz de este tipo (id 0), creada por
// SceneRegistry::createDefaultRoot y re-creada al cargar el binario raiz.
class ObjetoEscena : public SimpleObject {
private:
    void init() { std::snprintf(inputName, sizeof(inputName), "Scene"); }

public:
    explicit ObjetoEscena(Entity* origin) : SimpleObject(origin) { init(); }
    ObjetoEscena() : SimpleObject() { init(); }
};

#endif