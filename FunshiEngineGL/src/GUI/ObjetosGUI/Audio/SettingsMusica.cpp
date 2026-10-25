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
#include "SettingsMusica.h"

#include "../../../Audio/AudioEngine.h"
#include "../../../Audio/ExtensionesAudio.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Musica.h"

#include <imgui.h>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Nombre del archivo sin su carpeta: la lista muestra el nombre, no la ruta
// entera (que puede ser larga).
std::string nombreArchivo(const std::string& ruta) {
    const std::size_t separador = ruta.find_last_of("/\\");
    return separador == std::string::npos ? ruta : ruta.substr(separador + 1);
}

} // namespace

SettingsMusica::SettingsMusica(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(Musica).name())), gameObject(gameObject) {}

void SettingsMusica::showDataComponent() {
	Musica* musica = gameObject->getComponent<Musica>();
	if (!musica) return;

	ImGui::Text("===Musica===");

	// La lista de pistas: cada una es la ruta de un archivo (el usuario la
	// arrastro aqui). El orden es el de reproduccion.
	const std::vector<std::string> rutas = musica->getRutas();
	ImGui::Text("Pistas (%zu)", rutas.size());
	ImGui::Separator();

	ImGui::PushID(musica);
	for (std::size_t i = 0; i < rutas.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		ImGui::Text("%zu. %s", i + 1, nombreArchivo(rutas[i]).c_str());
		ImGui::SameLine();
		// Subir y bajar: el orden de la lista es el orden en que suenan.
		if (ImGui::SmallButton("^")) {
			if (i > 0) musica->moverRuta(i, i - 1);
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("v")) {
			if (i + 1 < rutas.size()) musica->moverRuta(i, i + 1);
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("x")) musica->quitarRuta(i);
		ImGui::PopID();
	}

	// Zona para anadir una pista arrastrandola desde el explorador.
	if (ImGui::Button("[Arrastra una pista para anadirla]")) {
		// El boton es el destino del arrastre; el click no hace nada.
	}
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload =
		        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
			const char* rutaSoltada = static_cast<const char*>(payload->Data);
			if (rutaSoltada && *rutaSoltada && esArchivoAudio(rutaSoltada)) {
				musica->anadirRuta(rutaSoltada);
			} else if (rutaSoltada && *rutaSoltada) {
				std::cerr << "[Musica] el archivo arrastrado no es un formato de "
				             "audio (wav/ogg/mp3/flac); pista sin anadir: "
				          << rutaSoltada << std::endl;
			}
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::PopID();

	float volume = musica->getVolume();
	if (ImGui::SliderFloat("Volumen", &volume, 0.f, 1.f))
		musica->setVolume(volume);

	float fundido = musica->getDuracionFundido();
	if (ImGui::SliderFloat("Fundido (s)", &fundido, 0.f, 10.f))
		musica->setDuracionFundido(fundido);

	bool repetir = musica->isRepetirLista();
	if (ImGui::Checkbox("Repetir la lista", &repetir))
		musica->setRepetirLista(repetir);

	bool autoPlay = musica->isReproduccionAutomatica();
	if (ImGui::Checkbox("Reproducir al activar", &autoPlay))
		musica->setReproduccionAutomatica(autoPlay);

	// La escena inyecta el motor; sin motor (inspector aislado) los botones se
	// muestran deshabilitados.
	ImGui::BeginDisabled(motor == nullptr || musica->getRutas().empty());
	if (ImGui::Button("Reproducir")) musica->reproducir();
	ImGui::SameLine();
	if (ImGui::Button("Detener")) musica->detener();
	ImGui::EndDisabled();
}
