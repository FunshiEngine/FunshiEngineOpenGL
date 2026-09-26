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
#include "SettingsAudioSource.h"

#include "../../../Audio/AudioEngine.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/AudioSource.h"

#include <imgui.h>
#include <cstring>

SettingsAudioSource::SettingsAudioSource(GameObject* gameObject)
    : gameObject(gameObject) {}

void SettingsAudioSource::showDataComponent() {
	AudioSource* source = gameObject->getComponent<AudioSource>();
	if (!source) return;

	ImGui::Text("===Fuente de audio===");

	// Nombre del clip (texto editable): debe coincidir con un clip de la
	// carpeta Sonidos/ del proyecto. Si no existe, reproducir devuelve -1 y no
	// suena (tolerante; el clip puede agregarse despues).
	char buffer[512] = {};
	std::strncpy(buffer, source->getClip().c_str(), sizeof(buffer) - 1);
	if (ImGui::InputText("Clip", buffer, sizeof(buffer))) {
		source->setClip(buffer);
	}

	float volume = source->getVolume();
	if (ImGui::SliderFloat("Volumen", &volume, 0.f, 1.f))
		source->setVolume(volume);

	bool loop = source->isLoop();
	if (ImGui::Checkbox("Bucle", &loop))
		source->setLoop(loop);

	bool autoPlay = source->isReproduccionAutomatica();
	if (ImGui::Checkbox("Reproducir al activar", &autoPlay))
		source->setReproduccionAutomatica(autoPlay);

	// La escena inyecta el motor; sin motor (inspector aislado) los botones se
	// muestran deshabilitados.
	ImGui::BeginDisabled(motor == nullptr);
	if (ImGui::Button(source->isReproduciendo() ? "Detener" : "Reproducir")) {
		if (source->isReproduciendo()) source->detener();
		else source->reproducir();
	}
	ImGui::SameLine();
	if (ImGui::Button("Detener todo"))
		if (motor) motor->detenerTodo();
	ImGui::EndDisabled();
}

Component* SettingsAudioSource::getComponent() {
	return gameObject->getComponent<AudioSource>();
}