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

namespace SombrasTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace SombrasTestsDetalle
using namespace SombrasTestsDetalle;

// Proyecta un punto con la matriz y devuelve el NDC (la division por w).
glm::vec3 aNdc(const glm::mat4& m, const glm::vec3& p) {
    const glm::vec4 clip = m * glm::vec4(p, 1.0f);
    if (std::fabs(clip.w) < 1e-8f) return glm::vec3(0.f);
    return glm::vec3(clip) / clip.w;
}

// Direccion hacia la que mira la camara de una vista, en coordenadas del
// mundo: la tercera fila de la rotacion es el eje +Z de la camara (hacia
// atras), asi que la mirada es esa fila negada.
glm::vec3 adelanteDeVista(const glm::mat4& vista) {
    const glm::mat3 r(vista);
    return -glm::vec3(r[0][2], r[1][2], r[2][2]);
}

bool esFinita(const glm::mat4& m) {
    for (int c = 0; c < 4; ++c)
        for (int f = 0; f < 4; ++f) {
            const float v = m[c][f];
            if (!std::isfinite(v)) return false;
        }
    return true;
}

bool cercaDe(float a, float b, float tol) {
    return std::fabs(a - b) <= tol;
}

bool cercaDe(const glm::vec3& a, const glm::vec3& b, float tol) {
    return cercaDe(a.x, b.x, tol) && cercaDe(a.y, b.y, tol) &&
           cercaDe(a.z, b.z, tol);
}

// --- Matriz de luz ----------------------------------------------------------

void testVistaMiraEnLaDireccion() {
    const glm::vec3 dir(0.f, -1.f, 0.f);
    const glm::mat4 vista = vistaLuz(glm::vec3(0.f, 10.f, 0.f), dir);
    // La vista mira hacia su direccion (el eje -Z de la vista).
    CHECK(cercaDe(adelanteDeVista(vista), glm::vec3(0.f, -1.f, 0.f)),
          "la vista de la luz mira hacia su direccion");

    const glm::vec3 dirLateral(1.f, 0.f, 0.f);
    const glm::mat4 vistaLateral = vistaLuz(glm::vec3(0.f, 10.f, 0.f), dirLateral);
    CHECK(cercaDe(adelanteDeVista(vistaLateral), glm::vec3(1.f, 0.f, 0.f)),
          "la vista de la luz lateral mira hacia el este");
}

void testVistaVerticalNoEsNaN() {
    // Una luz que apunta justo hacia arriba tiene el vector auxiliar
    // (0,1,0) paralelo a su direccion: la vista se degenera. El auxiliar
    // tiene que cambiar para que la matriz siga siendo finita.
    const glm::mat4 vista = vistaLuz(glm::vec3(0.f, 0.f, 0.f),
                                      glm::vec3(0.f, 1.f, 0.f));
    CHECK(esFinita(vista), "la vista de una luz vertical es finita");
    CHECK(cercaDe(adelanteDeVista(vista), glm::vec3(0.f, 1.f, 0.f)),
          "la vista de una luz vertical mira hacia arriba");
}

void testVistaDireccionNula() {
    const glm::mat4 vista = vistaLuz(glm::vec3(0.f, 5.f, 0.f), glm::vec3(0.f));
    CHECK(esFinita(vista), "una direccion de luz nula no produce un NaN");
}

void testProyeccionAbragaLaCaja() {
    const float radio = 8.f;
    const float cerca = 0.5f;
    const float lejos = 40.f;
    const glm::mat4 proyeccion = proyeccionLuz(radio, cerca, lejos);

    // Las ocho esquinas de la caja del frustum, en espacio de la luz. En la
    // vista la camara mira hacia -Z, asi que lo que tiene delante lleva z
    // negativa.
    bool todosDentro = true;
    for (int i = 0; i < 8; ++i) {
        const glm::vec3 esquina((i & 1) ? radio : -radio,
                                (i & 2) ? radio : -radio,
                                (i & 4) ? -lejos : -cerca);
        const glm::vec3 ndc = aNdc(proyeccion, esquina);
        if (ndc.x < -1.001f || ndc.x > 1.001f || ndc.y < -1.001f ||
            ndc.y > 1.001f || ndc.z < -1.001f || ndc.z > 1.001f) {
            todosDentro = false;
        }
    }
    CHECK(todosDentro,
          "la proyeccion abarca la caja de lado 2*radio entre cerca y lejos");
}

