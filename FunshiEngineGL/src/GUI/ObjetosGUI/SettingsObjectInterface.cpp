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
#include "Skybox/SettingsSkybox.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Light.h"
#include "../../Objetos/Componentes/Material.h"
#include "../../Objetos/Componentes/CameraComponent.h"
#include "../../Objetos/Componentes/AudioSource.h"
#include "../../Objetos/Componentes/InterfaceComponent.h"
#include "../../Objetos/Componentes/Grid.h"
#include "../../Objetos/Componentes/Transform.h"
#include "../../Objetos/Componentes/Color.h"
#include "../../Objetos/Componentes/Script.h"
#include "../../Objetos/Componentes/Model.h"
#include "../../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../../Objetos/Componentes/Colliders/CubeCollider.h"
#include "../../Objetos/Componentes/Colliders/MallaCollider.h"
#include "../../Objetos/Componentes/Skybox.h"
#include "../../Scenes/EditorController.h"
#include "../../Events/EventBus.h"
#include "../../Herramientas/TypeUtils.h"
#include <imgui.h>
#include <typeinfo>

SettingsObjectInterface::SettingsObjectInterface(GameObject* object,
                                                 bool stateGUI)
	: GeneralUserInterface("Settings", stateGUI, ImGuiWindowFlags_MenuBar) {
	this->object = object;
	listaDESettingsComponent = new ListaDE<SettingsComponent*>();
	if (object && !object->getComponents()->isEmpty()) {
		loadComponents();
	}
}

SettingsObjectInterface::~SettingsObjectInterface() {
    if (events && eventSubscription != 0) {
        events->unsubscribe(eventSubscription);
        eventSubscription = 0;
    }
    desvincular();
    delete listaDESettingsComponent;
}

void SettingsObjectInterface::setEditor(EditorController* editor) {
	this->editor = editor;
}

// Desvincula el inspector del objeto actual: libera los Settings* y deja
// object = nullptr. Se llama desde la suscripcion al bus y desde el destructor.
void SettingsObjectInterface::desvincular() {
    while (!listaDESettingsComponent->isEmpty()) {
        Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
        delete pos->getElement();
        listaDESettingsComponent->remove(pos);
    }
    object = nullptr;
}

void SettingsObjectInterface::setEventBus(EventBus* bus) {
    if (events == bus) return;
    if (events && eventSubscription != 0) {
        events->unsubscribe(eventSubscription);
        eventSubscription = 0;
    }
    events = bus;
    if (events) {
        eventSubscription = events->subscribe([this](const SceneEvent& event) {
            if (event.type == SceneEventType::ObjectDeleted) {
                if (event.object == object) {
                    desvincular();
                }
            } else if (event.type == SceneEventType::SceneCleared) {
                desvincular();
            } else if (event.type == SceneEventType::ComponentStructureChanged) {
                if (event.object == object && !iterandoComponentes) {
                    // Solo el alta/baja de un componente invalida paneles. Un
                    // cambio de propiedad no llega por este evento.
                    reconciliarComponentes();
                }
                // Si estamos iterando, el caller (contentGUI) se encarga del reload diferido
            }
        });
    }
}

bool SettingsObjectInterface::tieneSettingsPara(Component* componente) {
	if (!componente || listaDESettingsComponent->isEmpty()) return false;
	Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
	while (pos != nullptr) {
		SettingsComponent* s = pos->getElement();
		if (s != nullptr && s->getComponent() == componente) return true;
		pos = (pos != listaDESettingsComponent->last())
		          ? listaDESettingsComponent->next(pos)
		          : nullptr;
	}
	return false;
}

// Borra los Settings cuyo componente ya no pertenece al objeto inspeccionado.
// Al liberar el componente, getComponent() deja de encontrarlo y el panel queda
// huerfano (dibujaria memoria liberada si sobreviviera).
void SettingsObjectInterface::purgarSettingsHuerfanos() {
	if (!object || listaDESettingsComponent->isEmpty()) return;
	ListaDE<Component*>* componentes = object->getComponents();
	Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
	while (pos != nullptr) {
		Position<SettingsComponent*>* siguiente =
		    (pos != listaDESettingsComponent->last())
		        ? listaDESettingsComponent->next(pos)
		        : nullptr;
		SettingsComponent* s = pos->getElement();
		Component* c = s ? s->getComponent() : nullptr;
		bool vigente = false;
		if (c != nullptr && componentes != nullptr && !componentes->isEmpty()) {
			Position<Component*>* pc = componentes->first();
			while (pc != nullptr) {
				if (pc->getElement() == c) {
					vigente = true;
					break;
				}
				pc = (pc != componentes->last())
				         ? componentes->next(pc)
				         : nullptr;
			}
		}
		if (!vigente) {
			listaDESettingsComponent->remove(pos);
			delete s;
		}
		pos = siguiente;
	}
}

