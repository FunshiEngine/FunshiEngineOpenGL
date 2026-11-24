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
#ifndef SHADERSOURCES_H
#define SHADERSOURCES_H

// Fuentes de los shaders incorporados al motor.
//
// Van embebidas en el binario (no en archivos): el ejecutable exportado no
// puede depender de assets extra. Solo las incluye el gestor de shaders,
// que es el dueno de los programas: el resto del motor pide el programa por
// nombre y no necesita saber de donde sale el GLSL.
//
// Las variables son inline (C++17) para que exista una sola copia en todo el
// programa, no una por unidad de traduccion que las incluya.

#include "ShaderMalla.h"
#include "ShaderLineas.h"
#include "Shader2D.h"
#include "ShaderCielo.h"
#include "ShaderSombras.h"

#endif
