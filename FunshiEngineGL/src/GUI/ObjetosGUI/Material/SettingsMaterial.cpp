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
#include "SettingsMaterial.h"

#include <cstring>

#include "../../../Herramientas/MaterialPresets.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Material.h"
#include <imgui.h>

SettingsMaterial::SettingsMaterial(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(Material).name())), gameObject(gameObject) {}

void SettingsMaterial::showDataComponent() {
	Material* material = gameObject->getComponent<Material>();
	if (!material) return;

	ImGui::Text("===Material===");

	// Presets de MaterialPresets como DATOS aplicados al material actual.
	static const char* const presets[] = {"Personalizado", "Bronce", "Cobre",
	                                      "Cromo", "Laton", "Jade", "Luz"};
	int combo = presetAplicado + 1;
	if (ImGui::Combo("Preset", &combo, presets,
	                 (int)(sizeof(presets) / sizeof(presets[0])))) {
		switch (combo) {
			case 1: MaterialPresets::bronze(*material); break;
			case 2: MaterialPresets::copper(*material); break;
			case 3: MaterialPresets::chrome(*material); break;
			case 4: MaterialPresets::brass(*material); break;
			case 5: MaterialPresets::jade(*material); break;
			case 6: MaterialPresets::luz(*material); break;
			default: break; // Personalizado
		}
		presetAplicado = combo - 1;
	}

	// Toda edicion manual marca el material como Personalizado.
	float amb[3] = {material->getAmbient()[0], material->getAmbient()[1],
	                material->getAmbient()[2]};
	if (ImGui::ColorEdit3("Ambient", amb)) {
		material->setAmbient(amb[0], amb[1], amb[2]);
		presetAplicado = -1;
	}

	float diff[3] = {material->getDiffuse()[0], material->getDiffuse()[1],
	                 material->getDiffuse()[2]};
	if (ImGui::ColorEdit3("Diffuse", diff)) {
		material->setDiffuse(diff[0], diff[1], diff[2]);
		presetAplicado = -1;
	}

	float spec[3] = {material->getSpecular()[0], material->getSpecular()[1],
	                 material->getSpecular()[2]};
	if (ImGui::ColorEdit3("Specular", spec)) {
		material->setSpecular(spec[0], spec[1], spec[2]);
		presetAplicado = -1;
	}

	float emiss[3] = {material->getEmission()[0], material->getEmission()[1],
	                  material->getEmission()[2]};
	if (ImGui::ColorEdit3("Emission", emiss)) {
		material->setEmission(emiss[0], emiss[1], emiss[2]);
		presetAplicado = -1;
	}

	float shin = material->getShininess();
	if (ImGui::SliderFloat("Shininess", &shin, 0.f, 128.f)) {
		material->setShininess(shin);
		presetAplicado = -1;
	}

	// Texturas (descriptores): paths a las imagenes compartidas por el
	// TextureManager de la escena. El renderer moderno resuelve cada slot; el
	// inmediato las ignora. Vacio = slot sin textura.
	auto campoTextura = [&](const char* label, const std::string& actual,
	                        void (Material::*setter)(const std::string&)) {
		char buf[1024];
		std::strncpy(buf, actual.c_str(), sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
		if (ImGui::InputText(label, buf, sizeof(buf)))
			(material->*setter)(buf);
	};
	ImGui::Separator();
	ImGui::Text("Texturas (vacias = sin textura):");
	campoTextura("Difusa", material->getDiffuseMapPath(),
	             &Material::setDiffuseMapPath);
	campoTextura("Especular", material->getSpecularMapPath(),
	             &Material::setSpecularMapPath);
	campoTextura("Normal", material->getNormalMapPath(),
	             &Material::setNormalMapPath);
	campoTextura("Emision", material->getEmissionMapPath(),
	             &Material::setEmissionMapPath);
}