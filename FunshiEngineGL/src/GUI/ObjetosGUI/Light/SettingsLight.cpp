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
#include "SettingsLight.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Light.h"
#include <imgui.h>

SettingsLight::SettingsLight(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(Light).name())), gameObject(gameObject) {}

void SettingsLight::showDataComponent() {
	Light* light = gameObject->getComponent<Light>();
	if (!light) return;

	ImGui::Text("===Luz===");

	const char* tipos[] = {"Direccional", "Puntual", "Spot"};
	int tipoActual = static_cast<int>(light->getType());
	if (ImGui::Combo("Tipo", &tipoActual, tipos, IM_ARRAYSIZE(tipos))) {
		light->setType(static_cast<LightType>(tipoActual));
	}

	float amb[3] = {light->getAmbient()[0], light->getAmbient()[1],
	                light->getAmbient()[2]};
	if (ImGui::ColorEdit3("Ambient", amb))
		light->setAmbient(amb[0], amb[1], amb[2]);

	float diff[3] = {light->getDiffuse()[0], light->getDiffuse()[1],
	                 light->getDiffuse()[2]};
	if (ImGui::ColorEdit3("Diffuse", diff))
		light->setDiffuse(diff[0], diff[1], diff[2]);

	float spec[3] = {light->getSpecular()[0], light->getSpecular()[1],
	                 light->getSpecular()[2]};
	if (ImGui::ColorEdit3("Specular", spec))
		light->setSpecular(spec[0], spec[1], spec[2]);

	if (light->getType() != LightType::DIRECTIONAL) {
		float constant = light->getConstant();
		float linear = light->getLinear();
		float quadratic = light->getQuadratic();
		if (ImGui::SliderFloat("At. constante", &constant, 0.f, 2.f) ||
		    ImGui::SliderFloat("At. lineal", &linear, 0.f, 1.f) ||
		    ImGui::SliderFloat("At. cuadratica", &quadratic, 0.f, 1.f)) {
			light->setAttenuation(constant, linear, quadratic);
		}
	}

	if (light->getType() == LightType::SPOT) {
		float cutoff = light->getSpotCutOff();
		if (ImGui::SliderFloat("Corte del cono", &cutoff, 1.f, 90.f))
			light->setSpotCutOff(cutoff);
	}
}