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
#include "TilemapCollider.h"

#include <vector>

#include <btBulletDynamicsCommon.h>

#include "../../../Rendering/Lineas/LineBuilder.h"
#include "../../../Rendering/Lineas/LineRenderer.h"
#include "../../GameObject.h"
#include "../Juego2D/Tilemap.h"

namespace {
// Ancho del wireframe del collider en pixeles (mismo criterio que los demas
// colliders).
constexpr float kAnchoWire = 2.0f;
// Media profundidad (Z) de las cajas: el tilemap es 2D, las cajas son finas.
constexpr float kMediaProfundidad = 0.5f;

// Un tramo horizontal de tiles solidos, en el espacio local del tilemap
// (1 tile = 1 unidad, fila 0 arriba). Se fusionan celdas contiguas para reducir
// el numero de cajas de la shape compuesta.
struct Tramo {
    float centroX = 0.0f;
    float centroY = 0.0f;
    float mitadX = 0.0f;
};

// Recorre las capas con colision del tilemap y devuelve sus tramos solidos.
std::vector<Tramo> recolectarTramos(const Tilemap& tilemap) {
    std::vector<Tramo> tramos;
    if (!tilemap.tilesetValido()) return tramos;
    const Tileset& ts = tilemap.getTileset();

    const auto esSolido = [&ts](std::uint32_t id) {
        return id > 0 && id < ts.solido.size() && ts.solido[id] != 0;
    };

    for (int capa = 0; capa < tilemap.cantidadCapas(); ++capa) {
        const CapaTilemap* c = tilemap.capa(capa);
        if (!c || !c->colision) continue;
        for (int y = 0; y < c->alto; ++y) {
            int x = 0;
            while (x < c->ancho) {
                const std::uint32_t id =
                    c->tiles[static_cast<std::size_t>(y) * c->ancho + x];
                if (!esSolido(id)) {
                    ++x;
                    continue;
                }
                const int inicio = x;
                while (x < c->ancho &&
                       esSolido(c->tiles[static_cast<std::size_t>(y) * c->ancho +
                                         x]))
                    ++x;
                const int largo = x - inicio;
                Tramo t;
                t.centroX = static_cast<float>(inicio) +
                            static_cast<float>(largo) * 0.5f;
                t.centroY = -(static_cast<float>(y) + 0.5f);
                t.mitadX = static_cast<float>(largo) * 0.5f;
                tramos.push_back(t);
            }
        }
    }
    return tramos;
}
} // namespace

TilemapCollider::TilemapCollider(float radio, Transform* transformOfDadObject,
                                 GameObject* owner)
    : Collider(radio, transformOfDadObject, owner) {}

std::unique_ptr<btCollisionShape> TilemapCollider::createCollisionShape() {
    auto* compound = new btCompoundShape();

    Tilemap* tilemap = owner ? owner->getComponent<Tilemap>() : nullptr;
    if (tilemap) {
        for (const Tramo& tramo : recolectarTramos(*tilemap)) {
            auto* caja = new btBoxShape(
                btVector3(tramo.mitadX, 0.5f, kMediaProfundidad));
            btTransform transformada;
            transformada.setIdentity();
            transformada.setOrigin(
                btVector3(tramo.centroX, tramo.centroY, 0.0f));
            compound->addChildShape(transformada, caja);
        }
    }
    return std::unique_ptr<btCollisionShape>(compound);
}

void TilemapCollider::dibujarCollider() {
    if (!owner) return;
    const Tilemap* tilemap = owner->getComponent<Tilemap>();
    if (!tilemap) return;

    Transform myT = getGlobalTransform();
    float modelArr[16];
    buildMatrixFromTransform(&myT, modelArr);

    const float color[4] = {0.2f, 0.9f, 0.4f, 1.0f};
    LineBuilder builder;
    for (const Tramo& t : recolectarTramos(*tilemap)) {
        const float origen[3] = {t.centroX, t.centroY, 0.0f};
        const float semilado[3] = {t.mitadX, 0.5f, kMediaProfundidad};
        builder.agregarCaja(origen, semilado, color);
    }
    lineRenderer().dibujar(builder, obtenerWireBatch(), modelArr, kAnchoWire);
}
