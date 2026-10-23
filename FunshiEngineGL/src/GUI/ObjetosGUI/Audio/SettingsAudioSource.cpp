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
#include "../../../Audio/ExtensionesAudio.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/AudioSource.h"

#include <imgui.h>
#include <iostream>
#include <string>

SettingsAudioSource::SettingsAudioSource(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(AudioSource).name())), gameObject(gameObject) {}

void SettingsAudioSource::showDataComponent() {
	AudioSource* source = gameObject->getComponent<AudioSource>();
	if (!source) return;

	ImGui::Text("===Fuente de audio===");

	// El archivo se asigna arrastrandolo desde el explorador de archivos
	// (igual que los scripts): el componente guarda la ruta y el motor
	// reproduce ese archivo. No hay carpeta ni catalogo: el archivo puede estar
	// en cualquier parte dentro del proyecto.
	const std::string& ruta = source->getRutaClip();
	std::string etiqueta = "[Arrastra el audio]";
	if (!ruta.empty()) {
		const std::size_t separador = ruta.find_last_of("/\\");
		etiqueta = (separador != std::string::npos) ? ruta.substr(separador + 1)
		                                            : ruta;
	}

	ImGui::PushID(source);
	if (ImGui::Button(etiqueta.c_str())) {
		// El boton es el destino del arrastre; el click no hace nada.
	}
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload =
		        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
			const char* rutaSoltada = static_cast<const char*>(payload->Data);
			// Se filtra aqui y no en el componente: el componente guarda
			// cualquier cadena porque todas las rutas se serializan, y el
			// formato solo importa al elegir el archivo que se va a reproducir.
			if (rutaSoltada && *rutaSoltada && esArchivoAudio(rutaSoltada)) {
				source->setRutaClip(rutaSoltada);
			} else if (rutaSoltada && *rutaSoltada) {
				std::cerr << "[Audio] el archivo arrastrado no es un formato de "
				             "audio (wav/ogg/mp3/flac); ruta sin asignar: "
				          << rutaSoltada << std::endl;
			}
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::PopID();

	if (!source->getRutaClip().empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
		ImGui::TextUnformatted(source->getRutaClip().c_str());
		ImGui::PopStyleColor();
	}

	float volume = source->getVolume();
	if (ImGui::SliderFloat("Volumen", &volume, 0.f, 1.f))
		source->setVolume(volume);

	bool espacial = source->isEspacial();
	ImGui::Checkbox("Espacial (3D)", &espacial);
	source->setEspacial(espacial);
	ImGui::SameLine();
	ImGui::TextDisabled("cuando esta desactivado suena global (2D)");

	bool loop = source->isLoop();
	if (ImGui::Checkbox("Bucle", &loop))
		source->setLoop(loop);

	bool autoPlay = source->isReproduccionAutomatica();
	if (ImGui::Checkbox("Reproducir al activar", &autoPlay))
		source->setReproduccionAutomatica(autoPlay);

	// La escena inyecta el motor; sin motor (inspector aislado) los botones se
	// muestran deshabilitados. Si el componente aun no tiene motor (modo
	// editor, antes de entrar en play), se comparte el motor del inspector
	// para que Reproducir/Detener funcionen sin tener que entrar en play.
	if (!source->getMotor() && motor)
		source->setMotor(motor);

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
