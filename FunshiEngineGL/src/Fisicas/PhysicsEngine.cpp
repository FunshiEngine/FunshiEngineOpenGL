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
#include "PhysicsEngine.h"

#include "BulletPhysicsAdapter.h"
#include "IPhysicsBackend.h"

PhysicsEngine::PhysicsEngine()
    : backend_(std::make_unique<BulletPhysicsAdapter>()) {}

PhysicsEngine::PhysicsEngine(std::unique_ptr<IPhysicsBackend> backend)
    : backend_(std::move(backend)) {}

PhysicsEngine::~PhysicsEngine() = default;

void PhysicsEngine::stepSimulation(float deltaTime) {
    if (backend_) backend_->stepSimulation(deltaTime);
}

void PhysicsEngine::addRigidBody(RigidBody* body) {
    if (backend_) backend_->addRigidBody(body);
}

void PhysicsEngine::removeRigidBody(RigidBody* body) {
    if (backend_) backend_->removeRigidBody(body);
}

std::vector<EventoContacto> PhysicsEngine::tomarEventosContacto() {
    return backend_ ? backend_->tomarEventosContacto()
                    : std::vector<EventoContacto>{};
}

void PhysicsEngine::fijarGravedad(float x, float y, float z) {
    if (backend_) backend_->fijarGravedad(x, y, z);
}

void PhysicsEngine::gravedad(float& x, float& y, float& z) const {
    if (backend_) {
        backend_->gravedad(x, y, z);
        return;
    }
    x = 0.0f;
    y = -1.0f;
    z = 0.0f;
}