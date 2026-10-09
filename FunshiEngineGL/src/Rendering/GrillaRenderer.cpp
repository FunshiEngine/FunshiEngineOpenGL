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
#include "GuiaEje.h"
#include "LineRenderer.h"

namespace {

// ---------------------------------------------------------------------------
// Constantes de diseño:
// - La separacion de las lineas es FIJA (GrillaRenderer.h, publica porque la
//   guia de eje se alinea a la celda principal). Lo que queda aca es privado de
//   este archivo.
// - El horizonte es un CIRCULO de radio "dif.fin" centrado en la camara (sobre
//   el plano del suelo) que actua COMO LIMITE DE DIBUJADO: las lineas se
//   recortan a lo que queda dentro del circulo (nada mas alla se pinta, dando
//   la ilusion de que la grilla continua) y se difuminan radialmente por
//   vertice (opacas cerca de la camara, transparentes en el borde). Como el
//   circulo persigue a la camara, moverse pinta grilla nueva por delante y
//   deja de pintar lo que queda atras.
// - El radio lo elige el usuario y llega ya recortado al rango admitido en el
//   Difuminado que arma el llamador (Difuminado::desdeRadio); aca solo se usa
//   el intervalo [dif.inicio, dif.fin] y la misma curva (dif.opacidad) que
//   aplica la guia de eje.
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

// Emite una linea de la grilla recortada por el circulo horizonte y con
// difuminado radial POR VERTICE. 'fija' es la coordenada X (si variaZ, linea
// paralela a Z) o Z (si no, linea paralela a X). Si la linea queda fuera del
// circulo no se pinta nada (ese circulo es el limite de dibujado). Los
// extremos del trozo interior caen en el borde (alpha 0) y cada vertice
// intermedio lleva el alpha de su propia distancia radial a la camara, por eso
// la linea se subdivide en kSubdivisiones trozos.
void emitirLineaPlano(LineBuilder& out, const float color[3], float fija,
                      float camX, float camZ, bool variaZ,
                      const Difuminado& dif) {
    const float d = std::fabs(fija - (variaZ ? camX : camZ));
    if (d >= dif.fin)
        return; // fuera del circulo: el difuminado es el limite
    const float h = std::sqrt(dif.fin * dif.fin - d * d);
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
        const float aa = dif.opacidad(da);
        const float ab = dif.opacidad(db);
        agregarSegmentoRGBA(out, color, xa, za, aa, xb, zb, ab);
    }
}

// Colores base de los ejes: los MISMOS que la guia de eje y el gizmo (X rojo,
// Y verde, Z azul), para que un eje se reconozca igual en las tres piezas. Se
// ajustan por contraste contra el color efectivo de la grilla para que siempre
// se vean.
void colorEjeConContraste(int eje, const float colorGrilla[3], float out[3]) {
    float rgba[4];
    GuiaEje::colorEfectivo(eje, colorGrilla, rgba);
    out[0] = rgba[0];
    out[1] = rgba[1];
    out[2] = rgba[2];
}

} // namespace

void GrillaRenderer::dibujar(const float model[16], const float colorGrilla[3],
                             const float camaraMundo[3],
                             const Difuminado& dif) {
    if (!model || !colorGrilla || !camaraMundo) return;
    // El Difuminado llega armado por el llamador (radio elegido por el usuario,
    // ya acotado), pero esta clase es publica: un radio invalido se acota aca
    // para no dibujar una grilla sin horizonte ni dividir por cero.
    const Difuminado difSeguro = Difuminado::desdeRadio(dif.fin, dif.subdivisiones);

    // La camara se transforma al espacio local del objeto "Grilla": el circulo
    // horizonte, la densidad fija y el difuminado siguen al objeto (que puede
    // moverse/escalarse/rotarse como cualquier otro componente).
    const glm::mat4 modelMat = glm::make_mat4(model);
    const glm::vec4 camaraLocal =
        glm::inverse(modelMat) *
        glm::vec4(camaraMundo[0], camaraMundo[1], camaraMundo[2], 1.0f);
    const float camX = camaraLocal.x;
    const float camZ = camaraLocal.z;

    // La geometria solo se rearma cuando cambia algo que la altera (camara
    // local, radio o color). Con la camara quieta se reusan los batches ya
    // subidos: el rearme son decenas de miles de vertices y su subida por cada
    // pasada (principal y cada vista previa), puro trabajo repetido.
    const bool claveCambio = !claveValida_ || claveCamX_ != camX ||
                             claveCamZ_ != camZ || claveRadio_ != difSeguro.fin ||
                             claveSubdivisiones_ != difSeguro.subdivisiones ||
                             claveColor_[0] != colorGrilla[0] ||
                             claveColor_[1] != colorGrilla[1] ||
                             claveColor_[2] != colorGrilla[2];
    if (claveCambio) {
        reconstruir(colorGrilla, camX, camZ, difSeguro);
        claveValida_ = true;
        claveCamX_ = camX;
        claveCamZ_ = camZ;
        claveRadio_ = difSeguro.fin;
        claveSubdivisiones_ = difSeguro.subdivisiones;
        claveColor_[0] = colorGrilla[0];
        claveColor_[1] = colorGrilla[1];
        claveColor_[2] = colorGrilla[2];
    }

    // El difuminado se funde con el fondo: hace falta blending durante la
    // grilla (y hay que restaurarlo, la pasada de objetos espera el estado base).
    auto& backend = Rendering::Backend::activeBackend();
    if (!backend.available()) return;  // Guard: backend no disponible (contexto perdido, no inicializado, etc.)
    backend.setBlendEnabled(true);

    // Secundarias (1px), principales (2px) y ejes (3px): mismo color efectivo,
    // solo cambia el ancho, que el shader resuelve en pixeles. El alpha radial ya
    // viene por vertice (cerca opaco, borde 0). Si la geometria no cambio, los
    // batches se dibujan sin volver a subirlos.
    auto& lineas = lineRenderer();
    if (claveCambio) {
        lineas.dibujar(secundario_, secundarioBatch_, model, 1.0f);
        lineas.dibujar(principal_, principalBatch_, model, 2.0f);
        lineas.dibujar(ejes_, ejesBatch_, model, 3.0f);
    } else {
        lineas.dibujar(secundarioBatch_, model, 1.0f);
        lineas.dibujar(principalBatch_, model, 2.0f);
        lineas.dibujar(ejesBatch_, model, 3.0f);
    }

    backend.setBlendEnabled(false);
}

