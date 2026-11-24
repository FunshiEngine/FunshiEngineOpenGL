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
#include "SombrasTestsDetalle.h"

void testElProyectorConservaElIndiceDeSuLuz() {
    // La sombra tiene que atenuar la contribucion de SU luz: por eso el
    // proyector lleva el indice de la luz a la que pertenece.
    CandidatoProyector candidato;
    candidato.tipo = TipoLuzProyector::Spot;
    candidato.indiceLuz = 3;
    candidato.sesgo = 0.002f;
    candidato.fuerza = 0.5f;

    ProyectorSombra salida[1];
    const ResultadoSeleccion resultado = seleccionarProyectores(
        &candidato, 1, 4, glm::vec3(0.f), salida);
    CHECK(resultado.elegidos == 1, "el candidato se elige");
    CHECK(salida[0].indiceLuz == 3,
          "el proyector conserva el indice de su luz");
    CHECK(salida[0].sesgo == 0.002f, "el proyector conserva el sesgo");
    CHECK(salida[0].fuerza == 0.5f, "el proyector conserva la fuerza");
}

void testElLimiteCoincideConElShader() {
    // El shader de mallas declara cuantos mapas puede consultar. Si el motor
    // sube el limite y el shader no, sobran mapas; si el shader sube y el
    // motor no, el motor escribe fuera del array.
    const std::string declaracion =
        "const int MAX_SHADOWS = " +
        std::to_string(Rendering::kMaxProyectoresSombra) + ";";
    CHECK(std::string(kDefaultFragmentShader).find(declaracion) !=
              std::string::npos,
          "el limite de mapas del shader es el mismo que el del motor");
}

// Cada cara del cubemap mira en la direccion de su cara (+X, -X, +Y, -Y, +Z,
// -Z): un punto delante de la luz en esa direccion cae en el centro de la cara.
void testCarasDelCuboMiranSuDireccion() {
    const glm::vec3 luz(1.f, 2.f, 3.f);
    glm::mat4 caras[kCarasCuboLuz];
    matricesCaraCubo(luz, 50.f, caras);
    const glm::vec3 dirs[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
                               {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    bool centradas = true;
    for (int c = 0; c < 6; ++c) {
        glm::vec4 clip = caras[c] * glm::vec4(luz + dirs[c] * 10.f, 1.f);
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (!cercaDe(ndc.x, 0.f) || !cercaDe(ndc.y, 0.f) || ndc.z <= -1.f || ndc.z >= 1.f)
            centradas = false;
    }
    CHECK(centradas, "cada cara del cubemap mira hacia su direccion");

    // Convencion de OpenGL en +X: arriba es -Y, asi que un punto con y mayor
    // que la luz queda en la mitad inferior de la cara (ndc.y < 0).
    glm::vec4 clip = caras[0] * glm::vec4(luz + glm::vec3(10.f, 3.f, 0.f), 1.f);
    CHECK(clip.y / clip.w < 0.f, "la cara +X usa -Y como arriba (convencion GL)");
}

// La spot usa perspectiva: un punto en el eje del cono cae en el centro y uno
// en el borde del cono queda dentro del mapa.
void testSpotEnPerspectiva() {
    const glm::vec3 pos(0.f, 10.f, 0.f);
    const glm::mat4 m = matrizLuzSpot(pos, glm::vec3(0.f, -1.f, 0.f), 30.f, 40.f);
    CHECK(esFinita(m), "la matriz de la spot es finita");
    glm::vec4 eje = m * glm::vec4(0.f, 0.f, 0.f, 1.f);
    CHECK(cercaDe(eje.x / eje.w, 0.f) && cercaDe(eje.y / eje.w, 0.f),
          "el eje de la spot cae en el centro del mapa");
    const float borde = 10.f * std::tan(glm::radians(30.f));
    glm::vec4 b = m * glm::vec4(borde, 0.f, 0.f, 1.f);
    CHECK(std::fabs(b.x / b.w) < 1.f, "el borde del cono queda dentro del mapa");
    glm::vec4 lejos = m * glm::vec4(0.f, -29.f, 0.f, 1.f);
    glm::vec4 cerca = m * glm::vec4(0.f, 5.f, 0.f, 1.f);
    CHECK(lejos.z / lejos.w > cerca.z / cerca.w, "la profundidad crece con la distancia");
}

// Barrer el centro a lo largo de una fraccion de texel deja la matriz
// direccional casi siempre igual (a lo sumo cambia al cruzar un borde de texel
// en cada eje): la sombra no "nada" al mover la camara. Sin estabilizar, cada
// posicion daria una matriz distinta.
void testDireccionalEstable() {
    const glm::vec3 dir(0.3f, -1.f, 0.2f);
    const float radio = 20.f;
    const int res = 1024;
    const float texel = 2.f * radio / res;
    auto igual = [](const glm::mat4& a, const glm::mat4& b) {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (std::fabs(a[c][r] - b[c][r]) > 1e-5f) return false;
        return true;
    };
    std::vector<glm::mat4> estables;
    std::vector<glm::mat4> libres;
    for (int i = 0; i <= 20; ++i) {
        const glm::vec3 centro(3.1f + i * 0.01f * texel, 0.7f, -2.3f);
        const glm::mat4 m = matrizDireccionalEstable(centro, dir, radio, 60.f, res);
        const glm::mat4 n = matrizLuz(focoDireccional(centro, dir, radio, 60.f));
        bool nueva = true;
        for (const glm::mat4& e : estables) if (igual(e, m)) nueva = false;
        if (nueva) estables.push_back(m);
        nueva = true;
        for (const glm::mat4& e : libres) if (igual(e, n)) nueva = false;
        if (nueva) libres.push_back(n);
    }
    CHECK(estables.size() <= 4, "la matriz direccional se ajusta a la grilla de texeles");
    CHECK(libres.size() > estables.size(), "sin ajuste la matriz cambia con cada posicion");
}

int main() {
    testVistaMiraEnLaDireccion();
    testVistaVerticalNoEsNaN();
    testVistaDireccionNula();
    testProyeccionAbragaLaCaja();
    testProyeccionVolumenValido();
    testMatrizLuzEnfocaElFoco();
    testMatrizCubreLaEscena();
    testLaDireccionManda();
    testValoresPorDefecto();
    testValoresAcotados();
    testViajeDeIdaYVuelta();
    testLecturaAcotaValores();
    testSeleccionDeProyectores();
    testSeleccionSinCandidatos();
    testMapaDeSombras();
    testMapaCubemap();
    testElProyectorConservaElIndiceDeSuLuz();
    testElLimiteCoincideConElShader();
    testCarasDelCuboMiranSuDireccion();
    testSpotEnPerspectiva();
    testDireccionalEstable();

    std::cout << "SombrasTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}