void testProyeccionVolumenValido() {
    // El plano cercano se proyecta a -1 y el lejano a +1.
    const glm::mat4 proyeccion = proyeccionLuz(5.f, 1.f, 20.f);
    const glm::vec3 ndcCerca = aNdc(proyeccion, glm::vec3(0.f, 0.f, -1.f));
    const glm::vec3 ndcLejos = aNdc(proyeccion, glm::vec3(0.f, 0.f, -20.f));
    CHECK(cercaDe(ndcCerca.z, -1.f), "el plano cercano se proyecta a -1");
    CHECK(cercaDe(ndcLejos.z, 1.f), "el plano lejano se proyecta a +1");

    // Con el plano lejano por delante del cercano la proyeccion se invertiria:
    // el motor lo corrige para que el frustum siga teniendo volumen.
    const glm::mat4 invertida = proyeccionLuz(5.f, 10.f, 1.f);
    const glm::vec3 ndcCorregidoCerca =
        aNdc(invertida, glm::vec3(0.f, 0.f, -10.f));
    const glm::vec3 ndcCorregidoLejos =
        aNdc(invertida, glm::vec3(0.f, 0.f, -10.001f));
    CHECK(cercaDe(ndcCorregidoCerca.z, -1.f) &&
              cercaDe(ndcCorregidoLejos.z, 1.f),
          "el frustum invertido se corrige en vez de dar la vuelta");
    CHECK(esFinita(proyeccionLuz(0.f, 1.f, 2.f)),
          "un radio nulo se corrige en vez de deformar la matriz");
}

void testMatrizLuzEnfocaElFoco() {
    const FocoLuz foco = focoPosicional(glm::vec3(0.f, 10.f, 0.f),
                                         glm::vec3(0.f, -1.f, 0.f), 12.f, 60.f);
    const glm::mat4 m = matrizLuz(foco);
    CHECK(esFinita(m), "la matriz de la luz es finita");

    // Un punto del eje de la luz (delante de ella) cae en el centro del
    // frustum: en el NDC, X e Y valen 0.
    const glm::vec3 enElEje = foco.posicion + foco.direccion * 20.f;
    const glm::vec3 ndcEje = aNdc(m, enElEje);
    CHECK(cercaDe(ndcEje.x, 0.f) && cercaDe(ndcEje.y, 0.f),
          "el eje de la luz cae en el centro del mapa de sombras");
    CHECK(ndcEje.z > -1.f && ndcEje.z < 1.f,
          "el eje de la luz cae dentro del rango de profundidad");
}

void testMatrizCubreLaEscena() {
    // Una caja de lado 2*radio alrededor del centro de la escena tiene que
    // caber en el mapa de sombras. La luz se toma lateral para que la caja
    // del mundo y el frustum tengan los mismos ejes.
    const glm::vec3 centro(0.f, 0.f, 0.f);
    const glm::vec3 dir(0.f, 0.f, -1.f);
    const float radio = 20.f;
    const float alcance = 50.f;
    const FocoLuz foco = focoDireccional(centro, dir, radio, alcance);
    const glm::mat4 m = matrizLuz(foco);
    CHECK(esFinita(m), "la matriz de una luz direccional es finita");

    bool todosDentro = true;
    for (int i = 0; i < 8; ++i) {
        const glm::vec3 esquina = centro + glm::vec3((i & 1) ? radio : -radio,
                                                     (i & 2) ? radio : -radio,
                                                     (i & 4) ? radio : -radio);
        const glm::vec3 ndc = aNdc(m, esquina);
        if (ndc.x < -1.05f || ndc.x > 1.05f || ndc.y < -1.05f || ndc.y > 1.05f) {
            todosDentro = false;
        }
    }
    CHECK(todosDentro,
          "la caja de la escena cabe en el mapa de sombras de una luz "
          "direccional");
}

