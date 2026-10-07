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
#ifndef JERARQUIA_ARBOL_H
#define JERARQUIA_ARBOL_H

#include "../../Entity/Entity.h"

// "Desanidar a raiz" solo tiene sentido si el objeto cuelga de un padre
// intermedio: un hijo directo de la raiz ya esta al nivel superior, y repetir
// la operacion solo reordenaria hermanos (el guard de ciclos de
// SceneRegistry::reparent sube hasta la raiz y no falla, pero no hace nada).
inline bool esCandidatoADesanidar(const Entity* objeto, const Entity* raiz) {
    return objeto != nullptr && raiz != nullptr &&
           objeto->getParentEntity() != nullptr &&
           objeto->getParentEntity() != raiz;
}

#endif
