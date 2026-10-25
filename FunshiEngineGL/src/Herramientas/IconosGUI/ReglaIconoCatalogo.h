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
#ifndef REGLAICONOCATALOGO_H
#define REGLAICONOCATALOGO_H

#include <string>

#include "IReglaIcono.h"

// Regla que toma el icono del catalogo de componentes.
//
// El catalogo es la fuente unica: cada componente declara ahi su icono, asi
// que no hay una segunda tabla de tipos que pueda desincronizarse.
class ReglaIconoCatalogo : public IReglaIcono {
public:
    // Ruta del icono de un tipo de componente, vacia si el tipo no esta en el
    // catalogo o si el componente no declara icono. Acepta el alias
    // retrocompatible (por ejemplo "Camera" por "CameraComponent").
    std::string rutaIconoPara(const std::string& tipo) const override;
};

#endif // REGLAICONOCATALOGO_H
