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
#ifndef RESOLVEDORMATERIALES_H
#define RESOLVEDORMATERIALES_H

#include "IResolutorMateriales.h"

// Resolvedor de materiales: decide que material se dibuja en cada region de la
// malla de un objeto.
//
// El orden es el del editor: primero el material de la region (si el objeto
// tiene lista de materiales por region y la region esta en ella), despues el
// material unico del objeto, y si no hay ninguno, nullptr (el renderer usa el
// color del objeto o los valores por defecto).
class ResolvedorMateriales : public IResolutorMateriales {
public:
    const IMaterial* materialDeRegion(const GameObject* objeto,
                                      const std::string& nombreRegion) const override;
};

#endif // RESOLVEDORMATERIALES_H
