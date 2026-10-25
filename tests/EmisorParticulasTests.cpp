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
// Pruebas headless del componente emisor de particulas.
//
// Cubren: la tasa de emision (cuantas particulas suelta por segundo y el
// acumulador entre frames), el tope de lo pendiente en un frame largo, el
// perfil de efecto (que manda sobre la vida y la velocidad del emisor) y el
// round-trip binario del componente.
//
// El componente usa la configuracion del editor para resolver la ruta del
// perfil, asi que la suite necesita el motor (a diferencia de la simulacion,
// que es CPU pura).

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "Objetos/Componentes/EmisorParticulas.h"
#include "Particulas/SistemaParticulas.h"

#include "TempPruebas.h"

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

// --- Tasa de emision ---------------------------------------------------------

void probarTasa() {
    SistemaParticulas sistema(1000);
    EmisorParticulas emisor;
    emisor.parametros().cantidadPorSegundo = 10;
    emisor.parametros().vidaMinima = 5.0f;
    emisor.parametros().vidaMaxima = 5.0f;

    const float origen[3] = {0.0f, 0.0f, 0.0f};
    CHECK(emisor.emitir(sistema, origen, 0.1f) == 1,
          "Tasa: con 10 por segundo y un frame de 0.1 s sale una particula");
    CHECK(sistema.cantidad() == 1, "Tasa: la particula llego al sistema");
    CHECK(emisor.emitir(sistema, origen, 0.1f) == 1,
          "Tasa: el frame siguiente suelta otra");
    CHECK(sistema.cantidad() == 2, "Tasa: hay dos particulas");
    // El tiempo que no alcanza para una particula se guarda.
    CHECK(emisor.emitir(sistema, origen, 0.05f) == 0,
          "Tasa: con medio frame no sale ninguna particula");
    CHECK(emisor.emitir(sistema, origen, 0.05f) == 1,
          "Tasa: el tiempo sobrante se acumula y suelta la particula");

    // Un frame largo no puede soltar una avalancha.
    emisor.parametros().cantidadPorSegundo = 1000;
    SistemaParticulas sistema2(1000);
    CHECK(emisor.emitir(sistema2, origen, 10.0f) <= 10,
          "Tasa: un frame largo no suelta una avalancha de particulas");

    // Un emisor apagado no suelta nada.
    SistemaParticulas sistema3(1000);
    emisor.setActiva(false);
    CHECK(emisor.emitir(sistema3, origen, 1.0f) == 0,
          "Tasa: un emisor inactivo no suelta particulas");
    CHECK(sistema3.cantidad() == 0, "Tasa: el sistema sigue vacio");
}

// --- Perfil de efecto ---------------------------------------------------------

void probarPerfil() {
    TempPruebas::CarpetaPrueba carpeta("funshi_emisor_particulas_tests");
    const std::string base = carpeta.ruta().string();

    const std::string ruta = base + "/perfil.json";
    {
        std::ofstream archivo(ruta);
        archivo << R"({
  "nombre": "Humo",
  "vida": [2.0, 3.0],
  "velocidad": [4.0, 5.0],
  "tamanos": [0.5, 1.0]
})";
    }

    EmisorParticulas emisor;
    emisor.parametros().vidaMinima = 0.1f;
    emisor.parametros().vidaMaxima = 0.2f;
    emisor.parametros().velocidadMinima = 0.1f;
    emisor.parametros().velocidadMaxima = 0.2f;
    emisor.setRutaPerfil(ruta);

    CHECK(emisor.errorPerfil().empty(),
          "Perfil: un perfil valido se lee sin error");
    CHECK(emisor.perfil() != nullptr, "Perfil: el emisor tiene perfil");
    if (!emisor.perfil()) return;
    CHECK(emisor.perfil()->nombre == "Humo", "Perfil: se lee el nombre");

    // Con perfil, la vida y la velocidad son las del perfil, no las del emisor.
    SistemaParticulas sistema(100);
    emisor.parametros().cantidadPorSegundo = 10;
    const float origen[3] = {0.0f, 0.0f, 0.0f};
    emisor.emitir(sistema, origen, 0.1f);
    CHECK(sistema.cantidad() == 1, "Perfil: se emitio una particula");
    if (sistema.cantidad() != 1) return;
    const Particula& particula = sistema.particulas().front();
    CHECK(particula.vidaMaxima >= 2.0f && particula.vidaMaxima <= 3.0f,
          "Perfil: la vida sale del perfil y no del emisor");

    // Un perfil que no existe deja al emisor sin perfil, con el motivo.
    emisor.setRutaPerfil(base + "/no-existe.json");
    CHECK(emisor.perfil() == nullptr, "Perfil: un perfil inexistente no se carga");
    CHECK(!emisor.errorPerfil().empty(),
          "Perfil: el motivo del perfil inexistente se explica");

    // Sin perfil, la vida vuelve a ser la del emisor.
    emisor.setRutaPerfil("");
    CHECK(emisor.perfil() == nullptr, "Perfil: sin ruta no hay perfil");
    CHECK(emisor.errorPerfil().empty(), "Perfil: sin ruta no hay error");
}

