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
#ifndef PHYSICSENGINE_H
#define PHYSICSENGINE_H

#include <memory>
#include <vector>

#include "ContactoFisico.h"
class IPhysicsBackend;
class RigidBody;

// Fachada del mundo fisico (patron Facade).
//
// Header liviano: no expone Bullet. El backend concreto (BulletPhysicsAdapter)
// se oculta detras de IPhysicsBackend (patron Strategy) y se inyecta por
// constructor; con nullptr se puede montar la fachada sin mundo (tests/editor).
class PhysicsEngine {
public:
    PhysicsEngine();
    explicit PhysicsEngine(std::unique_ptr<IPhysicsBackend> backend);
    ~PhysicsEngine();

    PhysicsEngine(const PhysicsEngine&) = delete;
    PhysicsEngine& operator=(const PhysicsEngine&) = delete;

    void stepSimulation(float deltaTime);
    void addRigidBody(RigidBody* body);
    void removeRigidBody(RigidBody* body);
    std::vector<EventoContacto> tomarEventosContacto();

private:
    std::unique_ptr<IPhysicsBackend> backend_;
};
#endif