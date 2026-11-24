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
#include "MenuViewDetalle.h"
#include "TextosImportar.h"
#include "../../Configuracion/ProjectPaths.h"
#include "../../FileManager/FileManager.h"
#include "../../Localizacion/Localizacion.h"
#include "../Tema/TemaEditor.h"

#include <cstdio>

using Localizacion::Texto;

// Seccion "Importar proyecto" de Configurar proyecto. La vista elige el origen
// con el dialogo del sistema, lo analiza UNA vez (no en cada frame) y pide la
// importacion al modelo; main hace la copia en disco (GestorDeProyectos) y
// deja el resultado, que esta vista muestra.

void MenuView::elegirOrigenImportar(const std::string& ruta) {
    if (ruta.empty()) return;  // el usuario cancelo el dialogo
    origenImportar = ImportarProyecto::analizar(ruta);
    hayOrigenImportar = true;
    std::snprintf(nombreImportarBuffer, sizeof(nombreImportarBuffer), "%s",
                  origenImportar.nombre.c_str());
}

void MenuView::renderizarSeccionImportar() {
    ImGui::TextUnformatted(model->traducir(Texto::ImportarProyecto));
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("%s", model->traducir(Texto::ImportarAyuda));
    ImGui::Spacing();

    if (ImGui::Button(etiqueta(model, Texto::ElegirCarpeta).c_str()))
        elegirOrigenImportar(FileManager::seleccionarCarpetaSistema());
    ImGui::SameLine();
    if (ImGui::Button(etiqueta(model, Texto::ElegirArchivoFnep).c_str()))
        elegirOrigenImportar(FileManager::seleccionarArchivoSistema());

    // Resultado de la ultima importacion (lo deja main al terminar la copia).
    const MenuModel::ResultadoImportar& resultado = model->getResultadoImportacion();
    if (resultado.hay) {
        ImGui::Spacing();
        char mensaje[512];
        std::snprintf(mensaje, sizeof(mensaje), model->traducir(resultado.texto),
                      resultado.nombre.c_str());
        ImGui::PushStyleColor(ImGuiCol_Text,
                              TemaEditor::color(resultado.exito ? TemaEditor::RolColor::Exito
                                                                : TemaEditor::RolColor::Error));
        ImGui::TextWrapped("%s", mensaje);
        ImGui::PopStyleColor();
    }

    if (!hayOrigenImportar) return;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped(model->traducir(Texto::OrigenImportar), origenImportar.carpeta.c_str());
    if (!origenImportar.valido()) {
        ImGui::PushStyleColor(ImGuiCol_Text, TemaEditor::color(TemaEditor::RolColor::Error));
        ImGui::TextWrapped("%s", model->traducir(textoProblemaImportar(origenImportar.problema)));
        ImGui::PopStyleColor();
        return;
    }

    // Nombre con el que entra: por defecto el del origen; si ya hay un
    // proyecto con ese nombre, el usuario escribe otro.
    ImGui::TextUnformatted(model->traducir(Texto::NombreEnProyectos));
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##nombreImportar", nombreImportarBuffer, sizeof(nombreImportarBuffer));
    const std::string nombre = nombreImportarBuffer;
    const char* aviso = nullptr;
    if (nombre.empty())
        aviso = model->traducir(Texto::NombreVacio);
    else if (!ProjectPaths::esNombreValido(nombre) || nombre[0] == '.')
        aviso = model->traducir(Texto::ImportarNombreInvalido);
    else if (model->existeProyecto(nombre))
        aviso = model->traducir(Texto::NombreRepetido);
    if (aviso) ImGui::TextDisabled("%s", aviso);

    ImGui::Spacing();
    if (aviso) ImGui::BeginDisabled();
    const bool importar = ImGui::Button(etiqueta(model, Texto::Importar).c_str(), ImVec2(140, 0));
    if (aviso) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(etiqueta(model, Texto::Cancelar).c_str(), ImVec2(140, 0)))
        hayOrigenImportar = false;
    if (importar && !aviso) {
        model->pedirImportacion(origenImportar.carpeta, nombre);
        hayOrigenImportar = false;
    }
}
