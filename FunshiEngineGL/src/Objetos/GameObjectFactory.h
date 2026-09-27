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
#ifndef GAME_OBJECT_FACTORY_H
#define GAME_OBJECT_FACTORY_H

#include <memory>

class GameObject;

// Factoria de objetos de escena.
//
// Ademas de instanciar, pone el NOMBRE POR DEFECTO del objeto: `inputName` nace
// vacio y, si nadie lo llena, el arbol de escena muestra el nombre de la clase
// (fallback de SceneObjectTree) y el vacio se guarda y se recarga. Ver
// NombreUnico.h para el criterio de sufijo numerico.
//
// H-7: el camino de "agregar modelo 3D desde el explorador" no existe hoy, asi
// que createModelObject() (sin ningun llamador) se retiro en vez de dejarlo
// sin usar. Cuando ese camino aparezca, se agrega de vuelta aqui.
class GameObjectFactory {
public:
    // `raiz` es el arbol contra el que se evitan repetir nombres; puede ser
    // nulo (el objeto nace con un nombre libre igual).
    static std::unique_ptr<GameObject> createSimpleObject(const GameObject* raiz = nullptr);
};

#endif
