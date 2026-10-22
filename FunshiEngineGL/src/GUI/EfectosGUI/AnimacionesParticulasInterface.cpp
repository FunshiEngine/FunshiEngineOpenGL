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
#include "AnimacionesParticulasInterface.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <imgui.h>

#include "../../Assets/StbImageLoader.h"
#include "../../Configuracion/EditorConfig.h"
#include "../../Configuracion/ProjectPaths.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Prefab.h"
#include "../../Objetos/PrefabLibrary.h"
#include "../../Particulas/EscritorPerfil.h"
#include "../../Particulas/LectorPerfil.h"
#include "../../Scenes/EditorController.h"
#include "../WindowNames.h"

namespace {

bool esJson(const char* ruta) {
    std::string extension = std::filesystem::path(ruta).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".json";
}

bool esPrefab(const char* ruta) {
    std::string extension = std::filesystem::path(ruta).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".prefab";
}

}  // namespace

AnimacionesParticulasInterface::AnimacionesParticulasInterface()
    : GeneralUserInterface(WindowNames::AnimacionesParticulas, true,
                           ImGuiWindowFlags_None) {}

void AnimacionesParticulasInterface::printGUI() {
    const bool mostrar = ImGui::Begin(nameGUI.c_str(), &stateGUI, flagGUI);
    if (mostrar) contentGUI();
    ImGui::End();
}

