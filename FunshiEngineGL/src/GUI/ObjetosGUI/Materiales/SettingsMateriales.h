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
#ifndef SETTINGSMATERIALES_H
#define SETTINGSMATERIALES_H

#include <string>
#include <vector>

#include "../SettingsComponent.h"

class EditorController;
class GameObject;
class IPreviewMaterial;

// Panel de los materiales por region: una entrada por region de la malla (la
// cara de un cubo, la parte de un personaje) con su material y sus texturas.
class SettingsMateriales : public SettingsComponent {
private:
	GameObject* gameObject;
	EditorController* editor = nullptr;
	// Vista previa de material (la esfera de muestra). Puede ser nullptr.
	IPreviewMaterial* preview = nullptr;
	// Preset aplicado por region (indice en la lista de regiones).
	std::vector<int> presetsAplicados_;
	// Nombre de la region nueva que se esta anadiendo.
	std::string nombreNuevaRegion_;

public:
	SettingsMateriales(GameObject* gameObject, EditorController* editor,
	                   IPreviewMaterial* preview = nullptr);
	~SettingsMateriales();

	virtual void showDataComponent() override;
	virtual Component* getComponent() override;
};

#endif // SETTINGSMATERIALES_H
