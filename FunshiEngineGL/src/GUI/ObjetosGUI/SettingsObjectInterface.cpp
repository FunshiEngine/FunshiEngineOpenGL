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
#include "SettingsObjectInterface.h"

#include "SettingsComponent.h"
#include "Transform/SettingsTransform.h"
#include "Color/SettingsColor.h"
#include "Colliders/SettingsColliderEsfera.h"
#include "Colliders/SettingsColliderCubo.h"
#include "Colliders/SettingsColliderMalla.h"
#include "RigidBody/SettingsRigidBody.h"
#include "Script/SettingsScript.h"
#include "Model/SettingsModel.h"
#include "Material/SettingsMaterial.h"
#include "Light/SettingsLight.h"
#include "Camera/SettingsCamera.h"
#include "Audio/SettingsAudioSource.h"
#include "Interface/SettingsInterface.h"
#include "Grid/SettingsGrid.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Light.h"
#include "../../Objetos/Componentes/Material.h"
#include "../../Objetos/Componentes/CameraComponent.h"
#include "../../Objetos/Componentes/AudioSource.h"
#include "../../Objetos/Componentes/InterfaceComponent.h"
#include "../../Objetos/Componentes/Grid.h"
#include "../../Scenes/EditorController.h"
#include "../../Herramientas/TypeUtils.h"
#include <imgui.h>
#include <typeinfo>

SettingsObjectInterface::SettingsObjectInterface(GameObject* object,
                                                 bool stateGUI)
	: GeneralUserInterface("Settings", stateGUI, ImGuiWindowFlags_MenuBar) {
	this->object = object;
	momentaneantID = object ? object->getId() : 0;
	listaDESettingsComponent = new ListaDE<SettingsComponent*>();
	if (!object->getComponents()->isEmpty()) {
		loadComponents();
	}
}

SettingsObjectInterface::~SettingsObjectInterface() {
	while (!listaDESettingsComponent->isEmpty()) {
		Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
		delete pos->getElement();
		listaDESettingsComponent->remove(pos);
	}
	delete listaDESettingsComponent;
}

void SettingsObjectInterface::setEditor(EditorController* editor) {
	this->editor = editor;
}

void SettingsObjectInterface::loadComponents() {
	Transform* transformComponent = object->getComponent<Transform>();
	if (transformComponent != nullptr) {
		listaDESettingsComponent->addLast(
		    new SettingsTransform(transformComponent, object));
	}
	Color* colorComponent = object->getComponent<Color>();
	if (colorComponent != nullptr) {
		listaDESettingsComponent->addLast(new SettingsColor(object));
	}
	Material* materialComponent = object->getComponent<Material>();
	if (materialComponent != nullptr) {
		listaDESettingsComponent->addLast(new SettingsMaterial(object));
	}
	Light* lightComponent = object->getComponent<Light>();
	if (lightComponent != nullptr) {
		listaDESettingsComponent->addLast(new SettingsLight(object));
	}
	CameraComponent* cameraComponent = object->getComponent<CameraComponent>();
	if (cameraComponent != nullptr) {
		listaDESettingsComponent->addLast(new SettingsCamera(object));
	}
	// SOPORTE PARA AMBOS COLLIDER
	Collider* collider = object->getComponent<EsfereCollider>();
	if (collider != nullptr) {
		SettingsColliderEsfera* s = new SettingsColliderEsfera(object);
		s->setEditor(editor);
		listaDESettingsComponent->addLast(s);
	}
	collider = object->getComponent<CubeCollider>();
	if (collider != nullptr) {
		SettingsColliderCubo* s = new SettingsColliderCubo(object);
		s->setEditor(editor);
		listaDESettingsComponent->addLast(s);
	}
	collider = object->getComponent<MallaCollider>();
	if (collider != nullptr) {
		SettingsColliderMalla* s = new SettingsColliderMalla(object);
		s->setEditor(editor);
		listaDESettingsComponent->addLast(s);
	}
	RigidBody* rigidBody = object->getComponent<RigidBody>();
	if (rigidBody != nullptr) {
		listaDESettingsComponent->addLast(new SettingsRigidBody(object));
	}
	Script* script = object->getComponent<Script>();
	if (script != nullptr) {
		listaDESettingsComponent->addLast(new SettingsScript(object));
	}
	Model* model = object->getComponent<Model>();
	if (model != nullptr) {
		listaDESettingsComponent->addLast(new SettingsModel(object));
	}
	Grid* grid = object->getComponent<Grid>();
	if (grid != nullptr) {
		listaDESettingsComponent->addLast(new SettingsGrid(object));
	}
	AudioSource* audioSource = object->getComponent<AudioSource>();
	if (audioSource != nullptr) {
		SettingsAudioSource* settingsAudioSource = new SettingsAudioSource(object);
		settingsAudioSource->setAudioEngine(audioMotor);
		listaDESettingsComponent->addLast(settingsAudioSource);
	}
	InterfaceComponent* interfaceComp = object->getComponent<InterfaceComponent>();
	if (interfaceComp != nullptr) {
		listaDESettingsComponent->addLast(new SettingsInterface(object));
	}
}

