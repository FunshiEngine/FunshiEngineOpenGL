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
#include "Componentes/CameraComponent.h"
#include "Componentes/Color.h"
#include "Componentes/RigidBody/RigidBody.h"
#include "Componentes/Script.h"
#include "TagRegistry.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <utility>

namespace {
constexpr std::uint32_t kTagMagic = 0x31474154;
constexpr std::uint32_t kTagVersion = 1;
constexpr std::size_t kTagMaxLength = 1024;
}

GameObject::GameObject(Entity* origin) : Entity(origin) {
    TagRegistry::registrar(tag_);
}

GameObject::GameObject() : Entity() {
    TagRegistry::registrar(tag_);
}

GameObject::~GameObject() = default;
















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

void GameObject::setTag(std::string tag) {
    if (tag.empty()) tag = "Untagged";
    tag_ = std::move(tag);
    TagRegistry::registrar(tag_);
}


void GameObject::update(float deltaTime) {

    if (RigidBody* body =
            getComponent<RigidBody>())
    {
        body->syncPhysicsToGameObject();
    }

    if (components && !components->isEmpty()) {
        Position<Component*>* position = components->first();
        while (position) {
            Component* componente = position->getElement();
            if (Script* script = dynamic_cast<Script*>(componente))
                script->actualizar(this, deltaTime);
            // Hook generico por frame (animacion y demas componentes con
            // estado): cada componente decide si hace algo.
            if (componente) componente->onUpdate(*this, deltaTime);
            position = position != components->last()
                           ? components->next(position)
                           : nullptr;
        }
    }
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

    const std::uint32_t magic = kTagMagic;
    const std::uint32_t version = kTagVersion;
    const std::uint32_t length = static_cast<std::uint32_t>(tag_.size());
    file->write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file->write(reinterpret_cast<const char*>(&version), sizeof(version));
    file->write(reinterpret_cast<const char*>(&length), sizeof(length));
    file->write(tag_.data(), static_cast<std::streamsize>(length));
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

    deserializeEntityComponents(*this);

    tag_ = "Untagged";
    if (file->peek() == std::char_traits<char>::eof()) {
        file->clear();
        return;
    }

    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t length = 0;
    file->read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file->read(reinterpret_cast<char*>(&version), sizeof(version));
    file->read(reinterpret_cast<char*>(&length), sizeof(length));
    if (!*file || magic != kTagMagic || version != kTagVersion ||
        length == 0 || length > kTagMaxLength) {
        std::cerr << "Metadatos de tag invalidos en el objeto\n";
        file->setstate(std::ios::failbit);
        return;
    }

    std::string tag(length, '\0');
    file->read(tag.data(), static_cast<std::streamsize>(length));
    if (!*file) {
        std::cerr << "Tag truncado en el objeto\n";
        return;
    }
    setTag(std::move(tag));
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
bool GameObject::saveEntity(std::string filename) {

    const std::string path =
        filename +
        "/ObjectN" +
        std::to_string(getId()) +
        ".db";

    myBinario =
        std::make_unique<Binario>(path);

    if (!myBinario->ofOpenBinary())
        return false;

    serializeEntity();

    myBinario->ofCloseBinary();
    return true;
}


bool GameObject::loadEntity(std::string filename) {

    const std::string path =
        filename +
        "/ObjectN" +
        std::to_string(getId()) +
        ".db";

    myBinario =
        std::make_unique<Binario>(path);

    if (!myBinario->ifOpenBinary())
        return false;

    deserializeEntity();

    myBinario->ifCloseBinary();
    return true;
}


