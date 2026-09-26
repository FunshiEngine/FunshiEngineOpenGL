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
#include "GrillaRenderer.h"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Configuracion/Apariencia.h"
#include "Backend/IRenderBackend.h"
#include "LineRenderer.h"

namespace {

// ---------------------------------------------------------------------------
// Constantes de diseño (NO configurables por el usuario desde la GUI):
// - Densidad, horizonte y difuminado viven en GrillaRenderer.h (publicas) porque
//   la guia de eje los reusa; lo que queda aca es privado de este archivo.
// - El horizonte es un CIRCULO de radio kFadeFin centrado en la camara (sobre
//   el plano del suelo) que actua COMO LIMITE DE DIBUJADO: las lineas se
//   recortan a lo que queda dentro del circulo (nada mas alla se pinta, dando
//   la ilusion de que la grilla continua) y se difuminan radialmente por
//   vertice (opacas cerca de la camara, transparentes en el borde). Como el
//   circulo persigue a la camara, moverse pinta grilla nueva por delante y
//   deja de pintar lo que queda atras.
// ---------------------------------------------------------------------------
constexpr float kEjeYLongitud = 70.0f;   // longitud del eje perpendicular (Y)

// Empuja un segmento de la grilla con su alpha por extremo: el batch de lineas
// interpola de un color al otro a lo largo del segmento, que es el difuminado.
void agregarSegmentoRGBA(LineBuilder& out, const float color[3], float ax,
                         float az, float alphaA, float bx, float bz,
                         float alphaB) {
    const float a[3] = {ax, 0.0f, az};
    const float b[3] = {bx, 0.0f, bz};
    const float rgbaA[4] = {color[0], color[1], color[2], alphaA};
    const float rgbaB[4] = {color[0], color[1], color[2], alphaB};
    out.agregarSegmento(a, b, rgbaA, rgbaB);
}

// Opacidad del difuminado radial a distancia horizontal 'd' de la camara: 1.0
// en la zona central y caida cuadratica hasta 0.0 en el borde del circulo.
float alphaDifuminado(float d) {
    // t se acota a [0,1] ANTES de elevar al cuadrado: sin ese recorte, los
    // puntos mas cercanos que kFadeInicio dan t negativo y el cuadrado los baja
    // de opacos, con lo que hasta el centro de la vista se veria translucido.
    float t = (d - GrillaRenderer::kFadeInicio) /
              (GrillaRenderer::kFadeFin - GrillaRenderer::kFadeInicio);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const float a = 1.0f - t * t;
    return a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
}

// Emite una linea de la grilla recortada por el circulo horizonte y con
// difuminado radial POR VERTICE. 'fija' es la coordenada X (si variaZ, linea
// paralela a Z) o Z (si no, linea paralela a X). Si la linea queda fuera del
// circulo no se pinta nada (ese circulo es el limite de dibujado). Los
// extremos del trozo interior caen en el borde (alpha 0) y cada vertice
// intermedio lleva el alpha de su propia distancia radial a la camara, por eso
// la linea se subdivide en kSubdivisiones trozos.
void emitirLineaPlano(LineBuilder& out, const float color[3], float fija,
                      float camX, float camZ, bool variaZ) {
    const float d = std::fabs(fija - (variaZ ? camX : camZ));
    if (d >= GrillaRenderer::kFadeFin)
        return; // fuera del circulo: el difuminado es el limite
    const float h = std::sqrt(GrillaRenderer::kFadeFin *
                                  GrillaRenderer::kFadeFin -
                              d * d);
    const float t0 = (variaZ ? camZ : camX) - h;
    const float t1 = (variaZ ? camZ : camX) + h;
    const float largo = t1 - t0;
    for (int k = 0; k < GrillaRenderer::kSubdivisiones; ++k) {
        const float ta =
            t0 + largo * (static_cast<float>(k) / GrillaRenderer::kSubdivisiones);
        const float tb = t0 + largo * (static_cast<float>(k + 1) /
                                        GrillaRenderer::kSubdivisiones);
        float xa, za, xb, zb;
        if (variaZ) {
            xa = fija;
            za = ta;
            xb = fija;
            zb = tb;
        } else {
            xa = ta;
            za = fija;
            xb = tb;
            zb = fija;
        }
        const float da = std::sqrt((xa - camX) * (xa - camX) +
                                   (za - camZ) * (za - camZ));
        const float db = std::sqrt((xb - camX) * (xb - camX) +
                                   (zb - camZ) * (zb - camZ));
        const float aa = alphaDifuminado(da);
        const float ab = alphaDifuminado(db);
        agregarSegmentoRGBA(out, color, xa, za, aa, xb, zb, ab);
    }
}

// Colores base de los ejes (X rojo, Z verde, Y amarillo). Se pasan por
// ejeContraste contra el color efectivo de la grilla para que siempre se vean.
const float kEjeBaseRojo[3] = {1.0f, 0.3f, 0.3f};
const float kEjeBaseVerde[3] = {0.3f, 1.0f, 0.3f};
const float kEjeBaseAmarillo[3] = {1.0f, 1.0f, 0.3f};

} // namespace

