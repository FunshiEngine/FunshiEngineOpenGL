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
#include "SceneRegistryDetalle.h"

SceneRegistry::SceneRegistry() { createDefaultRoot(); }

SceneRegistry::~SceneRegistry() {
    gameObjects.clear();
    while (!entitys.isEmpty()) entitys.deleteRoot();
    ownedGameObjects.clear();
}

void SceneRegistry::createDefaultRoot() {
    viewDirty = true;
    // La raiz de la escena es el GameObject "Scene": un contenedor sin
    // geometria que agrupa como hijos a todas las entidades (objetos y
    // camaras). Queda excluida de la vista plana (refreshGameObjectView) y no
    // se dibuja; solo estructura la jerarquia. Es compatible con escenas
    // viejas: al cargar un binario raiz que habia guardado malla, los bytes
    // sobrantes quedan sin leer (EOF) y no rompen el formato.
    auto root = std::make_unique<ObjetoEscena>();
    root->setId(0);
    root->setParentEntity(nullptr);
    GameObject* rootRaw = root.get();
    ownedGameObjects.emplace_back(std::move(root));
    entitys.createRoot(rootRaw);
}

GameObject* SceneRegistry::getRoot() const noexcept {
    if (entitys.isEmpty()) return nullptr;
    return const_cast<ArbolEnlazado<GameObject*>*>(&entitys)
        ->rootOfTree()->getElement();
}

bool SceneRegistry::contains(GameObject* object) const noexcept {
    return object != nullptr &&
           std::any_of(ownedGameObjects.begin(), ownedGameObjects.end(),
                       [object](const std::unique_ptr<GameObject>& candidate) {
                           return candidate.get() == object;
                       });
}

void SceneRegistry::refreshTransformOrigins(Position<GameObject*>* parent) {
    if (!parent || !entitys.isInternal(parent)) return;
    auto* children = entitys.childsOf(parent);
    if (children && !children->isEmpty()) {
        Position<Position<GameObject*>*>* child = children->first();
        while (child) {
            Position<GameObject*>* childPosition = child->getElement();
            childPosition->getElement()->setParentEntity(parent->getElement());
            refreshTransformOrigins(childPosition);
            child = (child != children->last()) ? children->next(child) : nullptr;
        }
    }
    delete children;
}

void SceneRegistry::refreshGameObjectView() {
    // La vista solo se reconstruye cuando hubo alguna mutacion.
    if (!viewDirty) return;
    viewDirty = false;

    gameObjects.clear();
    if (entitys.isEmpty()) return;

    refreshTransformOrigins(entitys.rootOfTree());
    std::function<void(Position<GameObject*>*)> appendChildren =
        [&](Position<GameObject*>* parent) {
            if (!entitys.isInternal(parent)) return;
            auto* children = entitys.childsOf(parent);
            if (children && !children->isEmpty()) {
                Position<Position<GameObject*>*>* child = children->first();
                while (child) {
                    Position<GameObject*>* childPosition = child->getElement();
                    gameObjects.addLast(childPosition->getElement());
                    appendChildren(childPosition);
                    child = (child != children->last()) ? children->next(child) : nullptr;
                }
            }
            delete children;
        };
    appendChildren(entitys.rootOfTree());
}

GameObject* SceneRegistry::createObject(std::unique_ptr<GameObject> object,
                                        GameObject* parent) {
    if (!object) return nullptr;
    if (entitys.isEmpty()) {
        viewDirty = true;
        object->setOriginTransform(nullptr);
        GameObject* raw = object.get();
        ownedGameObjects.emplace_back(std::move(object));
        entitys.createRoot(raw);
        refreshGameObjectView();
        return raw;
    }

    if (!parent) parent = getRoot();
    Position<GameObject*>* parentPosition =
        parent ? entitys.whatIsPositionOf(parent) : nullptr;
    if (!parentPosition) return nullptr;

    if (object->getId() == 0) {
        object->setId(siguienteIdDisponible());
    }

    GameObject* raw = object.get();
    viewDirty = true;
    ownedGameObjects.emplace_back(std::move(object));
    entitys.addNodeChildOf(parentPosition, raw);
    raw->setParentEntity(parent);
    refreshGameObjectView();
    return raw;
}

