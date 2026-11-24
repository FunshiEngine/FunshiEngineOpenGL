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
// Preambulo compartido por las unidades de RenderingTests.cpp (generado al dividirlo).
#ifndef RENDERINGTESTSDETALLE_H
#define RENDERINGTESTSDETALLE_H

// Geometria de lineas del pipeline moderno (LineBuilder: la expansion de cada
// segmento al quad que dibuja kLineVertexShader) y calculo de la recta de la
// guia de eje (GuiaEje) sin pila grafica ni GL: solo std C++17.

#include <cmath>
#include <iostream>
#include <string>
#include "../FunshiEngineGL/src/Assets/Mallas/Primitivas2D.h"
#include "../FunshiEngineGL/src/Configuracion/Apariencia.h"
#include "../FunshiEngineGL/src/Rendering/Cielo/CacheCubemap.h"
#include "../FunshiEngineGL/src/Rendering/Cielo/Cielo.h"
#include "../FunshiEngineGL/src/Rendering/Cielo/Difuminado.h"
#include "../FunshiEngineGL/src/Rendering/Lineas/GuiaEje.h"
#include "../FunshiEngineGL/src/Rendering/Lineas/LineBuilder.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/ShaderSources.h"
#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

using LineBuilder = ::LineBuilder;

// Difuminado de la guia: el MISMO tipo y el mismo radio que usa la grilla
// (Difuminado, en Difuminado.h), para que la recta se desvanezca en el mismo
// horizonte que el piso.
constexpr Difuminado kDif = {40.0f, 150.0f, 16};

constexpr float kEps = 1e-4f;

// Estado de la unidad original: una sola instancia (se define en RenderingTests.cpp).

namespace RenderingTestsDetalle {

extern int total;
extern int fallos;

}  // namespace RenderingTestsDetalle
using namespace RenderingTestsDetalle;

// Funciones repartidas entre las unidades.

const float* vertice(const LineBuilder& b, std::size_t v);
void testConstantes();
void testVacio();
void testSegmento();
void testColorPorExtremo();
void testPolilinea();
void testAristas();
void testCaja();
void testAcumulacion();
void matrizTraslada(float tx, float ty, float tz, float m[16]);
bool cerca(float a, float b);
void testGuiaGlobal();
void testGuiaLocal();
void testGuiaIgnoraEscala();
void testOpacidad();
void testEmiteHastaElHorizonte();
void testDifuminadoPorVertice();
void testGuiaDefensiva();
void testEmiteDefensiva();
void testGuiaColor();
void testGuiaColorEfectivo();
void testDifuminadoDesdeRadio();
void testDifuminadoAcotaElRadio();
void testDifuminadoOpacidad();
void testShaderLineasRespetaHorizonteRadial();
void testCieloColoresEfectivos();
void testCacheCubemapClave();
void testPrimitivaQuad2D();
void testShaderSprites();

#endif
