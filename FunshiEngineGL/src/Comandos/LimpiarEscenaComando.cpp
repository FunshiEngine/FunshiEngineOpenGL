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
#include "LimpiarEscenaComando.h"
#include "Scenes/SceneRegistry.h"
#include "Objetos/GameObject.h"
#include <sstream>

LimpiarEscenaComando::LimpiarEscenaComando(EditorController* ec,
                                           SceneRegistry* sr)
    : editorController(ec), sceneRegistry(sr) {}

void LimpiarEscenaComando::ejecutar() {
    if (!sceneRegistry) return;

    objetosEliminados = sceneRegistry->takeAllNonRoot();
}

void LimpiarEscenaComando::deshacer() {
    if (!sceneRegistry || objetosEliminados.empty()) return;

    GameObject* root = sceneRegistry->getRoot();
    if (!root) return;

    sceneRegistry->restoreSubtree(std::move(objetosEliminados), root);
}

std::string LimpiarEscenaComando::descripcion() const {
    return "Limpiar escena";
}
