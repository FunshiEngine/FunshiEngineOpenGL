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
#include "CameraComponent.h"

#include <cmath>
#include <cstring>

#include "../GameObject.h"
#include "../Componentes/Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kGradosARadianes = kPi / 180.f;

// Tope del pitch de la camara (mouse look): sin esto la camara se "da vuelta"
// al mirar hacia arriba/abajo y el vector "left" degenere (m_up fijo y
// m_left ortogonal a dir apuntando casi vertical) y la vista hace cosas raras.
constexpr float kPitchMaxGrados = 89.0f;

// Rota el vector "vec" (0,0,-1) o (0,1,0) segun angulo/eje del Transform;
// mismo helper que usa LightSystem para derivar la direccion de las luces.
void rotarVector(const float* vec, float anguloGrados, const float* eje,
                 float out[3]) {
    out[0] = vec[0]; out[1] = vec[1]; out[2] = vec[2];

    if (anguloGrados == 0.f) return;

    float k[3] = {eje[0], eje[1], eje[2]};
    const float norma = std::sqrt(k[0] * k[0] + k[1] * k[1] + k[2] * k[2]);
    if (norma < 1e-6f) return;

    k[0] /= norma; k[1] /= norma; k[2] /= norma;

    const float radianes = anguloGrados * kGradosARadianes;
    const float c = std::cos(radianes);
    const float s = std::sin(radianes);
    const float dot = vec[0] * k[0] + vec[1] * k[1] + vec[2] * k[2];
    const float cross[3] = {
        k[1] * vec[2] - k[2] * vec[1],
        k[2] * vec[0] - k[0] * vec[2],
        k[0] * vec[1] - k[1] * vec[0]};

    out[0] = vec[0] * c + cross[0] * s + k[0] * dot * (1.f - c);
    out[1] = vec[1] * c + cross[1] * s + k[1] * dot * (1.f - c);
    out[2] = vec[2] * c + cross[2] * s + k[2] * dot * (1.f - c);
}

} // namespace

CameraComponent::CameraComponent() {
    m_dir[0] = 0.f; m_dir[1] = 0.f; m_dir[2] = -1.f;
    m_left[0] = -1.f; m_left[1] = 0.f; m_left[2] = 0.f;
    m_up[0] = 0.f; m_up[1] = 1.f; m_up[2] = 0.f;
}

void CameraComponent::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    file->write(reinterpret_cast<const char*>(&fov), sizeof(float));
    file->write(reinterpret_cast<const char*>(&nearPlane), sizeof(float));
    file->write(reinterpret_cast<const char*>(&farPlane), sizeof(float));
    file->write(reinterpret_cast<const char*>(&speed), sizeof(float));
    file->write(reinterpret_cast<const char*>(&pintar), sizeof(bool));
}

void CameraComponent::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;
    file->read(reinterpret_cast<char*>(&fov), sizeof(float));
    file->read(reinterpret_cast<char*>(&nearPlane), sizeof(float));
    file->read(reinterpret_cast<char*>(&farPlane), sizeof(float));
    file->read(reinterpret_cast<char*>(&speed), sizeof(float));
    file->read(reinterpret_cast<char*>(&pintar), sizeof(bool));
}

void CameraComponent::saveComponent(std::ofstream* file) {
    serializeComponent(file);
}

void CameraComponent::loadComponent(std::ifstream* file) {
    deserializeComponent(file);
}

void CameraComponent::onLoaded(GameObject& owner) {
    setUp(&owner);
    sincronizarConTransform();
}

Transform* CameraComponent::getLocalTransform() const {
    if (!owner) return nullptr;
    return owner->getComponent<Transform>();
}

