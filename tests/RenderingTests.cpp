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

// Geometria de lineas del pipeline moderno (LineBuilder: la expansion de cada
// segmento al quad que dibuja kLineVertexShader) y calculo de la recta de la
// guia de eje (GuiaEje) sin pila grafica ni GL: solo std C++17.

#include <cmath>
#include <iostream>

#include "../FunshiEngineGL/src/Rendering/GuiaEje.h"
#include "../FunshiEngineGL/src/Rendering/LineBuilder.h"

namespace {

int total = 0;
int fallos = 0;

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

// Floats del vertice v del builder, para no repetir los offsets a mano.
const float* vertice(const LineBuilder& b, std::size_t v) {
    return b.vertices() + v * LineBuilder::kFloatsPorVertice;
}

void testConstantes() {
    // 12 floats por vertice: inicio(3) + fin(3) + lado(1) + avance(1) + rgba(4).
    CHECK(LineBuilder::kFloatsPorVertice == 12, "12 floats por vertice");
    CHECK(LineBuilder::kVerticesPorSegmento == 6, "6 vertices por segmento");
    CHECK(LineBuilder::stride() == 48, "stride de 48 bytes");
    CHECK(LineBuilder::offsetInicio() == 0, "inicio en el float 0");
    CHECK(LineBuilder::offsetFin() == 12, "fin en el byte 12");
    CHECK(LineBuilder::offsetLado() == 24, "lado en el byte 24");
    CHECK(LineBuilder::offsetAvance() == 28, "avance en el byte 28");
    CHECK(LineBuilder::offsetColor() == 32, "color en el byte 32");
}

void testVacio() {
    LineBuilder b;
    CHECK(b.vacio(), "un builder nuevo esta vacio");
    CHECK(b.cantidadVertices() == 0, "sin vertices");
    CHECK(b.cantidadSegmentos() == 0, "sin segmentos");
    CHECK(b.vertices() == nullptr, "vertices() es null si esta vacio");
    CHECK(b.bytes() == 0, "bytes() es 0 si esta vacio");

    // Una polilinea de un solo punto no genera nada (no hay segmento).
    const float punto[3] = {0.f, 0.f, 0.f};
    const float rgba[4] = {1.f, 1.f, 1.f, 1.f};
    b.agregarPolilinea(punto, 1, false, rgba);
    CHECK(b.vacio(), "un punto suelto no genera geometria");
    b.agregarPolilinea(punto, 1, true, rgba);
    CHECK(b.vacio(), "un punto suelto no genera geometria ni al cerrar");

    // Punteros nulos: no debe tocar memoria.
    b.agregarSegmento(nullptr, punto, rgba);
    b.agregarSegmento(punto, nullptr, rgba);
    b.agregarSegmento(punto, punto, nullptr);
    b.agregarPolilinea(nullptr, 4, true, rgba);
    b.agregarAristas(punto, 1, nullptr, 1, rgba);
    b.agregarCaja(nullptr, punto, rgba);
    CHECK(b.vacio(), "los punteros nulos se ignoran sin geometria");

    b.agregarSegmento(punto, punto, rgba);
    CHECK(!b.vacio(), "agregarSegmento produce geometria");
    b.limpiar();
    CHECK(b.vacio(), "limpiar() vacia el builder");
}

void testSegmento() {
    LineBuilder b;
    const float a[3] = {1.f, 2.f, 3.f};
    const float c[3] = {-1.f, 0.f, 5.f};
    const float rgba[4] = {0.2f, 0.4f, 0.6f, 0.8f};

    b.agregarSegmento(a, c, rgba);
    CHECK(b.cantidadSegmentos() == 1, "un segmento");
    CHECK(b.cantidadVertices() == 6, "un segmento son 6 vertices");
    CHECK(b.bytes() == 6 * 12 * sizeof(float), "bytes del segmento");

    // Los 6 vertices tienen que cubrir los dos triangulos del quad: cada uno con
    // inicio/fin correctos, un lado de +-1 y un avance de 0 o 1, y el color.
    int conAvance0 = 0, conAvance1 = 0, conLadoMenos = 0, conLadoMas = 0;
    for (std::size_t v = 0; v < 6; ++v) {
        const float* p = vertice(b, v);
        CHECK(p[0] == a[0] && p[1] == a[1] && p[2] == a[2],
              "el extremo inicio del segmento es el punto a");
        CHECK(p[3] == c[0] && p[4] == c[1] && p[5] == c[2],
              "el extremo fin del segmento es el punto c");
        CHECK(std::fabs(p[6]) == 1.f, "el lado es -1 o +1");
        CHECK(p[7] == 0.f || p[7] == 1.f, "el avance es 0 o 1");
        CHECK(p[8] == rgba[0] && p[9] == rgba[1] && p[10] == rgba[2] &&
                  p[11] == rgba[3],
              "el color del vertice es el del segmento");
        if (p[7] == 0.f) ++conAvance0;
        if (p[7] == 1.f) ++conAvance1;
        if (p[6] < 0.f) ++conLadoMenos;
        if (p[6] > 0.f) ++conLadoMas;
    }
    CHECK(conAvance0 == 3 && conAvance1 == 3, "3 vertices en cada extremo");
    CHECK(conLadoMenos == 3 && conLadoMas == 3, "3 vertices a cada lado");

    // Dos triangulos = 6 indices si se indexara el quad: aca se dibuja con
    // drawArrays, pero el conteo de vertices por segmento ya lo fija el layout.
    CHECK(b.cantidadVertices() % 3 == 0,
          "la cuenta de vertices es multiplo de 3 (triangulos)");
}

void testColorPorExtremo() {
    LineBuilder b;
    const float a[3] = {0.f, 0.f, 0.f};
    const float c[3] = {1.f, 0.f, 0.f};
    const float rgbaA[4] = {1.f, 0.f, 0.f, 1.f};
    const float rgbaC[4] = {0.f, 0.f, 1.f, 0.f}; // transparente: difuminado

    b.agregarSegmento(a, c, rgbaA, rgbaC);
    // Los vertices con avance 0 toman el color del inicio y los de avance 1 el
    // del fin: es lo que permite el difuminado por vertice de la grilla.
    for (std::size_t v = 0; v < 6; ++v) {
        const float* p = vertice(b, v);
        const float* esperado = (p[7] == 0.f) ? rgbaA : rgbaC;
        CHECK(p[8] == esperado[0] && p[9] == esperado[1] &&
                  p[10] == esperado[2] && p[11] == esperado[3],
              "cada extremo del quad toma el color de su punto");
    }
}

void testPolilinea() {
    LineBuilder b;
    const float puntos[4][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    const float rgba[4] = {1.f, 1.f, 1.f, 1.f};

    b.agregarPolilinea(&puntos[0][0], 4, false, rgba);
    CHECK(b.cantidadSegmentos() == 3, "una polilinea abierta de 4 puntos son 3");
    b.limpiar();

    b.agregarPolilinea(&puntos[0][0], 4, true, rgba);
    CHECK(b.cantidadSegmentos() == 4, "cerrada suma el segmento de vuelta");
    b.limpiar();

    b.agregarPolilinea(&puntos[0][0], 2, true, rgba);
    CHECK(b.cantidadSegmentos() == 2,
          "con 2 puntos, cerrar agrega el segmento inverso");
    b.limpiar();

    b.agregarPolilinea(&puntos[0][0], 0, true, rgba);
    CHECK(b.vacio(), "una polilinea sin puntos no genera geometria");

    // Color por vertice: el difuminado de la grilla (7 floats por punto).
    LineBuilder g;
    const float puntosRGBA[3][7] = {
        {0, 0, 0, 1, 1, 1, 1}, {1, 0, 0, 1, 1, 1, 0.5f}, {2, 0, 0, 1, 1, 1, 0}};
    g.agregarPolilineaVertices(&puntosRGBA[0][0], 3, false);
    CHECK(g.cantidadSegmentos() == 2, "polilinea coloreada de 3 puntos");
    const float* primero = vertice(g, 0);
    const float* ultimo = vertice(g, 11); // ultimo del SEGUNDO segmento (0->1, 1->2)
    CHECK(primero[11] == 1.f, "el primer vertice toma el alpha del punto inicial");
    CHECK(ultimo[11] == 0.f, "el ultimo vertice toma el alpha del punto final");
}

void testAristas() {
    // Wireframe de caja: 8 vertices y 12 aristas, como los colliders.
    LineBuilder b;
    const float v[8][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                           {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    const int aristas[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7},
                                {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    const float rgba[4] = {1.f, 0.8f, 0.f, 1.f};

    b.agregarAristas(&v[0][0], 8, &aristas[0][0], 12, rgba);
    CHECK(b.cantidadSegmentos() == 12, "las 12 aristas de la caja");

    // Indice fuera de rango o negativo: se ignora, sin leer fuera del vector.
    const int malas[3][2] = {{0, 99}, {-1, 2}, {7, 3}};
    b.limpiar();
    b.agregarAristas(&v[0][0], 8, &malas[0][0], 3, rgba);
    CHECK(b.cantidadSegmentos() == 1, "solo la arista valida se dibuja");
    const float* p = vertice(b, 0);
    CHECK(p[3] == v[3][0] && p[5] == v[3][2], "la arista valida une 7 con 3");
}

void testCaja() {
    LineBuilder b;
    const float centro[3] = {1.f, 2.f, 3.f};
    const float semilado[3] = {1.f, 2.f, 0.5f};
    const float rgba[4] = {0.f, 1.f, 0.f, 1.f};

    b.agregarCaja(centro, semilado, rgba);
    CHECK(b.cantidadSegmentos() == 12, "una caja son 12 aristas");
    CHECK(b.cantidadVertices() == 72, "12 aristas x 6 vertices");

    // Todos los corners deben caer en centro +- semilado y el lado mas largo
    // debe medir 2*semilado.
    bool dentro = true;
    float maxLado[3] = {0.f, 0.f, 0.f};
    for (std::size_t v = 0; v < b.cantidadVertices(); ++v) {
        const float* ini = vertice(b, v);
        for (int i = 0; i < 3; ++i) {
            const float d = std::fabs(ini[i] - centro[i]);
            if (d > semilado[i] + 1e-5f) dentro = false;
            if (d > maxLado[i]) maxLado[i] = d;
        }
    }
    CHECK(dentro, "todos los corners de la caja estan dentro del semilado");
    CHECK(maxLado[0] == 1.f && maxLado[1] == 2.f && maxLado[2] == 0.5f,
          "la caja toca las 6 caras del semilado");

    // Ningun segmento puede tener longitud cero (seria un triangulo degenerado
    // que el shader no puede ensanchar).
    bool sinDegenerar = true;
    for (std::size_t s = 0; s < b.cantidadSegmentos(); ++s) {
        const float* p = vertice(b, s * 6);
        const float dx = p[3] - p[0], dy = p[4] - p[1], dz = p[5] - p[2];
        if (std::fabs(dx) + std::fabs(dy) + std::fabs(dz) < 1e-6f)
            sinDegenerar = false;
    }
    CHECK(sinDegenerar, "ninguna arista de la caja tiene longitud cero");
}

void testAcumulacion() {
    // Varios lotes en un mismo builder: la cuenta y el layout se mantienen.
    LineBuilder b;
    const float rgba[4] = {1.f, 1.f, 1.f, 1.f};
    const float centro[3] = {0.f, 0.f, 0.f};
    const float semilado[3] = {1.f, 1.f, 1.f};

    b.agregarCaja(centro, semilado, rgba);
    CHECK(b.cantidadSegmentos() == 12, "primera caja");
    const float punto[3] = {5.f, 5.f, 5.f};
    b.agregarSegmento(punto, punto, rgba);
    CHECK(b.cantidadSegmentos() == 13, "un segmento mas se acumula");
    CHECK(b.cantidadVertices() == 13 * 6, "vertices acumulados");
    CHECK(b.bytes() == b.cantidadVertices() * 12 * sizeof(float),
          "bytes() es coherente con la cuenta de vertices");
}

// Difuminado de la guia: los mismos valores que usa la grilla, para que la
// recta se desvanzca en el mismo horizonte que el piso.
constexpr GuiaEje::Difuminado kDif = {40.0f, 150.0f, 16};
constexpr float kEps = 1e-4f;

// Matriz identidad (column-major) con la traslacion en tx,ty,tz.
void matrizTraslada(float tx, float ty, float tz, float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

bool cerca(float a, float b) { return std::fabs(a - b) < kEps; }

// En GLOBAL la direccion es el eje del mundo: las otras dos coordenadas quedan
// FIJADAS a las del objeto, que es el contrato de la funcionalidad. El largo ya
// no se decide aqui (la recta llega al horizonte), asi que se comprueba que el
// origen es la posicion del objeto y la direccion es la del eje pedido.
void testGuiaGlobal() {
    float m[16];
    matrizTraslada(3.0f, 2.0f, -5.0f, m);
    GuiaEje::Eje e;

    // X: direccion sobre el X del mundo, origen en la posicion del objeto, con Y
    // e Z intactos (quedan clavados por definicion de la recta).
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, &e),
          "guia de X en coordenadas globales");
    CHECK(cerca(e.dir[0], 1.0f) && cerca(e.dir[1], 0.0f) && cerca(e.dir[2], 0.0f),
          "la guia de X apunta al X del mundo");
    CHECK(cerca(e.origen[0], 3.0f) && cerca(e.origen[1], 2.0f) &&
              cerca(e.origen[2], -5.0f),
          "la guia de X sale de la posicion del objeto (Y y Z quedan fijadas)");

    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeY, true, &e),
          "guia de Y en coordenadas globales");
    CHECK(cerca(e.dir[1], 1.0f) && cerca(e.dir[0], 0.0f) && cerca(e.dir[2], 0.0f),
          "la guia de Y apunta al Y del mundo");

    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeZ, true, &e),
          "guia de Z en coordenadas globales");
    CHECK(cerca(e.dir[2], 1.0f) && cerca(e.dir[0], 0.0f) && cerca(e.dir[1], 0.0f),
          "la guia de Z apunta al Z del mundo");
}

