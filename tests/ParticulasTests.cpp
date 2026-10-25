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
// Pruebas headless del sistema de particulas (src/Particulas).
//
// La simulacion es CPU pura: no toca OpenGL ni glm, asi que estas pruebas
// compilan y corren tambien en el build sin motor (BUILD_ENGINE=OFF).
//
// Cubren: la emision (vida, tamano, color y cono de dispersion), el
// presupuesto de particulas vivas, la integracion (rozamiento, gravedad y
// posicion), la muerte por vida agotada y la reproducibilidad de la
// simulacion con la misma semilla.

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Particulas/LectorPerfil.h"
#include "../FunshiEngineGL/src/Particulas/ParticulasBuilder.h"
#include "../FunshiEngineGL/src/Particulas/ParametrosEmisor.h"
#include "../FunshiEngineGL/src/Particulas/Particula.h"
#include "../FunshiEngineGL/src/Particulas/PerfilParticula.h"
#include "../FunshiEngineGL/src/Particulas/SistemaParticulas.h"
#include "TempPruebas.h"

#include <fstream>

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

bool cerca(float a, float b, float tolerancia = 1e-4f) {
    return std::fabs(a - b) <= tolerancia;
}

// Parametros deterministas: vida y tamano fijos, para poder comprobar el
// resultado sin depender del azar.
ParametrosEmisor parametrosFijos() {
    ParametrosEmisor p;
    p.cantidadPorSegundo = 10;
    p.vidaMinima = 1.0f;
    p.vidaMaxima = 1.0f;
    p.velocidadMinima = 2.0f;
    p.velocidadMaxima = 2.0f;
    p.tamanoMinimo = 0.5f;
    p.tamanoMaximo = 0.5f;
    p.direccion[0] = 0.0f;
    p.direccion[1] = 1.0f;
    p.direccion[2] = 0.0f;
    p.dispersion = 0.0f;
    p.gravedad[0] = 0.0f;
    p.gravedad[1] = 0.0f;
    p.gravedad[2] = 0.0f;
    p.rozamiento = 0.0f;
    return p;
}

// --- Emision ---------------------------------------------------------------

void probarEmision() {
    SistemaParticulas sistema(100);
    sistema.fijarSemilla(7);

    const float origen[3] = {1.0f, 2.0f, 3.0f};
    ParametrosEmisor p = parametrosFijos();
    p.color[0] = 0.25f;
    p.color[1] = 0.5f;
    p.color[2] = 0.75f;
    p.color[3] = 1.0f;
    sistema.emitir(p, origen);

    CHECK(sistema.cantidad() == 1, "Emision: hay una particula");
    if (sistema.cantidad() != 1) return;
    const Particula& particula = sistema.particulas().front();

    CHECK(cerca(particula.posicion[0], 1.0f) && cerca(particula.posicion[1], 2.0f) &&
              cerca(particula.posicion[2], 3.0f),
          "Emision: la particula nace en el punto de emision");
    CHECK(cerca(particula.vida, 1.0f) && cerca(particula.vidaMaxima, 1.0f),
          "Emision: la vida es la del parametro");
    CHECK(cerca(particula.tamano, 0.5f), "Emision: el tamano es el del parametro");
    CHECK(cerca(particula.color[0], 0.25f) && cerca(particula.color[3], 1.0f),
          "Emision: el color es el del parametro");
    CHECK(cerca(particula.fraccionVida(), 0.0f),
          "Emision: la particula arranca con la vida completa");
    CHECK(cerca(particula.velocidad[1], 2.0f) && cerca(particula.velocidad[0], 0.0f),
          "Emision: la velocidad sigue la direccion y la magnitud del parametro");
    CHECK(particula.viva(), "Emision: la particula esta viva");
}

// --- Cono de dispersion -----------------------------------------------------

