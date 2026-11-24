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
