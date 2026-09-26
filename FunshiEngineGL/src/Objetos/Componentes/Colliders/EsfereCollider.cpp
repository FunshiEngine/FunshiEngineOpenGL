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
#include "EsfereCollider.h"

#include <cmath>
#include <memory>
#include <btBulletDynamicsCommon.h>

#include "../../../Rendering/LineBuilder.h"
#include "../../../Rendering/LineRenderer.h"

namespace {
// Ancho del wireframe del collider en pixeles (2 px: se ve al-redondeado sin
// tapar la geometria).
constexpr float kAnchoWire = 2.0f;
} // namespace

EsfereCollider::EsfereCollider(float radio, Transform* transformOfDadObject,
                               GameObject* owner)
    : Collider(radio, transformOfDadObject, owner) {}

std::unique_ptr<btCollisionShape> EsfereCollider::createCollisionShape() {
    return std::make_unique<btSphereShape>(radio);
}

void EsfereCollider::dibujarCollider() {
    Transform globalT = getGlobalTransform();

    const float PI = 3.14159265358979323846f;

    float modelArr[16];
    buildMatrixFromTransform(&globalT, modelArr);

    const float verde[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    const int meridians = 8;
    const int parallels = 4;

    // Solo geometria: los segmentos de la esfera se arman con el LineBuilder y
    // los dibuja la capa de Rendering con un unico batch.
    LineBuilder builder;
    float pts[40][3];

    for (int m = 0; m < meridians; ++m) {
        float angle = (2.0f * PI * m) / meridians;

        for (int p = 0; p <= 20; ++p) {
            float lat = PI * float(p) / 20.0f - PI / 2.0f;

            pts[p][0] = radio * cosf(lat) * cosf(angle);
            pts[p][1] = radio * sinf(lat);
            pts[p][2] = radio * cosf(lat) * sinf(angle);
        }

        builder.agregarPolilinea(&pts[0][0], 21, false, verde);
    }

    for (int p = 1; p <= parallels; ++p) {
        float lat = PI * p / (parallels + 1) - PI / 2.0f;

        for (int m = 0; m < 40; ++m) {
            float lon = 2.0f * PI * m / 40.0f;

            pts[m][0] = radio * cosf(lat) * cosf(lon);
            pts[m][1] = radio * sinf(lat);
            pts[m][2] = radio * cosf(lat) * sinf(lon);
        }

        builder.agregarPolilinea(&pts[0][0], 40, true, verde);
    }

    lineRenderer().dibujar(builder, obtenerWireBatch(), modelArr, kAnchoWire);
}
