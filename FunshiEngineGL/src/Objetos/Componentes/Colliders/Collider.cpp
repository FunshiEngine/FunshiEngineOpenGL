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
#include "Collider.h"

#include <cmath>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../../Objetos/GameObject.h"
#include "../../../Rendering/LineBatch.h"

void Collider::serializeComponent(std::ofstream* fileNamePathContentObject) {
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&radio),
                                     sizeof(float));
    transformOfDadObject->saveComponent(fileNamePathContentObject);
    myTransform->saveComponent(fileNamePathContentObject);
}

void Collider::deserializeComponent(std::ifstream* fileNamePathContentObject) {
    fileNamePathContentObject->read(reinterpret_cast<char*>(&radio),
                                    sizeof(float));
    transformOfDadObject->loadComponent(fileNamePathContentObject);
    myTransform->loadComponent(fileNamePathContentObject);
    // el flag de gizmo no se serializa: al cargar una escena el offset queda
    // dormido de nuevo (mismo estado por defecto que al crearlo).
    myTransform->gizmoHabilitado = false;
}

Collider::Collider(float radio, Transform* transformOfDadObject,
                   GameObject* owner)
    : radio(radio),
      transformOfDadObject(transformOfDadObject),
      myTransform(std::make_unique<Transform>()),
      owner(owner) {
    // El gizmo del offset del collider arranca DORMIDO: si estuviera activo
    // por defecto, al seleccionar un objeto con collider el gizmo editaria el
    // offset y no el transform del objeto (molesto, se activaria a cada rato).
    // El usuario lo enciende con el checkbox "Gizmo activo" del transform del
    // collider en los settings.
    myTransform->gizmoHabilitado = false;
}

Collider::~Collider() = default;

LineBatch& Collider::obtenerWireBatch() {
    if (!wireBatch_) {
        wireBatch_.reset(new LineBatch());
    }
    return *wireBatch_;
}

void Collider::saveComponent(std::ofstream* fileNamePathContentObject) {
    serializeComponent(fileNamePathContentObject);
}

void Collider::loadComponent(std::ifstream* fileNamePathContentObject) {
    deserializeComponent(fileNamePathContentObject);
}

void Collider::setRadio(float radio) {
    if (radio > 0) {
        this->radio = radio;
    }
}

float Collider::getRadio() { return radio; }
Transform Collider::getGlobalTransform() const {
    // Composicion real con matrices: global = owner->getGlobalTransform()
    // (incluye jerarquia, rotacion y escala de ancestros) x myTransform.
    if (owner) {
        Transform* ownerGlobal = owner->getGlobalTransform();
        if (ownerGlobal) {
            float parentMat[16], localMat[16], globalMat[16];
            buildMatrixFromTransform(ownerGlobal, parentMat);
            buildMatrixFromTransform(myTransform.get(), localMat);

            glm::mat4 mParent = glm::make_mat4(parentMat);
            glm::mat4 mLocal = glm::make_mat4(localMat);
            glm::mat4 mGlobal = mParent * mLocal;
            const float* ptr = glm::value_ptr(mGlobal);
            for (int i = 0; i < 16; ++i) globalMat[i] = ptr[i];

            Transform resultado;
            decomposeMatrixToTransform(globalMat, &resultado);
            return resultado;
        }
    }

    // Fallback legacy (sin owner): MISMA composicion de matrices usando el
    // transform del padre. La version vieja sumaba posiciones a ciegas (dx =
    // myPos + dadPos); eso ignora la MAGNITUD del offset cuando el padre esta
    // rotado o escalado, y la coordenada global del collider se corre respecto
    // de donde tendria que colisionar (el objeto 'flota'). Con la matriz, la
    // rotacion y escala del padre se aplican de verdad sobre el offset local.
    if (transformOfDadObject) {
        float parentMat[16], localMat[16], globalMat[16];
        buildMatrixFromTransform(transformOfDadObject, parentMat);
        buildMatrixFromTransform(myTransform.get(), localMat);

        glm::mat4 mGlobal = glm::make_mat4(parentMat) * glm::make_mat4(localMat);
        const float* ptr = glm::value_ptr(mGlobal);
        for (int i = 0; i < 16; ++i) globalMat[i] = ptr[i];

        Transform resultado;
        decomposeMatrixToTransform(globalMat, &resultado);
        return resultado;
    }

    // Ultimo recurso: copia del local (mas que nada por seguridad).
    Transform resultado;
    float* myPos = myTransform->getTranslatef();
    resultado.setTranslatef(myPos[0], myPos[1], myPos[2]);
    float* myRots = myTransform->getRotatef();
    resultado.setRotatef(myRots[0], myRots[1], myRots[2], myRots[3]);
    float* myScales = myTransform->getScalef();
    resultado.setScalef(myScales[0], myScales[1], myScales[2]);
    return resultado;
}

btCollisionShape* Collider::getCollisionShape() {
    if (!collisionShape) {
        collisionShape = createCollisionShape();
        // Respaldo defensivo: nunca devolver nullptr a RigidBody/Bullet.
        if (!collisionShape) {
            collisionShape = std::make_unique<btSphereShape>(radio);
        }
    }
    return collisionShape.get();
}

void Collider::invalidateCollisionShape() {
    collisionShape.reset();
}

bool Collider::isCollision(Collider* other) {
    if (!other) return false;

    Transform myT = this->getGlobalTransform();
    Transform otherT = other->getGlobalTransform();

    float* myPos = myT.getTranslatef();
    float* otherPos = otherT.getTranslatef();

    float dxmyTransform = myPos[0];
    float dymyTransform = myPos[1];
    float dzmyTransform = myPos[2];

    float dxotherTransform = otherPos[0];
    float dyotherTransform = otherPos[1];
    float dzotherTransform = otherPos[2];

    float distanciaSinProcesar =
        (dxmyTransform - dxotherTransform) *
            (dxmyTransform - dxotherTransform) +
        (dymyTransform - dyotherTransform) *
            (dymyTransform - dyotherTransform) +
        (dzmyTransform - dzotherTransform) * (dzmyTransform - dzotherTransform);
    float distancia = sqrt(distanciaSinProcesar);
    return distancia < getRadio() + other->getRadio();
}