void CameraComponent::leerDesdeTransform() {
    Transform* transform = owner ? owner->getGlobalTransform() : nullptr;
    if (!transform) {
        m_pos[0] = 0.f; m_pos[1] = 0.f; m_pos[2] = 0.f;
        m_dir[0] = 0.f; m_dir[1] = 0.f; m_dir[2] = -1.f;
        m_up[0] = 0.f; m_up[1] = 1.f; m_up[2] = 0.f;
        yawX = 0.f; yawY = 0.f;
        m_left[0] = -1.f; m_left[1] = 0.f; m_left[2] = 0.f;
        return;
    }

    const float* t = transform->getTranslatef();
    m_pos[0] = t[0]; m_pos[1] = t[1]; m_pos[2] = t[2];

    const float adelante[3] = {0.f, 0.f, -1.f};
    const float arriba[3] = {0.f, 1.f, 0.f};
    rotarVector(adelante, transform->getRotatef()[0],
                &transform->getRotatef()[1], m_dir);
    rotarVector(arriba, transform->getRotatef()[0],
                &transform->getRotatef()[1], m_up);

    // Reconstruir yaw/pitch desde la direccion actual del Transform para que
    // la navegacion continue donde dejo el gizmo o el inspector. TODO se
    // maneja en GRADOS (el mouse y su tope usan grados); los atan2/asin de
    // abajo salen en radianes y se convierten.
    yawX = std::atan2(m_dir[0], -m_dir[2]) / kGradosARadianes;
    float yDir = m_dir[1] > 1.f ? 1.f : m_dir[1];
    yDir = yDir < -1.f ? -1.f : yDir;
    yawY = std::asin(-yDir) / kGradosARadianes;
    if (yawY > kPitchMaxGrados) yawY = kPitchMaxGrados;
    if (yawY < -kPitchMaxGrados) yawY = -kPitchMaxGrados;

    m_left[0] = m_up[1] * m_dir[2] - m_up[2] * m_dir[1];
    m_left[1] = m_up[2] * m_dir[0] - m_up[0] * m_dir[2];
    m_left[2] = m_up[0] * m_dir[1] - m_up[1] * m_dir[0];
    const float len = std::sqrt(m_left[0] * m_left[0] +
                                m_left[1] * m_left[1] +
                                m_left[2] * m_left[2]);
    if (len > 0.f) {
        m_left[0] /= len; m_left[1] /= len; m_left[2] /= len;
    }
}

// Escribe posicion y orientacion FPS (yaw/pitch) de vuelta al Transform local.
void CameraComponent::escribirATransform() {
    Transform* transform = getLocalTransform();
    if (!transform) return;

    const float* scale = transform->getScalef();
    const float axisY[3] = {0.f, 1.f, 0.f};
    const float axisX[3] = {1.f, 0.f, 0.f};

    glm::mat4 mat(1.0f);
    mat = glm::translate(mat, glm::vec3(m_pos[0], m_pos[1], m_pos[2]));
    // R = RY(-yawX) * RX(-yawY), misma convencion que la dir FPS:
    // dir = (+sin yawX cos yawY, -sin yawY, -cos yawX cos yawY).
    // yawX/yawY se conservan en grados; glm::rotate espera radianes.
    mat = glm::rotate(mat, -yawX * kGradosARadianes,
                      glm::vec3(axisY[0], axisY[1], axisY[2]));
    mat = glm::rotate(mat, -yawY * kGradosARadianes,
                      glm::vec3(axisX[0], axisX[1], axisX[2]));
    mat = glm::scale(mat, glm::vec3(scale[0], scale[1], scale[2]));

    const float* ptr = glm::value_ptr(mat);
    float arr[16];
    std::memcpy(arr, ptr, sizeof(float) * 16);
    decomposeMatrixToTransform(arr, transform);
}

void CameraComponent::calculardireccion() {
    const float radX = yawX * kGradosARadianes;
    const float radY = yawY * kGradosARadianes;
    m_dir[0] = std::sin(radX) * std::cos(radY);
    m_dir[1] = -std::sin(radY);
    m_dir[2] = -std::cos(radX) * std::cos(radY);
    float length = std::sqrt(m_dir[0] * m_dir[0] + m_dir[1] * m_dir[1] +
                             m_dir[2] * m_dir[2]);
    if (length > 0.f) {
        m_dir[0] /= length; m_dir[1] /= length; m_dir[2] /= length;
    }
    m_left[0] = m_up[1] * m_dir[2] - m_up[2] * m_dir[1];
    m_left[1] = m_up[2] * m_dir[0] - m_up[0] * m_dir[2];
    m_left[2] = m_up[0] * m_dir[1] - m_up[1] * m_dir[0];
    length = std::sqrt(m_left[0] * m_left[0] + m_left[1] * m_left[1] +
                       m_left[2] * m_left[2]);
    if (length > 0.f) {
        m_left[0] /= length; m_left[1] /= length; m_left[2] /= length;
    }
}

void CameraComponent::move(const float direction[3], float velocity) {
    m_pos[0] += direction[0] * velocity;
    m_pos[1] += direction[1] * velocity;
    m_pos[2] += direction[2] * velocity;
}

void CameraComponent::forward(float dt) {
    leerDesdeTransform(); move(m_dir, speed * dt); escribirATransform();
}
void CameraComponent::back(float dt) {
    leerDesdeTransform(); move(m_dir, -speed * dt); escribirATransform();
}
void CameraComponent::left(float dt) {
    leerDesdeTransform(); move(m_left, speed * dt); escribirATransform();
}
void CameraComponent::right(float dt) {
    leerDesdeTransform(); move(m_left, -speed * dt); escribirATransform();
}
void CameraComponent::up(float dt) {
    leerDesdeTransform(); move(m_up, speed * dt); escribirATransform();
}
void CameraComponent::down(float dt) {
    leerDesdeTransform(); move(m_up, -speed * dt); escribirATransform();
}