void probarCono() {
    SistemaParticulas sistema(500);
    sistema.fijarSemilla(11);

    ParametrosEmisor p = parametrosFijos();
    p.dispersion = 0.5f;  // cono de 45 grados
    p.velocidadMinima = 1.0f;
    p.velocidadMaxima = 1.0f;

    const float origen[3] = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 200; ++i) sistema.emitir(p, origen);

    CHECK(sistema.cantidad() == 200, "Cono: se emitieron todas las particulas");

    // Con dispersion 0.5 el cono abre 45 grados: el producto escalar entre la
    // velocidad y el eje tiene que ser al menos cos(45 grados).
    const float minimo = std::cos(45.0f * 3.14159265f / 180.0f) - 1e-3f;
    bool dentro = true;
    for (const Particula& particula : sistema.particulas()) {
        const float largo = std::sqrt(particula.velocidad[0] * particula.velocidad[0] +
                                      particula.velocidad[1] * particula.velocidad[1] +
                                      particula.velocidad[2] * particula.velocidad[2]);
        if (largo <= 1e-6f) continue;
        const float producto = particula.velocidad[1] / largo;
        if (producto < minimo) dentro = false;
    }
    CHECK(dentro, "Cono: ninguna particula sale del cono de dispersion");

    // Sin direccion previa la direccion es aleatoria en toda la esfera.
    ParametrosEmisor libre = parametrosFijos();
    libre.direccion[0] = 0.0f;
    libre.direccion[1] = 0.0f;
    libre.direccion[2] = 0.0f;
    libre.dispersion = 1.0f;
    sistema.limpiar();
    sistema.emitir(libre, origen);
    CHECK(sistema.cantidad() == 1, "Cono: sin direccion previa igual se emite");
    if (sistema.cantidad() == 1) {
        const float largo = std::sqrt(sistema.particulas().front().velocidad[0] *
                                          sistema.particulas().front().velocidad[0] +
                                      sistema.particulas().front().velocidad[1] *
                                          sistema.particulas().front().velocidad[1] +
                                      sistema.particulas().front().velocidad[2] *
                                          sistema.particulas().front().velocidad[2]);
        CHECK(cerca(largo, 2.0f),
              "Cono: sin direccion previa la magnitud se respeta igual");
    }
}

// --- Presupuesto ------------------------------------------------------------

void probarPresupuesto() {
    SistemaParticulas sistema(3);
    sistema.fijarSemilla(3);

    ParametrosEmisor p = parametrosFijos();
    p.vidaMinima = 1.0f;
    p.vidaMaxima = 1.0f;
    const float origen[3] = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 10; ++i) sistema.emitir(p, origen);

    CHECK(sistema.cantidad() == 3, "Presupuesto: no se superan las particulas vivas");
    CHECK(sistema.presupuesto() == 3, "Presupuesto: el limite se consulta");

    // Al llenarse el presupuesto se descarta la mas vieja: la que queda es la
    // ultima emitida.
    CHECK(cerca(sistema.particulas().back().vida, 1.0f),
          "Presupuesto: la particula mas reciente sigue viva");

    // Bajar el presupuesto descarta las mas viejas.
    sistema.fijarPresupuesto(1);
    CHECK(sistema.cantidad() == 1, "Presupuesto: bajar el limite descarta las viejas");
    sistema.fijarPresupuesto(0);
    CHECK(sistema.cantidad() == 0, "Presupuesto: un limite de cero deja el sistema vacio");
    sistema.emitir(p, origen);
    CHECK(sistema.cantidad() == 0, "Presupuesto: con limite cero no se emite");
}

// --- Integracion ------------------------------------------------------------