// En LOCAL la direccion sale de la columna del eje en la matriz global, o sea
// rota con el objeto.
void testGuiaLocal() {
    // Rotacion de +90 grados alrededor de Y (column-major):
    //   eje X local -> -Z del mundo, Y local -> Y, Z local -> +X.
    float m[16];
    matrizTraslada(0.0f, 0.0f, 0.0f, m);
    m[0] = 0.0f;  m[1] = 0.0f;  m[2] = -1.0f;  // columna X
    m[4] = 0.0f;  m[5] = 1.0f;  m[6] = 0.0f;   // columna Y
    m[8] = 1.0f;  m[9] = 0.0f;  m[10] = 0.0f;  // columna Z
    GuiaEje::Eje e;

    // Pulsar X sobre un objeto rotado 90 grados: la recta cae sobre el Z del
    // mundo, NO sobre el X (que es lo que haria el modo global).
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, false, &e),
          "guia de X en coordenadas locales");
    CHECK(cerca(e.dir[2], -1.0f) && cerca(e.dir[0], 0.0f) && cerca(e.dir[1], 0.0f),
          "en local la guia de X sigue el Z del mundo (eje X rotado)");

    // El mismo objeto en global: la guia de X si va sobre el X del mundo.
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, &e),
          "el mismo objeto en global");
    CHECK(cerca(e.dir[0], 1.0f) && cerca(e.dir[2], 0.0f),
          "en global la guia de X sigue el X del mundo");

    // Y y Z locales.
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeY, false, &e),
          "guia de Y en coordenadas locales");
    CHECK(cerca(e.dir[1], 1.0f), "el Y local coincide con el Y del mundo");
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeZ, false, &e),
          "guia de Z en coordenadas locales");
    CHECK(cerca(e.dir[0], 1.0f) && cerca(e.dir[2], 0.0f),
          "en local la guia de Z sigue el X del mundo (eje Z rotado)");
}

