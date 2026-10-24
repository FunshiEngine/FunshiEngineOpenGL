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
#ifndef IPHYSICS_BACKEND_H
#define IPHYSICS_BACKEND_H

#include <vector>

#include "ContactoFisico.h"

class RigidBody;

struct ResultadoRaycast {
    bool  impacto = false;
    float distancia = 0.0f;
    float puntoX = 0.0f, puntoY = 0.0f, puntoZ = 0.0f;
    float normalX = 0.0f, normalY = 1.0f, normalZ = 0.0f;
    Collider* collider = nullptr;
};

class IPhysicsBackend {
public:
    virtual ~IPhysicsBackend() = default;
    virtual void stepSimulation(float deltaTime) = 0;
    virtual void addRigidBody(RigidBody* body) = 0;
    virtual void removeRigidBody(RigidBody* body) = 0;
    // Sin comportamiento por defecto: cada backend declara que hace con los
    // contactos y con la gravedad global. Un backend que no implemente esto
    // no compila, en vez de heredar un valor silencioso.
    virtual std::vector<EventoContacto> tomarEventosContacto() = 0;
    // Gravedad global del mundo (el backend fija su valor inicial).
    virtual void fijarGravedad(float x, float y, float z) = 0;
    virtual void gravedad(float& x, float& y, float& z) const = 0;
    // Raycast / hitscan: dispara un rayo desde origen hacia direccion con
    // un alcance dado. Devuelve si hubo impacto, su distancia, punto y
    // normal, y el collider golpeado. Default: sin impacto.
    virtual ResultadoRaycast raycast(float ox, float oy, float oz,
                                     float dx, float dy, float dz,
                                     float alcance) = 0;
};

#endif
