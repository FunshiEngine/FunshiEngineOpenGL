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
#include "../../Objetos/PrefabLibrary.h"
#include "../../Objetos/Prefab.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/TagRegistry.h"
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
#include "../../Objetos/SimpleObject.h"
#include "../../Objetos/Componentes/Skybox.h"
#include "../../Scenes/EditorController.h"
#include "../../Comandos/AgregarComponenteComando.h"
#include "../../Comandos/QuitarComponenteComando.h"
#include "../../Comandos/GestorComandos.h"
#include "../../Events/EventBus.h"
#include "../../Herramientas/TypeUtils.h"
#include <imgui.h>
#include <typeinfo>
#include <algorithm>
#include <cstring>

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
	// Si ya hay objeto inspeccionado, reconciliar para crear panel Prefab si falta.
	if (object && editor) {
		crearSettingsFaltantes();
	}
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
    tagBufferOwner_ = nullptr;
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
// Usa el nombre de tipo almacenado en SettingsComponent (estable en construccion)
// para evitar desreferenciar punteros colgantes.
void SettingsObjectInterface::purgarSettingsHuerfanos() {
	if (!object || listaDESettingsComponent->isEmpty()) return;
	ListaDE<Component*>* componentes = object->getComponents();
	if (!componentes || componentes->isEmpty()) {
		// Si no hay componentes, todos los settings son huerfanos
		while (!listaDESettingsComponent->isEmpty()) {
			Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
			delete pos->getElement();
			listaDESettingsComponent->remove(pos);
		}
		return;
	}

	Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
	while (pos != nullptr) {
		Position<SettingsComponent*>* siguiente =
		    (pos != listaDESettingsComponent->last())
		        ? listaDESettingsComponent->next(pos)
		        : nullptr;
		SettingsComponent* s = pos->getElement();
		if (!s) {
			listaDESettingsComponent->remove(pos);
			pos = siguiente;
			continue;
		}

		// Verificar si existe un componente de este tipo en el objeto
		const std::string& tipoEsperado = s->getTipoComponente();
		bool vigente = false;
		if (!tipoEsperado.empty()) {
			Position<Component*>* pc = componentes->first();
			while (pc != nullptr) {
				Component* c = pc->getElement();
				if (c) {
					std::string tipoActual = demangle(typeid(*c).name());
					if (tipoActual == tipoEsperado) {
						vigente = true;
						break;
					}
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
		SettingsRigidBody* s = new SettingsRigidBody(object);
		s->setEditor(editor);
		listaDESettingsComponent->addLast(s);
	}
	ListaDE<Component*>* componentes = object->getComponents();
	if (componentes && !componentes->isEmpty()) {
		Position<Component*>* posicion = componentes->first();
		while (posicion) {
			if (Script* script =
			        dynamic_cast<Script*>(posicion->getElement())) {
				if (!tieneSettingsPara(script))
					listaDESettingsComponent->addLast(
					    new SettingsScript(script));
			}
			posicion = posicion != componentes->last()
			               ? componentes->next(posicion)
			               : nullptr;
		}
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
    tagBufferOwner_ = nullptr;
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
    if (!object) return;

    if (tagBufferOwner_ != object) {
        tagBuffer_.fill('\0');
        const std::string& tag = object->getTag();
        std::memcpy(tagBuffer_.data(), tag.data(),
                    std::min(tag.size(), tagBuffer_.size() - 1));
        tagBufferOwner_ = object;
    }

    ImGui::TextUnformatted("Tag");
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::InputText("##TagObject", tagBuffer_.data(),
                         tagBuffer_.size(),
                         ImGuiInputTextFlags_EnterReturnsTrue))
        object->setTag(tagBuffer_.data());
    ImGui::SameLine();
    const std::string tagActual = object->getTag();
    if (ImGui::BeginCombo("##TagsRegistrados", tagActual.c_str())) {
        for (const std::string& tag : TagRegistry::registrados()) {
            const bool seleccionado = tag == tagActual;
            if (ImGui::Selectable(tag.c_str(), seleccionado)) {
                object->setTag(tag);
                tagBuffer_.fill('\0');
                std::memcpy(tagBuffer_.data(), tag.data(),
                            std::min(tag.size(), tagBuffer_.size() - 1));
            }
            if (seleccionado) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Separator();

	// MOSTRAMOS COMPONENTES
	iterandoComponentes = true;
	componenteABorrar = nullptr;

	// Purga preventiva: elimina settings de componentes que ya no existen
	// en el objeto (evita punteros colgantes si el evento de estructura
	// no se proceso a tiempo).
	purgarSettingsHuerfanos();

	if (!listaDESettingsComponent->isEmpty()) {
		Position<SettingsComponent*>* position =
		    listaDESettingsComponent->first();

		while (position != nullptr && position->getElement() != nullptr) {
			SettingsComponent* comp = position->getElement();
			if (!comp) {
				position = (position != listaDESettingsComponent->last())
				               ? listaDESettingsComponent->next(position)
				               : nullptr;
				continue;
			}
			ImGui::PushID(comp);

			// Obtener componente subyacente de forma segura
			Component* componenteReal = comp->getComponent();
			if (!componenteReal) {
				ImGui::PopID();
				position = (position != listaDESettingsComponent->last())
				               ? listaDESettingsComponent->next(position)
				               : nullptr;
				continue;
			}

			// Verificacion adicional: el componente debe seguir en el objeto
			// Usamos el tipo almacenado para no depender de punteros
			bool componenteValido = false;
			if (object) {
				ListaDE<Component*>* componentes = object->getComponents();
				if (componentes && !componentes->isEmpty()) {
					const std::string& tipoEsperado = comp->getTipoComponente();
					if (!tipoEsperado.empty()) {
						Position<Component*>* pc = componentes->first();
						while (pc != nullptr) {
							Component* c = pc->getElement();
							if (c) {
								std::string tipoActual = demangle(typeid(*c).name());
								if (tipoActual == tipoEsperado) {
									componenteValido = true;
									break;
								}
							}
							pc = (pc != componentes->last())
							         ? componentes->next(pc)
							         : nullptr;
						}
					}
				}
			}
			if (!componenteValido) {
				ImGui::PopID();
				position = (position != listaDESettingsComponent->last())
				               ? listaDESettingsComponent->next(position)
				               : nullptr;
				continue;
			}

			std::string compName;
			if (Script* script = dynamic_cast<Script*>(componenteReal)) {
				std::string nombreScript = script->nombreParaMostrar();
				if (nombreScript.empty()) nombreScript = "Script";
				compName = "Script: " + nombreScript;
			} else {
				compName = comp->getTipoComponente();
				if (compName.empty() && componenteReal) {
					compName = demangle(typeid(*componenteReal).name());
				}
			}

			// Asegurar que compName no esté vacío
			if (compName.empty()) compName = "Componente";

			SettingsScript* scriptSettings =
			    dynamic_cast<SettingsScript*>(comp);
			if (scriptSettings && scriptSettings->estaEditandoNombre())
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			bool open = ImGui::CollapsingHeader(
			    compName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

			if (ImGui::BeginPopupContextItem(
			        "ComponentContext",
			        ImGuiPopupFlags_MouseButtonRight)) {
				if (scriptSettings &&
				    ImGui::MenuItem("Renombrar componente"))
					scriptSettings->iniciarEdicionNombre();
				if (ImGui::MenuItem("Eliminar Componente")) {
					Component* target = comp->getComponent();
					// Por el gestor: Ctrl+Z restaura el componente quitado.
					if (target && editor && editor->getGestorComandos())
						editor->getGestorComandos()->ejecutar(
						    std::make_unique<QuitarComponenteComando>(
						        editor, object,
						        demangle(typeid(*target).name()),
						        editor->getScene()));
					else if (editor)
						editor->removeComponent(object, target);
					else
						object->deleteComponent(target);
					componenteABorrar = comp;
					ImGui::EndPopup();
					ImGui::PopID();
					break;
				}
				ImGui::EndPopup();
			}

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
// Drag & drop: .prefab -> instanciar; .obj -> crear objeto con Model
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload =
				        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
					const char* path = static_cast<const char*>(payload->Data);
					if (path && editor) {
						std::string pathStr(path);
						std::cout << "[DEBUG Inspector] Drag-drop received: " << pathStr << std::endl;
						if (pathStr.size() >= 7 &&
						    pathStr.substr(pathStr.size() - 7) == ".prefab") {
							std::string nombre =
							    std::filesystem::path(pathStr).stem().string();
							if (auto* prefabLib = editor->getPrefabLibrary()) {
								if (Prefab* prefab = prefabLib->obtener(nombre)) {
									GameObject* instancia =
									    prefab->instanciar(editor, nullptr);
									if (instancia)
										editor->selectObject(instancia);
								}
							}
						} else if (pathStr.size() >= 4 &&
						           pathStr.substr(pathStr.size() - 4) == ".obj") {
							auto obj = std::make_unique<SimpleObject>();
							std::string nombre =
							    std::filesystem::path(pathStr).stem().string();
							std::snprintf(obj->inputName,
							              sizeof(obj->inputName), "%s",
							              nombre.c_str());
							GameObject* creado =
							    editor->createGameObject(std::move(obj), nullptr);
							if (creado) {
								auto* model = new Model();
								model->setPath(pathStr);
								creado->addComponent(
								    std::unique_ptr<Component>(model));
								editor->selectObject(creado);
							}
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (open) {
				comp->setMostrarVisualesDepuracion(
				    mostrarVisualesDepuracion_);
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
			// Alta por el gestor: Ctrl+Z quita el componente agregado.
			auto agregarViaComando =
			    [&](std::unique_ptr<Component> componente) {
				    if (GestorComandos* comandos = editor->getGestorComandos())
					    comandos->ejecutar(
					        std::make_unique<AgregarComponenteComando>(
					            editor, object, std::move(componente),
					            editor->getScene()));
				    else
					    editor->addComponent(object, std::move(componente));
			    };

			if (ImGui::MenuItem("Transform")) {
				agregarViaComando(std::make_unique<Transform>());
			}
			if (ImGui::MenuItem("Color")) {
				agregarViaComando(std::make_unique<Color>());
			}
			if (ImGui::MenuItem("Material")) {
				agregarViaComando(std::make_unique<Material>());
			}
			if (ImGui::MenuItem("Light")) {
				agregarViaComando(std::make_unique<Light>());
			}
			if (ImGui::MenuItem("CameraComponent")) {
				agregarViaComando(std::make_unique<CameraComponent>());
			}
			if (transform) {
				if (ImGui::BeginMenu("Add Collider")) {
					if (ImGui::MenuItem("EsfereCollider")) {
						agregarViaComando(std::make_unique<EsfereCollider>(5.0f, transform, object));
					}
					if (ImGui::MenuItem("CubeCollider")) {
						agregarViaComando(std::make_unique<CubeCollider>(5.0f, transform, object));
					}
					if (ImGui::MenuItem("MallaCollider")) {
						agregarViaComando(std::make_unique<MallaCollider>(5.0f, transform, object));
					}
					ImGui::EndMenu();
				}
			}
			if (collider) {
				if (ImGui::MenuItem("RigidBody")) {
					agregarViaComando(std::make_unique<RigidBody>(collider, 1.0f));
				}
			}
			if (ImGui::MenuItem("Script")) {
				agregarViaComando(std::make_unique<Script>());
			}
			if (ImGui::MenuItem("Model")) {
				agregarViaComando(std::make_unique<Model>());
			}
			if (ImGui::MenuItem("Grid")) {
				agregarViaComando(std::make_unique<Grid>());
			}
			if (ImGui::MenuItem("Skybox")) {
				agregarViaComando(std::make_unique<Skybox>());
			}
			if (ImGui::MenuItem("AudioSource")) {
				agregarViaComando(std::make_unique<AudioSource>());
			}
			if (ImGui::MenuItem("InterfaceComponent")) {
				agregarViaComando(std::make_unique<InterfaceComponent>());
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