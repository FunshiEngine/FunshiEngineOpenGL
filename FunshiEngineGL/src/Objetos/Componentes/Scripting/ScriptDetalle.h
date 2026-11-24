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
// Preambulo compartido por las unidades de Script.cpp (generado al dividirlo).
#ifndef SCRIPTDETALLE_H
#define SCRIPTDETALLE_H

#include "Script.h"
#include "../Behaviour/ComportamientoCargado.h"
#include "../Behaviour/IScriptBehaviour.h"
#include "../Behaviour/ScriptGameObject.h"
#include "../Behaviour/ScriptRuntime.h"
#include "Configuracion/EditorConfig.h"
#include "../Objetos/GameObject.h"
#include "Diagnosticos/RegistroDiagnosticos.h"
#include "Herramientas/LimitesSerializacion.h"
#include <iostream>
#include <utility>

namespace {

constexpr uint32_t MAGIC_SCRIPT = 0x31535346; // 'F','S','S','1'

constexpr uint32_t VERSION_SCRIPT = 3;

ReflejoScripts::DefCampo metadatosCampo(
    const ReflejoScripts::DefCampo& campo) {
    ReflejoScripts::DefCampo copia;
    copia.nombre = campo.nombre;
    copia.tag = campo.tag;
    copia.subcampos.reserve(campo.subcampos.size());
    for (const ReflejoScripts::DefCampo& subcampo : campo.subcampos)
        copia.subcampos.push_back(metadatosCampo(subcampo));
    return copia;
}

std::vector<ReflejoScripts::DefCampo> metadatosCampos(
    const std::vector<ReflejoScripts::DefCampo>& campos) {
    std::vector<ReflejoScripts::DefCampo> copia;
    copia.reserve(campos.size());
    for (const ReflejoScripts::DefCampo& campo : campos)
        copia.push_back(metadatosCampo(campo));
    return copia;
}

}  // namespace

#endif
