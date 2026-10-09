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
#include "SettingsColliderEsfera.h"

#include "../Transform/SettingsTransform.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../../../Scenes/EditorController.h"
#include <imgui.h>

SettingsColliderEsfera::SettingsColliderEsfera(GameObject* objeto)
    : SettingsComponent(demangle(typeid(EsfereCollider).name())),
      myCollider(objeto ? objeto->getComponent<EsfereCollider>() : nullptr),
      newRadio(myCollider ? myCollider->getRadio() : 0.0f) {
	this->settingsTransform =
	    new SettingsTransform(myCollider ? myCollider->getTransform() : nullptr, objeto);
}

SettingsColliderEsfera::~SettingsColliderEsfera() { delete settingsTransform; }

void SettingsColliderEsfera::setEditor(EditorController* editor) {
	this->editor = editor;
}

void SettingsColliderEsfera::showDataComponent() {
	bool visible = myCollider->estaVisibleEnEscena();
	if (ImGui::Checkbox("Visible en escena", &visible))
		myCollider->setVisibleEnEscena(visible);
	ImGui::InputFloat("Radio", &newRadio);
	if (ImGui::Button("Confirmar")) {
		if (newRadio > 0 && newRadio != myCollider->getRadio()) {
			myCollider->setRadio(newRadio);
			// La shape de Bullet se cachea con el radio viejo: sin
			// reconstruirla, la fisica sigue chocando con la esfera del radio
			// INICIAL (p.ej. 5) y el cuerpo en el mundo seria recreado sin la
			// shape nueva. refreshRigidBody invalida la shape, saca el cuerpo
			// viejo del mundo, lo recrea y re-registra.
			if (editor && myCollider->getOwner())
				editor->refreshRigidBody(myCollider->getOwner());
			else
				myCollider->invalidateCollisionShape();
		}
	}
	settingsTransform->showDataComponent();
}