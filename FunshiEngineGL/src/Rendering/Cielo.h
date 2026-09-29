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
#ifndef CIELO_H
#define CIELO_H

#include "../Configuracion/Apariencia.h"

// Modulo CPU puro para el cielo degradado del editor: dado el perfil de
// apariencia, expone los dos colores del degradado (superior/inferior) ya
// resueltos con la logica B/N y tema. Lo usan el renderer del cielo y las
// pruebas headless sin necesidad de OpenGL ni ImGui.
namespace Cielo {

// Colores efectivos del degradado (superior e inferior del viewport).
// En modo B/N ambas partes se fuerzan a blanco (tema claro) o negro (tema
// oscuro). En modo normal usan Apariencia::fondoSuperior/Inferior.
inline void coloresEfectivos(const Apariencia& ap, float sup[3], float inf[3]) {
    AparienciaUtil::fondoEfectivoSuperior(ap, sup);
    AparienciaUtil::fondoEfectivoInferior(ap, inf);
}

} // namespace Cielo

#endif // CIELO_H