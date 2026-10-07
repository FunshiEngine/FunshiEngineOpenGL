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
#ifndef RIGIDBODY_H
#define RIGIDBODY_H
#include <memory>
#include "../Colliders/Collider.h"

// El cuerpo fisico de bullet se usa por puntero; el include pesado de bullet
// solo lo necesita RigidBody.cpp.
class btRigidBody;
class btDefaultMotionState;

// RAII: es duenio de su btRigidBody y su btDefaultMotionState. createRigidBody
// es reentrante (descarta el cuerpo anterior antes de recrearlo).
class RigidBody : public Component {
private:
    std::unique_ptr<btRigidBody> rigidBody;
    std::unique_ptr<btDefaultMotionState> motionState;
    Collider* collider = nullptr;
    float mass = 1.0f;
    // Guarda la posicion y rotacion para serializar
    float pos[3];
    float rot[4]; // quaternion x,y,z,w
    bool activo = true;
    bool enSuelo = false;
    // Respuesta a la gravedad por cuerpo: si no usa, el mundo no lo acelera;
    // si usa, se acelera con la gravedad del mundo por este factor (1 = igual
    // que el mundo, 0 = flota, 2 = cae el doble). Se aplica con
    // btRigidBody::setGravity, sin reescribir la integracion de Bullet.
    bool usarGravedad = true;
    float escalaGrav = 1.0f;
    // Friccion de contacto (btCollisionObject::setFriction). 0 = desliza.
    float friccionCuerpo = 0.5f;
    // Freeze por ejes (estilo Unity): cada eje de posicion/rotacion bloqueado
    // se fija con linearFactor/angularFactor en cero.
    bool freezePos[3] = {false, false, false};
    bool freezeRot[3] = {false, false, false};
    // Ultima gravedad del mundo conocida (la empuja el backend al registrar
    // el cuerpo y al cambiar la gravedad global). No se serializa: el mundo
    // la repone al cargar la escena.
    float gravedadMundo[3] = {0.0f, -1.0f, 0.0f};

    void serializeComponent(std::ofstream* fileNamePathContentObject) override;
    void deserializeComponent(std::ifstream* fileNamePathContentObject) override;
    // Empuja friccion, freeze y gravedad a la instancia Bullet actual.
    void aplicarPropiedades();

public:
    RigidBody(Collider* collider, float mass);
    ~RigidBody();

    void createRigidBody();

    // Deja al cuerpo inerte sin su collider: null al puntero (todas las
    // sync/creacion ya lo tienen guardado) y destruye el btRigidBody y su
    // motion state (reentrante). Llamar SIEMPRE con el cuerpo ya fuera del
    // mundo de fisica (removeRigidBody): resetear un btRigidBody registrado
    // dejaria un puntero colgante en la broadphase.
    void detachCollider();

    // ALTERAR EL DAD TRANSFORM
    void syncPhysicsToGameObject();

    // Empuja el Transform del GameObject (collider+padre) hacia el cuerpo
    // fisico. Se usa cuando el gizmo mueve/rota/escala el objeto para que la
    // simulacion parta de la posicion visual del editor.
    void syncGameObjectToPhysics(bool restablecerVelocidades = true);
    bool estaActivo() const { return activo; }
    void setActivo(bool activo);
    bool fijarVelocidadHorizontal(float x, float z);
    bool saltar(float velocidad);
    void actualizarEstadoSuelo(bool apoyado) { enSuelo = apoyado; }
    // Masa (0 = estatico). Recalcula inercia sobre la shape actual.
    float masa() const { return mass; }
    bool fijarMasa(float masa);
    // Gravedad por cuerpo (ver campos): sin ella el cuerpo no cae.
    bool usaGravedad() const { return usarGravedad; }
    void fijarUsoGravedad(bool usar);
    float escalaGravedad() const { return escalaGrav; }
    bool fijarEscalaGravedad(float escala);
    // Friccion de contacto (0 = desliza).
    float friccion() const { return friccionCuerpo; }
    bool fijarFriccion(float friccion);
    // Freeze por ejes: eje 0 = X, 1 = Y, 2 = Z.
    bool posicionCongelada(int eje) const;
    void fijarFreezePosicion(bool x, bool y, bool z);
    bool rotacionCongelada(int eje) const;
    void fijarFreezeRotacion(bool x, bool y, bool z);
    // La llama el backend con la gravedad vigente del mundo (al registrar el
    // cuerpo y al cambiar la gravedad global). Cachea y aplica.
    void aplicarGravedadMundo(float x, float y, float z);

    void saveComponent(std::ofstream* fileNamePathContentObject) override;
    void loadComponent(std::ifstream* fileNamePathContentObject) override;

    btRigidBody* getRigidBody() { return rigidBody.get(); }
    Collider* getCollider() const { return collider; }
};
#endif