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
// Preambulo compartido por las unidades de EditorInput.cpp (generado al dividirlo).
#ifndef EDITORINPUTDETALLE_H
#define EDITORINPUTDETALLE_H

#define GLFW_INCLUDE_NONE
#include "EditorInput.h"
#include <cmath>
#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include "../Scenes/GameScene.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/Componentes/Escena/CameraComponent.h"
#include "../States/ApplicationStateMachine.h"
#include "../States/OrquestadorEstadoGUI.h"
#include "../Scenes/Editor/OperacionGizmo.h"
#include "../Scenes/Editor/EditorController.h"
#include "../Scenes/Registro/SceneRegistry.h"
#include "../Comandos/Objetos/BorrarObjetoComando.h"
#include "AtajosEditor.h"

namespace {

void trazarTeclaDescartada(const char* tecla, const std::string& motivo) {
    std::cout << "[input] tecla " << tecla << " descartada: " << motivo
              << std::endl;
}

}  // namespace

#endif
