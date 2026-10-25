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
#ifndef IREGLAICONO_H
#define IREGLAICONO_H

#include <string>

// Regla que dice que icono corresponde a cada tipo de componente.
//
// Es un contrato aparte del que resuelve la textura: la regla es CPU pura
// (solo decide una ruta) y se puede probar en headless, mientras que resolver
// la textura necesita el backend grafico. La GUI consume el resolvedor, que
// por dentro usa la regla.
class IReglaIcono {
public:
    virtual ~IReglaIcono() = default;

    // Ruta del icono de un tipo de componente, relativa a la carpeta de
    // imagenes del motor (por ejemplo
    // "GUI-Image/Component-Image/Transform/comp-transform-white.png").
    // Devuelve una cadena vacia si el tipo no tiene icono.
    virtual std::string rutaIconoPara(const std::string& tipo) const = 0;
};

#endif // IREGLAICONO_H
