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
#include "SettingsScript.h"

#include <cstring>
#include <string>
#include <vector>

#include "../../../Behaviour/Reflection/BehaviourReflection.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Script.h"
#include <imgui.h>

SettingsScript::SettingsScript(GameObject* objeto) {
	myScript = objeto->getComponent<Script>();
}

namespace {

// Widget para un valor escalar/objeto segun su tag, SIN etiqueta. Devuelve
// true si el widget modifico `valor` (el arbol se reescribe solo aca).
bool editarScalar(ReflejoScripts::ValorCampo& valor) {
	using namespace ReflejoScripts;
	switch (valor.tag) {
	case TagTipo::Entero: {
		int dato = valor.como<int>();
		if (ImGui::DragInt("##v", &dato, 0.5f)) {
			valor.contenido = dato;
			return true;
		}
		break;
	}
	case TagTipo::Flotante: {
		float dato = valor.como<float>();
		if (ImGui::DragFloat("##v", &dato, 0.05f)) {
			valor.contenido = dato;
			return true;
		}
		break;
	}
	case TagTipo::Doble: {
		double dato = valor.como<double>();
		if (ImGui::DragScalar("##v", ImGuiDataType_Double, &dato, 0.01)) {
			valor.contenido = dato;
			return true;
		}
		break;
	}
	case TagTipo::Booleano: {
		bool dato = valor.como<bool>();
		if (ImGui::Checkbox("##v", &dato)) {
			valor.contenido = dato;
			return true;
		}
		break;
	}
	case TagTipo::Texto: {
		char buffer[1024];
		const std::string& texto = valor.como<std::string>();
		std::strncpy(buffer, texto.c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		if (ImGui::InputText("##v", buffer, sizeof(buffer))) {
			valor.contenido = std::string(buffer);
			return true;
		}
		break;
	}
	case TagTipo::Vec3: {
		vec3 dato = valor.como<vec3>();
		float comp[3] = {dato.x, dato.y, dato.z};
		if (ImGui::DragFloat3("##v", comp, 0.05f)) {
			dato.x = comp[0];
			dato.y = comp[1];
			dato.z = comp[2];
			valor.contenido = dato;
			return true;
		}
		break;
	}
	case TagTipo::Objeto: {
		char buffer[256];
		const std::string& nombre = valor.como<std::string>();
		std::strncpy(buffer, nombre.c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		if (ImGui::InputText("##v", buffer, sizeof(buffer))) {
			valor.contenido = std::string(buffer);
			return true;
		}
		break;
	}
	default:
		break;
	}
	return false;
}

// Edita una fila: etiqueta (que es el boton del TreeNodeEx si corresponde) y
// el widget sin etiqueta. Reusa el mismo valor escalar para members de arrays.
} // namespace

void SettingsScript::showDataComponent() {
	using namespace ReflejoScripts;

	// 1. Selector del fuente (drag & drop) con la convencion <ClassName>.cpp
	const std::string& className = myScript->getNameClass();
	const char* displayText =
	    className.empty() ? "[Arrastra script]" : className.c_str();

	float textWidth = ImGui::CalcTextSize(displayText).x;
	float buttonWidth = textWidth + ImGui::GetStyle().FramePadding.x * 2;

	ImGui::SetNextItemWidth(buttonWidth);
	if (ImGui::Button(displayText)) {
		// Recompilar manualmente (aplicar cambios editados del fuente).
		myScript->recargar(nullptr);
	}

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload =
		        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
			const char* path = (const char*)payload->Data;
			// setDllPath invalida la carga; cargarSiNecesario la fuerza para
			// poder mostrar/editar los SerializeField en el editor.
			myScript->setDllPath(path);
			myScript->cargarSiNecesario();
		}
		ImGui::EndDragDropTarget();
	}

	if (!myScript->getPath().empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
		ImGui::TextUnformatted(myScript->getPath().c_str());
		ImGui::PopStyleColor();
	}

	// Los errores de carga/compilacion NO se muestran aca: se informan en la
	// ventana Estado y en el log del motor (logs/), igual que el resto de los
	// mensajes del scripting. El panel queda solo con el fuente asignado y sus
	// SerializeField, para no repetir el mismo mensaje en dos lados.

	const std::vector<DefCampo>& campos = myScript->obtenerCampos();
	std::vector<ValorCampo>& valores = myScript->obtenerValores();
	if (campos.empty()) {
		ImGui::Separator();
		ImGui::TextDisabled("Sin campos SerializeField. Agrega REFLECT_CAMPO en "
		                   "el script (ver template).");
		return;
	}

