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
// Preambulo compartido por las unidades de BehaviourReflection.cpp (generado al dividirlo).
#ifndef BEHAVIOURREFLECTIONDETALLE_H
#define BEHAVIOURREFLECTIONDETALLE_H

#include "BehaviourReflection.h"
#include <algorithm>
#include "../../Objetos/GameObject.h"
#include "../../Herramientas/LimitesSerializacion.h"

namespace ReflejoScripts {

namespace {

void escribirTag(std::ofstream& out, TagTipo tag) {
    const uint8_t byte = static_cast<uint8_t>(tag);
    out.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
}

// Lee UN valor por tag (supone escribirTag/guardarValoresCampos del llamador).
// El arbol se reconstruye de forma autodescriptiva (los grupos llevan sus
// valores hijo con nombre + tag).
// Tope de anidamiento de los grupos reflejados. Sin el, un archivo corrupto
// con grupos profundamente anidados desborda la pila al leerlos.
constexpr unsigned kProfundidadMaxima =
    static_cast<unsigned>(LimitesSerializacion::kProfundidad);

}  // namespace

}  // namespace ReflejoScripts

#endif
