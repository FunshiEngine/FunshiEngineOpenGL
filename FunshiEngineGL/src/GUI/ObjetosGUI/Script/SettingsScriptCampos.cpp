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
#include "SettingsScriptDetalle.h"
#include "../../../Localizacion/Localizacion.h"

using Localizacion::Texto;

bool editarReferenciaObjeto(std::string& nombre) {
    const char* etiqueta = nombre.empty() ? "[Ninguno]" : nombre.c_str();
    ImGui::Button(etiqueta);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localizacion::tr(Texto::ArrastraUnObjetoDeLa));
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

// Referencia a un COMPONENTE de otro objeto (TagTipo::Componente): el nombre
// guardado es el del OBJETO dueño del componente. Se asigna arrastrando el
// componente desde su inspector (p. ej. la interfaz del jugador) al campo.
bool editarReferenciaComponente(std::string& nombre, const std::string& tipo) {
    const char* etiqueta = nombre.empty() ? "[Ninguno]" : nombre.c_str();
    ImGui::Button(etiqueta);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(Localizacion::tr(Texto::ArrastraElComponenteDeUn),
                          tipo.c_str());
    bool modificado = false;
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("COMPONENTE_INTERFAZ")) {
            // El payload lleva el nombre del objeto dueno del componente
            // (la interfaz web o la nativa).
            if (tipo == "InterfaceComponent" || tipo == "UIComponent") {
                const char* dueno = static_cast<const char*>(payload->Data);
                if (dueno && *dueno) {
                    nombre = dueno;
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
