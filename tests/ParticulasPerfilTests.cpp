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
// Pruebas headless del escritor de perfiles de efecto y del flujo completo
// hasta el emisor:
//
//   - guardarPerfilAJson produce un JSON que cargarPerfilDesdeJson devuelve
//     con los mismos valores (ida y vuelta por el archivo).
//   - El escritor exige las mismas invariantes que el lector: un perfil que
//     no las cumple no se escribe.
//   - setRutaPerfil de EmisorParticulas carga un perfil recien escrito (el
//     mismo camino que recorre el drag & drop del inspector).
//   - La serializacion binaria del emisor conserva la ruta del perfil (la
//     recarga del perfil ocurre al cargar la escena, no al deserializar).
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Objetos/Componentes/EmisorParticulas.h"
#include "../FunshiEngineGL/src/Particulas/EscritorPerfil.h"
#include "../FunshiEngineGL/src/Particulas/LectorPerfil.h"
#include "TempPruebas.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Comparacion de flotantes con margen: los valores viajan por texto (JSON) y
// por binario (serializacion del emisor), y se exige igualdad redondeada.
bool cerca(float a, float b) { return std::fabs(a - b) <= 1e-5f; }

PerfilParticula perfilCompleto() {
    PerfilParticula perfil;
    perfil.nombre = "Humo de prueba";
    perfil.textura = "particulas/humo.png";
    perfil.vidaMinima = 0.6f;
    perfil.vidaMaxima = 1.2f;
    perfil.velocidadMinima = 1.0f;
    perfil.velocidadMaxima = 2.5f;
    perfil.aceleracion[0] = 0.0f;
    perfil.aceleracion[1] = -1.5f;
    perfil.aceleracion[2] = 0.25f;
    perfil.rozamiento = 0.2f;
    perfil.tamanos = {0.4f, 1.0f, 1.2f};
    perfil.colores = {{1.0f, 1.0f, 1.0f, 0.6f}, {0.5f, 0.5f, 0.5f, 0.0f}};
    return perfil;
}

void testIdaYVuelta() {
    TempPruebas::CarpetaPrueba carpeta("funshi_particulas_perfil_idavuelta");
    const std::string ruta = (carpeta.ruta() / "perfiles" / "humo.json").string();

    const PerfilParticula original = perfilCompleto();
    std::string error = "sin tocar";
    CHECK(guardarPerfilAJson(ruta, original, error),
          "guardarPerfilAJson escribe el perfil completo");
    CHECK(error.empty(), "sin error al escribir un perfil valido");

    PerfilParticula leida;
    CHECK(cargarPerfilDesdeJson(ruta, leida, error),
          "cargarPerfilDesdeJson lee el archivo recien escrito");
    CHECK(error.empty(), "sin error al leer el perfil escrito");

    CHECK(leida.nombre == original.nombre, "el nombre sobrevive el ciclo");
    CHECK(leida.textura == original.textura, "la textura sobrevive el ciclo");
    CHECK(cerca(leida.vidaMinima, original.vidaMinima) &&
              cerca(leida.vidaMaxima, original.vidaMaxima),
          "el rango de vida sobrevive el ciclo");
    CHECK(cerca(leida.velocidadMinima, original.velocidadMinima) &&
              cerca(leida.velocidadMaxima, original.velocidadMaxima),
          "el rango de velocidad sobrevive el ciclo");
    CHECK(cerca(leida.aceleracion[0], original.aceleracion[0]) &&
              cerca(leida.aceleracion[1], original.aceleracion[1]) &&
              cerca(leida.aceleracion[2], original.aceleracion[2]),
          "la aceleracion sobrevive el ciclo");
    CHECK(cerca(leida.rozamiento, original.rozamiento),
          "el rozamiento sobrevive el ciclo");
    CHECK(leida.tamanos.size() == original.tamanos.size(),
          "la curva de tamanos conserva su cantidad de puntos");
    bool tamanosIguales = leida.tamanos.size() == original.tamanos.size();
    for (std::size_t i = 0; tamanosIguales && i < leida.tamanos.size(); ++i)
        tamanosIguales = cerca(leida.tamanos[i], original.tamanos[i]);
    CHECK(tamanosIguales, "la curva de tamanos conserva sus valores");
    CHECK(leida.colores.size() == original.colores.size(),
          "la curva de colores conserva su cantidad de puntos");
    bool coloresIguales = leida.colores.size() == original.colores.size();
    for (std::size_t i = 0; coloresIguales && i < leida.colores.size(); ++i)
        for (std::size_t c = 0; coloresIguales && c < 4; ++c)
            coloresIguales = cerca(leida.colores[i][c], original.colores[i][c]);
    CHECK(coloresIguales, "la curva de colores conserva sus valores");
}

