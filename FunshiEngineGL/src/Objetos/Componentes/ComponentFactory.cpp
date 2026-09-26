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
#include "ComponentFactory.h"

#include "AudioSource.h"
#include "Colliders/CubeCollider.h"
#include "Colliders/EsfereCollider.h"
#include "Colliders/MallaCollider.h"
#include "CameraComponent.h"
#include "Color.h"
#include "Grid.h"
#include "InterfaceComponent.h"
#include "Light.h"
#include "Material.h"
#include "Model.h"
#include "RigidBody/RigidBody.h"
#include "Script.h"
#include "Transform.h"
#include "../GameObject.h"

namespace {

// Tolera nombres de componente ya persistidos por builds viejos de Windows:
// MSVC no demanglea y typeid().name() guardaba "class Transform"/"struct X".
std::string normalizarNombre(const std::string& typeName) {
    constexpr const char* prefijos[] = {"class ", "struct ", "union ", "enum "};
    for (const char* prefijo : prefijos) {
        const std::size_t len = std::char_traits<char>::length(prefijo);
        if (typeName.rfind(prefijo, 0) == 0)
            return typeName.substr(len);
    }
    return typeName;
}

} // namespace

std::unique_ptr<Component> ComponentFactory::create(const std::string& typeName,
                                                    GameObject& owner) {
    Transform* transform = owner.getComponent<Transform>();
    const std::string nombre = normalizarNombre(typeName);

    if (nombre == "Transform") return std::make_unique<Transform>();
    if (nombre == "Color") return std::make_unique<Color>();
    if (nombre == "Material") return std::make_unique<Material>();
    if (nombre == "Light") return std::make_unique<Light>();
    // El nombre con el que se serializa es el demangle RTTI de la clase
    // ("CameraComponent"); "Camera" se conserva como alias hacia atras.
    if (nombre == "CameraComponent" || nombre == "Camera")
        return std::make_unique<CameraComponent>();
    if (nombre == "EsfereCollider" && transform)
        return std::make_unique<EsfereCollider>(5.0f, transform, &owner);
    if (nombre == "CubeCollider" && transform)
        return std::make_unique<CubeCollider>(5.0f, transform, &owner);
    if (nombre == "MallaCollider" && transform)
        return std::make_unique<MallaCollider>(5.0f, transform, &owner);
    if (nombre == "RigidBody") {
        Collider* collider = owner.getComponent<Collider>();
        if (collider) return std::make_unique<RigidBody>(collider, 1.0f);
    }
    if (nombre == "Script") return std::make_unique<Script>();
    if (nombre == "AudioSource") return std::make_unique<AudioSource>();
    if (nombre == "InterfaceComponent")
        return std::make_unique<InterfaceComponent>();
    if (nombre == "Model") return std::make_unique<Model>();
    if (nombre == "Grid") return std::make_unique<Grid>();

    return nullptr;
}
