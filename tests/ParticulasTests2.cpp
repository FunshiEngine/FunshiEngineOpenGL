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
#include "ParticulasTestsDetalle.h"

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
