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
#ifndef IRESOLUTORMATERIALES_H
#define IRESOLUTORMATERIALES_H

#include <string>

class GameObject;
class IMaterial;

// Resuelve el material que corresponde a una region de la malla de un objeto.
//
// Es el punto unico donde se decide que material se dibuja: el renderer
// pregunta por cada region de la malla y usa lo que le devuelvan, sin saber
// si el objeto tiene un material unico, materiales por region, o ninguno.
class IResolutorMateriales {
public:
    virtual ~IResolutorMateriales() = default;

    // Material de la region indicada (por el nombre con el que la declaro el
    // archivo del modelo). Devuelve nullptr si el objeto no tiene material
    // para esa region: el renderer usa entonces el color del objeto o los
    // valores por defecto.
    virtual const IMaterial* materialDeRegion(const GameObject* objeto,
                                              const std::string& nombreRegion) const = 0;
};

#endif // IRESOLUTORMATERIALES_H
