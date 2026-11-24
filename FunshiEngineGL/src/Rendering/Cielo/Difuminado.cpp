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
#include "Difuminado.h"

#include <cmath>

Difuminado Difuminado::desdeRadio(float radio, int subdivisiones) {
    // El acotado va ANTES de derivar el inicio: es lo que garantiza que
    // siempre haya horizonte (fin > inicio > 0) y que la curva no divida por
    // cero, sin importar que traiga el archivo de configuracion.
    if (!std::isfinite(radio))
        radio = AparienciaUtil::kRadioDifuminadoPorDefecto;
    if (radio < AparienciaUtil::kRadioDifuminadoMinimo)
        radio = AparienciaUtil::kRadioDifuminadoMinimo;
    if (radio > AparienciaUtil::kRadioDifuminadoMaximo)
        radio = AparienciaUtil::kRadioDifuminadoMaximo;

    Difuminado d;
    d.fin = radio;
    d.inicio = radio * kProporcionInicio;
    d.subdivisiones = subdivisiones > 0 ? subdivisiones : 1;
    return d;
}

float Difuminado::opacidad(float distancia) const {
    // Una distancia no finita no tiene un lugar donde situarse en la curva: se
    // trata como el punto mas cercano (opaco) en vez de propagar el NaN al
    // alpha del vertice.
    if (!std::isfinite(distancia)) return 1.0f;
    const float rango = fin - inicio;
    if (rango <= 0.0f)
        return distancia <= inicio ? 1.0f : 0.0f;

    // t se acota a [0,1] ANTES de elevar al cuadrado: sin ese recorte, los
    // puntos mas cercanos que "inicio" dan t negativo y el cuadrado los baja de
    // opacos, con lo que hasta el centro de la vista se veria translucido.
    float t = (distancia - inicio) / rango;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const float a = 1.0f - t * t;
    return a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
}
