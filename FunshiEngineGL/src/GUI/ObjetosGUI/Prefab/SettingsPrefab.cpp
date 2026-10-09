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
#include "SettingsPrefab.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/PrefabLibrary.h"
#include "../../../Objetos/Prefab.h"
#include "../../../Scenes/EditorController.h"
#include <imgui.h>
#include <algorithm>

SettingsPrefab::SettingsPrefab(GameObject* objeto, EditorController* editor, PrefabLibrary* biblioteca)
    : SettingsComponent("Prefab"),
      objeto(objeto), editor(editor), biblioteca(biblioteca) {}

void SettingsPrefab::setEditor(EditorController* editor) {
    this->editor = editor;
}

void SettingsPrefab::setBiblioteca(PrefabLibrary* biblioteca) {
    this->biblioteca = biblioteca;
}

void SettingsPrefab::showDataComponent() {
    if (!objeto || !editor || !biblioteca) return;

    // --- Drag & Drop: soltar .prefab para instanciar ---
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* path = static_cast<const char*>(payload->Data);
            if (path) {
                std::string ruta(path);
                // Verificar extension .prefab (case-insensitive)
                if (ruta.size() >= 6) {
                    std::string ext = ruta.substr(ruta.size() - 6);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".prefab") {
                        // Extraer nombre sin extension ni ruta
                        size_t posSlash = ruta.find_last_of("/\\");
                        std::string nombre = (posSlash == std::string::npos) ? ruta : ruta.substr(posSlash + 1);
                        if (nombre.size() > 6) nombre = nombre.substr(0, nombre.size() - 6); // quitar .prefab

                        Prefab* prefab = biblioteca->obtener(nombre);
                        if (prefab) {
                            // Instanciar en el objeto seleccionado como padre, o en raiz
                            GameObject* padre = objeto;
                            GameObject* instancia = prefab->instanciar(editor, padre);
                            if (instancia && editor) {
                                editor->selectObject(instancia);
                            }
                        } else {
                            std::cerr << "[Prefab Drag&Drop] No se encontro prefab: " << nombre << std::endl;
                        }
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // --- Guardar como prefab ---
    if (ImGui::Button("Guardar como prefab")) {
        mostrarGuardar = true;
        recienAbiertoGuardar = true;
        nombreNuevoPrefab = objeto->inputName;
        std::snprintf(nombreNuevoPrefabBuf, sizeof(nombreNuevoPrefabBuf), "%s", nombreNuevoPrefab.c_str());
    }

    if (mostrarGuardar) {
        if (recienAbiertoGuardar) {
            ImGui::OpenPopup("GuardarPrefabPopup");
            recienAbiertoGuardar = false;
        }
        if (ImGui::BeginPopupModal("GuardarPrefabPopup", &mostrarGuardar,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nombre del prefab:");
            ImGui::InputText("##NombrePrefab", nombreNuevoPrefabBuf, sizeof(nombreNuevoPrefabBuf),
                             ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Separator();
            if (ImGui::Button("Guardar", ImVec2(120, 0))) {
                nombreNuevoPrefab = nombreNuevoPrefabBuf;
                if (!nombreNuevoPrefab.empty()) {
                    biblioteca->crearPrefab(nombreNuevoPrefab, objeto, editor);
                }
                mostrarGuardar = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                mostrarGuardar = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::Separator();

    // --- Instanciar prefab ---
    if (ImGui::Button("Instanciar prefab")) {
        mostrarInstanciar = true;
        recienAbiertoInstanciar = true;
        prefabSeleccionado.clear();
        biblioteca->recargar();
    }

    if (mostrarInstanciar) {
        if (recienAbiertoInstanciar) {
            ImGui::OpenPopup("InstanciarPrefabPopup");
            recienAbiertoInstanciar = false;
        }
        if (ImGui::BeginPopupModal("InstanciarPrefabPopup", &mostrarInstanciar,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Seleccionar prefab:");
            
            auto nombres = biblioteca->listarNombres();
            std::sort(nombres.begin(), nombres.end());
            
            if (nombres.empty()) {
                ImGui::TextColored(ImVec4(1, 0.5, 0.5, 1),
                                   "No hay prefabs guardados.");
            } else {
                for (const auto& nombre : nombres) {
                    bool seleccionado = (prefabSeleccionado == nombre);
                    if (ImGui::Selectable(nombre.c_str(), seleccionado)) {
                        prefabSeleccionado = nombre;
                    }
                }
            }
            
            ImGui::Separator();
            bool puedeInstanciar = !prefabSeleccionado.empty();
            if (!puedeInstanciar) ImGui::BeginDisabled();
            if (ImGui::Button("Instanciar", ImVec2(120, 0))) {
                if (puedeInstanciar) {
                    Prefab* prefab = biblioteca->obtener(prefabSeleccionado);
                    if (prefab) {
                        GameObject* instancia = prefab->instanciar(editor, nullptr);
                        if (instancia && editor)
                            editor->selectObject(instancia);
                    }
                }
                mostrarInstanciar = false;
                ImGui::CloseCurrentPopup();
            }
            if (!puedeInstanciar) ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                mostrarInstanciar = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::Separator();

    // --- Desconectar prefab ---
    ImGui::TextDisabled("Desconectar prefab: (requiere tracking de instancia)");
}