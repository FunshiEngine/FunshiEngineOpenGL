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
#ifndef CATALOGOICONOS_H
#define CATALOGOICONOS_H

#include <string>

#include "IResolutorIconos.h"

class IconosGUI;
class IReglaIcono;

// Resolvedor de iconos de componente.
//
// Une las dos piezas: la regla dice que archivo corresponde a cada tipo de
// componente y el gestor de iconos lo carga en la GPU (a demanda). La GUI
// solo ve este contrato.
class CatalogoIconos : public IResolutorIconos {
public:
    CatalogoIconos(IconosGUI& iconos, const IReglaIcono& regla);

    // Textura del icono de un tipo de componente. Devuelve
    // ImTextureID_Invalid si el tipo no tiene icono o si la imagen no se pudo
    // cargar.
    ImTextureID iconoPara(const std::string& tipo) const override;

private:
    IconosGUI* iconos_;
    const IReglaIcono* regla_;
};

#endif // CATALOGOICONOS_H