// La escala del objeto no debe cambiar la direccion (la guia no se alarga ni se
// acorta por escala).
void testGuiaIgnoraEscala() {
    float m[16];
    matrizTraslada(1.0f, 1.0f, 1.0f, m);
    m[0] = 0.0f;  m[1] = 0.0f;  m[2] = -7.0f;  // eje X local escalado x7
    GuiaEje::Eje e;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, false, &e),
          "guia local con el objeto escalado");
    CHECK(cerca(e.dir[2], -1.0f) && cerca(e.dir[0], 0.0f),
          "la direccion se normaliza (la escala no la deforma)");
}

// La opacidad cae como en la grilla: plena cerca, 0 en el horizonte.
void testOpacidad() {
    CHECK(cerca(GuiaEje::opacidad(0.0f, kDif), 1.0f), "opaca en la camara");
    CHECK(cerca(GuiaEje::opacidad(kDif.inicio, kDif), 1.0f),
          "opaca hasta el inicio del difuminado");
    CHECK(GuiaEje::opacidad(95.0f, kDif) < 1.0f &&
              GuiaEje::opacidad(95.0f, kDif) > 0.0f,
          "a mitad de camino esta a medio opaca");
    CHECK(cerca(GuiaEje::opacidad(kDif.fin, kDif), 0.0f),
          "invisible en el horizonte");
    CHECK(cerca(GuiaEje::opacidad(kDif.fin * 3.0f, kDif), 0.0f),
          "invisible mas alla del horizonte (no seNegative)");
}

