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
#include "ColocacionInterfaz.h"

namespace ColocacionInterfaz {

void fraccionAncla(AnclaInterfaz ancla, float& fx, float& fy) {
    const int indice = static_cast<int>(ancla);
    // Los enumeradores van en filas de tres: arriba, medio, abajo.
    const int columna = (indice >= 0 && indice < 9) ? indice % 3 : 1;
    const int fila = (indice >= 0 && indice < 9) ? indice / 3 : 1;
    fx = 0.5f * static_cast<float>(columna);
    fy = 0.5f * static_cast<float>(fila);
}

RectInterfaz calcular(AnclaInterfaz ancla, float desplazamientoX,
                      float desplazamientoY, float anchoVista, float altoVista,
                      float anchoContenedor, float altoContenedor) {
    float fx = 0.5f, fy = 0.5f;
    fraccionAncla(ancla, fx, fy);
    RectInterfaz r;
    r.ancho = anchoVista;
    r.alto = altoVista;
    r.x = (anchoContenedor - anchoVista) * fx + desplazamientoX;
    r.y = (altoContenedor - altoVista) * fy + desplazamientoY;
    return r;
}

}  // namespace ColocacionInterfaz