void testLaDireccionManda() {
    const glm::vec3 centro(0.f, 0.f, 0.f);
    const glm::mat4 abajo = matrizLuz(
        focoDireccional(centro, glm::vec3(0.f, -1.f, 0.f), 10.f, 30.f));
    const glm::mat4 arriba = matrizLuz(
        focoDireccional(centro, glm::vec3(0.f, 1.f, 0.f), 10.f, 30.f));
    CHECK(abajo != arriba,
          "una luz que apunta en sentido contrario da otra matriz");

    // La direccion de la luz direccional es la que se normaliza: una
    // direccion larga da la misma matriz que su version unitaria.
    const glm::mat4 corta = matrizLuz(
        focoDireccional(centro, glm::vec3(0.f, -1.f, 0.f), 10.f, 30.f));
    const glm::mat4 larga = matrizLuz(
        focoDireccional(centro, glm::vec3(0.f, -7.f, 0.f), 10.f, 30.f));
    CHECK(corta == larga, "la direccion de la luz se normaliza");
}

// --- Componente de sombras ---------------------------------------------------

void testValoresPorDefecto() {
    Sombras sombras;
    CHECK(sombras.estaActivado(), "la sombra proyecta por defecto");
    CHECK(sombras.getResolucion() == 1024,
          "el mapa de sombras es de 1024 por defecto");
    CHECK(sombras.getSesgo() > 0.f, "el sesgo por defecto no es cero");
    CHECK(cercaDe(sombras.getFuerza(), 1.f), "la sombra es completa por defecto");
    CHECK(sombras.getRadio() > 0.f, "el radio por defecto es positivo");
    CHECK(sombras.getDistancia() > 0.f, "la distancia por defecto es positiva");
}

void testValoresAcotados() {
    Sombras sombras;

    sombras.setResolucion(100);
    CHECK(sombras.getResolucion() == 256,
          "una resolucion ridicula se acota al minimo");
    sombras.setResolucion(99999);
    CHECK(sombras.getResolucion() == 4096,
          "una resolucion enorme se acota al maximo");
    sombras.setResolucion(1000);
    CHECK(sombras.getResolucion() == 512 || sombras.getResolucion() == 1024,
          "la resolucion se ajusta a una potencia de dos");

    sombras.setSesgo(-1.f);
    CHECK(sombras.getSesgo() == 0.f, "un sesgo negativo se acota a cero");
    sombras.setSesgo(10.f);
    CHECK(sombras.getSesgo() <= 0.1f, "un sesgo enorme se acota");

    sombras.setFuerza(5.f);
    CHECK(sombras.getFuerza() == 1.f, "una fuerza mayor que uno se acota");
    sombras.setFuerza(-1.f);
    CHECK(sombras.getFuerza() == 0.f, "una fuerza negativa se acota a cero");

    sombras.setRadio(0.f);
    CHECK(sombras.getRadio() > 0.f, "un radio cero se acota a un valor util");
    sombras.setDistancia(0.f);
    CHECK(sombras.getDistancia() > 0.f,
          "una distancia cero se acota a un valor util");
}

void testViajeDeIdaYVuelta() {
    TempPruebas::CarpetaPrueba carpeta("funshi_sombras_tests");
    const auto ruta = carpeta.ruta() / "sombras.bin";

    Sombras original;
    original.setActivado(false);
    original.setResolucion(2048);
    original.setSesgo(0.004f);
    original.setFuerza(0.6f);
    original.setRadio(35.f);
    original.setDistancia(120.f);

    {
        std::ofstream salida(ruta, std::ios::binary);
        CHECK(salida.is_open(), "se abre el archivo para escribir");
        original.saveComponent(&salida);
    }

    Sombras leida;
    {
        std::ifstream entrada(ruta, std::ios::binary);
        CHECK(entrada.is_open(), "se abre el archivo para leer");
        leida.loadComponent(&entrada);
    }

    CHECK(leida.estaActivado() == false, "el estado de la sombra se conserva");
    CHECK(leida.getResolucion() == 2048, "la resolucion se conserva");
    CHECK(cercaDe(leida.getSesgo(), 0.004f), "el sesgo se conserva");
    CHECK(cercaDe(leida.getFuerza(), 0.6f), "la fuerza se conserva");
    CHECK(cercaDe(leida.getRadio(), 35.f), "el radio se conserva");
    CHECK(cercaDe(leida.getDistancia(), 120.f), "la distancia se conserva");
}

