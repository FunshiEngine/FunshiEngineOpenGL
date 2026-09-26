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
#ifndef CAMERACOMPONENT_H
#define CAMERACOMPONENT_H

#include "Component.h"

class GameObject;
class Transform;

// Camara como Component (mismo patron que Light): se adjunta a un GameObject,
// normalmente un objeto vacio con su Transform. La posicion/orientacion de la
// vista se deriva del Transform global del duenio; la navegacion (WASD + mouse
// FPS) mantiene el estado interno y lo sincroniza de vuelta al Transform, de
// modo que el gizmo y el inspector pueden editar la camara como a cualquier
// otro objeto.
class CameraComponent : public Component {
private:
    GameObject* owner = nullptr;
    float m_pos[3] = {0.f, 0.f, 0.f};
    float m_dir[3] = {0.f, 0.f, -1.f};
    float m_left[3] = {-1.f, 0.f, 0.f};
    float m_up[3] = {0.f, 1.f, 0.f};
    float speed = 5.0f;
    float yawX = 0.0f;
    float yawY = 0.0f;
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    bool pintar = false;

    Transform* getLocalTransform() const;
    void leerDesdeTransform();
    void calculardireccion();
    void move(const float direction[3], float velocity);

public:
    void escribirATransform();
    void sincronizarConTransform() { leerDesdeTransform(); }

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    CameraComponent();

    void setUp(GameObject* objeto) { owner = objeto; }

    void saveComponent(std::ofstream* file) override;
    void loadComponent(std::ifstream* file) override;

    // Post-carga: vincula el dueno y deriva el estado de la vista local desde
    // su Transform global. Reemplaza el despacho por nombre de tipo que hacia
    // deserializeEntityComponents ("CameraComponent"/"Camera").
    void onLoaded(GameObject& owner) override;

    // Fuerza lectura de posicion/direccion desde el Transform del duenio.
    void refreshFromTransform() { leerDesdeTransform(); }

    // Vista actual segun el Transform del duenio (nunca cachead) : el gizmo,
    // el picking y el render comparten la misma fuente de verdad.
    void getViewMatrix(float* outMatrix) const;
    void getProjectionMatrix(float* outMatrix, float aspect) const;

    // Navegacion FPS (misma API que el viejo Camera).
    void forward(float dt);
    void back(float dt);
    void left(float dt);
    void right(float dt);
    void up(float dt);
    void down(float dt);
    void forwardRight(float dt);
    void forwardLeft(float dt);
    void backRight(float dt);
    void backLeft(float dt);
    // Movimiento por vector unitario combinado (WASD en diagonal): el caller
    // pasa el vector [derecha, arriba, adelante] ya normalizado y esto aplica
    // speed*dt a cada eje. Reemplaza el encadenamiento de forward/left que
    // hacian las callbacks antes (la diagonal salia a sqrt(2) y con jitter).
    void moverDireccion(const float direccion[3], float dt);
    void updateYaw(float dYawX, float dYawY);

    // Orbita: la camara gira alrededor de un punto origen manteniendo la
    // distancia (radio) y mirando hacia ese punto. El caller pasa el delta
    // de yaw/pitch, el origen y el radio (ajustable con rueda del mouse);
    // se recalcula posicion y orientacion. Devuelve el radio efectivo usado
    // (clampado a [radioMin, radioMax]).
    float orbitAround(const float* origen, float radio, float dYawX, float dYawY);

    // Limites de radio para orbita.
    static constexpr float radioMin = 3.0f;
    static constexpr float radioMax = 50.0f;

    const float* getPosition() const { return m_pos; }
    void setPosition(const float pos[3]) { m_pos[0] = pos[0]; m_pos[1] = pos[1]; m_pos[2] = pos[2]; }
    const float* getDirection() const { return m_dir; }

    float getYawX() const { return yawX; }

    float getFov() const { return fov; }
    void setFov(float v) { fov = v; }
    float getNearPlane() const { return nearPlane; }
    void setNearPlane(float v) { nearPlane = v; }
    float getFarPlane() const { return farPlane; }
    void setFarPlane(float v) { farPlane = v; }
    float getSpeed() const { return speed; }
    void setSpeed(float v) { speed = v; }

    // Vista previa viva: el motor pinta la escena desde esta camara a una
    // textura y la muestra en una ventana ImGui (Fase 2).
    bool getPintar() const { return pintar; }
    void setPintar(bool v) { pintar = v; }
};
#endif