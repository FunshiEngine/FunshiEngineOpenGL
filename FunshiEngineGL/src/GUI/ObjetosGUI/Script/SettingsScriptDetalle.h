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
// Preambulo compartido por las unidades de SettingsScript.cpp (generado al dividirlo).
#ifndef SETTINGSSCRIPTDETALLE_H
#define SETTINGSSCRIPTDETALLE_H

#include "SettingsScript.h"
#include "../../Tema/TemaEditor.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include "../../../Behaviour/Reflection/BehaviourReflection.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Scripting/Script.h"
#include "../../../Herramientas/TypeUtils.h"
#include <imgui.h>

namespace {

// Edita una fila: etiqueta (que es el boton del TreeNodeEx si corresponde) y
// el widget sin etiqueta. Reusa el mismo valor escalar para members de arrays.

}  // namespace

// Auxiliares repartidas entre las unidades (enlace externo).

bool editarReferenciaObjeto(std::string& nombre);
bool editarReferenciaComponente(std::string& nombre, const std::string& tipo);
bool editarScalar(ReflejoScripts::ValorCampo& valor);

#endif
