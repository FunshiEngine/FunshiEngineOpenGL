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
#include "GameObject.h"
#include "Componentes/ComponentFactory.h"
#include "Componentes/CameraComponent.h"

#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <typeinfo>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

GameObject::GameObject(Entity* origin) : Entity(origin) {}

GameObject::GameObject() : Entity() {}

GameObject::~GameObject() = default;


void GameObject::addComponent(Component* component) {
    addComponent(std::unique_ptr<Component>(component));
}


void GameObject::addComponent(
    std::unique_ptr<Component> component)
{
    if (!component || components->isElement(component.get()))
        return;

    Component* rawComponent = component.get();

    rawComponent->settingsObjectComponent = true;

    componentOwners.emplace_back(
        std::move(component)
    );

    components->addLast(rawComponent);
}


void GameObject::deleteComponent(Component* component) {

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


std::unique_ptr<Component> GameObject::extractComponent(Component* component) {
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


Component* GameObject::getComponentByName(const std::string& typeName) {
    if (!components || components->isEmpty()) return nullptr;
    Position<Component*>* position = components->first();
    while (position != nullptr) {
        Component* comp = position->getElement();
        if (comp) {
            std::string name = demangle(typeid(*comp).name());
            if (name == typeName) return comp;
        }
        position = (position != components->last())
                       ? components->next(position)
                       : nullptr;
    }
    return nullptr;
}


bool GameObject::hasComponent(const std::string& typeName) {
    return getComponentByName(typeName) != nullptr;
}


ListaDE<Component*>* GameObject::getComponents() {
    return components.get();
}


int GameObject::compareTo(GameObject* other) {

    Transform* mine =
        getComponent<Transform>();

    Transform* theirs =
        other
            ? other->getComponent<Transform>()
            : nullptr;

    if (!mine || !theirs)
        return 0;

    float* p =
        mine->getTranslatef();

    float* q =
        theirs->getTranslatef();

    const float a =
        p[0] * p[0] +
        p[1] * p[1] +
        p[2] * p[2];

    const float b =
        q[0] * q[0] +
        q[1] * q[1] +
        q[2] * q[2];

    return
        (a < b)
            ? -1
            : (a > b)
                ? 1
                : 0;
}


float GameObject::distanciaA(GameObject* other) {

    Transform* mine =
        getComponent<Transform>();

    Transform* theirs =
        other
            ? other->getComponent<Transform>()
            : nullptr;

    if (!mine || !theirs)
        return FLT_MAX;

    float* p =
        mine->getTranslatef();

    float* q =
        theirs->getTranslatef();

    const float dx =
        p[0] - q[0];

    const float dy =
        p[1] - q[1];

    const float dz =
        p[2] - q[2];

    return std::sqrt(
        dx * dx +
        dy * dy +
        dz * dz
    );
}


void GameObject::setId(int value) {
    id = value;
}


void GameObject::setState(bool value) {
    state = value;
}


void GameObject::setTam(int value) {
    tam = value;
}


void GameObject::setColor(color value) {

    if (Color* component =
            getComponent<Color>())
    {
        component->setColor(value);
    }
}


const float* GameObject::getColor(
    float*& /*arr*/,
    int /*size*/)
{
    return getComponent<Color>()->getColor();
}


Color* GameObject::getColor() {
    return getComponent<Color>();
}


int GameObject::getId() {
    return id;
}


bool GameObject::getState() {
    return state;
}


int GameObject::getTam() {
    return tam;
}


void GameObject::update(float deltaTime) {

    if (RigidBody* body =
            getComponent<RigidBody>())
    {
        body->syncPhysicsToGameObject();
    }

    if (Script* script = getComponent<Script>()) {
        script->actualizar(this, deltaTime);
    }
}


void GameObject::serializeGlobalAtributes() {

    serializeExternalAtributes();

    serializeLocalAtributes();
}


void GameObject::serializeLocalAtributes() {

    auto* file =
        myBinario->getOfBinariFile();

    file->write(
        reinterpret_cast<const char*>(&state),
        sizeof(bool)
    );

    file->write(
        reinterpret_cast<const char*>(&id),
        sizeof(int)
    );

    file->write(
        reinterpret_cast<const char*>(&tam),
        sizeof(int)
    );

    file->write(
        inputName,
        sizeof(inputName)
    );

    serializeEntityComponents();
}


void GameObject::serializeExternalAtributes() {
    serializeTransformOrigin();
}


void GameObject::serializeTransformOrigin() {

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


void GameObject::deserializeGlobalAtributes() {

    deserializeExternalAtributes();

    deserializeLocalAtributes();
}


void GameObject::deserializeLocalAtributes() {

    auto* file =
        myBinario->getIfBinariFile();

    file->read(
        reinterpret_cast<char*>(&state),
        sizeof(bool)
    );

    file->read(
        reinterpret_cast<char*>(&id),
        sizeof(int)
    );

    file->read(
        reinterpret_cast<char*>(&tam),
        sizeof(int)
    );

    file->read(
        inputName,
        sizeof(inputName)
    );

    deserializeEntityComponents();
}


void GameObject::deserializeExternalAtributes() {
    deserializeTransformOrigin();
}


void GameObject::deserializeTransformOrigin() {

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


void GameObject::serializeEntityComponents() {

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
            demangle(typeid(*component).name());

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


void GameObject::deserializeEntityComponents() {

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
                *this
            );

        if (component != nullptr) {

            component->loadComponent(file);

            // Post-carga polimorfica: cada componente aplica su estado al dueno
            // (p. ej. CameraComponent vincula su dueno y deriva la vista del
            // Transform; Color refleja su valor en auxColor para el inspector).
            // Sin dispatch manual por nombre de tipo aqui.
            component->onLoaded(*this);

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


void GameObject::serializeEntity() {
    serializeGlobalAtributes();
}


void GameObject::deserializeEntity() {
    deserializeGlobalAtributes();
}


/*
 * ================================================================
 * GUARDADO DEL BINARIO DEL GAMEOBJECT
 * ================================================================
 *
 * Esta función YA NO modifica BBDDObjetos.txt.
 *
 * Su única responsabilidad es generar:
 *
 *      filename/ObjectN#.db
 *
 * La estructura de la escena y BBDDObjetos.txt es responsabilidad
 * exclusiva de SceneSerializer.
 */
void GameObject::saveEntity(std::string filename) {

    const std::string path =
        filename +
        "/ObjectN" +
        std::to_string(getId()) +
        ".db";

    myBinario =
        std::make_unique<Binario>(path);

    myBinario->ofOpenBinary();

    serializeEntity();

    myBinario->ofCloseBinary();
}


void GameObject::loadEntity(std::string filename) {

    const std::string path =
        filename +
        "/ObjectN" +
        std::to_string(getId()) +
        ".db";

    myBinario =
        std::make_unique<Binario>(path);

    myBinario->ifOpenBinary();

    deserializeEntity();

    myBinario->ifCloseBinary();
}


Transform* GameObject::getGlobalTransform() {
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
