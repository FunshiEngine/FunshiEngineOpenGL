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
#include "SceneRegistry.h"

#include <algorithm>
#include <functional>

#include "../Objetos/GameObject.h"
#include "../Objetos/Modelos3D.h"
#include "../Objetos/ObjetoEscena.h"
#include "../Objetos/Componentes/Transform.h"

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
        int nextId = 1;
        for (const auto& candidate : ownedGameObjects)
            nextId = std::max(nextId, candidate->getId() + 1);
        object->setId(nextId);
    }

    GameObject* raw = object.get();
    viewDirty = true;
    ownedGameObjects.emplace_back(std::move(object));
    entitys.addNodeChildOf(parentPosition, raw);
    raw->setParentEntity(parent);
    refreshGameObjectView();
    return raw;
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
    object->setParentEntity(parent);
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

std::unique_ptr<GameObject> SceneRegistry::takeObject(GameObject* object) {
    if (!object || object == getRoot() || !contains(object)) {
        return nullptr;
    }
    Position<GameObject*>* position = nullptr;
    try {
        position = entitys.whatIsPositionOf(object);
    } catch (...) {
        return nullptr;
    }
    if (!position) {
        return nullptr;
    }

    viewDirty = true;
    try {
        entitys.deleteNode(position);
    } catch (...) {
        return nullptr;
    }

    for (auto it = ownedGameObjects.begin(); it != ownedGameObjects.end(); ++it) {
        if (it->get() == object) {
            auto result = std::move(*it);
            ownedGameObjects.erase(it);
            refreshGameObjectView();
            return result;
        }
    }

    refreshGameObjectView();
    return nullptr;
}

std::vector<std::unique_ptr<GameObject>> SceneRegistry::takeSubtree(GameObject* object) {
    if (!object || object == getRoot() || !contains(object)) {
        return {};
    }

    Position<GameObject*>* objPosition = nullptr;
    try {
        objPosition = entitys.whatIsPositionOf(object);
    } catch (...) {
        return {};
    }
    if (!objPosition) {
        return {};
    }

    std::vector<GameObject*> subtreeObjects;
    std::function<void(Position<GameObject*>*)> collectSubtree =
        [&](Position<GameObject*>* pos) {
            if (!pos) return;
            GameObject* go = pos->getElement();
            if (!go) return;
            subtreeObjects.push_back(go);

            ListaDE<Position<GameObject*>*>* children = nullptr;
            try {
                children = entitys.childsOf(pos);
            } catch (...) {
                return;
            }
            if (children && !children->isEmpty()) {
                Position<Position<GameObject*>*>* child = children->first();
                while (child) {
                    collectSubtree(child->getElement());
                    child = (child != children->last())
                                  ? children->next(child)
                                  : nullptr;
                }
            }
            delete children;
        };
    collectSubtree(objPosition);

    viewDirty = true;

    for (auto rit = subtreeObjects.rbegin(); rit != subtreeObjects.rend();
         ++rit) {
        try {
            Position<GameObject*>* pos = entitys.whatIsPositionOf(*rit);
            if (pos) {
                entitys.deleteNode(pos);
            }
        } catch (...) {
        }
    }

    std::vector<std::unique_ptr<GameObject>> result;
    for (GameObject* go : subtreeObjects) {
        for (auto it = ownedGameObjects.begin(); it != ownedGameObjects.end();
             ++it) {
            if (it->get() == go) {
                result.push_back(std::move(*it));
                ownedGameObjects.erase(it);
                break;
            }
        }
    }

    refreshGameObjectView();
    return result;
}

std::vector<std::unique_ptr<GameObject>> SceneRegistry::takeAllNonRoot() {
    if (entitys.isEmpty()) {
        return {};
    }
    GameObject* root = getRoot();
    if (!root) {
        return {};
    }

    std::vector<GameObject*> toTake;
    for (const auto& object : ownedGameObjects) {
        if (object && object->getId() != 0) {
            toTake.push_back(object.get());
        }
    }

    if (toTake.empty()) {
        return {};
    }

    viewDirty = true;

    for (auto rit = toTake.rbegin(); rit != toTake.rend(); ++rit) {
        try {
            Position<GameObject*>* pos = entitys.whatIsPositionOf(*rit);
            if (pos) {
                entitys.deleteNode(pos);
            }
        } catch (...) {
        }
    }

    std::vector<std::unique_ptr<GameObject>> result;
    for (GameObject* go : toTake) {
        for (auto it = ownedGameObjects.begin(); it != ownedGameObjects.end();
             ++it) {
            if (it->get() == go) {
                result.push_back(std::move(*it));
                ownedGameObjects.erase(it);
                break;
            }
        }
    }

    refreshGameObjectView();
    return result;
}

GameObject* SceneRegistry::restoreSubtree(
    std::vector<std::unique_ptr<GameObject>> objects,
    GameObject* parent) {
    if (objects.empty()) {
        return nullptr;
    }

    GameObject* restoreParent = parent ? parent : getRoot();
    if (!restoreParent || !contains(restoreParent)) {
        return nullptr;
    }

    Position<GameObject*>* parentPosition = nullptr;
    try {
        parentPosition = entitys.whatIsPositionOf(restoreParent);
    } catch (...) {
        return nullptr;
    }
    if (!parentPosition) {
        return nullptr;
    }

    viewDirty = true;

    std::vector<GameObject*> restoredObjects;
    for (auto& obj : objects) {
        if (!obj) continue;
        if (obj->getId() == 0) continue;
        if (contains(obj.get())) continue;

        restoredObjects.push_back(obj.get());
        ownedGameObjects.emplace_back(std::move(obj));
    }

    bool added = true;
    while (added && !restoredObjects.empty()) {
        added = false;
        for (GameObject* obj : restoredObjects) {
            Position<GameObject*>* pos = nullptr;
            try {
                pos = entitys.whatIsPositionOf(obj);
            } catch (...) {
                pos = nullptr;
            }
            if (pos) continue;

            GameObject* objParent = static_cast<GameObject*>(obj->getParentEntity());
            GameObject* treeParent = objParent;

            if (!treeParent ||
                std::find(restoredObjects.begin(), restoredObjects.end(),
                          treeParent) == restoredObjects.end()) {
                treeParent = restoreParent;
            }

            Position<GameObject*>* treeParentPos = nullptr;
            try {
                treeParentPos = entitys.whatIsPositionOf(treeParent);
            } catch (...) {
                treeParentPos = nullptr;
            }
            if (!treeParentPos) continue;

            entitys.addNodeChildOf(treeParentPos, obj);
            obj->setParentEntity(treeParent);
            added = true;
        }
    }

    refreshGameObjectView();

    for (GameObject* obj : restoredObjects) {
        if (obj) return obj;
    }
    return nullptr;
}