void CameraComponent::forwardRight(float dt) {
    leerDesdeTransform();
    const float v = speed * dt;
    m_pos[0] += (m_dir[0] - m_left[0]) * v;
    m_pos[1] += (m_dir[1] - m_left[1]) * v;
    m_pos[2] += (m_dir[2] - m_left[2]) * v;
    escribirATransform();
}
void CameraComponent::forwardLeft(float dt) {
    leerDesdeTransform();
    const float v = speed * dt;
    m_pos[0] += (m_dir[0] + m_left[0]) * v;
    m_pos[1] += (m_dir[1] + m_left[1]) * v;
    m_pos[2] += (m_dir[2] + m_left[2]) * v;
    escribirATransform();
}
void CameraComponent::backRight(float dt) {
    leerDesdeTransform();
    const float v = speed * dt;
    m_pos[0] += (-m_dir[0] - m_left[0]) * v;
    m_pos[1] += (-m_dir[1] - m_left[1]) * v;
    m_pos[2] += (-m_dir[2] - m_left[2]) * v;
    escribirATransform();
}
void CameraComponent::backLeft(float dt) {
    leerDesdeTransform();
    const float v = speed * dt;
    m_pos[0] += (-m_dir[0] + m_left[0]) * v;
    m_pos[1] += (-m_dir[1] + m_left[1]) * v;
    m_pos[2] += (-m_dir[2] + m_left[2]) * v;
    escribirATransform();
}

void CameraComponent::moverDireccion(const float direccion[3], float dt) {
    leerDesdeTransform();
    // direccion = [x: derecho, y: arriba, z: adelante]; "derecho" = -left.
    const float v = speed * dt;
    m_pos[0] += (-m_left[0] * direccion[0] + m_up[0] * direccion[1] +
                 m_dir[0] * direccion[2]) * v;
    m_pos[1] += (-m_left[1] * direccion[0] + m_up[1] * direccion[1] +
                 m_dir[1] * direccion[2]) * v;
    m_pos[2] += (-m_left[2] * direccion[0] + m_up[2] * direccion[1] +
                 m_dir[2] * direccion[2]) * v;
    escribirATransform();
}

void CameraComponent::updateYaw(float dYawX, float dYawY) {
    leerDesdeTransform();
    yawX += dYawX;
    yawY += dYawY;
    if (yawY > kPitchMaxGrados) yawY = kPitchMaxGrados;
    if (yawY < -kPitchMaxGrados) yawY = -kPitchMaxGrados;
    calculardireccion();
    escribirATransform();
}

float CameraComponent::orbitAround(const float* origen, float radio, float dYawX, float dYawY) {
    leerDesdeTransform();

    // Clamp radio a limites.
    if (radio < radioMin) radio = radioMin;
    if (radio > radioMax) radio = radioMax;

    // Orbita horizontal: solo yawX (rotacion en plano XZ).
    // La altura (Y) se mantiene fija; el radio se pasa como parametro
    // (ajustable con la rueda del mouse durante la orbita).
    yawX += dYawX;
    // yawY NO cambia: orbita puramente horizontal, sin pitch.
    calculardireccion();

    // Direccion horizontal pura (yawY=0) para posicionar en el circulo XZ.
    const float radX = yawX * kGradosARadianes;
    float horizDir[3] = {
        std::sin(radX),
        0.f,
        -std::cos(radX)
    };

    // Nueva posicion en el circulo horizontal (misma Y, radio constante).
    // Usamos origen - horizDir * radio para que la camara mire HACIA el origen.
    m_pos[0] = origen[0] - horizDir[0] * radio;
    // m_pos[1] se mantiene igual (altura fija)
    m_pos[2] = origen[2] - horizDir[2] * radio;

    escribirATransform();
    return radio;
}

void CameraComponent::getViewMatrix(float* outMatrix) const {
    const_cast<CameraComponent*>(this)->leerDesdeTransform();
    glm::vec3 eye(m_pos[0], m_pos[1], m_pos[2]);
    glm::vec3 center(m_pos[0] + m_dir[0], m_pos[1] + m_dir[1],
                     m_pos[2] + m_dir[2]);
    glm::vec3 up(m_up[0], m_up[1], m_up[2]);
    glm::mat4 v = glm::lookAt(eye, center, up);
    const float* ptr = glm::value_ptr(v);
    std::memcpy(outMatrix, ptr, sizeof(float) * 16);
}

void CameraComponent::getProjectionMatrix(float* outMatrix, float aspect) const {
    glm::mat4 p = glm::perspective(glm::radians(fov), aspect, nearPlane,
                                   farPlane);
    const float* ptr = glm::value_ptr(p);
    std::memcpy(outMatrix, ptr, sizeof(float) * 16);
}