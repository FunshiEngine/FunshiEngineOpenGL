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

#include "SettingsMateriales.h"

#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "../../../Assets/AssetManager.h"
#include "../../../Assets/Mesh.h"
#include "../../../Objetos/Componentes/Materiales.h"
#include "../../../Objetos/Componentes/Model.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Rendering/Preview/IPreviewMaterial.h"
#include "../../../Scenes/EditorController.h"
#include "../Material/EditorMaterialGUI.h"
#include <imgui.h>

SettingsMateriales::SettingsMateriales(GameObject* gameObject,
                                         EditorController* editor,
                                         IPreviewMaterial* preview)
    : SettingsComponent(demangle(typeid(Materiales).name())),
      gameObject(gameObject),
      editor(editor),
      preview(preview) {}

SettingsMateriales::~SettingsMateriales() {}

Component* SettingsMateriales::getComponent() {
    return gameObject->getComponent<Materiales>();
}

void SettingsMateriales::showDataComponent() {
    Materiales* materiales = gameObject->getComponent<Materiales>();
    if (!materiales) return;

    ImGui::Text("===Materiales por region===");
    ImGui::TextWrapped(
        "Cada region de la malla (la cara de un cubo, la parte de un "
        "personaje) puede llevar su propio material y sus texturas. La region "
        "se identifica con el nombre del material del archivo.");

    // Sincronizar con el modelo: agrega las regiones que declara el archivo.
    if (ImGui::Button("Sincronizar con el modelo")) {
        std::vector<std::string> nombres;
        Model* model = gameObject->getComponent<Model>();
        if (model && editor && editor->getAssetManager()) {
            try {
                std::shared_ptr<const Mesh> malla =
                    editor->getAssetManager()->getMesh(model->getPath());
                if (malla) {
                    for (const SubMalla& region : malla->subMallas) {
                        nombres.push_back(region.nombreMaterial);
                    }
                }
            } catch (const std::exception&) {
                // El modelo no se pudo cargar: se avisa abajo con el conteo.
            }
        }
        const int anadidas = materiales->sincronizarCon(nombres);
        if (nombres.empty()) {
            ImGui::Text("El modelo no declara regiones con material.");
        } else {
            ImGui::Text("Regiones anadidas: %d", anadidas);
        }
    }

    ImGui::Separator();

    // Anadir una region a mano (por si el archivo no declara material).
    char buffer[256] = {0};
    std::strncpy(buffer, nombreNuevaRegion_.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    if (ImGui::InputText("Nueva region", buffer, sizeof(buffer))) {
        nombreNuevaRegion_ = buffer;
    }
    ImGui::SameLine();
    if (ImGui::Button("Anadir") && !nombreNuevaRegion_.empty()) {
        if (!materiales->agregarRegion(nombreNuevaRegion_)) {
            ImGui::Text("Ya existe una region con ese nombre.");
        }
        nombreNuevaRegion_.clear();
    }

    ImGui::Separator();

    // Una entrada por region: su material y sus texturas.
    const std::size_t cantidad = materiales->cantidadRegiones();
    if (presetsAplicados_.size() < cantidad) {
        presetsAplicados_.resize(cantidad, -1);
    }
    for (std::size_t i = 0; i < cantidad; ++i) {
        const Materiales::Region& region = materiales->regiones()[i];
        ImGui::PushID(static_cast<int>(i));
        const bool abierto =
            ImGui::CollapsingHeader(region.nombre.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen);
        if (abierto && region.material) {
            // Cada region tiene su propia vista previa: la clave la
            // distingue (con el id del objeto y el nombre de la region), para
            // que se puedan ver a la vez y no se pisen entre objetos.
            const std::string clave = "region:" +
                                      std::to_string(gameObject->getId()) +
                                      ":" + region.nombre;
            dibujarEditorMaterial(*region.material, presetsAplicados_[i],
                                  preview, clave);
        }
        if (ImGui::SmallButton("Quitar region")) {
            materiales->quitarRegion(region.nombre);
            ImGui::PopID();
            break; // la lista cambio: se vuelve a dibujar en el proximo frame
        }
        ImGui::PopID();
    }
}
