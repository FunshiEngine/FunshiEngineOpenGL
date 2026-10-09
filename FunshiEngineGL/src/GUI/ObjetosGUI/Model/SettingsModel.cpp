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
#include "SettingsModel.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Model.h"
#include "../../../Assets/AssimpMeshLoader.h"
#include <imgui.h>
#include <iostream>
#include <string>

SettingsModel::SettingsModel(GameObject* objeto)
    : SettingsComponent(demangle(typeid(Model).name())),
      myModel(objeto ? objeto->getComponent<Model>() : nullptr) {}

void SettingsModel::showDataComponent() {
	// 1. Obtener texto a mostrar
	const std::string& path = myModel->getPath();
	const char* displayText = path.empty() ? "[Arrastra modelo]" : path.c_str();

	// 2. Calcular ancho exacto del texto + padding
	float textWidth = ImGui::CalcTextSize(displayText).x;
	float buttonWidth = textWidth + ImGui::GetStyle().FramePadding.x * 2;

	// 4. Boton ajustado al texto (pegado a izquierda)
	ImGui::SetNextItemWidth(buttonWidth);
	if (ImGui::Button(displayText)) {
		// Accion opcional al click
	}

	// 5. Drag & Drop
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload =
		        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
			const char* path = (const char*)payload->Data;
			// Se filtra aqui y no en Model::setPath: el componente guarda
			// cualquier cadena porque todas las rutas se serializan, y el
			// formato solo importa al elegir el archivo que se va a cargar.
			if (AssimpMeshLoader::puedeLeerFormato(path)) {
				myModel->setPath(path);
			} else {
				std::cerr << "[Model] archivo no es un formato 3D que Assimp "
				             "pueda importar; ruta sin asignar: "
				          << path << std::endl;
			}
		}
		ImGui::EndDragDropTarget();
	}
}