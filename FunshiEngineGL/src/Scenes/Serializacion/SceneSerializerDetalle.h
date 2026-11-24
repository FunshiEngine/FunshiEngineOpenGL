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
// Preambulo compartido por las unidades de SceneSerializer.cpp (generado al dividirlo).
#ifndef SCENESERIALIZERDETALLE_H
#define SCENESERIALIZERDETALLE_H

#include "SceneSerializer.h"
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "../Editor/EditorController.h"
#include "../Registro/SceneRegistry.h"
#include "../../Diagnosticos/RegistroDiagnosticos.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Render/Model.h"
#include "../../Objetos/ObjetoEscena.h"

namespace {

void recolectarIdsArbol(Position<GameObject*>* root,
                        std::unordered_set<int>& ids) {
    (void)root;
    (void)ids;
}

void publicarFalloEscena(RegistroDiagnosticos* diagnosticos,
                         const std::string& codigo,
                         const std::string& mensaje,
                         NivelDiagnostico nivel) {
    if (diagnosticos) {
        registrarFalloDiagnostico(*diagnosticos, "Scenes/SceneSerializer",
                                  codigo, mensaje, nivel);
    } else {
        registrarFalloDiagnostico("Scenes/SceneSerializer", codigo, mensaje,
                                  nivel);
    }
}

/*
 * getline que ademas recorta el '\r' final de los archivos con CRLF.
 *
 * SceneBBDDObjetos.txt se abre en BINARIO (ver load()): la traduccion de
 * CRLF del CRT en modo texto rompe los offsets de tellg/seekg del
 * look-ahead (la releitura de la siguiente linea arranca en un offset
 * erroneo y sale truncada, ver DocuTecnicoBugs.md). En binario los offsets
 * son bytes exactos y el '\r' queda en la linea: se recorta aca.
 */
bool getlineLimpio(std::ifstream& file, std::string& line) {
    if (!std::getline(file, line))
        return false;
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    return true;
}

}  // namespace

#endif