// Reconciliacion completa: reutiliza los paneles vigentes, borra los huerfanos
// y crea los que falten. Unico punto que incrementa el contador de
// reconstrucciones (un cambio estructural por vez).
void SettingsObjectInterface::reconciliarComponentes() {
	if (!object) return;
	++reconciliaciones;
	purgarSettingsHuerfanos();
	crearSettingsFaltantes();
}

void SettingsObjectInterface::loadComponents() {
	if (!object) return;
	purgarSettingsHuerfanos();
	crearSettingsFaltantes();
}

void SettingsObjectInterface::crearSettingsFaltantes() {
	const int creadosAntes = listaDESettingsComponent->tam();
	Transform* transformComponent = object->getComponent<Transform>();
	if (transformComponent != nullptr && !tieneSettingsPara(transformComponent)) {
		listaDESettingsComponent->addLast(
		    new SettingsTransform(transformComponent, object));
	}
	Color* colorComponent = object->getComponent<Color>();
	if (colorComponent != nullptr && !tieneSettingsPara(colorComponent)) {
		listaDESettingsComponent->addLast(new SettingsColor(object));
	}
	Material* materialComponent = object->getComponent<Material>();
	if (materialComponent != nullptr && !tieneSettingsPara(materialComponent)) {
		listaDESettingsComponent->addLast(new SettingsMaterial(object));
	}
	Light* lightComponent = object->getComponent<Light>();
	if (lightComponent != nullptr && !tieneSettingsPara(lightComponent)) {
		listaDESettingsComponent->addLast(new SettingsLight(object));
	}
	CameraComponent* cameraComponent = object->getComponent<CameraComponent>();
	if (cameraComponent != nullptr && !tieneSettingsPara(cameraComponent)) {
		listaDESettingsComponent->addLast(new SettingsCamera(object));
	}
	// SOPORTE PARA AMBOS COLLIDER
	Collider* collider = object->getComponent<EsfereCollider>();
	if (collider != nullptr && !tieneSettingsPara(collider)) {
		SettingsColliderEsfera* s = new SettingsColliderEsfera(object);
		s->setEditor(editor);
		listaDESettingsComponent->addLast(s);
	}
	collider = object->getComponent<CubeCollider>();
	if (collider != nullptr && !tieneSettingsPara(collider)) {
		SettingsColliderCubo* s = new SettingsColliderCubo(object);
		s->setEditor(editor);
		listaDESettingsComponent->addLast(s);
	}
	collider = object->getComponent<MallaCollider>();
	if (collider != nullptr && !tieneSettingsPara(collider)) {
		SettingsColliderMalla* s = new SettingsColliderMalla(object);
		s->setEditor(editor);
		listaDESettingsComponent->addLast(s);
	}
	RigidBody* rigidBody = object->getComponent<RigidBody>();
	if (rigidBody != nullptr && !tieneSettingsPara(rigidBody)) {
		listaDESettingsComponent->addLast(new SettingsRigidBody(object));
	}
	Script* script = object->getComponent<Script>();
	if (script != nullptr && !tieneSettingsPara(script)) {
		listaDESettingsComponent->addLast(new SettingsScript(object));
	}
	Model* model = object->getComponent<Model>();
	if (model != nullptr && !tieneSettingsPara(model)) {
		listaDESettingsComponent->addLast(new SettingsModel(object));
	}
	Grid* grid = object->getComponent<Grid>();
	if (grid != nullptr && !tieneSettingsPara(grid)) {
		listaDESettingsComponent->addLast(new SettingsGrid(object));
	}
	Skybox* skybox = object->getComponent<Skybox>();
	if (skybox != nullptr && !tieneSettingsPara(skybox)) {
		listaDESettingsComponent->addLast(new SettingsSkybox(object));
	}
	AudioSource* audioSource = object->getComponent<AudioSource>();
	if (audioSource != nullptr && !tieneSettingsPara(audioSource)) {
		SettingsAudioSource* settingsAudioSource = new SettingsAudioSource(object);
		settingsAudioSource->setAudioEngine(audioMotor);
		listaDESettingsComponent->addLast(settingsAudioSource);
	}
	InterfaceComponent* interfaceComp = object->getComponent<InterfaceComponent>();
	if (interfaceComp != nullptr && !tieneSettingsPara(interfaceComp)) {
		listaDESettingsComponent->addLast(new SettingsInterface(object));
	}
	creacionesSettings += static_cast<size_t>(
	    listaDESettingsComponent->tam() - creadosAntes);
}

