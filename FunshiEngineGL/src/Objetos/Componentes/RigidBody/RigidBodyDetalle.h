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
// Preambulo compartido por las unidades de RigidBody.cpp (generado al dividirlo).
#ifndef RIGIDBODYDETALLE_H
#define RIGIDBODYDETALLE_H

#include "RigidBody.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>
#include <cstdint>

namespace {

constexpr std::uint32_t kRigidBodyMagic = 0x32444252;

constexpr std::uint32_t kRigidBodyVersion = 3;

}  // namespace

// Posicion y rotacion GLOBAL del cuerpo. El cuerpo vive en el GLOBAL del
// collider (getGlobalTransform = ownerGlobal x myTransform), NO en el del
// objeto: cualquier offset del collider se incluye en la pose del body.
static void bulletWorldToMatrices(const btTransform& t, glm::vec3& pos,
                                  glm::quat& rot) {
    const btVector3 origin = t.getOrigin();
    const btQuaternion q = t.getRotation();
    pos = glm::vec3(origin.x(), origin.y(), origin.z());
    rot = glm::quat(q.w(), q.x(), q.y(), q.z());
}

#endif