void testLecturaAcotaValores() {
    // Un archivo con valores imposibles (editado a mano o de otra version)
    // no puede dejar la sombra en un estado imposible.
    TempPruebas::CarpetaPrueba carpeta("funshi_sombras_tests_basura");
    const auto ruta = carpeta.ruta() / "sombras_basura.bin";

    {
        std::ofstream salida(ruta, std::ios::binary);
        const bool activado = true;
        const int resolucion = 999999;
        const float sesgo = -5.f;
        const float fuerza = 42.f;
        const float radio = -3.f;
        const float distancia = 0.f;
        salida.write(reinterpret_cast<const char*>(&activado), sizeof(bool));
        salida.write(reinterpret_cast<const char*>(&resolucion), sizeof(int));
        salida.write(reinterpret_cast<const char*>(&sesgo), sizeof(float));
        salida.write(reinterpret_cast<const char*>(&fuerza), sizeof(float));
        salida.write(reinterpret_cast<const char*>(&radio), sizeof(float));
        salida.write(reinterpret_cast<const char*>(&distancia), sizeof(float));
    }

    Sombras leida;
    {
        std::ifstream entrada(ruta, std::ios::binary);
        leida.loadComponent(&entrada);
    }
    CHECK(leida.getResolucion() <= 4096 && leida.getResolucion() >= 256,
          "una resolucion absurda se acota al leer");
    CHECK(leida.getSesgo() >= 0.f, "un sesgo negativo se acota al leer");
    CHECK(leida.getFuerza() >= 0.f && leida.getFuerza() <= 1.f,
          "una fuerza fuera de rango se acota al leer");
    CHECK(leida.getRadio() > 0.f, "un radio negativo se acota al leer");
    CHECK(leida.getDistancia() > 0.f, "una distancia cero se acota al leer");
}

void testSeleccionDeProyectores() {
    CandidatoProyector candidatos[4];
    // Dos luces que si pueden proyectar.
    candidatos[0].tipo = TipoLuzProyector::Direccional;
    candidatos[0].direccion = glm::vec3(0.f, -1.f, 0.f);
    candidatos[1].tipo = TipoLuzProyector::Spot;
    candidatos[1].posicion = glm::vec3(5.f, 10.f, 5.f);
    candidatos[1].direccion = glm::vec3(0.f, -1.f, 0.f);
    candidatos[1].resolucion = 512;
    // Una luz puntual: ya puede proyectar sombra (cubemap de 6 caras).
    candidatos[2].tipo = TipoLuzProyector::Puntual;
    // Y otra direccional, que se queda fuera por el limite.
    candidatos[3].tipo = TipoLuzProyector::Direccional;

    ProyectorSombra salida[4];
    const ResultadoSeleccion resultado = seleccionarProyectores(
        candidatos, 4, 2, glm::vec3(0.f), salida);

    CHECK(resultado.elegidos == 2, "se eligen los proyectores que caben");
    CHECK(resultado.porLimite == 2,
          "los proyectores que exceden el limite se cuentan aparte");
    CHECK(salida[0].resolucion == 1024,
          "el primer proyector conserva su resolucion");
    CHECK(salida[1].resolucion == 512,
          "el segundo proyector conserva su resolucion");
    CHECK(esFinita(salida[0].matrizLuz),
          "la matriz del proyector es finita");
    CHECK(esFinita(salida[1].matrizLuz),
          "la matriz del proyector es finita");
    CHECK(salida[0].textura == Rendering::Backend::kInvalidHandle,
          "el proyector todavia no tiene textura (la pone el mapa de sombras)");
}

