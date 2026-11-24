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
#include "SettingsInterface.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Interfaz/InterfaceComponent.h"

#include <imgui.h>

#include <cctype>
#include <cstring>
#include <string>

namespace {

const char* NOMBRES_ANCLA[] = {
    "Arriba izquierda", "Arriba centro", "Arriba derecha",
    "Medio izquierda",  "Centro",        "Medio derecha",
    "Abajo izquierda",  "Abajo centro",  "Abajo derecha"};

constexpr int TOTAL_ANCLAS = 9;

// El filtro del .html se hace aqui y no en el componente: el componente guarda
// cualquier ruta (todas se serializan); el formato solo importa al elegir el
// archivo que se va a cargar.
bool esHtml(const std::string& ruta) {
    if (ruta.size() < 5) return false;
    const std::string ext = ruta.substr(ruta.size() - 5);
    for (std::size_t i = 0; i < ext.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(ext[i])) !=
            ".html"[i])
            return false;
    }
    return true;
}

} // namespace

SettingsInterface::SettingsInterface(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(InterfaceComponent).name())), gameObject(gameObject) {}

void SettingsInterface::showDataComponent() {
    InterfaceComponent* comp = objetivo<InterfaceComponent>(gameObject);
    if (!comp) return;


    // Origen de arrastre: lleva el nombre del objeto dueño de la interfaz para
    // asignarla a un campo TagTipo::Componente de un script.
    const std::string etiquetaDrag =
        std::string("Interfaz de: ") +
        (gameObject->inputName[0] != '\0' ? gameObject->inputName
                                          : "(objeto)");
    ImGui::Button(etiquetaDrag.c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Arrastra a un campo de script para asignar esta interfaz");
    if (ImGui::BeginDragDropSource()) {
        const char* dueno = gameObject->inputName;
        ImGui::SetDragDropPayload("COMPONENTE_INTERFAZ", dueno,
                                  std::strlen(dueno) + 1);
        ImGui::TextUnformatted("Asignar interfaz a un script");
        ImGui::EndDragDropSource();
    }

    // Boton con la ruta actual: es el destino del arrastre desde el explorador.
    const std::string& ruta = comp->getRutaHtml();
    const char* texto = ruta.empty() ? "[Arrastra un .html]" : ruta.c_str();
    const float ancho = ImGui::CalcTextSize(texto).x +
                        ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(ancho);
    ImGui::Button(texto);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaSoltada = static_cast<const char*>(payload->Data);
            if (esHtml(rutaSoltada))
                comp->setRutaHtml(rutaSoltada);
        }
        ImGui::EndDragDropTarget();
    }

    bool activo = comp->estaActiva();
    if (ImGui::Checkbox("Activa", &activo))
        comp->setActivo(activo);

    int ancla = static_cast<int>(comp->getAncla());
    if (ImGui::Combo("Ancla", &ancla, NOMBRES_ANCLA, TOTAL_ANCLAS))
        comp->setAncla(static_cast<AnclaInterfaz>(ancla));

    glm::vec2 desplazamiento = comp->getDesplazamiento();
    if (ImGui::DragFloat2("Desplazamiento", &desplazamiento.x, 1.0f))
        comp->setDesplazamiento(desplazamiento);

    bool tamanoAuto = comp->usaTamanoAuto();
    if (ImGui::Checkbox("Tamano automatico (CSS)", &tamanoAuto))
        comp->setTamanoAuto(tamanoAuto);
    if (!comp->usaTamanoAuto()) {
        glm::vec2 tamano = comp->getTamano();
        if (ImGui::DragFloat2("Tamano", &tamano.x, 1.0f))
            comp->setTamano(tamano);
    }

    float z = comp->getZ();
    if (ImGui::DragFloat("Z", &z, 0.1f))
        comp->setZ(z);

    ImGui::TextDisabled("Se muestra en modo play; el CSS define el tamano");
}
