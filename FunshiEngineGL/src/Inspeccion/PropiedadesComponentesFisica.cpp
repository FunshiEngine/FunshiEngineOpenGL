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
#include "PropiedadesComponentesDetalle.h"

namespace MotorInspeccion {

// Propiedades de cuerpo rigido y colliders.
void CatalogoPropiedades::registrarFisica() {
    // --- RigidBody -------------------------------------------------------
    registrar("RigidBody", {
        propiedadFlotanteValidado<RigidBody>(
            "mass", "Masa (0 = estatico)",
            [](RigidBody* r) { return r->masa(); },
            [](RigidBody* r, float v) { return r->fijarMasa(v); }),
        propiedadBooleana<RigidBody>(
            "active", "Activo",
            [](RigidBody* r) { return r->estaActivo(); },
            [](RigidBody* r, bool v) { r->setActivo(v); }),
        propiedadBooleana<RigidBody>(
            "useGravity", "Usa gravedad",
            [](RigidBody* r) { return r->usaGravedad(); },
            [](RigidBody* r, bool v) { r->fijarUsoGravedad(v); }),
        propiedadFlotanteValidado<RigidBody>(
            "gravityScale", "Escala de gravedad",
            [](RigidBody* r) { return r->escalaGravedad(); },
            [](RigidBody* r, float v) { return r->fijarEscalaGravedad(v); }),
        propiedadFlotanteValidado<RigidBody>(
            "friction", "Friccion",
            [](RigidBody* r) { return r->friccion(); },
            [](RigidBody* r, float v) { return r->fijarFriccion(v); }),
        propiedadBooleana<RigidBody>(
            "sensor", "Sensor (trigger)",
            [](RigidBody* r) { return r->esSensor(); },
            [](RigidBody* r, bool v) { r->fijarSensor(v); }),
        // Los freeze son por eje: al escribir uno se releen los otros dos para
        // no borrar los que ya estaban puestos.
        propiedadBooleana<RigidBody>(
            "freezePositionX", "Congelar posicion X",
            [](RigidBody* r) { return r->posicionCongelada(0); },
            [](RigidBody* r, bool v) {
                r->fijarFreezePosicion(v, r->posicionCongelada(1),
                                       r->posicionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezePositionY", "Congelar posicion Y",
            [](RigidBody* r) { return r->posicionCongelada(1); },
            [](RigidBody* r, bool v) {
                r->fijarFreezePosicion(r->posicionCongelada(0), v,
                                       r->posicionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezePositionZ", "Congelar posicion Z",
            [](RigidBody* r) { return r->posicionCongelada(2); },
            [](RigidBody* r, bool v) {
                r->fijarFreezePosicion(r->posicionCongelada(0),
                                       r->posicionCongelada(1), v);
            }),
        propiedadBooleana<RigidBody>(
            "freezeRotationX", "Congelar rotacion X",
            [](RigidBody* r) { return r->rotacionCongelada(0); },
            [](RigidBody* r, bool v) {
                r->fijarFreezeRotacion(v, r->rotacionCongelada(1),
                                       r->rotacionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezeRotationY", "Congelar rotacion Y",
            [](RigidBody* r) { return r->rotacionCongelada(1); },
            [](RigidBody* r, bool v) {
                r->fijarFreezeRotacion(r->rotacionCongelada(0), v,
                                       r->rotacionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezeRotationZ", "Congelar rotacion Z",
            [](RigidBody* r) { return r->rotacionCongelada(2); },
            [](RigidBody* r, bool v) {
                r->fijarFreezeRotacion(r->rotacionCongelada(0),
                                       r->rotacionCongelada(1), v);
            }),
    });

    // --- Colliders -------------------------------------------------------
    // El radio solo existe en la esfera y en el cubo: en la malla y en el
    // tilemap la forma la define la malla o el mapa, no un radio.
    registrar("EsfereCollider", {
        propiedadFlotanteValidado<EsfereCollider>(
            "radius", "Radio",
            [](EsfereCollider* c) { return c->getRadio(); },
            [](EsfereCollider* c, float v) {
                // Collider::setRadio ignora en silencio los radios que no son
                // positivos: se rechaza aqui para que el error sea visible.
                if (v <= 0.0f) return false;
                c->setRadio(v);
                return true;
            }),
        propiedadBooleana<EsfereCollider>(
            "visibleInScene", "Visible en la escena",
            [](EsfereCollider* c) { return c->estaVisibleEnEscena(); },
            [](EsfereCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });
    registrar("CubeCollider", {
        propiedadFlotanteValidado<CubeCollider>(
            "radius", "Semilado del cubo",
            [](CubeCollider* c) { return c->getRadio(); },
            [](CubeCollider* c, float v) {
                if (v <= 0.0f) return false;
                c->setRadio(v);
                return true;
            }),
        propiedadBooleana<CubeCollider>(
            "visibleInScene", "Visible en la escena",
            [](CubeCollider* c) { return c->estaVisibleEnEscena(); },
            [](CubeCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });
    registrar("MallaCollider", {
        propiedadBooleana<MallaCollider>(
            "visibleInScene", "Visible en la escena",
            [](MallaCollider* c) { return c->estaVisibleEnEscena(); },
            [](MallaCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });
    registrar("TilemapCollider", {
        propiedadBooleana<TilemapCollider>(
            "visibleInScene", "Visible en la escena",
            [](TilemapCollider* c) { return c->estaVisibleEnEscena(); },
            [](TilemapCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });
}

}  // namespace MotorInspeccion