GameObject* SettingsObjectInterface::getObjectInInspector() { return object; }

void SettingsObjectInterface::setTargetObject(GameObject* newObject) {
    if (object == newObject || newObject == nullptr) return;
    object = newObject;
    while (!listaDESettingsComponent->isEmpty()) {
        Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
        delete pos->getElement();
        listaDESettingsComponent->remove(pos);
    }
    momentaneantID = object->getId();
    loadComponents();
}

void SettingsObjectInterface::initGUI() {
	ImGui::Begin(getNameGui().c_str(), &stateGUI, getFlagGui());
	ImGui::PushID(object);
}

void SettingsObjectInterface::contentGUI() {
	ImGui::InputInt("Id", &momentaneantID);
	if (ImGui::Button("Confirmar")) {
		object->setId(momentaneantID);
		momentaneantID = 0;
	}

	ImGui::InputText("Nombre", object->inputName, IM_ARRAYSIZE(object->inputName));

	ImGui::Separator();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
	if (ImGui::Button("Eliminar Objeto", ImVec2(-1, 0)) && editor) {
		GameObject* target = object;
		editor->deleteGameObject(target);
		stateGUI = false;
		ImGui::PopStyleColor();
		return;
	}
	ImGui::PopStyleColor();

	if (ImGui::BeginPopupContextWindow("AddComponentPopup",
	                                   ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("Agregar Transform") &&
		    object->getComponent<Transform>() == nullptr) {
			object->addComponent(new Transform());
			listaDESettingsComponent->addLast(
			    new SettingsTransform(object->getComponent<Transform>(),
			                          object));
		}
		if (ImGui::MenuItem("Agregar Color") &&
		    object->getComponent<Color>() == nullptr) {
			object->addComponent(new Color());
			listaDESettingsComponent->addLast(new SettingsColor(object));
		}
		if (ImGui::MenuItem("Agregar Material") &&
		    object->getComponent<Material>() == nullptr) {
			object->addComponent(new Material());
			listaDESettingsComponent->addLast(new SettingsMaterial(object));
		}
		if (ImGui::MenuItem("Agregar Luz") &&
		    object->getComponent<Light>() == nullptr) {
			object->addComponent(new Light());
			listaDESettingsComponent->addLast(new SettingsLight(object));
		}
		if (ImGui::MenuItem("Agregar Camara") &&
		    object->getComponent<CameraComponent>() == nullptr) {
			object->addComponent(new CameraComponent());
			listaDESettingsComponent->addLast(new SettingsCamera(object));
		}
		if (ImGui::TreeNodeEx("Agregar Collider",
		                      ImGuiTreeNodeFlags_OpenOnArrow |
		                          ImGuiTreeNodeFlags_SpanAvailWidth)) {
			if (ImGui::Selectable("EsfereCollider")) {
				if (object->getComponent<Collider>() == nullptr &&
				    object->getComponent<Transform>() != nullptr) {
					object->addComponent(
					    new EsfereCollider(5.0f,
					                       object->getComponent<Transform>(),
					                       object));
					listaDESettingsComponent->addLast(
					    new SettingsColliderEsfera(object));
				}
			}
			if (ImGui::Selectable("CubeCollider")) {
				if (object->getComponent<Collider>() == nullptr &&
				    object->getComponent<Transform>() != nullptr) {
					object->addComponent(
					    new CubeCollider(5.0f,
					                     object->getComponent<Transform>(),
					                     object));
					listaDESettingsComponent->addLast(
					    new SettingsColliderCubo(object));
				}
			}
			if (ImGui::Selectable("MallaCollider")) {
				if (object->getComponent<Collider>() == nullptr &&
				    object->getComponent<Transform>() != nullptr) {
					object->addComponent(
					    new MallaCollider(
					        5.0f, object->getComponent<Transform>(), object));
					listaDESettingsComponent->addLast(
					    new SettingsColliderMalla(object));
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::MenuItem("RigidBody")) {
			if (object->getComponent<RigidBody>() == nullptr &&
			    object->getComponent<Collider>() != nullptr) {
				RigidBody* rb =
				    new RigidBody(object->getComponent<Collider>(), 1.0f);
				if (editor) {
					// El controller agrega el componente y lo registra en la
					// fisica de forma centralizada.
					editor->addComponent(object,
					                     std::unique_ptr<RigidBody>(rb));
				} else {
					object->addComponent(rb);
				}
				listaDESettingsComponent->addLast(new SettingsRigidBody(object));
			}
		}
		if (ImGui::MenuItem("Script")) {
			if (object->getComponent<Script>() == nullptr) {
				object->addComponent(new Script());
				listaDESettingsComponent->addLast(new SettingsScript(object));
			}
		}
		if (ImGui::MenuItem("Model")) {
			if (object->getComponent<Model>() == nullptr) {
				object->addComponent(new Model());
				listaDESettingsComponent->addLast(new SettingsModel(object));
			}
		}
		if (ImGui::MenuItem("Agregar Fuente de audio")) {
			if (object->getComponent<AudioSource>() == nullptr) {
				object->addComponent(new AudioSource());
				SettingsAudioSource* settingsAudioSource =
				    new SettingsAudioSource(object);
				settingsAudioSource->setAudioEngine(audioMotor);
				listaDESettingsComponent->addLast(settingsAudioSource);
			}
		}
		if (ImGui::MenuItem("Grilla") &&
		    object->getComponent<Grid>() == nullptr &&
		    object->getComponent<Transform>() != nullptr) {
			object->addComponent(new Grid());
			listaDESettingsComponent->addLast(new SettingsGrid(object));
		}
		if (ImGui::MenuItem("Agregar Interfaz") &&
		    object->getComponent<InterfaceComponent>() == nullptr) {
			object->addComponent(new InterfaceComponent());
			listaDESettingsComponent->addLast(new SettingsInterface(object));
		}
		ImGui::EndPopup();
	}

	// MOSTRAMOS COMPONENTES
	if (!listaDESettingsComponent->isEmpty()) {
		Position<SettingsComponent*>* position =
		    listaDESettingsComponent->first();

		while (position != nullptr && position->getElement() != nullptr) {
			SettingsComponent* comp = position->getElement();
			ImGui::PushID(comp);

			// Usamos demangle para obtener un nombre legible
			std::string compName = demangle(typeid(*comp).name());

			bool open = ImGui::CollapsingHeader(
			    compName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

			if (ImGui::BeginDragDropSource(
			        ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
				ImGui::SetDragDropPayload("COMPONENT_DRAG", &comp,
				                          sizeof(SettingsComponent*));
				ImGui::Text("%s", compName.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload =
				        ImGui::AcceptDragDropPayload("COMPONENT_DRAG")) {
					SettingsComponent* draggedComp =
					    *(SettingsComponent**)payload->Data;
					auto posA =
					    listaDESettingsComponent->whatElementPosition(
					        draggedComp);
					auto posB = listaDESettingsComponent->whatElementPosition(
					    comp);
					if (posA && posB && posA != posB) {
						listaDESettingsComponent->swapPositions(posA, posB);
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextItem("DeleteComponent",
			                                 ImGuiPopupFlags_MouseButtonRight)) {
				if (ImGui::MenuItem("Eliminar Componente")) {
					Component* target = comp->getComponent();
					if (editor) {
						// Centralizado: des-registra de la fisica ANTES de
						// liberar el componente (evita punteros colgantes).
						editor->removeComponent(object, target);
					} else {
						object->deleteComponent(target);
					}
					listaDESettingsComponent->remove(position);
					delete comp;
					ImGui::EndPopup();
					ImGui::PopID();
					break;
				}
				ImGui::EndPopup();
			}

			if (open) {
				comp->showDataComponent();
			}

			ImGui::PopID();
			position = (position != listaDESettingsComponent->last())
			               ? listaDESettingsComponent->next(position)
			               : nullptr;
		}
	}
}

void SettingsObjectInterface::endGUI() {
	ImGui::PopID();
	ImGui::End();
}

void SettingsObjectInterface::printGUI() {
	if (stateGUI) {
		initGUI();
		contentGUI();
		endGUI();
	}
}