GameObject* SettingsObjectInterface::getObjectInInspector() { return object; }

SettingsComponent* SettingsObjectInterface::settingsEnIndice(size_t indice) {
	if (listaDESettingsComponent->isEmpty()) return nullptr;
	size_t i = 0;
	Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
	while (pos != nullptr) {
		if (i == indice) return pos->getElement();
		++i;
		pos = (pos != listaDESettingsComponent->last())
		          ? listaDESettingsComponent->next(pos)
		          : nullptr;
	}
	return nullptr;
}

void SettingsObjectInterface::setTargetObject(GameObject* newObject) {
    if (object == newObject || newObject == nullptr) return;
    object = newObject;
    while (!listaDESettingsComponent->isEmpty()) {
        Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
        delete pos->getElement();
        listaDESettingsComponent->remove(pos);
    }
    loadComponents();
}

void SettingsObjectInterface::initGUI() {
	ImGui::Begin(getNameGui().c_str(), &stateGUI, getFlagGui());
	ImGui::PushID(object);
}

void SettingsObjectInterface::contentGUI() {
	// MOSTRAMOS COMPONENTES
	iterandoComponentes = true;
	componenteABorrar = nullptr;

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
					// Borrado diferido: encolar para procesar DESPUÉS de la iteracion
					componenteABorrar = comp;
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

	iterandoComponentes = false;

	// Procesar borrado diferido fuera de la iteracion
	if (componenteABorrar) {
		// El componente ya se libero: reconciliar purga el panel huerfano y
		// crea los que falten, sin descartar el estado del resto de paneles.
		componenteABorrar = nullptr;
		reconciliarComponentes();
	}

	// Menu contextual en area vacia del inspector: agregar componente
	if (ImGui::BeginPopupContextWindow(
	        "InspectorAddComponentPopup",
	        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
		if (object && editor) {
			Transform* transform = object->getComponent<Transform>();
			Collider* collider = object->getComponent<Collider>();

			if (ImGui::MenuItem("Transform")) {
				editor->addComponent(object, std::make_unique<Transform>());
			}
			if (ImGui::MenuItem("Color")) {
				editor->addComponent(object, std::make_unique<Color>());
			}
			if (ImGui::MenuItem("Material")) {
				editor->addComponent(object, std::make_unique<Material>());
			}
			if (ImGui::MenuItem("Light")) {
				editor->addComponent(object, std::make_unique<Light>());
			}
			if (ImGui::MenuItem("CameraComponent")) {
				editor->addComponent(object, std::make_unique<CameraComponent>());
			}
			if (transform) {
				if (ImGui::BeginMenu("Add Collider")) {
					if (ImGui::MenuItem("EsfereCollider")) {
						editor->addComponent(object, std::make_unique<EsfereCollider>(5.0f, transform, object));
					}
					if (ImGui::MenuItem("CubeCollider")) {
						editor->addComponent(object, std::make_unique<CubeCollider>(5.0f, transform, object));
					}
					if (ImGui::MenuItem("MallaCollider")) {
						editor->addComponent(object, std::make_unique<MallaCollider>(5.0f, transform, object));
					}
					ImGui::EndMenu();
				}
			}
			if (collider) {
				if (ImGui::MenuItem("RigidBody")) {
					editor->addComponent(object, std::make_unique<RigidBody>(collider, 1.0f));
				}
			}
			if (ImGui::MenuItem("Script")) {
				editor->addComponent(object, std::make_unique<Script>());
			}
			if (ImGui::MenuItem("Model")) {
				editor->addComponent(object, std::make_unique<Model>());
			}
			if (ImGui::MenuItem("Grid")) {
				editor->addComponent(object, std::make_unique<Grid>());
			}
			if (ImGui::MenuItem("Skybox")) {
				editor->addComponent(object, std::make_unique<Skybox>());
			}
			if (ImGui::MenuItem("AudioSource")) {
				editor->addComponent(object, std::make_unique<AudioSource>());
			}
			if (ImGui::MenuItem("InterfaceComponent")) {
				editor->addComponent(object, std::make_unique<InterfaceComponent>());
			}
		}
		ImGui::EndPopup();
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