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
#ifndef SCENE_REGISTRY_H
#define SCENE_REGISTRY_H

#include <memory>
#include <vector>

#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../Estructuras/Trees/ArbolesEnlazados/ArbolEnlazado.h"

class GameObject;
class Modelos3D;
template <typename E> class Position;

/**
 * Owns all scene GameObjects and the structural hierarchy.
 *
 * The tree and linear list expose raw pointers only as non-owning views. A
 * caller must never delete an object returned by this class.
 */
class SceneRegistry {
private:
    std::vector<std::unique_ptr<GameObject>> ownedGameObjects;
    ArbolEnlazado<GameObject*> entitys;
    ListaDE<GameObject*> gameObjects;
    // La vista lineal solo se reconstruye tras una mutacion; GameScene la
    // consulta 2 veces por frame y reconstruir siempre fugaba nodos.
    bool viewDirty = true;

    void createDefaultRoot();
    void refreshTransformOrigins(Position<GameObject*>* parent);

public:
    SceneRegistry();
    ~SceneRegistry();

    // Non-owning compatibility views. The registry remains the sole owner.
    ArbolEnlazado<GameObject*>* getEntitysTree() noexcept { return &entitys; }
    ListaDE<GameObject*>* getGameObjects() noexcept { return &gameObjects; }

    void refreshGameObjectView();
    // Non-owning pointer into ownedGameObjects.
    GameObject* getRoot() const noexcept;
    bool contains(GameObject* object) const noexcept;

    // Ownership of object is transferred to the registry.
    GameObject* createObject(std::unique_ptr<GameObject> object,
                             GameObject* parent = nullptr);
    bool replaceRoot(std::unique_ptr<GameObject> root);
    bool deleteObject(GameObject* object);
    bool deleteObjectByID(int id);
    bool reparent(GameObject* object, GameObject* parent);
    void clear();

    // Operaciones de undo/redo: desacoplan ownership del GameObject de la
    // estructura de arbol. take* devuelven los unique_ptr que poseia el registro
    // (y desconectan los nodos del arbol) para que un comando los conserve y
    // los restaure con restore*. No se permite desacoplar la raiz (id 0).
    GameObject* getObjectByID(int id) const;
    std::unique_ptr<GameObject> takeObject(GameObject* object);
    std::vector<std::unique_ptr<GameObject>> takeSubtree(GameObject* object);
    std::vector<std::unique_ptr<GameObject>> takeAllNonRoot();
    GameObject* restoreSubtree(std::vector<std::unique_ptr<GameObject>> objects,
                               GameObject* parent);
};

#endif
