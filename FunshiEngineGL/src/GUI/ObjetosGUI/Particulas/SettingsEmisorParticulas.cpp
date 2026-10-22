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
#include "SettingsEmisorParticulas.h"

#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/EmisorParticulas.h"
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <iostream>

#include "../../../Assets/StbImageLoader.h"

namespace {

// Un perfil de efecto es un JSON (el mismo formato que lee cargarPerfilDesdeJson).
// La comparacion es normalizada a minusculas: la extension la pone el usuario.
bool esPerfilJson(const char* ruta) {
    std::string extension = std::filesystem::path(ruta).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".json";
}

} // namespace

SettingsEmisorParticulas::SettingsEmisorParticulas(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(EmisorParticulas).name())),
      gameObject(gameObject) {}

void SettingsEmisorParticulas::showDataComponent() {
    EmisorParticulas* emisor =
        gameObject ? gameObject->getComponent<EmisorParticulas>() : nullptr;
    if (!emisor) return;

    ImGui::Text("===Emisor de particulas===");

    bool activa = emisor->activa();
    if (ImGui::Checkbox("Activo", &activa)) emisor->setActiva(activa);

    ParametrosEmisor& parametros = emisor->parametros();

    int cantidad = parametros.cantidadPorSegundo;
    if (ImGui::SliderInt("Particulas por segundo", &cantidad, 0, 500))
        parametros.cantidadPorSegundo = cantidad;

    if (ImGui::DragFloat("Vida minima", &parametros.vidaMinima, 0.05f, 0.0f,
                         60.0f, "%.2f s") ||
        ImGui::DragFloat("Vida maxima", &parametros.vidaMaxima, 0.05f, 0.0f,
                         60.0f, "%.2f s")) {
        if (parametros.vidaMaxima < parametros.vidaMinima)
            parametros.vidaMaxima = parametros.vidaMinima;
    }

    if (ImGui::DragFloat("Velocidad minima", &parametros.velocidadMinima, 0.1f,
                         0.0f, 50.0f, "%.2f") ||
        ImGui::DragFloat("Velocidad maxima", &parametros.velocidadMaxima, 0.1f,
                         0.0f, 50.0f, "%.2f")) {
        if (parametros.velocidadMaxima < parametros.velocidadMinima)
            parametros.velocidadMaxima = parametros.velocidadMinima;
    }

    if (ImGui::DragFloat("Tamano minimo", &parametros.tamanoMinimo, 0.01f,
                         0.0f, 10.0f, "%.2f") ||
        ImGui::DragFloat("Tamano maximo", &parametros.tamanoMaximo, 0.01f,
                         0.0f, 10.0f, "%.2f")) {
        if (parametros.tamanoMaximo < parametros.tamanoMinimo)
            parametros.tamanoMaximo = parametros.tamanoMinimo;
    }

    ImGui::ColorEdit4("Color", parametros.color);

    ImGui::DragFloat3("Direccion", parametros.direccion, 0.05f, -1.0f, 1.0f);
    ImGui::SliderFloat("Dispersion", &parametros.dispersion, 0.0f, 1.0f,
                       "%.2f");
    ImGui::DragFloat3("Gravedad", parametros.gravedad, 0.05f, -20.0f, 20.0f);
    ImGui::SliderFloat("Rozamiento", &parametros.rozamiento, 0.0f, 5.0f,
                       "%.2f");
    ImGui::DragFloat("Rotacion", &parametros.rotacionInicial, 0.05f, -10.0f,
                     10.0f, "%.2f rad");
    ImGui::DragFloat("Velocidad de rotacion", &parametros.velocidadRotacion,
                     0.05f, -20.0f, 20.0f, "%.2f rad/s");

    // Perfil de efecto: la ruta del JSON. El perfil manda sobre la vida, la
    // velocidad, la gravedad y el rozamiento, y agrega las curvas de tamano y
    // color.
    ImGui::Separator();
    ImGui::Text("Perfil de efecto");
    char buffer[1024];
    const std::string ruta = emisor->rutaPerfil();
    std::snprintf(buffer, sizeof(buffer), "%s", ruta.c_str());
    if (ImGui::InputText("Ruta", buffer, sizeof(buffer))) {
        emisor->setRutaPerfil(std::string(buffer));
    }
    // El perfil tambien se asigna arrastrando el JSON desde el explorador de
    // contenido. Solo se aceptan .json; si el contenido no es un perfil
    // valido, el motivo queda en errorPerfil y se muestra abajo.
    ImGui::PushID("PerfilEfecto");
    ImGui::Button(ruta.empty() ? "[Arrastra un perfil .json]" : ruta.c_str());
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaSoltada = static_cast<const char*>(payload->Data);
            if (rutaSoltada && *rutaSoltada && esPerfilJson(rutaSoltada)) {
                emisor->setRutaPerfil(rutaSoltada);
            } else if (rutaSoltada && *rutaSoltada) {
                std::cerr << "[Particulas] el archivo arrastrado no es un "
                             "perfil de efecto (.json): "
                          << rutaSoltada << std::endl;
            }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    if (ImGui::Button("Quitar")) {
        emisor->setRutaPerfil(std::string());
    }
    ImGui::PopID();
    if (!emisor->errorPerfil().empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s",
                            emisor->errorPerfil().c_str());
    } else if (emisor->perfil()) {
        ImGui::Text("Perfil: %s", emisor->perfil()->nombre.c_str());
    }

    // Textura de particula: se puede asignar arrastrando una imagen desde el
    // explorador de contenido (showfolder). Sobreescribe la textura del perfil.
    ImGui::Separator();
    ImGui::Text("Textura de particula");
    const std::string& textura = emisor->texturaParticula();
    const char* textoBoton = textura.empty() ? "[Arrastra una imagen]"
                                               : textura.c_str();
    ImGui::PushID(emisor);
    ImGui::Button(textoBoton);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaSoltada = static_cast<const char*>(payload->Data);
            int ancho = 0, alto = 0;
            if (rutaSoltada && *rutaSoltada &&
                StbImageLoader::dimensiones(rutaSoltada, ancho, alto)) {
                emisor->setTexturaParticula(rutaSoltada);
            } else if (rutaSoltada && *rutaSoltada) {
                std::cerr << "[Particulas] el archivo arrastrado no es una "
                             "imagen valida: "
                          << rutaSoltada << std::endl;
            }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    if (ImGui::Button("Quitar")) {
        emisor->setTexturaParticula(std::string());
    }
    ImGui::PopID();
}
