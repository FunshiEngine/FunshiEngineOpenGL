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
#include "Entity.h"
#include "../Herramientas/TypeUtils.h"
#include "../Objetos/Componentes/ComponentFactory.h"
#include "../Objetos/GameObject.h"
#include <algorithm>
#include <iostream>
#include <typeinfo>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

Entity::Entity()
    : components(std::make_unique<ListaDE<Component*>>()) {
    auto transform = std::make_unique<Transform>();
    components->addLast(transform.get());
    componentOwners.push_back(std::move(transform));
}

Entity::Entity(Entity* origin)
    : components(std::make_unique<ListaDE<Component*>>()),
      parentEntity(origin),
      transformOrigin(origin != nullptr ? origin->getComponent<Transform>() : nullptr) {
    auto transform = std::make_unique<Transform>();
    components->addLast(transform.get());
    componentOwners.push_back(std::move(transform));
    if (parentEntity) {
        parentEntity->childEntities.push_back(this);
    }
}

Entity::~Entity() {
    if (parentEntity) {
        auto& sibs = parentEntity->childEntities;
        sibs.erase(std::remove(sibs.begin(), sibs.end(), this), sibs.end());
        parentEntity = nullptr;
    }
    for (auto* child : childEntities) {
        if (child) {
            child->parentEntity = nullptr;
            child->transformOrigin = nullptr;
        }
    }
    childEntities.clear();
    clearComponents();
}

void Entity::clearComponents() {
    if (components) components->clear();
    componentOwners.clear();
}

Binario* Entity::getMyBinario() { return myBinario.get(); }

void Entity::setOriginTransform(Transform* newOriginTransform) {
    ownedTransformOrigin.reset();
    transformOrigin = newOriginTransform;
}

void Entity::setParentEntity(Entity* parent) {
    if (parentEntity == parent) return;
    if (parentEntity) {
        auto& sibs = parentEntity->childEntities;
        sibs.erase(std::remove(sibs.begin(), sibs.end(), this), sibs.end());
    }
    parentEntity = parent;
    if (parentEntity) {
        parentEntity->childEntities.push_back(this);
        setOriginTransform(parentEntity->getComponent<Transform>());
    } else {
        setOriginTransform(nullptr);
    }
}


void Entity::addComponent(Component* component) {
    addComponent(std::unique_ptr<Component>(component));
}

void Entity::addComponent(
    std::unique_ptr<Component> component)
{
    if (!component || components->isElement(component.get()))
        return;

    Component* rawComponent = component.get();

    componentOwners.emplace_back(
        std::move(component)
    );

    components->addLast(rawComponent);
}

void Entity::deleteComponent(Component* component) {

    if (component == nullptr ||
        !components ||
        !components->isElement(component))
        return;

    components->deleteByElement(component);

    componentOwners.erase(
        std::remove_if(
            componentOwners.begin(),
            componentOwners.end(),
            [component](
                const std::unique_ptr<Component>& owned)
            {
                return owned.get() == component;
            }
        ),
        componentOwners.end()
    );
}

std::unique_ptr<Component> Entity::extractComponent(Component* component) {
    if (component == nullptr || !components ||
        !components->isElement(component))
        return nullptr;

    components->deleteByElement(component);

    for (auto it = componentOwners.begin(); it != componentOwners.end();
         ++it) {
        if (it->get() == component) {
            auto result = std::move(*it);
            componentOwners.erase(it);
            return result;
        }
    }
    return nullptr;
}

Component* Entity::getComponentByName(const std::string& typeName) {
    if (!components || components->isEmpty()) return nullptr;
    Position<Component*>* position = components->first();
    while (position != nullptr) {
        Component* comp = position->getElement();
        if (comp) {
            std::string name = nombreSimpleTipo(typeid(*comp));
            if (name == typeName) return comp;
        }
        position = (position != components->last())
                       ? components->next(position)
                       : nullptr;
    }
    return nullptr;
}

bool Entity::hasComponent(const std::string& typeName) {
    return getComponentByName(typeName) != nullptr;
}

ListaDE<Component*>* Entity::getComponents() {
    return components.get();
}

void Entity::serializeGlobalAtributes() {

    serializeExternalAtributes();

    serializeLocalAtributes();
}

void Entity::serializeExternalAtributes() {
    serializeTransformOrigin();
}

void Entity::serializeTransformOrigin() {

    auto* file =
        myBinario->getOfBinariFile();

    const bool hasTransform =
        transformOrigin != nullptr;

    file->write(
        reinterpret_cast<const char*>(&hasTransform),
        sizeof(bool)
    );

    if (hasTransform) {

        const std::string typeName =
            "TransformOrigin";

        const size_t length =
            typeName.size();

        file->write(
            reinterpret_cast<const char*>(&length),
            sizeof(size_t)
        );

        file->write(
            typeName.c_str(),
            length
        );

        transformOrigin->saveComponent(file);
    }
}

void Entity::deserializeGlobalAtributes() {

    deserializeExternalAtributes();

    deserializeLocalAtributes();
}

void Entity::deserializeExternalAtributes() {
    deserializeTransformOrigin();
}

