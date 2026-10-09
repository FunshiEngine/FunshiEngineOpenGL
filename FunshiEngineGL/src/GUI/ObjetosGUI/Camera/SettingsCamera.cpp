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
#include "SettingsCamera.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/CameraComponent.h"
#include <imgui.h>

SettingsCamera::SettingsCamera(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(CameraComponent).name())), gameObject(gameObject) {}

void SettingsCamera::showDataComponent() {
	CameraComponent* camera = gameObject->getComponent<CameraComponent>();
	if (!camera) return;

	ImGui::Text("===Camara===");

	float fov = camera->getFov();
	if (ImGui::SliderFloat("FOV", &fov, 1.f, 120.f))
		camera->setFov(fov);

	float nearPlane = camera->getNearPlane();
	if (ImGui::SliderFloat("Near", &nearPlane, 0.01f, 10.f))
		camera->setNearPlane(nearPlane);

	float farPlane = camera->getFarPlane();
	if (ImGui::SliderFloat("Far", &farPlane, 10.f, 5000.f))
		camera->setFarPlane(farPlane);

	float speed = camera->getSpeed();
	if (ImGui::SliderFloat("Velocidad", &speed, 0.5f, 30.f))
		camera->setSpeed(speed);

	bool pintar = camera->getPintar();
	if (ImGui::Checkbox("Vista previa", &pintar))
		camera->setPintar(pintar);
}