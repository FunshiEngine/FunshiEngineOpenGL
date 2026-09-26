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
#include "GuiaEje.h"

#include <cmath>

namespace GuiaEje {
namespace {

// Amplitud por debajo de la cual la columna de un eje se considera degenerada
// (escala ~0 en ese eje): sin direccion utilizable no hay guia que dibujar.
constexpr float kAmplitudMinima = 1e-6f;

} // namespace

bool calcularEje(const float matrizGlobal[16], int eje, bool coordenadasGlobales,
                 Eje* out) {
    if (!matrizGlobal || !out) return false;
    if (eje < kEjeX || eje > kEjeZ) return false;

    // Defensa contra NaN/Inf: una matriz corrupta (por ejemplo si un comando de
    // undo dejo datos invalidos) propagaria NaN a la geometria y la guia se
    // dibujaria como un vertice perdido en la nada, como ya pasa con el gizmo.
    for (int i = 0; i < 16; ++i) {
        if (!std::isfinite(matrizGlobal[i])) return false;
    }

    // Origen: la traslacion de la matriz global (column-major, indices 12..14).
    out->origen[0] = matrizGlobal[12];
    out->origen[1] = matrizGlobal[13];
    out->origen[2] = matrizGlobal[14];

    // Direccion. En GLOBAL es el eje del mundo; en LOCAL es la columna de ese
    // eje en la matriz global (ya incluye la rotacion del objeto), normalizada
    // para que la escala no la alargue.
    if (coordenadasGlobales) {
        out->dir[0] = out->dir[1] = out->dir[2] = 0.0f;
        out->dir[eje] = 1.0f;
        return true;
    }

    const int col = eje * 4;
    float dir[3] = {matrizGlobal[col], matrizGlobal[col + 1],
                    matrizGlobal[col + 2]};
    const float amplitud =
        std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (amplitud < kAmplitudMinima) return false;
    for (int i = 0; i < 3; ++i) out->dir[i] = dir[i] / amplitud;
    return true;
}

float opacidad(float distancia, const Difuminado& dif) {
    if (!std::isfinite(distancia) || dif.fin <= dif.inicio) return 1.0f;
    // t se acota a [0,1] ANTES de elevar al cuadrado: sin ese recorte, los
    // puntos mas cercanos que "inicio" dan t negativo y el cuadrado los baja de
    // opacos, con lo que hasta el centro de la vista se veria translucido.
    float t = (distancia - dif.inicio) / (dif.fin - dif.inicio);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const float a = 1.0f - t * t;
    return a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
}

void emitir(LineBuilder& out, const Eje& eje, const float camaraMundo[3],
            const float color[3], const Difuminado& dif) {
    if (!camaraMundo || !color) return;
    if (!std::isfinite(dif.fin) || dif.fin <= 0.0f) return;
    if (dif.subdivisiones < 1) return;

    // Recorte analitico al horizonte: los puntos de la recta parametrizada como
    // P(t) = origen + dir*t cumplen |P(t) - camara|^2 = dif.fin^2. Como "dir" es
    // unitaria, eso es la cuadratica t^2 + 2*b*t + (c - fin^2) = 0 con
    // b = dir.(origen-camara) y c = |origen-camara|^2, cuyas raices son los dos
    // extremos visibles. Asi la guia se dibuja entera hasta el horizonte en vez
    // de cortarse a una distancia fija.
    float rel[3] = {eje.origen[0] - camaraMundo[0], eje.origen[1] - camaraMundo[1],
                    eje.origen[2] - camaraMundo[2]};
    const float b = eje.dir[0] * rel[0] + eje.dir[1] * rel[1] + eje.dir[2] * rel[2];
    const float c = rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2];
    const float disc = b * b - c + dif.fin * dif.fin;
    // Sin interseccion con la esfera del horizonte: la recta esta enteramente
    // fuera del limite de dibujado, asi que no se pinta (mismo criterio que la
    // grilla con su circulo).
    if (disc <= 0.0f) return;

    const float raiz = std::sqrt(disc);
    float t0 = -b - raiz;   // extremo detras del origen
    float t1 = -b + raiz;   // extremo adelante del origen

    // Recorte adicional al plano visual del suelo (la grilla esta en y = -0.5):
    // no se dibuja nada por debajo de ese nivel para que la guia no se vea
    // "hundida" bajo la grilla. P(t).y = origen.y + dir.y * t >= -0.5.
    const float kSueloY = -0.5f;
    if (std::fabs(eje.dir[1]) < 1e-6f) {
        // Linea horizontal (ejes X o Z en global, o cualquier eje local sin
        // componente Y): si el origen esta por debajo del suelo, no hay nada
        // que dibujar.
        if (eje.origen[1] < kSueloY) return;
    } else {
        const float tSuelo = (kSueloY - eje.origen[1]) / eje.dir[1];
        if (eje.dir[1] > 0.0f) {
            // La linea sube: la parte valida es t >= tSuelo.
            if (t1 < tSuelo) return; // toda la linea esta por debajo
            if (t0 < tSuelo) t0 = tSuelo;
        } else {
            // La linea baja: la parte valida es t <= tSuelo.
            if (t0 > tSuelo) return; // toda la linea esta por debajo
            if (t1 > tSuelo) t1 = tSuelo;
        }
    }

    const int tramos = dif.subdivisiones;
    const float paso = (t1 - t0) / static_cast<float>(tramos);

    // Se recorre la recta ya recortada y se emite un segmento por tramo, con el
    // alpha de cada extremo: el batch interpola el color de un extremo al otro
    // dentro del tramo, que es el mismo difuminado por vertice que usa la
    // grilla (y por eso hay que subdividir, si no el degradado se ve a saltos).
    float puntoAnterior[3] = {0.f, 0.f, 0.f};
    float alphaAnterior = 0.0f;
    for (int i = 0; i <= tramos; ++i) {
        const float t = t0 + paso * static_cast<float>(i);
        const float punto[3] = {eje.origen[0] + eje.dir[0] * t,
                                eje.origen[1] + eje.dir[1] * t,
                                eje.origen[2] + eje.dir[2] * t};
        const float d[3] = {punto[0] - camaraMundo[0], punto[1] - camaraMundo[1],
                            punto[2] - camaraMundo[2]};
        const float distancia =
            std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const float alpha = opacidad(distancia, dif);

        if (i > 0) {
            const float rgbaA[4] = {color[0], color[1], color[2], alphaAnterior};
            const float rgbaB[4] = {color[0], color[1], color[2], alpha};
            out.agregarSegmento(puntoAnterior, punto, rgbaA, rgbaB);
        }
        puntoAnterior[0] = punto[0];
        puntoAnterior[1] = punto[1];
        puntoAnterior[2] = punto[2];
        alphaAnterior = alpha;
    }
}

void colorEje(int eje, float rgba[4]) {
    if (!rgba) return;
    switch (eje) {
        case kEjeX:  // rojo
            rgba[0] = 1.0f; rgba[1] = 0.25f; rgba[2] = 0.25f; break;
        case kEjeY:  // verde
            rgba[0] = 0.3f; rgba[1] = 1.0f; rgba[2] = 0.3f; break;
        case kEjeZ:  // azul
            rgba[0] = 0.3f; rgba[1] = 0.5f; rgba[2] = 1.0f; break;
        default:
            rgba[0] = rgba[1] = rgba[2] = 1.0f; break;
    }
    rgba[3] = 1.0f;
}

} // namespace GuiaEje