// La recta llega al horizonte en los dos sentidos y no se sale de la esfera:
// todos los vertices emitidos estan a distancia <= dif.fin de la camara, y
// hay geometria de los dos lados del objeto.
void testEmiteHastaElHorizonte() {
    float m[16];
    matrizTraslada(0.0f, 0.0f, 0.0f, m);
    GuiaEje::Eje e;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, &e), "eje calculado");

    // Camara mirando desde +Z, como en la app (pos = (0, 8, 40)).
    const float camara[3] = {0.0f, 8.0f, 40.0f};
    const float color[3] = {1.0f, 0.25f, 0.25f};
    LineBuilder b;
    GuiaEje::emitir(b, e, camara, color, kDif);

    // Un segmento por subdivision, con 6 vertices cada uno.
    CHECK(b.cantidadSegmentos() == static_cast<std::size_t>(kDif.subdivisiones),
          "un segmento por tramo de la subdivision");
    CHECK(b.cantidadVertices() ==
              static_cast<std::size_t>(kDif.subdivisiones) * 6,
          "la guia son 6 vertices por tramo");

    // Todos los vertices dentro de la esfera del horizonte, y alguno pegado al
    // borde: eso prueba que la recta se dibuja hasta el horizonte y no se corto
    // antes. La distancia es la 3D completa (no por componente).
    bool dentro = true;
    bool alcanzoBorde = false;
    const std::size_t n = b.cantidadVertices();
    for (std::size_t v = 0; v < n; ++v) {
        const float* f = vertice(b, v);
        const float dx = f[0] - camara[0];
        const float dy = f[1] - camara[1];
        const float dz = f[2] - camara[2];
        const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist > kDif.fin + 0.5f) dentro = false;
        if (std::fabs(dist - kDif.fin) < 0.5f) alcanzoBorde = true;
    }
    CHECK(dentro, "ningun vertex sale de la esfera del horizonte");
    CHECK(alcanzoBorde, "la guia alcanza el horizonte (no se corta antes)");

    // Va en los dos sentidos: hay vertices con X negativa y positiva.
    bool negativo = false, positivo = false;
    for (std::size_t v = 0; v < n; ++v) {
        const float* f = vertice(b, v);
        if (f[0] < -1.0f) negativo = true;
        if (f[0] > 1.0f) positivo = true;
    }
    CHECK(negativo && positivo, "la guia se dibuja a los dos lados del objeto");
}

