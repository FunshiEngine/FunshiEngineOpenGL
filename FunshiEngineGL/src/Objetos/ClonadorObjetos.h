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
#ifndef CLONADOROBJETOS_H
#define CLONADOROBJETOS_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

class GameObject;

// Copia profunda de un objeto y su subarbol por roundtrip de la serializacion
// binaria de cada componente (el mismo formato que guarda la escena: los
// Script conservan clase, fuente y SerializeField, y el RigidBody sus
// propiedades fisicas).
//
// No inserta nada en la escena: devuelve pares (copia, padreDestino) en
// preorden para que el llamador los inserte con su puerta (EditorController
// en el editor, la cola diferida de scripts en play). `tomados` son los
// nombres ocupados y se completa con los asignados, asi pegar varias veces
// el mismo objeto uniciza cada copia.
class ClonadorObjetos {
public:
    static std::vector<std::pair<std::unique_ptr<GameObject>, GameObject*>>
    clonar(GameObject* original, GameObject* padre,
           std::vector<std::string> tomados);

    // `base` si esta libre, si no `base 1`, `base 2`, ... Anota el elegido en
    // `tomados` para que la proxima copia del lote no lo repita.
    static std::string nombreLibre(const std::string& base,
                                   std::vector<std::string>& tomados);
};

#endif