void probarIntegracion() {
    SistemaParticulas sistema(10);
    sistema.fijarSemilla(5);

    ParametrosEmisor p = parametrosFijos();
    const float origen[3] = {0.0f, 0.0f, 0.0f};
    sistema.emitir(p, origen);
    CHECK(sistema.cantidad() == 1, "Integracion: la particula se emitio");

    sistema.actualizar(0.5f);
    if (sistema.cantidad() != 1) {
        CHECK(false, "Integracion: la particula sigue viva");
        return;
    }
    const Particula& particula = sistema.particulas().front();
    CHECK(cerca(particula.posicion[1], 1.0f),
          "Integracion: la posicion avanza con la velocidad por el delta");
    CHECK(cerca(particula.vida, 0.5f), "Integracion: la vida baja con el delta");
    CHECK(cerca(particula.fraccionVida(), 0.5f),
          "Integracion: la fraccion de vida avanza con el tiempo");

    // La gravedad suma a la velocidad y el rozamiento la reduce.
    SistemaParticulas conGravedad(10);
    conGravedad.fijarSemilla(5);
    ParametrosEmisor g = parametrosFijos();
    g.velocidadMinima = 0.0f;
    g.velocidadMaxima = 0.0f;
    g.vidaMinima = 5.0f;
    g.vidaMaxima = 5.0f;
    g.gravedad[1] = -2.0f;
    g.rozamiento = 1.0f;
    conGravedad.emitir(g, origen);
    conGravedad.actualizar(1.0f);
    CHECK(conGravedad.cantidad() == 1, "Integracion: la particula con gravedad sigue viva");
    if (conGravedad.cantidad() == 1) {
        const Particula& p2 = conGravedad.particulas().front();
        CHECK(cerca(p2.velocidad[1], -2.0f),
              "Integracion: la gravedad suma a la velocidad");
        CHECK(cerca(p2.posicion[1], -2.0f),
              "Integracion: la posicion usa la velocidad ya actualizada");
    }

    // El rozamiento frena: una particula con velocidad y sin gravedad acaba
    // mas lenta.
    SistemaParticulas conFreno(10);
    conFreno.fijarSemilla(5);
    ParametrosEmisor f = parametrosFijos();
    f.velocidadMinima = 4.0f;
    f.velocidadMaxima = 4.0f;
    f.vidaMinima = 5.0f;
    f.vidaMaxima = 5.0f;
    f.rozamiento = 1.0f;
    conFreno.emitir(f, origen);
    conFreno.actualizar(1.0f);
    CHECK(conFreno.cantidad() == 1, "Integracion: la particula frenada sigue viva");
    if (conFreno.cantidad() == 1) {
        const float velocidad = conFreno.particulas().front().velocidad[1];
        CHECK(velocidad < 4.0f && velocidad > 0.0f,
              "Integracion: el rozamiento reduce la velocidad");
    }
}

// --- Muerte -----------------------------------------------------------------

void probarMuerte() {
    SistemaParticulas sistema(10);
    sistema.fijarSemilla(9);

    ParametrosEmisor p = parametrosFijos();
    p.vidaMinima = 0.2f;
    p.vidaMaxima = 0.2f;
    const float origen[3] = {0.0f, 0.0f, 0.0f};
    sistema.emitir(p, origen);
    CHECK(sistema.cantidad() == 1, "Muerte: la particula se emitio");

    sistema.actualizar(0.1f);
    CHECK(sistema.cantidad() == 1, "Muerte: con vida restante sigue viva");
    sistema.actualizar(0.2f);
    CHECK(sistema.cantidad() == 0, "Muerte: al agotarse la vida se retira");

    // Un delta cero no mueve nada.
    sistema.emitir(p, origen);
    sistema.actualizar(0.0f);
    CHECK(sistema.cantidad() == 1, "Muerte: un delta cero no gasta vida");
    sistema.limpiar();
    CHECK(sistema.cantidad() == 0, "Muerte: limpiar vacia el sistema");
}

// --- Reproducibilidad -------------------------------------------------------

