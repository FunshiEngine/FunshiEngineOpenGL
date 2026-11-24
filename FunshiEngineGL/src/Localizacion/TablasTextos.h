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
#ifndef LOCALIZACION_TABLASTEXTOS_H
#define LOCALIZACION_TABLASTEXTOS_H

#include <vector>

// Tablas de textos por idioma (uso interno del modulo Localizacion). Cada
// idioma vive en su propio archivo de Textos/: agregar un idioma es agregar un
// archivo y una fila en la lista de idiomas, sin tocar los demas.
namespace Localizacion {
namespace Detalle {

struct EntradaTexto {
    const char* clave;
    const char* texto;
};

const std::vector<EntradaTexto>& textosEs();
const std::vector<EntradaTexto>& textosEn();

} // namespace Detalle
} // namespace Localizacion

#endif
