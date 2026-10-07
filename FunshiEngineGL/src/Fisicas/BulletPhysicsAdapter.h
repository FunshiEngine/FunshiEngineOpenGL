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
#ifndef BULLET_PHYSICS_ADAPTER_H
#define BULLET_PHYSICS_ADAPTER_H

#include "IPhysicsBackend.h"
#include <btBulletDynamicsCommon.h>
#include <set>
#include <unordered_set>
#include <vector>

class BulletPhysicsAdapter final : public IPhysicsBackend {
private:
    btDefaultCollisionConfiguration* collisionConfig;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
    btCollisionShape* groundShape;
    btDefaultMotionState* groundMotionState;
    btRigidBody* groundRigidBody;
    std::unordered_set<RigidBody*> cuerpos_;
    using ParColliders = std::pair<Collider*, Collider*>;
    std::set<ParColliders> contactosAnteriores_;
    std::vector<EventoContacto> eventosContacto_;

public:
    BulletPhysicsAdapter();
    ~BulletPhysicsAdapter() override;
    void stepSimulation(float deltaTime) override;
    void addRigidBody(RigidBody* body) override;
    void removeRigidBody(RigidBody* body) override;
    std::vector<EventoContacto> tomarEventosContacto() override;
    void fijarGravedad(float x, float y, float z) override;
    void gravedad(float& x, float& y, float& z) const override;

private:
    static ParColliders ordenarPar(Collider* a, Collider* b);
};

#endif
