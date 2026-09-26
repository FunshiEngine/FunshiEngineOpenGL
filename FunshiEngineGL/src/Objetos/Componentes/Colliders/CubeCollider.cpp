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
#include "CubeCollider.h"

#include <memory>
#include <btBulletDynamicsCommon.h>

#include "../../../Rendering/LineBuilder.h"
#include "../../../Rendering/LineRenderer.h"

namespace {
// Ancho del wireframe del collider en pixeles (2 px: se ve al-redondeado sin
// tapar la geometria).
constexpr float kAnchoWire = 2.0f;
} // namespace

CubeCollider::CubeCollider(float radio, Transform* transformOfDadObject,
                           GameObject* owner)
    : Collider(radio, transformOfDadObject, owner) {}

std::unique_ptr<btCollisionShape> CubeCollider::createCollisionShape() {
    // Usar el radio como la mitad de tamano (box usa half extents)
    return std::make_unique<btBoxShape>(btVector3(radio, radio, radio));
}

void CubeCollider::dibujarCollider() {
    Transform globalT = getGlobalTransform();

    float modelArr[16];
    buildMatrixFromTransform(&globalT, modelArr);

    // El collider expone solo geometria local (cubo al radio); el dibujado lo
    // hace la capa de Rendering con el batch de lineas.
    const float verde[4] = {0.0f, 1.0f, 0.0f, 1.0f};

    // Cubo en origen local
    const float semilado[3] = {radio, radio, radio};
    const float origen[3] = {0.0f, 0.0f, 0.0f};

    LineBuilder builder;
    builder.agregarCaja(origen, semilado, verde);
    lineRenderer().dibujar(builder, obtenerWireBatch(), modelArr, kAnchoWire);
}
