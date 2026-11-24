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
#include "RigidBodyDetalle.h"

RigidBody::RigidBody(Collider* collider, float mass)
    : collider(collider), mass(mass) {
    pos[0] = pos[1] = pos[2] = 0.0f;
    // rot es un cuaternion x, y, z, w: sin collider queda la identidad (antes
    // (0, 0, 1, 0), un giro de 180 grados en Z que se serializaba asi).
    rot[0] = 0.0f;
    rot[1] = 0.0f;
    rot[2] = 0.0f;
    rot[3] = 1.0f;
    createRigidBody();
}

RigidBody::~RigidBody() = default;

void RigidBody::createRigidBody() {
    // Reentrante: descarta el cuerpo/anterior antes de recrear
    rigidBody.reset();
    motionState.reset();

    if (!collider) return;

    // 1. Obtener la posicion GLOBAL del collider (no la local)
    Transform globalTransform = collider->getGlobalTransform();
    float* tr = globalTransform.getTranslatef();
    pos[0] = tr[0];
    pos[1] = tr[1];
    pos[2] = tr[2];

    // 2. Copiar la rotacion: getRotatef devuelve punteros a arrays internos
    //    del objeto: [angulo, ejeX, ejeY, ejeZ].
    float* rotAxisAngle = globalTransform.getRotatef();
    const float anguloGrados = rotAxisAngle[0];
    btVector3 axis(rotAxisAngle[1], rotAxisAngle[2], rotAxisAngle[3]);

    // Convertir de angulo+eje a cuaternion
    if (axis.length2() == 0) {
        axis = btVector3(0, 1, 0); // eje por defecto si no hay rotacion
    }
    btQuaternion q;
    q.setRotation(axis.normalized(), anguloGrados * SIMD_RADS_PER_DEG);

    // Guardar cuaternion en rot[]
    rot[0] = q.x();
    rot[1] = q.y();
    rot[2] = q.z();
    rot[3] = q.w();

    // Shape prestada del collider (el collider es duenio y la mantiene viva)
    btCollisionShape* shape = collider->getCollisionShape();
    if (!shape) return;

    // Escalar la shape segun la escala global del collider (heredada del padre)
    float* scales = globalTransform.getScalef();
    shape->setLocalScaling(btVector3(scales[0], scales[1], scales[2]));

    // Configurar transform inicial del rigid body
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(pos[0], pos[1], pos[2]));
    startTransform.setRotation(q);

    // Calcular inercia (si mass != 0)
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.f) shape->calculateLocalInertia(mass, localInertia);

    // Crear motion state y rigid body
    motionState = std::make_unique<btDefaultMotionState>(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState.get(),
                                                    shape, localInertia);
    rigidBody = std::make_unique<btRigidBody>(rbInfo);
    // El backend de fisica resuelve el RigidBody de cada contacto por aca.
    rigidBody->setUserPointer(this);
    if (!activo) rigidBody->forceActivationState(DISABLE_SIMULATION);
    aplicarPropiedades();
}

void RigidBody::aplicarPropiedades() {
    if (!rigidBody) return;
    rigidBody->setFriction(friccionCuerpo);
    rigidBody->setLinearFactor(btVector3(freezePos[0] ? 0.0f : 1.0f,
                                         freezePos[1] ? 0.0f : 1.0f,
                                         freezePos[2] ? 0.0f : 1.0f));
    rigidBody->setAngularFactor(btVector3(freezeRot[0] ? 0.0f : 1.0f,
                                          freezeRot[1] ? 0.0f : 1.0f,
                                          freezeRot[2] ? 0.0f : 1.0f));
    // Sensor: sin respuesta de contacto (el cuerpo se atraviesa). La
    // generacion de manifolds no depende de este flag, asi que los eventos de
    // colision se siguen reportando en el stepSimulation.
    int collisionFlags = rigidBody->getCollisionFlags();
    if (sensor)
        collisionFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
    else
        collisionFlags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;
    rigidBody->setCollisionFlags(collisionFlags);
    if (!usarGravedad) {
        rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
        return;
    }
    rigidBody->setGravity(btVector3(gravedadMundo[0] * escalaGrav,
                                    gravedadMundo[1] * escalaGrav,
                                    gravedadMundo[2] * escalaGrav));
}

void RigidBody::detachCollider() {
    collider = nullptr;
    createRigidBody();
}