// El difuminado va POR VERTICE: el primer vertice (cerca de la camara) es opaco
// y el ultimo (en el horizonte) es transparente.
void testDifuminadoPorVertice() {
    float m[16];
    matrizTraslada(0.0f, 0.0f, 0.0f, m);
    GuiaEje::Eje e;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeZ, true, &e), "eje calculado");

    const float camara[3] = {0.0f, 8.0f, 40.0f};
    const float color[3] = {0.3f, 0.5f, 1.0f};
    LineBuilder b;
    GuiaEje::emitir(b, e, camara, color, kDif);
    CHECK(!b.vacio(), "la guia emite geometria");

    // El vertice 0 es el extremo mas cercano a la camara; el ultimo de cada
    // tramo es el mas lejano. Con el difuminado, el alpha arranca en ~0 (el
    // origen esta lejos de la camara) y los dos extremos lejanos valen 0.
    const float* primero = vertice(b, 0);
    const float* ultimo = vertice(b, b.cantidadVertices() - 1);
    const int off = static_cast<int>(LineBuilder::offsetColor() / sizeof(float));
    CHECK(primero[off + 3] < 0.05f, "el extremo lejano arranca casi invisible");
    CHECK(ultimo[off + 3] < 0.05f, "el extremo del horizonte es invisible");

    // Y el color es el del eje en todos los vertices (solo varia el alpha).
    bool colorOk = true;
    for (std::size_t v = 0; v < b.cantidadVertices(); ++v) {
        const float* f = vertice(b, v);
        if (!cerca(f[off], color[0]) || !cerca(f[off + 1], color[1]) ||
            !cerca(f[off + 2], color[2]))
            colorOk = false;
    }
    CHECK(colorOk, "el color del eje se mantiene, solo cambia la opacidad");
}

