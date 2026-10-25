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

#include "EditorMaterialGUI.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Material.h"
#include <imgui.h>

SettingsMaterial::SettingsMaterial(GameObject* gameObject,
                                   IPreviewMaterial* preview)
    : SettingsComponent(demangle(typeid(Material).name())), gameObject(gameObject),
      preview(preview) {}

SettingsMaterial::~SettingsMaterial() {}

void SettingsMaterial::showDataComponent() {
	Material* material = gameObject->getComponent<Material>();
	if (!material) return;

	ImGui::Text("===Material===");
	// La vista previa (esfera de muestra) tiene su propia clave: la del
	// material unico del objeto, con su id (asi el material de otro objeto no
	// pisa el de este, que se dibuja al seleccionar otro objeto).
	const std::string clave =
		"material:" + std::to_string(gameObject->getId());
	dibujarEditorMaterial(*material, presetAplicado, preview, clave);
}

Component* SettingsMaterial::getComponent() {
	return gameObject->getComponent<Material>();
}