void probarReproducibilidad() {
    ParametrosEmisor p = parametrosFijos();
    p.vidaMinima = 0.5f;
    p.vidaMaxima = 1.5f;
    p.tamanoMinimo = 0.1f;
    p.tamanoMaximo = 0.9f;
    p.velocidadMinima = 0.5f;
    p.velocidadMaxima = 3.0f;
    p.dispersion = 0.7f;
    const float origen[3] = {0.0f, 0.0f, 0.0f};

    SistemaParticulas primera(20);
    primera.fijarSemilla(4242);
    SistemaParticulas segunda(20);
    segunda.fijarSemilla(4242);
    for (int i = 0; i < 10; ++i) {
        primera.emitir(p, origen);
        segunda.emitir(p, origen);
    }

    CHECK(primera.cantidad() == segunda.cantidad(),
          "Reproducibilidad: las dos simulaciones tienen la misma cantidad");
    bool iguales = true;
    auto ita = primera.particulas().begin();
    auto itb = segunda.particulas().begin();
    for (; ita != primera.particulas().end() && itb != segunda.particulas().end();
         ++ita, ++itb) {
        for (int i = 0; i < 3; ++i) {
            if (!cerca(ita->posicion[i], itb->posicion[i]) ||
                !cerca(ita->velocidad[i], itb->velocidad[i]))
                iguales = false;
        }
    }
    CHECK(iguales, "Reproducibilidad: la misma semilla da la misma simulacion");

    // Otra semilla da otra secuencia.
    SistemaParticulas otra(20);
    otra.fijarSemilla(1);
    for (int i = 0; i < 10; ++i) otra.emitir(p, origen);
    bool distinta = false;
    auto ita2 = primera.particulas().begin();
    auto itc = otra.particulas().begin();
    for (; ita2 != primera.particulas().end() && itc != otra.particulas().end();
         ++ita2, ++itc) {
        if (!cerca(ita2->velocidad[1], itc->velocidad[1])) distinta = true;
    }
    CHECK(distinta, "Reproducibilidad: cambiar la semilla cambia la secuencia");
}

// --- Perfil de efecto: curvas ------------------------------------------------