	ImGui::Separator();
	if (ImGui::CollapsingHeader("SerializeField",
	                            ImGuiTreeNodeFlags_DefaultOpen)) {
		for (int i = 0; i < static_cast<int>(campos.size()); ++i) {
			const DefCampo& def = campos[static_cast<std::size_t>(i)];
			ValorCampo& valor = valores[static_cast<std::size_t>(i)];
			bool camb = false;

			ImGui::PushID(i);
			switch (def.tag) {
			case TagTipo::Grupo: {
				if (ImGui::TreeNodeEx(def.nombre.c_str(),
				                      ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& subs = valor.como<std::vector<ValorCampo>>();
					for (int j = 0;
					     j < static_cast<int>(subs.size()) &&
					     j < static_cast<int>(def.subcampos.size());
					     ++j) {
						ImGui::PushID(j);
						ImGui::TextUnformatted(
						    def.subcampos[static_cast<std::size_t>(j)]
						        .nombre.c_str());
						ImGui::SameLine();
						camb = editarScalar(
						           subs[static_cast<std::size_t>(j)]) ||
						       camb;
						ImGui::PopID();
					}
					ImGui::TreePop();
				}
				break;
			}
			case TagTipo::Grupos: {
				if (ImGui::TreeNodeEx(def.nombre.c_str(),
				                      ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& lista = valor.como<
					    std::vector<std::vector<ValorCampo>>>();
					for (int g = 0; g < static_cast<int>(lista.size()); ++g) {
						ImGui::PushID(g);
						if (ImGui::TreeNodeEx(
						        ("Grupo " + std::to_string(g)).c_str(),
						        ImGuiTreeNodeFlags_DefaultOpen)) {
							auto& grupo =
							    lista[static_cast<std::size_t>(g)];
							for (int j = 0;
							     j < static_cast<int>(grupo.size()) &&
							     j < static_cast<int>(
							             def.subcampos.size());
							     ++j) {
								ImGui::PushID(j);
								ImGui::TextUnformatted(
								    def.subcampos[static_cast<std::size_t>(j)]
								        .nombre.c_str());
								ImGui::SameLine();
								camb = editarScalar(
								           grupo[static_cast<std::size_t>(j)]) ||
								       camb;
								ImGui::PopID();
							}
							if (ImGui::SmallButton("Eliminar grupo")) {
								lista.erase(lista.begin() + g);
								camb = true;
							}
							ImGui::TreePop();
						}
						ImGui::PopID();
					}
					if (ImGui::SmallButton("+ Agregar grupo")) {
						std::vector<ValorCampo> nuevo;
						for (const DefCampo& sub : def.subcampos)
							nuevo.push_back(valorPorDefecto(sub));
						lista.push_back(std::move(nuevo));
						camb = true;
					}
					ImGui::TreePop();
				}
				break;
			}
			case TagTipo::Enteros: {
				auto& lista = valor.como<std::vector<int>>();
				ImGui::TextUnformatted(def.nombre.c_str());
				for (int j = 0; j < static_cast<int>(lista.size()); ++j) {
					ImGui::PushID(j);
					int dato = lista[static_cast<std::size_t>(j)];
					if (ImGui::DragInt("##v", &dato, 0.5f)) {
						lista[static_cast<std::size_t>(j)] = dato;
						camb = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("x")) {
						lista.erase(lista.begin() + j);
						camb = true;
					}
					ImGui::PopID();
				}
				if (ImGui::SmallButton("+ Agregar")) {
					lista.push_back(0);
					camb = true;
				}
				break;
			}
			case TagTipo::Flotantes: {
				auto& lista = valor.como<std::vector<float>>();
				ImGui::TextUnformatted(def.nombre.c_str());
				for (int j = 0; j < static_cast<int>(lista.size()); ++j) {
					ImGui::PushID(j);
					float dato = lista[static_cast<std::size_t>(j)];
					if (ImGui::DragFloat("##v", &dato, 0.05f)) {
						lista[static_cast<std::size_t>(j)] = dato;
						camb = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("x")) {
						lista.erase(lista.begin() + j);
						camb = true;
					}
					ImGui::PopID();
				}
				if (ImGui::SmallButton("+ Agregar")) {
					lista.push_back(0.0f);
					camb = true;
				}
				break;
			}
			case TagTipo::Dobles: {
				auto& lista = valor.como<std::vector<double>>();
				ImGui::TextUnformatted(def.nombre.c_str());
				for (int j = 0; j < static_cast<int>(lista.size()); ++j) {
					ImGui::PushID(j);
					double dato = lista[static_cast<std::size_t>(j)];
					if (ImGui::DragScalar("##v", ImGuiDataType_Double, &dato,
					                      0.01)) {
						lista[static_cast<std::size_t>(j)] = dato;
						camb = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("x")) {
						lista.erase(lista.begin() + j);
						camb = true;
					}
					ImGui::PopID();
				}
				if (ImGui::SmallButton("+ Agregar")) {
					lista.push_back(0.0);
					camb = true;
				}
				break;
			}
			case TagTipo::Booleanos: {
				auto& lista = valor.como<std::vector<bool>>();
				ImGui::TextUnformatted(def.nombre.c_str());
				for (int j = 0; j < static_cast<int>(lista.size()); ++j) {
					ImGui::PushID(j);
					bool dato = lista[static_cast<std::size_t>(j)];
					if (ImGui::Checkbox("##v", &dato)) {
						lista[static_cast<std::size_t>(j)] = dato;
						camb = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("x")) {
						lista.erase(lista.begin() + j);
						camb = true;
					}
					ImGui::PopID();
				}
				if (ImGui::SmallButton("+ Agregar")) {
					lista.push_back(false);
					camb = true;
				}
				break;
			}
			case TagTipo::Textos: {
				auto& lista = valor.como<std::vector<std::string>>();
				ImGui::TextUnformatted(def.nombre.c_str());
				for (int j = 0; j < static_cast<int>(lista.size()); ++j) {
					ImGui::PushID(j);
					char buffer[1024];
					std::strncpy(buffer,
					             lista[static_cast<std::size_t>(j)].c_str(),
					             sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = '\0';
					if (ImGui::InputText("##v", buffer, sizeof(buffer))) {
						lista[static_cast<std::size_t>(j)] =
						    std::string(buffer);
						camb = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("x")) {
						lista.erase(lista.begin() + j);
						camb = true;
					}
					ImGui::PopID();
				}
				if (ImGui::SmallButton("+ Agregar")) {
					lista.push_back("");
					camb = true;
				}
				break;
			}
			case TagTipo::Vec3s: {
				auto& lista = valor.como<std::vector<vec3>>();
				ImGui::TextUnformatted(def.nombre.c_str());
				for (int j = 0; j < static_cast<int>(lista.size()); ++j) {
					ImGui::PushID(j);
					float comp[3] = {lista[static_cast<std::size_t>(j)].x,
					                 lista[static_cast<std::size_t>(j)].y,
					                 lista[static_cast<std::size_t>(j)].z};
					if (ImGui::DragFloat3("##v", comp, 0.05f)) {
						lista[static_cast<std::size_t>(j)].x = comp[0];
						lista[static_cast<std::size_t>(j)].y = comp[1];
						lista[static_cast<std::size_t>(j)].z = comp[2];
						camb = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("x")) {
						lista.erase(lista.begin() + j);
						camb = true;
					}
					ImGui::PopID();
				}
				if (ImGui::SmallButton("+ Agregar")) {
					lista.push_back(vec3(0.0f, 0.0f, 0.0f));
					camb = true;
				}
				break;
			}
			case TagTipo::Objetos: {
				auto& lista = valor.como<std::vector<std::string>>();
				ImGui::TextUnformatted(def.nombre.c_str());
				for (int j = 0; j < static_cast<int>(lista.size()); ++j) {
					ImGui::PushID(j);
					char buffer[256];
					std::strncpy(buffer,
					             lista[static_cast<std::size_t>(j)].c_str(),
					             sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = '\0';
					if (ImGui::InputText("##v", buffer, sizeof(buffer))) {
						lista[static_cast<std::size_t>(j)] =
						    std::string(buffer);
						camb = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("x")) {
						lista.erase(lista.begin() + j);
						camb = true;
					}
					ImGui::PopID();
				}
				if (ImGui::SmallButton("+ Agregar")) {
					lista.push_back("");
					camb = true;
				}
				break;
			}
			default: {
				ImGui::TextUnformatted(def.nombre.c_str());
				ImGui::SameLine();
				camb = editarScalar(valor);
				break;
			}
			}

			if (camb && i < static_cast<int>(valores.size()))
				myScript->escribirCampo(i, valor); // sincroniza instancia viva
			ImGui::PopID();
		}
	}
}

Component* SettingsScript::getComponent() { return myScript; }