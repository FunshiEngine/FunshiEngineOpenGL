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
#include "MallaCollider.h"

#include <vector>
#include <btBulletDynamicsCommon.h>

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Modelos3D.h"
#include "../../../Rendering/LineBuilder.h"
#include "../../../Rendering/LineRenderer.h"

namespace {
// Ancho del wireframe del collider en pixeles (2 px: se ve al-redondeado sin
// tapar la geometria).
constexpr float kAnchoWire = 2.0f;
} // namespace

MallaCollider::MallaCollider(float radio, Transform* transformOfDadObject,
                             GameObject* owner)
    : Collider(radio, transformOfDadObject, owner) {}

std::unique_ptr<btCollisionShape> MallaCollider::createCollisionShape() {
    // Convex hull de los vertices del modelo (es el colisionador geometrico
    // exacto de la malla). Sin malla cargada se cae a la shape de respaldo.
    Modelos3D* modelo = dynamic_cast<Modelos3D*>(owner);
    if (!modelo) return nullptr;

    const std::vector<vec3>& vertices = modelo->getVertices();
    if (vertices.size() < 4) return nullptr;

    auto hull = std::make_unique<btConvexHullShape>();
    for (const vec3& v : vertices) {
        hull->addPoint(btVector3(v.x, v.y, v.z));
    }
    return hull;
}

void MallaCollider::dibujarCollider() {
    Transform globalT = getGlobalTransform();
    float modelArr[16];
    buildMatrixFromTransform(&globalT, modelArr);

    const float verde[4] = {0.0f, 1.0f, 0.0f, 1.0f};

    LineBuilder builder;

    // Dibujo el hull real (bordes del convex hull) en vez de la caja
    // aproximada. getCollisionShape() construye la shape de forma lazy si
    // todavia no existe; es lo que permite ver el hull apenas se crea el
    // collider, sin esperar a que la fisica lo genere.
    btConvexHullShape* hull =
        dynamic_cast<btConvexHullShape*>(getCollisionShape());
    if (hull && hull->getNumPoints() > 0) {
        const btVector3* points = hull->getUnscaledPoints();
        const int numPoints = hull->getNumPoints();
        // Puntos del hull, en coordenadas locales, para la polilinea cerrada.
        std::vector<float> anillo(static_cast<size_t>(numPoints) * 3);
        for (int i = 0; i < numPoints; ++i) {
            anillo[3 * i + 0] = points[i].x();
            anillo[3 * i + 1] = points[i].y();
            anillo[3 * i + 2] = points[i].z();
        }
        builder.agregarPolilinea(anillo.data(),
                                 static_cast<std::size_t>(numPoints), true,
                                 verde);
    } else {
        // Respaldo grafico: caja envolvente rapida al radio.
        const float origen[3] = {0.0f, 0.0f, 0.0f};
        const float semilado[3] = {getRadio(), getRadio(), getRadio()};
        builder.agregarCaja(origen, semilado, verde);
    }

    lineRenderer().dibujar(builder, obtenerWireBatch(), modelArr, kAnchoWire);
}