// Casos de rechazo: sin guia valida no se dibuja nada.
void testGuiaDefensiva() {
    float m[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, m);
    GuiaEje::Eje e;
    e.origen[0] = 42.0f;  // se marca para verificar que no se toca
    e.dir[0] = 7.0f;

    CHECK(!GuiaEje::calcularEje(m, GuiaEje::kSinGuia, true, &e),
          "sin guia activa no hay eje");
    CHECK(!GuiaEje::calcularEje(m, 3, true, &e), "eje fuera de rango");
    CHECK(!GuiaEje::calcularEje(m, -2, true, &e), "eje negativo");
    CHECK(!GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, nullptr),
          "salida nula se rechaza");
    CHECK(!GuiaEje::calcularEje(nullptr, GuiaEje::kEjeX, true, &e),
          "matriz nula se rechaza");
    CHECK(cerca(e.origen[0], 42.0f) && cerca(e.dir[0], 7.0f),
          "el eje no se toca cuando se rechaza");

    // Matriz corrupta: no debe propagar NaN.
    float mala[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, mala);
    mala[12] = std::nanf("");
    CHECK(!GuiaEje::calcularEje(mala, GuiaEje::kEjeX, true, &e),
          "matriz con NaN se rechaza");
    float infinita[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, infinita);
    infinita[13] = HUGE_VALF;
    CHECK(!GuiaEje::calcularEje(infinita, GuiaEje::kEjeX, true, &e),
          "matriz con Inf se rechaza");

    // Escala nula en el eje pedido: la columna no da direccion utilizable.
    float degenerada[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, degenerada);
    degenerada[4] = degenerada[5] = degenerada[6] = 0.0f;
    CHECK(!GuiaEje::calcularEje(degenerada, GuiaEje::kEjeY, false, &e),
          "eje local degenerado (escala 0) se rechaza");
}

// emitir() con parametros invalidos no debe generar geometria ni leer memoria.
void testEmiteDefensiva() {
    float m[16];
    matrizTraslada(0.0f, 0.0f, 0.0f, m);
    GuiaEje::Eje e;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, &e), "eje calculado");

    const float camara[3] = {0.0f, 8.0f, 40.0f};
    const float color[3] = {1.0f, 0.25f, 0.25f};
    LineBuilder b;

    GuiaEje::Difuminado dif;
    dif.fin = 0.0f;
    GuiaEje::emitir(b, e, camara, color, dif);
    CHECK(b.vacio(), "un horizonte de radio 0 no dibuja nada");

    GuiaEje::emitir(b, e, nullptr, color, kDif);
    CHECK(b.vacio(), "camara nula no dibuja nada");

    GuiaEje::emitir(b, e, camara, nullptr, kDif);
    CHECK(b.vacio(), "color nulo no dibuja nada");

    // Recta enteramente fuera del horizonte: no se pinta (mismo criterio que la
    // grilla con su circulo).
    GuiaEje::Eje lejos;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, &lejos), "eje auxiliar");
    for (int k = 0; k < 3; ++k) lejos.origen[k] = 100000.0f;
    GuiaEje::emitir(b, lejos, camara, color, kDif);
    CHECK(b.vacio(), "una recta fuera del horizonte no se dibuja");
}

// El color identifica el eje con la convencion del gizmo y de la grilla.
void testGuiaColor() {
    float x[4], y[4], z[4];
    GuiaEje::colorEje(GuiaEje::kEjeX, x);
    GuiaEje::colorEje(GuiaEje::kEjeY, y);
    GuiaEje::colorEje(GuiaEje::kEjeZ, z);

    CHECK(x[0] > x[1] && x[0] > x[2], "la guia de X es roja");
    CHECK(y[1] > y[0] && y[1] > y[2], "la guia de Y es verde");
    CHECK(z[2] > z[0] && z[2] > z[1], "la guia de Z es azul");
    CHECK(x[3] == 1.0f && y[3] == 1.0f && z[3] == 1.0f,
          "los colores base son opacos (el fade va aparte)");
}
} // namespace

int main() {
    testConstantes();
    testVacio();
    testSegmento();
    testColorPorExtremo();
    testPolilinea();
    testAristas();
    testCaja();
    testAcumulacion();
    testGuiaGlobal();
    testGuiaLocal();
    testGuiaIgnoraEscala();
    testOpacidad();
    testEmiteHastaElHorizonte();
    testDifuminadoPorVertice();
    testGuiaDefensiva();
    testEmiteDefensiva();
    testGuiaColor();

    std::cout << "Resultado: " << (total - fallos) << "/" << total
              << " OK" << std::endl;
    if (fallos > 0) {
        std::cout << fallos << " prueba(s) fallaron." << std::endl;
        return 1;
    }
    return 0;
}
