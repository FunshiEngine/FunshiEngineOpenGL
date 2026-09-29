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
#ifndef DIFUMINADO_H
#define DIFUMINADO_H

#include "../Configuracion/Apariencia.h"

// Difuminado radial del piso del editor: opacidad plena hasta "inicio" y caida
// cuadratica hasta 0 en "fin", que es ademas el radio del circulo-horizonte y
// por lo tanto el LIMITE DE DIBUJADO (mas alla no se pinta nada). La grilla y
// la guia de eje comparten esta MISMA struct y esta misma curva: por eso un
// cambio de radio se ve en las dos a la vez y el degradado no puede quedar
// desincronizado entre ellas.
//
// El radio lo elige el usuario (Opciones) y es lo unico que se persiste: el
// inicio se DERIVA de el en proporcion constante (kProporcionInicio, la misma
// que daba 40 sobre el radio historico de 150), asi que agrandar el circulo
// agranda el degradado en vez de estirarlo. El radio se acota aqui mismo, antes
// de derivar, para que un valor corrupto en la configuracion no pueda dejar la
// grilla sin horizonte ni producir una division por cero en la curva.
//
// Modulo CPU puro (nada de OpenGL ni glm) para poder ejercitarse en los
// targets headless, como LineBuilder y GuiaEje.
struct Difuminado {
    // Fraccion del radio que queda completamente opaca (40 de 150 en el
    // diseno original).
    static constexpr float kProporcionInicio = 40.0f / 150.0f;

    float inicio = AparienciaUtil::kRadioDifuminadoPorDefecto * kProporcionInicio;
    float fin = AparienciaUtil::kRadioDifuminadoPorDefecto;
    int subdivisiones = 16;

    // Difuminado para el radio pedido, con las subdivisiones que elija el
    // llamador (la grilla necesita menos trozos que la guia de eje: su recta
    // es mucho mas larga para la misma distancia). Un radio no finito cae en el
    // valor por defecto y uno fuera de rango se acota al minimo o al maximo.
    static Difuminado desdeRadio(float radio, int subdivisiones);

    // Opacidad a "distancia" de la camara: 1.0 en la zona central, 0 en el
    // horizonte, y nunca negativa ni mayor que 1.
    float opacidad(float distancia) const;
};

#endif // DIFUMINADO_H