// --- Round-trip binario -------------------------------------------------------

void probarSerializacion() {
    TempPruebas::CarpetaPrueba carpeta("funshi_emisor_particulas_tests");
    const std::string base = carpeta.ruta().string();

    EmisorParticulas emisor;
    emisor.parametros().cantidadPorSegundo = 37;
    emisor.parametros().vidaMinima = 1.5f;
    emisor.parametros().vidaMaxima = 2.5f;
    emisor.parametros().velocidadMinima = 3.0f;
    emisor.parametros().velocidadMaxima = 4.0f;
    emisor.parametros().tamanoMinimo = 0.25f;
    emisor.parametros().tamanoMaximo = 0.75f;
    emisor.parametros().color[0] = 0.1f;
    emisor.parametros().color[1] = 0.2f;
    emisor.parametros().color[2] = 0.3f;
    emisor.parametros().color[3] = 0.4f;
    emisor.parametros().direccion[0] = 1.0f;
    emisor.parametros().direccion[1] = 0.5f;
    emisor.parametros().direccion[2] = 0.25f;
    emisor.parametros().dispersion = 0.4f;
    emisor.parametros().gravedad[0] = -1.0f;
    emisor.parametros().gravedad[1] = -2.0f;
    emisor.parametros().gravedad[2] = -3.0f;
    emisor.parametros().rozamiento = 0.75f;
    emisor.setActiva(false);
    emisor.setRutaPerfil("perfiles/humo.json");

    const std::string archivo = base + "/emisor.bin";
    {
        std::ofstream salida(archivo, std::ios::binary);
        emisor.saveComponent(&salida);
    }

    EmisorParticulas leido;
    {
        std::ifstream entrada(archivo, std::ios::binary);
        leido.loadComponent(&entrada);
    }

    const ParametrosEmisor& parametros = leido.parametros();
    CHECK(parametros.cantidadPorSegundo == 37, "Serializacion: la tasa se guarda");
    CHECK(parametros.vidaMinima == 1.5f && parametros.vidaMaxima == 2.5f,
          "Serializacion: la vida se guarda");
    CHECK(parametros.velocidadMinima == 3.0f && parametros.velocidadMaxima == 4.0f,
          "Serializacion: la velocidad se guarda");
    CHECK(parametros.tamanoMinimo == 0.25f && parametros.tamanoMaximo == 0.75f,
          "Serializacion: el tamano se guarda");
    CHECK(parametros.color[0] == 0.1f && parametros.color[3] == 0.4f,
          "Serializacion: el color se guarda");
    CHECK(parametros.direccion[1] == 0.5f && parametros.dispersion == 0.4f,
          "Serializacion: la direccion y la dispersion se guardan");
    CHECK(parametros.gravedad[2] == -3.0f && parametros.rozamiento == 0.75f,
          "Serializacion: la gravedad y el rozamiento se guardan");
    CHECK(!leido.activa(), "Serializacion: el estado de activacion se guarda");
    CHECK(leido.rutaPerfil() == "perfiles/humo.json",
          "Serializacion: la ruta del perfil se guarda");

    // Un componente guardado antes de que existiera se lee sin datos: el
    // emisor se queda con los valores por defecto y no se desalinea la
    // lectura.
    const std::string viejo = base + "/viejo.bin";
    {
        std::ofstream salida(viejo, std::ios::binary);
        const char basura[3] = {'a', 'b', 'c'};
        salida.write(basura, sizeof(basura));
    }
    EmisorParticulas sinDatos;
    {
        std::ifstream entrada(viejo, std::ios::binary);
        sinDatos.loadComponent(&entrada);
    }
    CHECK(sinDatos.parametros().cantidadPorSegundo == 20,
          "Serializacion: un componente viejo se lee con los valores por defecto");
    CHECK(sinDatos.rutaPerfil().empty(),
          "Serializacion: un componente viejo no tiene perfil");
}

}  // namespace

int main() {
    probarTasa();
    probarPerfil();
    probarSerializacion();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