void GrillaRenderer::dibujar(const float model[16], const float colorGrilla[3],
                             const float camaraMundo[3]) {
    if (!model || !colorGrilla || !camaraMundo) return;

    // La camara se transforma al espacio local del objeto "Grilla": el circulo
    // horizonte, la densidad fija y el difuminado siguen al objeto (que puede
    // moverse/escalarse/rotarse como cualquier otro componente).
    const glm::mat4 modelMat = glm::make_mat4(model);
    const glm::vec4 camaraLocal =
        glm::inverse(modelMat) *
        glm::vec4(camaraMundo[0], camaraMundo[1], camaraMundo[2], 1.0f);
    const float camX = camaraLocal.x;
    const float camZ = camaraLocal.z;

    secundario_.limpiar();
    principal_.limpiar();
    ejes_.limpiar();

    // Lineas del plano dentro del circulo de radio kFadeFin alrededor de la
    // camara, ancladas a multiplos exactos de kSeparacionMenor (no se desplazan al
    // moverse la camara: simplemente entran y salen del circulo). Cada
    // kMultiploMayor secundarias -> principal (mismo color, solo mas ancha). La
    // linea por el origen (i/j == 0) se salta: la pintan los ejes X/Z.
    const int iIni = static_cast<int>(std::ceil((camX - kFadeFin) / kSeparacionMenor));
    const int iFin = static_cast<int>(std::floor((camX + kFadeFin) / kSeparacionMenor));
    for (int i = iIni; i <= iFin; ++i) {
        if (i == 0) continue;
        const float x = static_cast<float>(i) * kSeparacionMenor;
        emitirLineaPlano((i % kMultiploMayor == 0) ? principal_ : secundario_,
                         colorGrilla, x, camX, camZ, true);
    }
    const int jIni = static_cast<int>(std::ceil((camZ - kFadeFin) / kSeparacionMenor));
    const int jFin = static_cast<int>(std::floor((camZ + kFadeFin) / kSeparacionMenor));
    for (int j = jIni; j <= jFin; ++j) {
        if (j == 0) continue;
        const float z = static_cast<float>(j) * kSeparacionMenor;
        emitirLineaPlano((j % kMultiploMayor == 0) ? principal_ : secundario_,
                         colorGrilla, z, camX, camZ, false);
    }

    // Ejes X y Z: paralelos al plano a traves del origen, recortados y
    // difuminados por el mismo circulo (mismo tratamiento que una linea de la
    // grilla, pero en su color). Eje Y perpendicular solo hacia arriba: no vive
    // en el plano y no lo recorta el circulo; se difumina con la distancia
    // horizontal de la camara al origen.
    float colorEjeX[3], colorEjeZ[3], colorEjeY[3];
    AparienciaUtil::ejeContraste(kEjeBaseRojo, colorGrilla, colorEjeX);
    AparienciaUtil::ejeContraste(kEjeBaseVerde, colorGrilla, colorEjeZ);
    AparienciaUtil::ejeContraste(kEjeBaseAmarillo, colorGrilla, colorEjeY);

    emitirLineaPlano(ejes_, colorEjeX, 0.0f, camX, camZ, false);
    emitirLineaPlano(ejes_, colorEjeZ, 0.0f, camX, camZ, true);
    {
        const float alphaEjeY =
            alphaDifuminado(std::sqrt(camX * camX + camZ * camZ));
        const float rgbaEjeY[4] = {colorEjeY[0], colorEjeY[1], colorEjeY[2],
                                   alphaEjeY};
        const float puntosEjeY[6] = {0.0f, 0.0f, 0.0f, 0.0f, kEjeYLongitud, 0.0f};
        ejes_.agregarPolilinea(puntosEjeY, 2, false, rgbaEjeY);
    }

    // El difuminado se funde con el fondo: hace falta blending durante la
    // grilla (y hay que restaurarlo, la pasada de objetos espera el estado base).
    auto& backend = Rendering::Backend::activeBackend();
    backend.setBlendEnabled(true);

    // Secundarias (1px), principales (2px) y ejes (3px): mismo color efectivo,
    // solo cambia el ancho, que el shader resuelve en pixeles. El alpha radial ya
    // viene por vertice (cerca opaco, borde 0).
    auto& lineas = lineRenderer();
    lineas.dibujar(secundario_, secundarioBatch_, model, 1.0f);
    lineas.dibujar(principal_, principalBatch_, model, 2.0f);
    lineas.dibujar(ejes_, ejesBatch_, model, 3.0f);

    backend.setBlendEnabled(false);
}

void GrillaRenderer::destruir() {
    secundario_.limpiar();
    principal_.limpiar();
    ejes_.limpiar();
}