// Geometria completa de la grilla en espacio local del objeto "Grilla": las
// lineas del plano dentro del circulo de radio dif.fin alrededor de la camara,
// ancladas a multiplos exactos de kSeparacionMenor (no se desplazan al moverse
// la camara: simplemente entran y salen del circulo). Cada kMultiploMayor
// secundarias -> principal (mismo color, solo mas ancha). La linea por el
// origen (i/j == 0) se salta: la pintan los ejes X/Z. Los ejes X y Z son
// paralelos al plano a traves del origen (recortados y difuminados por el mismo
// circulo); el eje Y es perpendicular solo hacia arriba y se difumina con la
// distancia horizontal de la camara al origen.
void GrillaRenderer::reconstruir(const float colorGrilla[3], float camX,
                                 float camZ, const Difuminado& dif) {
    secundario_.limpiar();
    principal_.limpiar();
    ejes_.limpiar();

    const float radio = dif.fin;
    const int iIni = static_cast<int>(std::ceil((camX - radio) / kSeparacionMenor));
    const int iFin = static_cast<int>(std::floor((camX + radio) / kSeparacionMenor));
    for (int i = iIni; i <= iFin; ++i) {
        if (i == 0) continue;
        const float x = static_cast<float>(i) * kSeparacionMenor;
        emitirLineaPlano((i % kMultiploMayor == 0) ? principal_ : secundario_,
                         colorGrilla, x, camX, camZ, true, dif);
    }
    const int jIni = static_cast<int>(std::ceil((camZ - radio) / kSeparacionMenor));
    const int jFin = static_cast<int>(std::floor((camZ + radio) / kSeparacionMenor));
    for (int j = jIni; j <= jFin; ++j) {
        if (j == 0) continue;
        const float z = static_cast<float>(j) * kSeparacionMenor;
        emitirLineaPlano((j % kMultiploMayor == 0) ? principal_ : secundario_,
                         colorGrilla, z, camX, camZ, false, dif);
    }

    float colorEjeX[3], colorEjeY[3], colorEjeZ[3];
    colorEjeConContraste(GuiaEje::kEjeX, colorGrilla, colorEjeX);
    colorEjeConContraste(GuiaEje::kEjeY, colorGrilla, colorEjeY);
    colorEjeConContraste(GuiaEje::kEjeZ, colorGrilla, colorEjeZ);

    emitirLineaPlano(ejes_, colorEjeX, 0.0f, camX, camZ, false, dif);
    emitirLineaPlano(ejes_, colorEjeZ, 0.0f, camX, camZ, true, dif);
    {
        const float alphaEjeY =
            dif.opacidad(std::sqrt(camX * camX + camZ * camZ));
        const float rgbaEjeY[4] = {colorEjeY[0], colorEjeY[1], colorEjeY[2],
                                   alphaEjeY};
        const float puntosEjeY[6] = {0.0f, 0.0f, 0.0f, 0.0f, kEjeYLongitud, 0.0f};
        ejes_.agregarPolilinea(puntosEjeY, 2, false, rgbaEjeY);
    }
}

void GrillaRenderer::destruir() {
    secundario_.limpiar();
    principal_.limpiar();
    ejes_.limpiar();
    // Con la geometria descartada, la clave queda invalida: la proxima pasada
    // tiene que rearmarla (dibujar sin subir solo sirve si el batch sigue
    // siendo el que se subio para esa misma geometria).
    claveValida_ = false;
}
