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
#ifndef ENTITY_H
#define ENTITY_H

#include <memory>
#include <vector>

#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../Objetos/Componentes/Component.h"
#include "../GestorDeArchivos/Binario.h"
#include "../Objetos/Componentes/Transform.h"

class Entity {
protected:
    // The list is a non-owning compatibility view. componentOwners owns every
    // component stored in it.
    std::unique_ptr<ListaDE<Component*>> components;
    std::vector<std::unique_ptr<Component>> componentOwners;
    std::unique_ptr<Binario> myBinario;
    Entity* parentEntity = nullptr;
    std::vector<Entity*> childEntities;
    Transform* transformOrigin = nullptr;
    std::unique_ptr<Transform> ownedTransformOrigin;

    void clearComponents();

public:
    Entity();
    Entity(Entity* origin);
    virtual ~Entity();

    Binario* getMyBinario();
    void setOriginTransform(Transform* newOriginTransform);
    Transform* getOriginTransform() const { return transformOrigin; }

    void setParentEntity(Entity* parent);
    Entity* getParentEntity() const { return parentEntity; }
    const std::vector<Entity*>& getChildEntities() const { return childEntities; }

    virtual void addComponent(Component* component) = 0;
    virtual void deleteComponent(Component* component) = 0;

    template <typename T>
    T* getComponent() {
        if (!components || components->isEmpty()) return nullptr;
        Position<Component*>* position = components->first();
        while (position != nullptr) {
            if (auto* component = dynamic_cast<T*>(position->getElement())) return component;
            position = (position != components->last()) ? components->next(position) : nullptr;
        }
        return nullptr;
    }

    virtual ListaDE<Component*>* getComponents() = 0;

protected:
    virtual void serializeGlobalAtributes() = 0;
    virtual void serializeLocalAtributes() = 0;
    virtual void serializeExternalAtributes() = 0;
    virtual void serializeTransformOrigin() = 0;
    virtual void deserializeGlobalAtributes() = 0;
    virtual void deserializeLocalAtributes() = 0;
    virtual void deserializeExternalAtributes() = 0;
    virtual void deserializeTransformOrigin() = 0;
    virtual void serializeEntityComponents() = 0;
    virtual void deserializeEntityComponents() = 0;
    virtual void serializeEntity() = 0;
    virtual void deserializeEntity() = 0;

public:
    // Devuelven false si el archivo binario no se pudo abrir (sin directorio,
    // sin permisos): quien llama (SceneSerializer) lo avisa en el log en vez de
    // dejar que cada write()/read() falle en silencio.
    virtual bool saveEntity(std::string filename) = 0;
    virtual bool loadEntity(std::string filename) = 0;
    virtual Transform* getGlobalTransform() = 0;
};

#endif
