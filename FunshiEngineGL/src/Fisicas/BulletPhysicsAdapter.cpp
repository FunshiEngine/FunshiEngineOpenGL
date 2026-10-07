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
#include "BulletPhysicsAdapter.h"

#include <functional>

#include "../Objetos/Componentes/RigidBody/RigidBody.h"

BulletPhysicsAdapter::BulletPhysicsAdapter()
    : collisionConfig(new btDefaultCollisionConfiguration()),
      dispatcher(new btCollisionDispatcher(collisionConfig)),
      overlappingPairCache(new btDbvtBroadphase()),
      solver(new btSequentialImpulseConstraintSolver()),
      dynamicsWorld(new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache,
                                                solver, collisionConfig)),
      groundShape(new btStaticPlaneShape(btVector3(0, 1, 0), 0)),
      groundMotionState(new btDefaultMotionState(
          btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)))),
      groundRigidBody(nullptr) {
    dynamicsWorld->setGravity(btVector3(0, -1.0f, 0));
    btRigidBody::btRigidBodyConstructionInfo info(
        0, groundMotionState, groundShape, btVector3(0, 0, 0));
    groundRigidBody = new btRigidBody(info);
    dynamicsWorld->addRigidBody(groundRigidBody);
}

BulletPhysicsAdapter::~BulletPhysicsAdapter() {
    if (dynamicsWorld && groundRigidBody)
        dynamicsWorld->removeRigidBody(groundRigidBody);
    delete groundRigidBody;
    delete groundMotionState;
    delete groundShape;
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfig;
}

void BulletPhysicsAdapter::stepSimulation(float deltaTime) {
    eventosContacto_.clear();
    for (RigidBody* body : cuerpos_) {
        if (!body) continue;
        body->actualizarEstadoSuelo(false);
        if (Collider* collider = body->getCollider())
            collider->limpiarContactos();
    }
    dynamicsWorld->stepSimulation(deltaTime);

    std::set<ParColliders> contactosActuales;
    for (int i = 0; i < dispatcher->getNumManifolds(); ++i) {
        const btPersistentManifold* manifold =
            dispatcher->getManifoldByIndexInternal(i);
        const auto* objetoA =
            static_cast<const btCollisionObject*>(manifold->getBody0());
        const auto* objetoB =
            static_cast<const btCollisionObject*>(manifold->getBody1());
        const auto* cuerpoA = static_cast<const btRigidBody*>(objetoA);
        const auto* cuerpoB = static_cast<const btRigidBody*>(objetoB);
        for (int contacto = 0; contacto < manifold->getNumContacts(); ++contacto) {
            const btManifoldPoint& punto = manifold->getContactPoint(contacto);
            if (punto.getDistance() > 0.01f) continue;
            const btVector3 normalHaciaA = punto.m_normalWorldOnB;
            RigidBody* bodyA = nullptr;
            RigidBody* bodyB = nullptr;
            for (RigidBody* body : cuerpos_) {
                if (!body || !body->estaActivo()) continue;
                if (body->getRigidBody() == cuerpoA) {
                    bodyA = body;
                } else if (body->getRigidBody() == cuerpoB) {
                    bodyB = body;
                }
                if (body->getRigidBody() == cuerpoA &&
                    normalHaciaA.y() > 0.5f) {
                    body->actualizarEstadoSuelo(true);
                } else if (body->getRigidBody() == cuerpoB &&
                           normalHaciaA.y() < -0.5f) {
                    body->actualizarEstadoSuelo(true);
                }
            }
            Collider* colliderA = bodyA ? bodyA->getCollider() : nullptr;
            Collider* colliderB = bodyB ? bodyB->getCollider() : nullptr;
            if (colliderA && colliderB && colliderA != colliderB) {
                colliderA->registrarContacto(colliderB);
                colliderB->registrarContacto(colliderA);
                contactosActuales.insert(ordenarPar(colliderA, colliderB));
            }
        }
    }

    for (const ParColliders& par : contactosActuales) {
        eventosContacto_.push_back(
            {par.first, par.second,
             contactosAnteriores_.count(par) ? TipoContacto::Persistencia
                                             : TipoContacto::Inicio});
    }
    for (const ParColliders& par : contactosAnteriores_)
        if (!contactosActuales.count(par))
            eventosContacto_.push_back(
                {par.first, par.second, TipoContacto::Fin});
    contactosAnteriores_ = std::move(contactosActuales);
}

void BulletPhysicsAdapter::addRigidBody(RigidBody* body) {
    if (!dynamicsWorld || !body || !body->getRigidBody()) return;
    dynamicsWorld->addRigidBody(body->getRigidBody());
    cuerpos_.insert(body);
    if (!body->estaActivo())
        body->getRigidBody()->forceActivationState(DISABLE_SIMULATION);
    // El cuerpo guarda su respuesta a la gravedad por separado (usar/escala):
    // al entrar al mundo se le empuja la gravedad vigente para que rija.
    const btVector3 actual = dynamicsWorld->getGravity();
    body->aplicarGravedadMundo(actual.x(), actual.y(), actual.z());
}

void BulletPhysicsAdapter::fijarGravedad(float x, float y, float z) {
    if (!dynamicsWorld) return;
    dynamicsWorld->setGravity(btVector3(x, y, z));
    for (RigidBody* body : cuerpos_) {
        if (body) body->aplicarGravedadMundo(x, y, z);
    }
}

void BulletPhysicsAdapter::gravedad(float& x, float& y, float& z) const {
    if (!dynamicsWorld) {
        x = 0.0f;
        y = -1.0f;
        z = 0.0f;
        return;
    }
    const btVector3 actual = dynamicsWorld->getGravity();
    x = actual.x();
    y = actual.y();
    z = actual.z();
}

void BulletPhysicsAdapter::removeRigidBody(RigidBody* body) {
    if (!body) return;
    cuerpos_.erase(body);
    if (dynamicsWorld && body->getRigidBody())
        dynamicsWorld->removeRigidBody(body->getRigidBody());
    for (RigidBody* registrado : cuerpos_)
        if (registrado && registrado->getCollider())
            registrado->getCollider()->limpiarContactos();
    if (Collider* collider = body->getCollider())
        collider->limpiarContactos();
    contactosAnteriores_.clear();
    eventosContacto_.clear();
}

BulletPhysicsAdapter::ParColliders BulletPhysicsAdapter::ordenarPar(
    Collider* a, Collider* b) {
    return std::less<Collider*>{}(b, a) ? ParColliders{b, a}
                                          : ParColliders{a, b};
}

std::vector<EventoContacto> BulletPhysicsAdapter::tomarEventosContacto() {
    std::vector<EventoContacto> eventos = std::move(eventosContacto_);
    eventosContacto_.clear();
    return eventos;
}