void Entity::deserializeTransformOrigin() {

    auto* file =
        myBinario->getIfBinariFile();

    bool hasTransform = false;

    file->read(
        reinterpret_cast<char*>(&hasTransform),
        sizeof(bool)
    );

    if (!hasTransform) {

        ownedTransformOrigin.reset();

        transformOrigin = nullptr;

        return;
    }

    size_t length = 0;

    file->read(
        reinterpret_cast<char*>(&length),
        sizeof(size_t)
    );

    // Misma cota de sanidad que en deserializeEntityComponents: un length
    // absurdo delata un stream corrupto o desalineado y no debe abortar el
    // proceso al construir el string.
    if (length == 0 || length > 256) {

        std::cerr
            << "Nombre de tipo invalido en el binario (len "
            << length
            << "); se descarta el TransformOrigin\n";

        ownedTransformOrigin.reset();

        transformOrigin = nullptr;

        return;
    }

    std::string typeName(
        length,
        '\0'
    );

    file->read(
        typeName.data(),
        length
    );

    if (typeName == "TransformOrigin") {

        if (!transformOrigin) {

            ownedTransformOrigin =
                std::make_unique<Transform>();

            transformOrigin =
                ownedTransformOrigin.get();
        }

        transformOrigin->loadComponent(file);

    }
    else {

        std::cerr
            << "Tipo de atributo heredado desconocido: "
            << typeName
            << '\n';
    }
}

void Entity::serializeEntityComponents() {

    auto* file =
        myBinario->getOfBinariFile();

    const size_t count =
        components->tam();

    file->write(
        reinterpret_cast<const char*>(&count),
        sizeof(size_t)
    );

    if (components->isEmpty())
        return;

    Position<Component*>* position =
        components->first();

    while (position != nullptr) {

        Component* component =
            position->getElement();

        std::string typeName =
            nombreSimpleTipo(typeid(*component));

        const size_t length =
            typeName.size();

        file->write(
            reinterpret_cast<const char*>(&length),
            sizeof(size_t)
        );

        file->write(
            typeName.c_str(),
            length
        );

        component->saveComponent(file);

        position =
            (position != components->last())
                ? components->next(position)
                : nullptr;
    }
}

void Entity::deserializeEntityComponents(GameObject& owner) {

    auto* file =
        myBinario->getIfBinariFile();

    size_t count = 0;

    file->read(
        reinterpret_cast<char*>(&count),
        sizeof(size_t)
    );

    clearComponents();

    for (size_t i = 0; i < count; ++i) {

        size_t length = 0;

        file->read(
            reinterpret_cast<char*>(&length),
            sizeof(size_t)
        );

        // Cota de sanidad: los nombres de tipo de los componentes son cortos.
        // Un length mayor delata un stream corrupto o desalineado y construir
        // el string con ese valor lanzaria std::length_error, abortando el
        // proceso. Se corta la lectura del objeto con un error claro.
        if (length == 0 || length > 256) {

            std::cerr
                << "Nombre de componente invalido en el binario (len "
                << length
                << "); se corta la lectura del objeto\n";

            return;
        }

        std::string typeName(
            length,
            '\0'
        );

        file->read(
            typeName.data(),
            length
        );

        std::unique_ptr<Component> component =
            ComponentFactory::create(
                typeName,
                owner
            );

        if (component != nullptr) {

            component->loadComponent(file);

            // Post-carga polimorfica: cada componente aplica su estado al dueno
            // (p. ej. CameraComponent vincula su dueno y deriva la vista del
            // Transform; Color refleja su valor en auxColor para el inspector).
            // Sin dispatch manual por nombre de tipo aqui.
            component->onLoaded(owner);

            addComponent(
                std::move(component)
            );

        }
        else {

            std::cerr
                << "Tipo de componente desconocido: "
                << typeName
                << '\n';
        }
    }
}

void Entity::serializeEntity() {
    serializeGlobalAtributes();
}

void Entity::deserializeEntity() {
    deserializeGlobalAtributes();
}

Transform* Entity::getGlobalTransform() {
    Transform* result = getComponent<Transform>();
    if (result == nullptr) {
        return nullptr;
    }

    Transform* parentGlobal = nullptr;
    if (parentEntity) {
        parentGlobal = parentEntity->getGlobalTransform();
    } else if (transformOrigin) {
        parentGlobal = transformOrigin;
    }

    if (parentGlobal == nullptr) {
        return result;
    }

    float parentMat[16], localMat[16];
    buildMatrixFromTransform(parentGlobal, parentMat);
    buildMatrixFromTransform(result, localMat);

    glm::mat4 mParent = glm::make_mat4(parentMat);
    glm::mat4 mLocal = glm::make_mat4(localMat);
    glm::mat4 mGlobal = mParent * mLocal;

    float globalMat[16];
    const float* ptr = glm::value_ptr(mGlobal);
    for (int i = 0; i < 16; ++i) globalMat[i] = ptr[i];
    decomposeMatrixToTransform(globalMat, &globalTransformCache);

    return &globalTransformCache;
}