void AnimacionesParticulasInterface::contentGUI() {
    if (ImGui::BeginTabBar("##pestanasEfectos")) {
        if (ImGui::BeginTabItem("Particulas")) {
            pestanaParticulas();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Animaciones")) {
            pestanaAnimaciones();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Prefabs")) {
            pestanaPrefabs();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // El resultado de la ultima operacion sirve en todas las pestañas: se
    // dibuja fuera de ellas para que no desaparezca al cambiar de pestana.
    if (!mensaje_.empty()) {
        ImGui::TextColored(
            mensajeError_ ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                          : ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
            "%s", mensaje_.c_str());
    }
}

void AnimacionesParticulasInterface::pestanaParticulas() {
    dibujarDatosGenerales();
    ImGui::Separator();
    dibujarCurvaTamanos();
    ImGui::Separator();
    dibujarCurvaColores();
    ImGui::Separator();
    dibujarZonaCarga();
}

void AnimacionesParticulasInterface::pestanaAnimaciones() {
    // El taller de animaciones 2D llegara cuando exista su formato de asset;
    // hoy la configuracion de una animacion vive en el inspector del
    // componente Animador2D (grilla, fps y rango de fotogramas).
    ImGui::TextWrapped("La configuracion de las animaciones 2D se hace en el "
                       "inspector del componente Animador2D: grilla de la "
                       "planilla, rango de fotogramas y fps.");
}

void AnimacionesParticulasInterface::pestanaPrefabs() {
    if (!editor_) {
        ImGui::TextDisabled("Abre una escena para trabajar con prefabs.");
        return;
    }
    PrefabLibrary* biblioteca = editor_->getPrefabLibrary();
    if (!biblioteca) {
        ImGui::TextDisabled("La escena no tiene biblioteca de prefabs.");
        return;
    }
    if (primeraVezPrefabs_) {
        biblioteca->recargar();
        primeraVezPrefabs_ = false;
    }

    // Instanciar arrastrando un .prefab desde el explorador de contenido. El
    // payload semantico llega solo para archivos .prefab; se acepta tambien
    // ARCHIVO_PATH con verificacion de extension por compatibilidad.
    ImGui::Button("Arrastra aqui un .prefab para instanciarlo");
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_ASSET");
        if (!payload) payload = ImGui::AcceptDragDropPayload("ARCHIVO_PATH");
        if (payload) {
            const char* rutaSoltada = static_cast<const char*>(payload->Data);
            if (rutaSoltada && *rutaSoltada && esPrefab(rutaSoltada))
                instanciarDesdeRuta(rutaSoltada);
            else if (rutaSoltada && *rutaSoltada)
                std::cerr << "[Efectos] el archivo arrastrado no es un prefab: "
                          << rutaSoltada << std::endl;
        }
        ImGui::EndDragDropTarget();
    }

    // Guardar el objeto seleccionado como prefab: su subarbol completo se
    // serializa a <raiz de assets>/Prefabs/<nombre>.prefab.
    ImGui::Separator();
    if (!editor_->getSelectedObject()) ImGui::BeginDisabled();
    if (ImGui::Button("Guardar la seleccion como prefab")) {
        std::snprintf(nombrePrefabBuf_, sizeof(nombrePrefabBuf_), "%s",
                      editor_->getSelectedObject()->inputName);
        ImGui::OpenPopup("GuardarPrefabPopup");
    }
    if (!editor_->getSelectedObject()) ImGui::EndDisabled();
    if (ImGui::BeginPopupModal("GuardarPrefabPopup", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Nombre", nombrePrefabBuf_, sizeof(nombrePrefabBuf_));
        if (ImGui::Button("Guardar", ImVec2(120, 0))) {
            const std::string nombre = nombrePrefabBuf_;
            if (!nombre.empty() && ProjectPaths::esNombreValido(nombre)) {
                biblioteca->crearPrefab(nombre, editor_->getSelectedObject(),
                                        editor_);
                biblioteca->recargar();
                mensaje_ = "Prefab guardado: " + nombre;
                mensajeError_ = false;
                ImGui::CloseCurrentPopup();
            } else {
                mensaje_ = "Nombre de prefab vacio o con caracteres no permitidos.";
                mensajeError_ = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Listado con las acciones sobre el prefab elegido.
    ImGui::Separator();
    ImGui::Text("Prefabs del proyecto");
    ImGui::SameLine();
    if (ImGui::SmallButton("Refrescar")) biblioteca->recargar();

    std::vector<std::string> nombres = biblioteca->listarNombres();
    std::sort(nombres.begin(), nombres.end());
    if (nombres.empty()) {
        ImGui::TextDisabled("No hay prefabs todavia.");
        prefabSeleccionado_.clear();
        return;
    }
    for (const std::string& nombre : nombres) {
        if (ImGui::Selectable(nombre.c_str(), prefabSeleccionado_ == nombre))
            prefabSeleccionado_ = nombre;
    }
    dibujarAccionesPrefab();
}

void AnimacionesParticulasInterface::dibujarAccionesPrefab() {
    if (prefabSeleccionado_.empty()) return;
    PrefabLibrary* biblioteca = editor_->getPrefabLibrary();
    if (!biblioteca) return;

    if (ImGui::Button("Instanciar")) instanciarPrefab(prefabSeleccionado_);

    ImGui::SameLine();
    ImGui::PushID("RenombrarPrefab");
    if (ImGui::Button("Renombrar")) {
        std::snprintf(nombrePrefabBuf_, sizeof(nombrePrefabBuf_), "%s",
                      prefabSeleccionado_.c_str());
        ImGui::OpenPopup("RenombrarPrefabPopup");
    }
    ImGui::PopID();
    if (ImGui::BeginPopupModal("RenombrarPrefabPopup", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Nuevo nombre", nombrePrefabBuf_,
                         sizeof(nombrePrefabBuf_));
        if (ImGui::Button("Renombrar", ImVec2(120, 0))) {
            const std::string nuevo = nombrePrefabBuf_;
            if (!nuevo.empty() && ProjectPaths::esNombreValido(nuevo) &&
                biblioteca->renombrar(prefabSeleccionado_, nuevo)) {
                prefabSeleccionado_ = nuevo;
                mensaje_ = "Prefab renombrado a " + nuevo;
                mensajeError_ = false;
                ImGui::CloseCurrentPopup();
            } else {
                mensaje_ = "No se pudo renombrar: nombre vacio, invalido o "
                           "ya en uso.";
                mensajeError_ = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::PushID("BorrarPrefab");
    if (ImGui::Button("Borrar")) ImGui::OpenPopup("BorrarPrefabPopup");
    ImGui::PopID();
    if (ImGui::BeginPopupModal("BorrarPrefabPopup", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        // Borrar quita el archivo .prefab del disco: la confirmacion evita
        // perder el efecto con un clic equivocado.
        ImGui::Text("Borrar '%s'? Se elimina el archivo del disco.",
                    prefabSeleccionado_.c_str());
        if (ImGui::Button("Borrar", ImVec2(120, 0))) {
            if (biblioteca->borrar(prefabSeleccionado_)) {
                mensaje_ = "Prefab borrado: " + prefabSeleccionado_;
                prefabSeleccionado_.clear();
                mensajeError_ = false;
            } else {
                mensaje_ = "No se pudo borrar el prefab.";
                mensajeError_ = true;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void AnimacionesParticulasInterface::instanciarPrefab(
    const std::string& nombre) {
    PrefabLibrary* biblioteca = editor_->getPrefabLibrary();
    if (!biblioteca) return;
    Prefab* prefab = biblioteca->obtener(nombre);
    if (!prefab) {
        mensaje_ = "No se encontro el prefab '" + nombre + "'.";
        mensajeError_ = true;
        return;
    }
    // La instancia nace en la raiz de la escena; el usuario la mueve donde
    // quiera (el gizmo y el inspector la dejan a tiro al seleccionarla).
    GameObject* instancia = prefab->instanciar(editor_, nullptr);
    if (!instancia) {
        mensaje_ = "No se pudo instanciar el prefab '" + nombre + "'.";
        mensajeError_ = true;
        return;
    }
    editor_->selectObject(instancia);
    mensaje_ = "Prefab instanciado: " + nombre;
    mensajeError_ = false;
}

void AnimacionesParticulasInterface::instanciarDesdeRuta(
    const std::string& ruta) {
    // Del archivo arrastrado se toma el nombre sin extension: es la clave con
    // la que la biblioteca carga y cachea los prefabs.
    const std::string archivo =
        std::filesystem::path(ruta).filename().string();
    if (archivo.size() <= 7) return;  // sin ".prefab" no hay nombre
    const std::string nombre = archivo.substr(0, archivo.size() - 7);
    if (nombre.empty()) {
        mensaje_ = "El nombre del prefab arrastrado esta vacio.";
        mensajeError_ = true;
        return;
    }
    instanciarPrefab(nombre);
}

void AnimacionesParticulasInterface::dibujarDatosGenerales() {
    // Nombre del efecto: es la etiqueta que el inspector muestra cuando el
    // emisor tiene el perfil cargado.
    char nombre[128];
    std::snprintf(nombre, sizeof(nombre), "%s", perfil_.nombre.c_str());
    if (ImGui::InputText("Nombre del efecto", nombre, sizeof(nombre)))
        perfil_.nombre = nombre;

    // Rango de vida y de velocidad, emparejados con el mismo acotado que usa
    // el inspector del emisor: el minimo nunca supera al maximo.
    if (ImGui::DragFloat("Vida minima", &perfil_.vidaMinima, 0.05f, 0.05f,
                         60.0f, "%.2f s") ||
        ImGui::DragFloat("Vida maxima", &perfil_.vidaMaxima, 0.05f, 0.05f,
                         60.0f, "%.2f s")) {
        if (perfil_.vidaMaxima < perfil_.vidaMinima)
            perfil_.vidaMaxima = perfil_.vidaMinima;
    }
    if (ImGui::DragFloat("Velocidad minima", &perfil_.velocidadMinima, 0.1f,
                         0.0f, 50.0f, "%.2f") ||
        ImGui::DragFloat("Velocidad maxima", &perfil_.velocidadMaxima, 0.1f,
                         0.0f, 50.0f, "%.2f")) {
        if (perfil_.velocidadMaxima < perfil_.velocidadMinima)
            perfil_.velocidadMaxima = perfil_.velocidadMinima;
    }
    ImGui::DragFloat3("Aceleracion", perfil_.aceleracion, 0.05f, -20.0f,
                      20.0f);
    ImGui::DragFloat("Rozamiento", &perfil_.rozamiento, 0.05f, 0.0f, 5.0f,
                     "%.2f");

    // Textura del efecto: relativa a la raiz de assets (el formato del perfil
    // es el mismo en el juego exportado). Se asigna arrastrando una imagen
    // desde el explorador de contenido, igual que en el emisor.
    ImGui::PushID("TexturaEfecto");
    ImGui::Button(perfil_.textura.empty() ? "[Arrastra una imagen]"
                                          : perfil_.textura.c_str());
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaSoltada = static_cast<const char*>(payload->Data);
            int ancho = 0, alto = 0;
            if (rutaSoltada && *rutaSoltada &&
                StbImageLoader::dimensiones(rutaSoltada, ancho, alto)) {
                perfil_.textura =
                    EditorConfig::relativizarRuta(rutaSoltada);
            } else if (rutaSoltada && *rutaSoltada) {
                std::cerr << "[Efectos] el archivo arrastrado no es una "
                             "imagen valida: "
                          << rutaSoltada << std::endl;
            }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    if (ImGui::Button("Quitar")) perfil_.textura.clear();
    ImGui::PopID();
}

void AnimacionesParticulasInterface::dibujarCurvaTamanos() {
    // Curva de multiplicadores de tamano: el primero al nacer y el ultimo al
    // morir; sin puntos el perfil no cambia el tamano. La cantidad se edita
    // con InputInt y cada punto con su propio deslizador.
    int puntos = static_cast<int>(perfil_.tamanos.size());
    ImGui::Text("Curva de tamano (del nacimiento a la muerte)");
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputInt("Puntos##tamanos", &puntos)) {
        if (puntos < 0) puntos = 0;
        if (puntos > 64) puntos = 64;
        perfil_.tamanos.resize(static_cast<std::size_t>(puntos), 1.0f);
    }
    for (std::size_t i = 0; i < perfil_.tamanos.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::DragFloat("##punto", &perfil_.tamanos[i], 0.01f, 0.0f, 10.0f,
                         "punto %d: %.2f");
        ImGui::PopID();
    }
}

void AnimacionesParticulasInterface::dibujarCurvaColores() {
    // Curva de colores RGBA, con la misma convencion que la de tamanos.
    int puntos = static_cast<int>(perfil_.colores.size());
    ImGui::Text("Curva de color (del nacimiento a la muerte)");
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputInt("Puntos##colores", &puntos)) {
        if (puntos < 0) puntos = 0;
        if (puntos > 64) puntos = 64;
        perfil_.colores.resize(static_cast<std::size_t>(puntos),
                               {1.0f, 1.0f, 1.0f, 1.0f});
    }
    for (std::size_t i = 0; i < perfil_.colores.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        char etiqueta[32];
        std::snprintf(etiqueta, sizeof(etiqueta), "punto %d", static_cast<int>(i));
        ImGui::ColorEdit4(etiqueta, perfil_.colores[i].data());
        ImGui::PopID();
    }
}

void AnimacionesParticulasInterface::dibujarZonaCarga() {
    // Retocar un efecto existente: arrastrar su .json lo trae al editor.
    ImGui::Button("Arrastra aqui un perfil .json para editarlo");
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
            const char* rutaSoltada = static_cast<const char*>(payload->Data);
            if (rutaSoltada && *rutaSoltada && esJson(rutaSoltada)) {
                PerfilParticula cargado;
                std::string error;
                if (cargarPerfilDesdeJson(rutaSoltada, cargado, error)) {
                    perfil_ = cargado;
                    std::snprintf(nombreArchivo_, sizeof(nombreArchivo_), "%s",
                                  cargado.nombre.c_str());
                    mensaje_ = "Perfil cargado desde el archivo.";
                    mensajeError_ = false;
                } else {
                    mensaje_ = "No es un perfil valido: " + error;
                    mensajeError_ = true;
                }
            } else if (rutaSoltada && *rutaSoltada) {
                std::cerr << "[Efectos] el archivo arrastrado no es un perfil "
                             "de efecto (.json): "
                          << rutaSoltada << std::endl;
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Generar: escribe <raiz de assets>/Particulas/<nombre>.json con el mismo
    // formato que lee el emisor, para que aparezca en el explorador de
    // contenido y pueda arrastrarse sobre un EmisorParticulas.
    ImGui::InputText("Nombre del archivo", nombreArchivo_,
                     sizeof(nombreArchivo_));
    if (ImGui::Button("Generar perfil")) generarPerfil();
}

void AnimacionesParticulasInterface::generarPerfil() {
    const std::string nombre = nombreArchivo_;
    if (nombre.empty()) {
        mensaje_ = "Escribe el nombre del archivo del efecto.";
        mensajeError_ = true;
        return;
    }
    if (!ProjectPaths::esNombreValido(nombre)) {
        mensaje_ = "El nombre del archivo tiene caracteres no permitidos.";
        mensajeError_ = true;
        return;
    }
    const std::string raiz = EditorConfig::raizAssetsFijada();
    if (raiz.empty()) {
        mensaje_ = "Abre un proyecto para generar perfiles.";
        mensajeError_ = true;
        return;
    }
    const std::string ruta =
        (std::filesystem::path(raiz) / "Particulas" / (nombre + ".json"))
            .string();
    if (std::filesystem::exists(ruta)) {
        mensaje_ = "Ya existe un efecto con ese nombre: elige otro.";
        mensajeError_ = true;
        return;
    }
    std::string error;
    if (!guardarPerfilAJson(ruta, perfil_, error)) {
        mensaje_ = "No se pudo generar el perfil: " + error;
        mensajeError_ = true;
        return;
    }
    mensaje_ = "Perfil generado en " +
               EditorConfig::relativizarRuta(ruta) +
               " (aparece en el explorador de contenido).";
    mensajeError_ = false;
}
