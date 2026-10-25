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

#include "ResolvedorMateriales.h"

#include "IMaterial.h"
#include "Material.h"
#include "Materiales.h"
#include "../GameObject.h"

const IMaterial* ResolvedorMateriales::materialDeRegion(
    const GameObject* objeto, const std::string& nombreRegion) const {
    if (!objeto) return nullptr;

    // Material de la region: el del objeto si tiene lista de materiales por
    // region y la region esta en ella.
    if (const Materiales* materiales = objeto->getComponent<Materiales>()) {
        if (const Material* material = materiales->materialDe(nombreRegion)) {
            return material;
        }
    }

    // Sin material para la region: el material unico del objeto (si lo tiene).
    if (const Material* material = objeto->getComponent<Material>()) {
        return material;
    }
    return nullptr;
}