void testSeleccionSinCandidatos() {
    ProyectorSombra salida[2];
    const ResultadoSeleccion sinNada = seleccionarProyectores(
        nullptr, 0, 4, glm::vec3(0.f), salida);
    CHECK(sinNada.elegidos == 0, "sin candidatos no hay proyectores");

    CandidatoProyector candidato;
    candidato.tipo = TipoLuzProyector::Direccional;
    const ResultadoSeleccion sinLimite = seleccionarProyectores(
        &candidato, 1, 0, glm::vec3(0.f), salida);
    CHECK(sinLimite.elegidos == 0, "sin limite no hay proyectores");
    CHECK(sinLimite.porLimite == 1,
          "el candidato se cuenta como descartado por el limite");
}

void testMapaDeSombras() {
    BackendFalso backend;
    MapaSombras mapa(backend);

    CHECK(mapa.preparar(0, 1024), "el mapa crea su objetivo de profundidad");
    CHECK(backend.creados == 1, "el mapa crea un objetivo de profundidad");
    CHECK(backend.ultimoTamano == 1024, "el objetivo tiene el tamano pedido");
    CHECK(mapa.activos() == 1, "el mapa cuenta un mapa activo");
    CHECK(mapa.mapa(0).textura != Rendering::Backend::kInvalidHandle,
          "el mapa expone la textura de profundidad");
    CHECK(mapa.mapa(0).resolucion == 1024,
          "el mapa recuerda la resolucion con la que se creo");

    // Pedirlo otra vez con la misma resolucion no crea otro objetivo.
    mapa.preparar(0, 1024);
    CHECK(backend.creados == 1, "el mapa se reutiliza si la resolucion no cambia");

    // Cambiar la resolucion rehace el objetivo.
    mapa.preparar(0, 512);
    CHECK(backend.creados == 2, "cambiar la resolucion rehace el mapa");
    CHECK(backend.destruidos == 1, "el mapa viejo se suelta");
    CHECK(backend.ultimoTamano == 512, "el objetivo nuevo tiene el tamano nuevo");

    // Un segundo proyector tiene su propio mapa.
    mapa.preparar(1, 1024);
    CHECK(mapa.activos() == 2, "cada proyector tiene su mapa");

    // Al soltar el segundo, el primero sigue vivo.
    mapa.liberarDesde(1);
    CHECK(mapa.activos() == 1, "los mapas que no se usan se sueltan");
    CHECK(backend.destruidos == 2, "el mapa del segundo proyector se destruye");

    // Un mapa que no sepidio esta vacio (no es un mapa valido).
    CHECK(mapa.mapa(5).target == Rendering::Backend::kInvalidHandle,
          "un mapa que no se pidio no existe");
}

void testMapaCubemap() {
    // Una luz puntual necesita un cubemap de 6 caras: preparar con
    // esCubemap = true debe crear un objetivo de profundidad de cubemap.
    BackendFalso backend;
    MapaSombras mapa(backend);

    CHECK(mapa.preparar(0, 1024, true), "el mapa crea un cubemap de profundidad");
    CHECK(backend.creados == 1, "el mapa crea un objetivo de profundidad");
    CHECK(mapa.mapa(0).esCubemap, "el mapa marca que es cubemap");
    CHECK(mapa.mapa(0).target != Rendering::Backend::kInvalidHandle,
          "el mapa expone el FBO del cubemap");
    CHECK(mapa.mapa(0).textura != Rendering::Backend::kInvalidHandle,
          "el cubemap tiene textura de profundidad");
    CHECK(mapa.activos() == 1, "el mapa cuenta un mapa activo");

    // Pedirlo otra vez con la misma resolucion y tipo no crea otro objetivo.
    mapa.preparar(0, 1024, true);
    CHECK(backend.creados == 1, "el cubemap se reutiliza si no cambia nada");

    // Cambiar a mapa 2D rehace el objetivo.
    mapa.preparar(0, 1024, false);
    CHECK(backend.creados == 2, "cambiar de cubemap a 2D rehace el mapa");
    CHECK(!mapa.mapa(0).esCubemap, "el mapa deja de ser cubemap");
}
