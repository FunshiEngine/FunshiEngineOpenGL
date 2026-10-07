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
#include "SettingsRigidBody.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../../../Scenes/EditorController.h"
#include <imgui.h>

SettingsRigidBody::SettingsRigidBody(GameObject* objeto) {
	myCollider = objeto->getComponent<RigidBody>();
}

void SettingsRigidBody::setEditor(EditorController* editor) {
	this->editor = editor;
}

void SettingsRigidBody::showDataComponent() {
	bool activo = myCollider->estaActivo();
	if (ImGui::Checkbox("Activo", &activo)) myCollider->setActivo(activo);
	float masa = myCollider->masa();
	if (ImGui::DragFloat("Masa", &masa, 0.1f, 0.0f, 100.0f)) {
		const bool eraEstatico = myCollider->masa() == 0.0f;
		if (myCollider->fijarMasa(masa) && editor &&
		    myCollider->getCollider() &&
		    myCollider->getCollider()->getOwner() &&
		    (eraEstatico != (masa == 0.0f))) {
			// Cruzar entre estatico y dinamico cambia la naturaleza del
			// cuerpo en Bullet: reconstruirlo via el editor lo re-registra.
			editor->refreshRigidBody(
			    myCollider->getCollider()->getOwner());
		}
	}
	bool usaGravedad = myCollider->usaGravedad();
	if (ImGui::Checkbox("Usa gravedad", &usaGravedad))
		myCollider->fijarUsoGravedad(usaGravedad);
	float escala = myCollider->escalaGravedad();
	if (ImGui::DragFloat("Escala gravedad", &escala, 0.05f, -5.0f, 5.0f))
		myCollider->fijarEscalaGravedad(escala);
	float friccion = myCollider->friccion();
	if (ImGui::DragFloat("Friccion", &friccion, 0.05f, 0.0f, 10.0f))
		myCollider->fijarFriccion(friccion);
	if (ImGui::TreeNode("Freeze posicion")) {
		bool x = myCollider->posicionCongelada(0);
		bool y = myCollider->posicionCongelada(1);
		bool z = myCollider->posicionCongelada(2);
		bool cambio = false;
		cambio |= ImGui::Checkbox("X", &x);
		ImGui::SameLine();
		cambio |= ImGui::Checkbox("Y", &y);
		ImGui::SameLine();
		cambio |= ImGui::Checkbox("Z", &z);
		if (cambio) myCollider->fijarFreezePosicion(x, y, z);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Freeze rotacion")) {
		bool x = myCollider->rotacionCongelada(0);
		bool y = myCollider->rotacionCongelada(1);
		bool z = myCollider->rotacionCongelada(2);
		bool cambio = false;
		cambio |= ImGui::Checkbox("X##rot", &x);
		ImGui::SameLine();
		cambio |= ImGui::Checkbox("Y##rot", &y);
		ImGui::SameLine();
		cambio |= ImGui::Checkbox("Z##rot", &z);
		if (cambio) myCollider->fijarFreezeRotacion(x, y, z);
		ImGui::TreePop();
	}
}

Component* SettingsRigidBody::getComponent() { return myCollider; }