void RigidBody::syncPhysicsToGameObject() {
    if (!activo || !rigidBody || !collider) return;

    Transform* dadTransform = collider->getDadTransform();
    Transform* colliderTransform = collider->getTransform();
    if (!dadTransform) {
        // Sin objeto padre: escribir directo en el collider.
        btTransform trans;
        rigidBody->getMotionState()->getWorldTransform(trans);
        const btVector3 origin = trans.getOrigin();
        colliderTransform->setTranslatef(origin.x(), origin.y(), origin.z());
        return;
    }

    // 1. Pose del cuerpo (posicion + rotacion del GLOBAL del collider).
    btTransform trans;
    rigidBody->getMotionState()->getWorldTransform(trans);
    glm::vec3 bodyPos;
    glm::quat bodyRot;
    bulletWorldToMatrices(trans, bodyPos, bodyRot);

    // 2. Reconstruir el objeto dueño:
    //      ownerGlobal = bodyWorld x inv(colliderLocal)
    //    La version anterior restaba a ciegas origin - offset en espacio
    //    mundo: solo es valida sin rotacion/escala en la jerarquia. Con un
    //    offset rotado o un ancestro escalado/rotado, la posicion del padre
    //    "se escapa" (es lo que deja al cuerpo 'frenando encima del suelo').
    glm::mat4 bodyWorld(1.0f);
    glm::mat3 rotMat = glm::mat3_cast(bodyRot);
    bodyWorld[0] = glm::vec4(rotMat[0], 0.0f);
    bodyWorld[1] = glm::vec4(rotMat[1], 0.0f);
    bodyWorld[2] = glm::vec4(rotMat[2], 0.0f);
    bodyWorld[3] = glm::vec4(bodyPos, 1.0f);

    float localArr[16];
    buildMatrixFromTransform(colliderTransform, localArr);
    glm::mat4 ownerGlobal = bodyWorld * glm::inverse(glm::make_mat4(localArr));

    // 3. Extrapolar la pose del padre, pero cuidando que jamás se corrompa
    //    con no-finito (un glm::inverse de matriz singular da Inf/NaN).
    const float* ptr = glm::value_ptr(ownerGlobal);
    for (int i = 0; i < 16; ++i) {
        if (!std::isfinite(ptr[i])) return; // conserva el transform anterior
    }

    // 4. Escribir SOLO posicion+rotacion al padre: la escala del objeto la
    //    controla el editor y la fisica no debe tocarla. La rotacion del
    //    collider (offset local) no se toca: quedo absorbida en los pasos 2-3.
    float ownerArr[16];
    for (int i = 0; i < 16; ++i) ownerArr[i] = ptr[i];
    Transform resultado;
    decomposeMatrixToTransform(ownerArr, &resultado);
    float* t = resultado.getTranslatef();
    dadTransform->setTranslatef(t[0], t[1], t[2]);
    float* r = resultado.getRotatef();
    dadTransform->setRotatef(r[0], r[1], r[2], r[3]);

    // Guardar datos en los arrays (pose global del body, como en createRigidBody)
    pos[0] = bodyPos.x;
    pos[1] = bodyPos.y;
    pos[2] = bodyPos.z;
    rot[0] = bodyRot.x;
    rot[1] = bodyRot.y;
    rot[2] = bodyRot.z;
    rot[3] = bodyRot.w;
}

void RigidBody::syncGameObjectToPhysics(bool restablecerVelocidades) {
    if (!rigidBody || !collider) return;

    // 1. Transform global actual del collider (padre + local)
    Transform globalTransform = collider->getGlobalTransform();
    float* tr = globalTransform.getTranslatef();
    pos[0] = tr[0];
    pos[1] = tr[1];
    pos[2] = tr[2];

    // 2. Rotacion: angulo+eje -> cuaternion
    float* rotAxisAngle = globalTransform.getRotatef();
    const float anguloGrados = rotAxisAngle[0];
    btVector3 axis(rotAxisAngle[1], rotAxisAngle[2], rotAxisAngle[3]);
    if (axis.length2() == 0) {
        axis = btVector3(0, 1, 0);
    }
    btQuaternion q;
    q.setRotation(axis.normalized(), anguloGrados * SIMD_RADS_PER_DEG);
    rot[0] = q.x();
    rot[1] = q.y();
    rot[2] = q.z();
    rot[3] = q.w();

    // 3. Aplicar escala global a la shape
    float* scales = globalTransform.getScalef();
    rigidBody->getCollisionShape()->setLocalScaling(
        btVector3(scales[0], scales[1], scales[2]));

    // 4. Mover el cuerpo y su motion state al transform visual del editor
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(pos[0], pos[1], pos[2]));
    startTransform.setRotation(q);
    rigidBody->setWorldTransform(startTransform);
    rigidBody->getMotionState()->setWorldTransform(startTransform);

    if (restablecerVelocidades) {
        rigidBody->setLinearVelocity(btVector3(0, 0, 0));
        rigidBody->setAngularVelocity(btVector3(0, 0, 0));
    }
    if (activo)
        rigidBody->activate();
    else
        rigidBody->forceActivationState(DISABLE_SIMULATION);
}

