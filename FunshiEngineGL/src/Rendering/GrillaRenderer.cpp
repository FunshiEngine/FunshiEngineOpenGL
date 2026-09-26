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

namespace {

// ---------------------------------------------------------------------------
// Constantes de diseño (NO configurables por el usuario desde la GUI):
// - Densidad FIJA: secundarias cada 1 unidad, principales cada 5.
// - El horizonte es un CIRCULO de radio kFadeFin centrado en la camara (sobre
//   el plano del suelo) que actua COMO LIMITE DE DIBUJADO: las lineas se
//   recortan a lo que queda dentro del circulo (nada mas alla se pinta, dando
//   la ilusion de que la grilla continua) y se difuminan radialmente por
//   vertice (opacas cerca de la camara, transparentes en el borde). Como el
//   circulo persigue a la camara, moverse pinta grilla nueva por delante y
//   deja de pintar lo que queda atras.
// ---------------------------------------------------------------------------
constexpr float kSepMenor = 1.0f;        // separacion fija de las secundarias
constexpr int   kMultiploMayor = 5;      // cada 5 secundarias -> una principal
constexpr float kFadeInicio = 40.0f;     // opacidad plena hasta aqui
constexpr float kFadeFin = 150.0f;       // radio del circulo-horizonte (limite)
constexpr int   kSubdivisiones = 6;      // trozos por linea para el difuminado
constexpr float kEjeYLongitud = 70.0f;   // longitud del eje perpendicular (Y)

// Empuja un vertice intercalado xyz+rgba (7 floats).
void agregarVerticeRGBA(std::vector<float>& out, const float color[3], float x,
                        float y, float z, float alpha) {
    out.push_back(x);
    out.push_back(y);
    out.push_back(z);
    out.push_back(color[0]);
    out.push_back(color[1]);
    out.push_back(color[2]);
    out.push_back(alpha);
}

// Opacidad del difuminado radial a distancia horizontal 'd' de la camara: 1.0
// en la zona central y caida cuadratica hasta 0.0 en el borde del circulo.
float alphaDifuminado(float d) {
    const float t = (d - kFadeInicio) / (kFadeFin - kFadeInicio);
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
void emitirLineaPlano(std::vector<float>& out, const float color[3], float fija,
                      float camX, float camZ, bool variaZ) {
    const float d = std::fabs(fija - (variaZ ? camX : camZ));
    if (d >= kFadeFin) return; // fuera del circulo: el difuminado es el limite
    const float h = std::sqrt(kFadeFin * kFadeFin - d * d);
    const float t0 = (variaZ ? camZ : camX) - h;
    const float t1 = (variaZ ? camZ : camX) + h;
    const float largo = t1 - t0;
    for (int k = 0; k < kSubdivisiones; ++k) {
        const float ta = t0 + largo * (static_cast<float>(k) / kSubdivisiones);
        const float tb =
            t0 + largo * (static_cast<float>(k + 1) / kSubdivisiones);
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
        agregarVerticeRGBA(out, color, xa, 0.0f, za, aa);
        agregarVerticeRGBA(out, color, xb, 0.0f, zb, ab);
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

    minorVertices_.clear();
    majorVertices_.clear();
    ejeXVertices_.clear();
    ejeZVertices_.clear();
    ejeYVertices_.clear();

    // Lineas del plano dentro del circulo de radio kFadeFin alrededor de la
    // camara, ancladas a multiplos exactos de kSepMenor (no se desplazan al
    // moverse la camara: simplemente entran y salen del circulo). Cada 5
    // secundarias -> principal (mismo color, solo mas ancha). La linea por el
    // origen (i/j == 0) se salta: la pintan los ejes X/Z.
    const int iIni = static_cast<int>(std::ceil((camX - kFadeFin) / kSepMenor));
    const int iFin = static_cast<int>(std::floor((camX + kFadeFin) / kSepMenor));
    for (int i = iIni; i <= iFin; ++i) {
        if (i == 0) continue;
        const float x = static_cast<float>(i) * kSepMenor;
        emitirLineaPlano((i % kMultiploMayor == 0) ? majorVertices_
                                                   : minorVertices_,
                         colorGrilla, x, camX, camZ, true);
    }
    const int jIni = static_cast<int>(std::ceil((camZ - kFadeFin) / kSepMenor));
    const int jFin = static_cast<int>(std::floor((camZ + kFadeFin) / kSepMenor));
    for (int j = jIni; j <= jFin; ++j) {
        if (j == 0) continue;
        const float z = static_cast<float>(j) * kSepMenor;
        emitirLineaPlano((j % kMultiploMayor == 0) ? majorVertices_
                                                   : minorVertices_,
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

    emitirLineaPlano(ejeXVertices_, colorEjeX, 0.0f, camX, camZ, false);
    emitirLineaPlano(ejeZVertices_, colorEjeZ, 0.0f, camX, camZ, true);
    const float alphaEjeY =
        alphaDifuminado(std::sqrt(camX * camX + camZ * camZ));
    agregarVerticeRGBA(ejeYVertices_, colorEjeY, 0.0f, 0.0f, 0.0f, alphaEjeY);
    agregarVerticeRGBA(ejeYVertices_, colorEjeY, 0.0f, kEjeYLongitud, 0.0f,
                       alphaEjeY);

    Rendering::Backend::IRenderBackend& b =
        Rendering::Backend::activeBackend();
    b.pushMatrix();
    b.multMatrix(model);
    b.setLightingEnabled(false);
    // Lineas suavizadas (con blending activo, lo que ademas usa el alpha del
    // difuminado para fundirse con el fondo en el horizonte).
    b.setLineSmoothing(true);

    // Secundarias (1px) y principales (2px): mismo color efectivo, solo cambia
    // el ancho. El alpha radial ya viene por vertice (cerca opaco, borde 0).
    if (!minorVertices_.empty()) {
        b.setLineWidth(1.0f);
        b.drawLinePairsRGBA(minorVertices_.data(),
                            static_cast<int>(minorVertices_.size() / 7));
    }
    if (!majorVertices_.empty()) {
        b.setLineWidth(2.0f);
        b.drawLinePairsRGBA(majorVertices_.data(),
                            static_cast<int>(majorVertices_.size() / 7));
    }

    // Ejes (3px) con su color por contraste.
    b.setLineWidth(3.0f);
    b.drawLinePairsRGBA(ejeXVertices_.data(),
                        static_cast<int>(ejeXVertices_.size() / 7));
    b.drawLinePairsRGBA(ejeZVertices_.data(),
                        static_cast<int>(ejeZVertices_.size() / 7));
    b.drawLinePairsRGBA(ejeYVertices_.data(),
                        static_cast<int>(ejeYVertices_.size() / 7));

    b.setLineWidth(1.0f); // Restaurar ancho por defecto
    b.setLineSmoothing(false);
    b.setLightingEnabled(true);
    b.popMatrix();
}

void GrillaRenderer::destruir() {
    minorVertices_.clear();
    majorVertices_.clear();
    ejeXVertices_.clear();
    ejeZVertices_.clear();
    ejeYVertices_.clear();
}