void probarCurvas() {
    PerfilParticula perfil;
    perfil.tamanos = {0.4f, 1.0f, 1.2f};
    perfil.colores = {{1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};

    CHECK(cerca(perfil.multiplicadorTamano(0.0f), 0.4f),
          "Curvas: el primer punto de la curva es el nacimiento");
    CHECK(cerca(perfil.multiplicadorTamano(0.5f), 1.0f),
          "Curvas: el punto medio de la curva se interpola");
    CHECK(cerca(perfil.multiplicadorTamano(1.0f), 1.2f),
          "Curvas: el ultimo punto de la curva es la muerte");
    CHECK(cerca(perfil.multiplicadorTamano(0.25f), 0.7f),
          "Curvas: entre dos puntos la interpolacion es lineal");
    CHECK(cerca(perfil.multiplicadorTamano(-1.0f), 0.4f),
          "Curvas: una fraccion fuera de rango se acota al inicio");
    CHECK(cerca(perfil.multiplicadorTamano(5.0f), 1.2f),
          "Curvas: una fraccion fuera de rango se acota al final");

    float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    perfil.colorEn(0.0f, color);
    CHECK(cerca(color[0], 1.0f) && cerca(color[1], 0.0f) && cerca(color[3], 1.0f),
          "Curvas: el color del nacimiento es el primer punto");
    perfil.colorEn(1.0f, color);
    CHECK(cerca(color[2], 1.0f) && cerca(color[3], 0.0f),
          "Curvas: el color de la muerte es el ultimo punto");
    perfil.colorEn(0.5f, color);
    CHECK(cerca(color[0], 0.5f) && cerca(color[2], 0.5f),
          "Curvas: el color se interpola entre los puntos");

    // Un perfil sin curvas no cambia el tamano ni el color.
    PerfilParticula vacio;
    CHECK(!vacio.tieneCurvas(), "Curvas: un perfil vacio no tiene curvas");
    CHECK(cerca(vacio.multiplicadorTamano(0.5f), 1.0f),
          "Curvas: sin curva de tamanos el multiplicador es uno");
    vacio.colorEn(0.5f, color);
    CHECK(cerca(color[0], 1.0f) && cerca(color[3], 1.0f),
          "Curvas: sin curva de colores el color es blanco opaco");
}

// --- Perfil de efecto: lectura del JSON ---------------------------------------

void escribirArchivo(const std::string& ruta, const std::string& contenido) {
    std::ofstream archivo(ruta);
    archivo << contenido;
}

void probarLectorPerfil() {
    TempPruebas::CarpetaPrueba carpeta("funshi_particulas_tests");
    const std::string base = carpeta.ruta().string();

    // Un perfil completo.
    const std::string ruta = base + "/humo.json";
    escribirArchivo(ruta, R"({
  "version": 1,
  "nombre": "Humo",
  "textura": "particulas/humo.png",
  "vida": [0.6, 1.2],
  "velocidad": [1.0, 2.0],
  "aceleracion": [0.0, -1.0, 0.5],
  "rozamiento": 0.25,
  "tamanos": [0.4, 1.0, 1.2],
  "colores": [[1, 1, 1, 0.6], [0.5, 0.5, 0.5, 0.0]]
})");
    PerfilParticula perfil;
    std::string error;
    CHECK(cargarPerfilDesdeJson(ruta, perfil, error),
          "Lector: un perfil valido se carga");
    CHECK(perfil.nombre == "Humo", "Lector: se lee el nombre");
    CHECK(perfil.textura == "particulas/humo.png", "Lector: se lee la textura");
    CHECK(cerca(perfil.vidaMinima, 0.6f) && cerca(perfil.vidaMaxima, 1.2f),
          "Lector: se lee el rango de vida");
    CHECK(cerca(perfil.velocidadMinima, 1.0f) && cerca(perfil.velocidadMaxima, 2.0f),
          "Lector: se lee el rango de velocidad");
    CHECK(cerca(perfil.aceleracion[1], -1.0f) && cerca(perfil.aceleracion[2], 0.5f),
          "Lector: se lee la aceleracion");
    CHECK(cerca(perfil.rozamiento, 0.25f), "Lector: se lee el rozamiento");
    CHECK(perfil.tamanos.size() == 3, "Lector: se lee la curva de tamanos");
    CHECK(perfil.colores.size() == 2, "Lector: se lee la curva de colores");
    CHECK(cerca(perfil.colores[0][3], 0.6f), "Lector: se lee el alfa del color");
    CHECK(perfil.tieneCurvas(), "Lector: un perfil con curvas lo dice");

    // Un perfil minimo: todos los campos son opcionales.
    const std::string minimo = base + "/minimo.json";
    escribirArchivo(minimo, "{ \"nombre\": \"Simple\" }");
    PerfilParticula simple;
    CHECK(cargarPerfilDesdeJson(minimo, simple, error),
          "Lector: un perfil con un solo campo se carga");
    CHECK(simple.nombre == "Simple", "Lector: se lee el nombre del perfil minimo");
    CHECK(!simple.tieneCurvas(), "Lector: el perfil minimo no tiene curvas");

    // Errores.
    PerfilParticula descartada;
    CHECK(!cargarPerfilDesdeJson(base + "/no-existe.json", descartada, error),
          "Lector: un archivo inexistente se rechaza");
    CHECK(!error.empty(), "Lector: el archivo inexistente se explica");

    const std::string roto = base + "/roto.json";
    escribirArchivo(roto, "{ esto no es json ");
    CHECK(!cargarPerfilDesdeJson(roto, descartada, error),
          "Lector: un JSON mal formado se rechaza");
    CHECK(!error.empty(), "Lector: el JSON mal formado se explica");

    const std::string vidaInvalida = base + "/vida.json";
    escribirArchivo(vidaInvalida, "{ \"vida\": [2.0, 1.0] }");
    CHECK(!cargarPerfilDesdeJson(vidaInvalida, descartada, error),
          "Lector: un rango de vida invertido se rechaza");
    CHECK(!error.empty(), "Lector: el rango invertido se explica");

    const std::string vidaMala = base + "/vida-mala.json";
    escribirArchivo(vidaMala, "{ \"vida\": 5 }");
    CHECK(!cargarPerfilDesdeJson(vidaMala, descartada, error),
          "Lector: un rango de vida que no es un array se rechaza");

    const std::string colorMalo = base + "/color.json";
    escribirArchivo(colorMalo, "{ \"colores\": [[1, 0, 0]] }");
    CHECK(!cargarPerfilDesdeJson(colorMalo, descartada, error),
          "Lector: un color con menos de 4 componentes se rechaza");
}

