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

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "../../../Behaviour/Reflection/BehaviourReflection.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Script.h"
#include "../../../Herramientas/TypeUtils.h"
#include <imgui.h>

SettingsScript::SettingsScript(Script* script)
    : SettingsComponent(demangle(typeid(Script).name())), myScript(script) {}

void SettingsScript::iniciarEdicionNombre() {
	const std::string& nombre = myScript->getNombreComponente();
	std::strncpy(bufferNombre_, nombre.c_str(), sizeof(bufferNombre_) - 1);
	bufferNombre_[sizeof(bufferNombre_) - 1] = '\0';
	editandoNombre_ = true;
}

namespace {

bool editarReferenciaObjeto(std::string& nombre) {
	const char* etiqueta = nombre.empty() ? "[Ninguno]" : nombre.c_str();
	ImGui::Button(etiqueta);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Arrastra un objeto de la escena para asignarlo");
	bool modificado = false;
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload =
		        ImGui::AcceptDragDropPayload("ENTITY_NODE")) {
			if (payload->DataSize == static_cast<int>(sizeof(GameObject*))) {
				GameObject* objeto = nullptr;
				std::memcpy(&objeto, payload->Data, sizeof(objeto));
				if (objeto) {
					nombre = objeto->inputName;
					modificado = true;
				}
			}
		}
		ImGui::EndDragDropTarget();
	}
	return modificado;
}

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
		std::string& nombre = valor.como<std::string>();
		bool modificado = editarReferenciaObjeto(nombre);
		if (!nombre.empty()) {
			ImGui::SameLine();
			if (ImGui::SmallButton("x")) {
				nombre.clear();
				modificado = true;
			}
		}
		if (modificado) {
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

	if (editandoNombre_) {
		const bool confirmar = ImGui::InputText(
		    "Nombre del componente", bufferNombre_, sizeof(bufferNombre_),
		    ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::SameLine();
		if (ImGui::Button("Aceptar") || confirmar) {
			myScript->setNombreComponente(bufferNombre_);
			editandoNombre_ = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancelar")) editandoNombre_ = false;
	}

	// 1. Selector del fuente (drag & drop) con la convencion <ClassName>.cpp
	const std::string& className = myScript->getNameClass();
	const char* displayText =
	    className.empty() ? "[Arrastra script]" : className.c_str();

	float textWidth = ImGui::CalcTextSize(displayText).x;
	float buttonWidth = textWidth + ImGui::GetStyle().FramePadding.x * 2;

	ImGui::SetNextItemWidth(buttonWidth);
	ImGui::PushID(myScript);
	if (ImGui::Button(displayText)) {
		// Recompilar manualmente (aplicar cambios editados del fuente).
		myScript->recargar(nullptr);
	}

	if (ImGui::BeginDragDropTarget()) {
		std::cout << "[DEBUG ScriptDragDrop] Target active for script: " 
		          << className << " path: " << myScript->getPath() << std::endl;
		if (const ImGuiPayload* payload =
		        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
			const char* path = (const char*)payload->Data;
			std::cout << "[DEBUG ScriptDragDrop] Dropped file: " << path 
			          << " onto script: " << className << std::endl;
			// setDllPath invalida la carga previa y fija la nueva ruta.
			// La compilacion/carga NO se hace aqui dentro (H-4): invocar
			// cl.exe o javac sincronicamente bloquea el hilo varios segundos
			// en plena re-entrada de ImGui y la ventana parece colgada. Se
			// marca pendiente para el frame siguiente, fuera del target.
			myScript->setDllPath(path);
			cargaDiferidaPendiente_ = true;
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::PopID();

	// Indicador de carga mientras se compila/carga el script
	// Procesar la carga diferida al comienzo del siguiente frame (fuera de
	// cualquier contexto de drag & drop).
	if (cargaDiferidaPendiente_) {
		cargaDiferidaPendiente_ = false;
		myScript->cargarSiNecesario();
	}

	if (!myScript->getPath().empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
		ImGui::TextUnformatted(myScript->getPath().c_str());
		ImGui::PopStyleColor();
	}

	// Los errores de carga/compilacion se informan en la ventana Estado y el
	// log; el panel conserva el fuente y los campos reflejados.

	const std::vector<DefCampo>& campos = myScript->obtenerCampos();
	std::vector<ValorCampo>& valores = myScript->obtenerValores();
	if (campos.empty()) {
		ImGui::Separator();
		ImGui::TextDisabled("Sin campos SerializeField. Agrega REFLECT_CAMPO en "
		                   "el script (ver template).");
		return;
	}

	ImGui::TextDisabled("Campos serializados");
	// El bucle va por el MENOR de los dos cardinales, no solo por `campos`.
		// Script::cargarSiNecesario deja `valores_` con una entrada por campo,
		// pero una escena guardada puede traer menos valores que los que expone
		// el script actual (un SerializeField agregado despues, o una reflexion
		// que no llego a correr). Iterar solo por `campos` leia `valores[i]` fuera
		// de rango, sobre basura, y el std::get siguiente lanzaba
		// bad_variant_access: eso terminaba el proceso al abrir el inspector del
		// script. El limite ya lo respetaba la escritura de la linea del
		// escribirCampo(), pero no esta lectura.
		const int nCampos = static_cast<int>(
		    std::min(campos.size(), valores.size()));
		for (int i = 0; i < nCampos; ++i) {
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
					camb = editarReferenciaObjeto(
					           lista[static_cast<std::size_t>(j)]) ||
					       camb;
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

			// `i` ya esta acotado por el menor de los dos cardinales, asi que
			// `i < valores.size()` es siempre cierto: la comprobacion que habia
			// aqui era la version correcta de un limite que faltaba en la
			// lectura del bucle.
			if (camb)
				myScript->escribirCampo(i, valor); // sincroniza instancia viva
ImGui::PopID();
	}
}