int SceneRegistry::siguienteIdDisponible() const noexcept {
    int nextId = 1;
    for (const auto& candidate : ownedGameObjects)
        nextId = std::max(nextId, candidate->getId() + 1);
    return nextId;
}

bool SceneRegistry::replaceRoot(std::unique_ptr<GameObject> root) {
    if (!root) return false;
    viewDirty = true;
    clear();
    entitys.deleteRoot();
    ownedGameObjects.clear();
    root->setParentEntity(nullptr);
    GameObject* raw = root.get();
    ownedGameObjects.emplace_back(std::move(root));
    entitys.createRoot(raw);
    refreshGameObjectView();
    return true;
}

bool SceneRegistry::deleteObject(GameObject* object) {
    if (!contains(object) || entitys.isEmpty()) return false;
    Position<GameObject*>* position = entitys.whatIsPositionOf(object);
    if (!position) return false;

    if (position == entitys.rootOfTree()) {
        clear();
        return true;
    }

    entitys.deleteNode(position);
    viewDirty = true;
    ownedGameObjects.erase(
        std::remove_if(ownedGameObjects.begin(), ownedGameObjects.end(),
                       [object](const std::unique_ptr<GameObject>& candidate) {
                           return candidate.get() == object;
                       }),
        ownedGameObjects.end());
    refreshGameObjectView();
    return true;
}

bool SceneRegistry::deleteObjectByID(int id) {
    for (const auto& object : ownedGameObjects) {
        if (object->getId() == id) return deleteObject(object.get());
    }
    return false;
}

bool SceneRegistry::reparent(GameObject* object, GameObject* parent) {
    if (!contains(object) || !contains(parent) || object == parent ||
        object == getRoot())
        return false;
    Position<GameObject*>* objectPosition = entitys.whatIsPositionOf(object);
    Position<GameObject*>* parentPosition = entitys.whatIsPositionOf(parent);
    if (!objectPosition || !parentPosition) return false;

    try {
        viewDirty = true;
        Position<GameObject*>* current = parentPosition;
        while (current != entitys.rootOfTree()) {
            if (current == objectPosition) return false;
            current = entitys.dadOf(current);
        }
        entitys.positionToChildOf(parentPosition, objectPosition);
    } catch (...) {
        return false;
    }
    // Preservar la pose mundial: el local se interpreta contra el padre nuevo,
    // asi que hay que reescribirlo con inverse(mundoPadre) * mundoHijo. Los
    // descendientes no se tocan: sus locales no cambian y el mundo del padre
    // tampoco, de modo que sus mundos se conservan solos.
    Transform* transformHijo = object->getComponent<Transform>();
    Transform* transformPadre = parent->getComponent<Transform>();
    float mundoHijo[16];
    const bool preservarPose =
        transformHijo != nullptr && transformPadre != nullptr;
    if (preservarPose)
        buildMatrixFromTransform(object->getGlobalTransform(), mundoHijo);

    object->setParentEntity(parent);

    if (preservarPose) {
        float mundoPadre[16];
        buildMatrixFromTransform(parent->getGlobalTransform(), mundoPadre);
        glm::mat4 localNuevo =
            glm::inverse(glm::make_mat4(mundoPadre)) * glm::make_mat4(mundoHijo);
        float localArr[16];
        const float* ptr = glm::value_ptr(localNuevo);
        for (int i = 0; i < 16; ++i) localArr[i] = ptr[i];
        decomposeMatrixToTransform(localArr, transformHijo);
    }
    refreshGameObjectView();
    return true;
}

void SceneRegistry::clear() {
    viewDirty = true;
    gameObjects.clear();
    while (!entitys.isEmpty()) entitys.deleteRoot();
    ownedGameObjects.clear();
    createDefaultRoot();
}

GameObject* SceneRegistry::getObjectByID(int id) const {
    for (const auto& object : ownedGameObjects) {
        if (object && object->getId() == id) {
            return object.get();
        }
    }
    return nullptr;
}
