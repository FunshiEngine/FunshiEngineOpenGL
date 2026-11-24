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
// Preambulo compartido por las unidades de GizmoController.cpp (generado al dividirlo).
#ifndef GIZMOCONTROLLERDETALLE_H
#define GIZMOCONTROLLERDETALLE_H

#include "GizmoController.h"
#include "RayoPantalla.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../../Comandos/Transform/TransformComando.h"
#include "../../GUI/SceneGUI/SceneSelectedInterface.h"
#include "../../Objetos/Componentes/Escena/CameraComponent.h"
#include "../../Objetos/Componentes/Colliders/Collider.h"
#include "../../Objetos/Componentes/Colliders/CubeCollider.h"
#include "../../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../../Objetos/Componentes/Escena/Transform.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Render/Model.h"
#include "../../Rendering/Lineas/GuiaEje.h"
#include "EditorController.h"
#include "../Registro/SceneRegistry.h"
#include "ImGuizmo.h"

// True si la matriz 4x4 tiene algun elemento no finito (NaN/Inf). El gizmo
// nunca debe operar ni escribir matrices no finitas: al tocar un gizmo con
// una matriz corrupta, todo el transform quedaria en -nan y el objeto
// desapareceria de la escena.
static bool matrizNoFinita(glm::mat4 m) {
    const float* p = glm::value_ptr(m);
    for (int i = 0; i < 16; ++i) {
        if (!std::isfinite(p[i])) return true;
    }
    return false;
}

// Rayo AABB: slab method clasico. Devuelve false si el rayo no toca la caja o
// si el impacto queda detras del origen; si lo toca, tHit es la distancia de
// entrada (0 si el origen ya esta dentro).
static bool intersectRayAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                             const glm::vec3& boxMin, const glm::vec3& boxMax,
                             float& tHit) {
    float tmin = -1e30f;
    float tmax = 1e30f;

    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDir[i]) < 1e-7f) {
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i])
                return false;
        } else {
            float invD = 1.0f / rayDir[i];
            float t1 = (boxMin[i] - rayOrigin[i]) * invD;
            float t2 = (boxMax[i] - rayOrigin[i]) * invD;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    if (tmax < 0.0f) return false;
    tHit = (tmin < 0.0f) ? 0.0f : tmin;
    return true;
}

#endif
