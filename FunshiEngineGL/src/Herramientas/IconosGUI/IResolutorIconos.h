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
#ifndef IRESOLUTORICONOS_H
#define IRESOLUTORICONOS_H

#include <imgui.h>
#include <string>

// Resuelve el icono de un tipo de componente a la textura que la GUI pinta.
//
// La GUI depende de este contrato y no de quien carga los PNG: asi el
// inspector no conoce ni el backend grafico ni el gestor de iconos.
class IResolutorIconos {
public:
    virtual ~IResolutorIconos() = default;

    // Textura del icono de un tipo de componente. Devuelve
    // ImTextureID_Invalid si el tipo no tiene icono o si la imagen no se pudo
    // cargar.
    virtual ImTextureID iconoPara(const std::string& tipo) const = 0;
};

#endif // IRESOLUTORICONOS_H
