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
#ifndef IPHYSICS_BACKEND_H
#define IPHYSICS_BACKEND_H

#include <vector>

#include "ContactoFisico.h"

class RigidBody;

class IPhysicsBackend {
public:
    virtual ~IPhysicsBackend() = default;
    virtual void stepSimulation(float deltaTime) = 0;
    virtual void addRigidBody(RigidBody* body) = 0;
    virtual void removeRigidBody(RigidBody* body) = 0;
    virtual std::vector<EventoContacto> tomarEventosContacto() { return {}; }
    // Gravedad global del mundo (por defecto la historica del motor).
    virtual void fijarGravedad(float x, float y, float z) {
        (void)x;
        (void)y;
        (void)z;
    }
    virtual void gravedad(float& x, float& y, float& z) const {
        x = 0.0f;
        y = -1.0f;
        z = 0.0f;
    }
};

#endif