bool RigidBody::fijarVelocidadHorizontal(float x, float z) {
    if (!rigidBody) return false;
    if (!activo || !std::isfinite(x) || !std::isfinite(z)) return true;
    const btVector3 velocidad = rigidBody->getLinearVelocity();
    rigidBody->setLinearVelocity(btVector3(x, velocidad.y(), z));
    rigidBody->activate();
    return true;
}

bool RigidBody::saltar(float velocidad) {
    if (!rigidBody || !activo || !enSuelo || !std::isfinite(velocidad))
        return false;
    btVector3 actual = rigidBody->getLinearVelocity();
    actual.setY(velocidad);
    rigidBody->setLinearVelocity(actual);
    rigidBody->activate();
    enSuelo = false;
    return true;
}

void RigidBody::setActivo(bool nuevoEstado) {
    if (activo == nuevoEstado) return;
    activo = nuevoEstado;
    if (!rigidBody) return;
    if (activo) {
        rigidBody->forceActivationState(ACTIVE_TAG);
        syncGameObjectToPhysics();
    } else {
        rigidBody->forceActivationState(DISABLE_SIMULATION);
        rigidBody->setLinearVelocity(btVector3(0, 0, 0));
        rigidBody->setAngularVelocity(btVector3(0, 0, 0));
    }
}

bool RigidBody::fijarMasa(float masa) {
    if (!std::isfinite(masa)) return false;
    if (masa < 0.0f) masa = 0.0f;
    mass = masa;
    if (rigidBody && collider) {
        btVector3 inercia(0, 0, 0);
        if (btCollisionShape* shape = collider->getCollisionShape()) {
            if (mass != 0.0f) shape->calculateLocalInertia(mass, inercia);
            rigidBody->setMassProps(mass, inercia);
            if (activo) rigidBody->activate();
        }
    }
    return true;
}

void RigidBody::fijarUsoGravedad(bool usar) {
    usarGravedad = usar;
    aplicarPropiedades();
}

bool RigidBody::fijarEscalaGravedad(float escala) {
    if (!std::isfinite(escala)) return false;
    escalaGrav = escala;
    aplicarPropiedades();
    return true;
}

bool RigidBody::fijarFriccion(float friccion) {
    if (!std::isfinite(friccion)) return false;
    friccionCuerpo = friccion < 0.0f ? 0.0f : friccion;
    aplicarPropiedades();
    return true;
}

bool RigidBody::posicionCongelada(int eje) const {
    if (eje < 0 || eje > 2) return false;
    return freezePos[eje];
}

void RigidBody::fijarFreezePosicion(bool x, bool y, bool z) {
    freezePos[0] = x;
    freezePos[1] = y;
    freezePos[2] = z;
    aplicarPropiedades();
}

bool RigidBody::rotacionCongelada(int eje) const {
    if (eje < 0 || eje > 2) return false;
    return freezeRot[eje];
}

void RigidBody::fijarFreezeRotacion(bool x, bool y, bool z) {
    freezeRot[0] = x;
    freezeRot[1] = y;
    freezeRot[2] = z;
    aplicarPropiedades();
}

void RigidBody::fijarRestricciones2D() {
    // Solo agrega las restricciones del plano: conserva los freeze que el
    // usuario haya fijado en el resto de los ejes.
    freezePos[2] = true;
    freezeRot[0] = true;
    freezeRot[1] = true;
    aplicarPropiedades();
}

void RigidBody::fijarSensor(bool esSensor) {
    sensor = esSensor;
    aplicarPropiedades();
}

void RigidBody::aplicarGravedadMundo(float x, float y, float z) {
    if (std::isfinite(x)) gravedadMundo[0] = x;
    if (std::isfinite(y)) gravedadMundo[1] = y;
    if (std::isfinite(z)) gravedadMundo[2] = z;
    aplicarPropiedades();
}