void testInvariantes() {
    TempPruebas::CarpetaPrueba carpeta("funshi_particulas_perfil_invariantes");
    const std::string ruta = (carpeta.ruta() / "perfil.json").string();

    // Las mismas reglas que impone el lector al cargar: si el escritor las
    // dejara pasar, el archivo quedaria escrito pero ilegible.
    PerfilParticula perfil = perfilCompleto();
    std::string error;

    perfil = perfilCompleto();
    perfil.vidaMinima = 0.0f;
    error.clear();
    CHECK(!guardarPerfilAJson(ruta, perfil, error),
          "no escribe una vida minima de cero");
    CHECK(!error.empty(), "informa el motivo de la vida invalida");

    perfil = perfilCompleto();
    perfil.vidaMinima = 2.0f;
    perfil.vidaMaxima = 1.0f;
    error.clear();
    CHECK(!guardarPerfilAJson(ruta, perfil, error),
          "no escribe una vida minima mayor que la maxima");
    CHECK(!error.empty(), "informa el motivo del rango de vida invertido");

    perfil = perfilCompleto();
    perfil.velocidadMinima = 3.0f;
    perfil.velocidadMaxima = 1.0f;
    error.clear();
    CHECK(!guardarPerfilAJson(ruta, perfil, error),
          "no escribe una velocidad minima mayor que la maxima");
    CHECK(!error.empty(), "informa el motivo del rango de velocidad invertido");

    perfil = perfilCompleto();
    perfil.rozamiento = -0.5f;
    error.clear();
    CHECK(!guardarPerfilAJson(ruta, perfil, error),
          "no escribe un rozamiento negativo");
    CHECK(!error.empty(), "informa el motivo del rozamiento invalido");
}

void testRutasMalas() {
    TempPruebas::CarpetaPrueba carpeta("funshi_particulas_perfil_rutas");
    // Un archivo ocupa el lugar de carpeta padre: create_directories no puede
    // preparar el destino y el escritor debe reportarlo en vez de fingir exito.
    const auto bloqueo = carpeta.ruta() / "bloqueo.txt";
    { std::ofstream salida(bloqueo); salida << "no soy una carpeta"; }
    const std::string ruta = (bloqueo / "sub" / "perfil.json").string();

    std::string error = "sin tocar";
    CHECK(!guardarPerfilAJson(ruta, perfilCompleto(), error),
          "no escribe si la carpeta destino no se puede preparar");
    CHECK(!error.empty(), "informa el motivo del destino inaccesible");
}

void testFlujoEmisor() {
    TempPruebas::CarpetaPrueba carpeta("funshi_particulas_perfil_emisor");
    const std::string ruta = (carpeta.ruta() / "humo.json").string();

    std::string error;
    CHECK(guardarPerfilAJson(ruta, perfilCompleto(), error),
          "el perfil del flujo se escribe sin problemas");

    // El mismo camino que el drag & drop del inspector: asignar la ruta y
    // dejar que el emisor recargue.
    EmisorParticulas emisor;
    CHECK(!emisor.perfil(), "el emisor empieza sin perfil");
    emisor.setRutaPerfil(ruta);
    CHECK(emisor.perfil() != nullptr,
          "setRutaPerfil carga el perfil recien escrito");
    if (emisor.perfil())
        CHECK(emisor.perfil()->nombre == "Humo de prueba",
              "el perfil cargado es el que se escribio");
    CHECK(emisor.errorPerfil().empty(), "sin error de perfil en la carga buena");

    // Ruta inexistente: el emisor queda sin perfil y el motivo queda
    // disponible para el inspector.
    emisor.setRutaPerfil((carpeta.ruta() / "no_existe.json").string());
    CHECK(emisor.perfil() == nullptr,
          "una ruta inexistente deja al emisor sin perfil");
    CHECK(!emisor.errorPerfil().empty(),
          "el motivo del fallo queda en errorPerfil");

    // Quitar el perfil: mismo efecto que el boton Quitar del inspector.
    emisor.setRutaPerfil(std::string());
    CHECK(emisor.perfil() == nullptr, "ruta vacia deja al emisor sin perfil");
    CHECK(emisor.errorPerfil().empty(),
          "quitar el perfil limpia el error anterior");
}

void testSerializacionRutaPerfil() {
    TempPruebas::CarpetaPrueba carpeta("funshi_particulas_perfil_serial");
    const auto archivo = carpeta.ruta() / "emisor.bin";
    const std::string rutaPerfil = (carpeta.ruta() / "humo.json").string();

    std::string error;
    CHECK(guardarPerfilAJson(rutaPerfil, perfilCompleto(), error),
          "el perfil del flujo de serializacion se escribe sin problemas");

    EmisorParticulas original;
    original.setRutaPerfil(rutaPerfil);
    CHECK(original.perfil() != nullptr,
          "el emisor original tiene su perfil cargado");

    {
        std::ofstream salida(archivo, std::ios::binary);
        CHECK(salida.is_open(), "se abre el archivo para escribir");
        original.saveComponent(&salida);
    }

    EmisorParticulas leida;
    {
        std::ifstream entrada(archivo, std::ios::binary);
        CHECK(entrada.is_open(), "se abre el archivo para leer");
        leida.loadComponent(&entrada);
    }

    CHECK(leida.rutaPerfil() == rutaPerfil,
          "la ruta del perfil se conserva tras serializar");
    // La recarga del perfil no ocurre aqui: el perfil se vuelve a leer cuando
    // la escena ya resolvio su raiz de assets (onLoaded), no al deserializar.
}

}  // namespace

int main() {
    std::cout << "== PARTICULAS PERFIL TESTS ==" << std::endl;
    testIdaYVuelta();
    testInvariantes();
    testRutasMalas();
    testFlujoEmisor();
    testSerializacionRutaPerfil();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "PARTICULAS PERFIL TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