// --- Geometria del lote ------------------------------------------------------

void probarGeometria() {
    ParticulasBuilder builder;
    CHECK(builder.vacio(), "Geometria: el lote arranca vacio");
    CHECK(builder.vertices() == nullptr, "Geometria: sin geometria no hay vertices");

    Particula particula;
    particula.posicion[0] = 5.0f;
    particula.posicion[1] = 0.0f;
    particula.posicion[2] = 1.0f;
    particula.tamano = 2.0f;  // lado del quad: el semilado es 1
    particula.color[0] = 0.5f;
    particula.color[1] = 0.25f;
    particula.color[2] = 0.125f;
    particula.color[3] = 1.0f;

    const float uv[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    builder.agregarParticula(particula, uv);

    CHECK(!builder.vacio(), "Geometria: hay geometria");
    CHECK(builder.cantidadParticulas() == 1, "Geometria: una particula agregada");
    CHECK(builder.cantidadVertices() == 6,
          "Geometria: cada particula aporta seis vertices (dos triangulos)");
    CHECK(builder.bytes() == 6 * ParticulasBuilder::kFloatsPorVertice * sizeof(float),
          "Geometria: el tamano en bytes es el del vertice por los vertices");
    CHECK(builder.vertices() != nullptr, "Geometria: hay vertices");

    const float* v = builder.vertices();
    // El vertice va de 9 floats: posicion (3), uv (2) y color (4). El primer
    // vertice es la esquina inferior izquierda del quad, en el plano XY, con
    // la particula en el centro: el lado del quad es 2, asi que el semilado
    // es 1 y la esquina cae en (4.5, -0.5).
    CHECK(cerca(v[0], 4.5f) && cerca(v[1], -0.5f) && cerca(v[2], 1.0f),
          "Geometria: el primer vertice es la esquina del quad");
    CHECK(cerca(v[3], 0.0f) && cerca(v[4], 0.0f),
          "Geometria: la uv del primer vertice es la esquina de la region");
    CHECK(cerca(v[5], 0.5f) && cerca(v[6], 0.25f) && cerca(v[8], 1.0f),
          "Geometria: el color del vertice es el de la particula");
    // El tercer vertice es la esquina superior derecha del quad.
    CHECK(cerca(v[18], 5.5f) && cerca(v[19], 0.5f),
          "Geometria: la tercera esquina esta en el otro extremo del quad");
    CHECK(cerca(v[21], 1.0f) && cerca(v[22], 1.0f),
          "Geometria: la uv de la tercera esquina es la opuesta");

    // Con rotacion de 90 grados las esquinas giran: la inferior izquierda
    // (-0.5, -0.5) pasa a (-0.5, 0.5) en el espacio rotado.
    builder.limpiar();
    Particula girada = particula;
    girada.rotacion = 1.5707963f;
    builder.agregarParticula(girada, uv);
    const float* g = builder.vertices();
    CHECK(cerca(g[0], 5.5f, 1e-3f) && cerca(g[1], -0.5f, 1e-3f),
          "Geometria: la rotacion gira el quad alrededor de la particula");

    // Varias particulas se acumulan.
    builder.limpiar();
    builder.agregarParticula(particula, uv);
    builder.agregarParticula(particula, uv);
    CHECK(builder.cantidadParticulas() == 2, "Geometria: se acumulan particulas");
    CHECK(builder.cantidadVertices() == 12, "Geometria: dos particulas, doce vertices");
}

}  // namespace

int main() {
    probarEmision();
    probarCono();
    probarPresupuesto();
    probarIntegracion();
    probarMuerte();
    probarReproducibilidad();
    probarCurvas();
    probarLectorPerfil();
    probarGeometria();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
