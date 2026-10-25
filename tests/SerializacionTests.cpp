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
// Pruebas headless de la lectura de datos serializados:
//
//  - las cotas de longitud compartida (la longitud la controla el archivo: sin
//    cota, una reserva enorme aborta el motor),
//  - el round-trip de los componentes que guardan texto leido del archivo
//    (script, fuente de audio y lista de pistas de musica),
//  - la lectura de un archivo corrupto: se corta con un error, sin abortar.
//
// Sin pila grafica: se ejercita el formato en disco directamente.

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Herramientas/LimitesSerializacion.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/AudioSource.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Musica.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Script.h"

namespace fs = std::filesystem;

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

// --- Cotas de longitud --------------------------------------------------------
// La longitud la controla el archivo: si esta corrupto, el valor puede ser
// enorme. Se acota con un maximo por tipo y se valida contra el contenido que
// queda en el stream.

static void escribirLongitud(const fs::path& ruta, uint32_t longitud,
                              std::size_t bytes, char relleno) {
    std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(&longitud), sizeof(longitud));
    const std::string datos(bytes, relleno);
    if (bytes > 0)
        out.write(datos.data(), static_cast<std::streamsize>(bytes));
    out.close();
}

static void testLimitesDeLongitud() {
    std::cout << "-- cotas de longitud: el dato se acota y se contrasta con el "
                 "contenido restante"
              << std::endl;

    TempPruebas::CarpetaPrueba carpeta("funshi_limites_serializacion");
    const fs::path ruta = carpeta.ruta() / "longitudes.bin";

    // Longitud valida: se lee y se acepta.
    escribirLongitud(ruta, 4, 4, 'a');
    {
        std::ifstream in(ruta, std::ios::binary);
        uint32_t n = 0;
        CHECK(LimitesSerializacion::leerLongitud(in, n, 64, 1),
              "una longitud dentro de la cota se acepta");
        CHECK(n == 4, "la longitud valida se lee tal cual");
    }

    // Longitud cero con minimo 1 (una cadena no puede estar vacia).
    escribirLongitud(ruta, 0, 0, 'a');
    {
        std::ifstream in(ruta, std::ios::binary);
        uint32_t n = 123;
        CHECK(!LimitesSerializacion::leerLongitud(in, n, 64, 1),
              "una longitud cero se rechaza con minimo 1");
        CHECK(n == 0, "la longitud rechazada queda en cero");
    }

    // Longitud cero con minimo 0 (una cantidad puede estar vacia).
    escribirLongitud(ruta, 0, 0, 'a');
    {
        std::ifstream in(ruta, std::ios::binary);
        uint32_t n = 123;
        CHECK(LimitesSerializacion::leerLongitud(in, n, 64, 1, 0),
              "una longitud cero se acepta con minimo 0");
        CHECK(n == 0, "la longitud cero se lee como cero");
    }

    // Longitud enorme: se rechaza sin intentar reservar.
    escribirLongitud(ruta, 0xFFFFFFFFu, 0, 'a');
    {
        std::ifstream in(ruta, std::ios::binary);
        uint32_t n = 0;
        CHECK(!LimitesSerializacion::leerLongitud(in, n, 64, 1),
              "una longitud enorme se rechaza");
    }

    // Longitud mayor que el contenido que queda en el archivo.
    escribirLongitud(ruta, 100, 4, 'a');
    {
        std::ifstream in(ruta, std::ios::binary);
        uint32_t n = 0;
        CHECK(!LimitesSerializacion::leerLongitud(in, n, 64, 1),
              "una longitud mayor que el contenido se rechaza");
    }

    // Longitud sin contenido alguno detrás.
    escribirLongitud(ruta, 7, 0, 'a');
    {
        std::ifstream in(ruta, std::ios::binary);
        uint32_t n = 0;
        CHECK(!LimitesSerializacion::leerLongitud(in, n, 64, 1),
              "una longitud sin contenido se rechaza");
    }
}

// --- Componente Script --------------------------------------------------------

static void testScriptSerializacion() {
    std::cout << "-- componente Script: round-trip y lectura de un archivo "
                 "corrupto"
              << std::endl;

    TempPruebas::CarpetaPrueba carpeta("funshi_script_serializacion");
    const fs::path ruta = carpeta.ruta() / "script.bin";

    // Round-trip: lo que se guarda se vuelve a leer igual.
    {
        Script original;
        original.setDllPath("src/Jugador/Scripts/Disparo.cpp");
        original.setNombreComponente("Disparo");
        {
            std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
            original.saveComponent(&out);
        }
        Script leido;
        {
            std::ifstream in(ruta, std::ios::binary);
            leido.loadComponent(&in);
        }
        CHECK(leido.getPath().find("Disparo.cpp") != std::string::npos,
              "el round-trip conserva la ruta del fuente");
        CHECK(leido.getNombreComponente() == "Disparo",
              "el round-trip conserva el nombre del componente");
    }

    // Archivo corrupto: la longitud de la ruta es absurda. La lectura se
    // corta con un error y el motor no aborta.
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        uint32_t magic = 0x31535346;  // 'F','S','S','1'
        uint32_t version = 3;
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        const size_t pathLength = static_cast<size_t>(0xFFFFFFFFFFFFFFF0ull);
        out.write(reinterpret_cast<const char*>(&pathLength), sizeof(pathLength));
        out.close();

        Script leido;
        std::ifstream in(ruta, std::ios::binary);
        leido.loadComponent(&in);
        CHECK(leido.getPath().empty(),
              "una longitud de ruta invalida deja el componente sin ruta");
        CHECK(!in.good(), "el stream queda marcado como fallido");
    }

    // Archivo corrupto: la longitud del nombre de clase es absurda.
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        uint32_t magic = 0x31535346;
        uint32_t version = 3;
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        const size_t pathLength = 4;
        out.write(reinterpret_cast<const char*>(&pathLength), sizeof(pathLength));
        out.write("a.cpp", 4);
        const size_t nameLength = static_cast<size_t>(0xFFFFFFFFFFFFFFF0ull);
        out.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        out.close();

        Script leido;
        std::ifstream in(ruta, std::ios::binary);
        leido.loadComponent(&in);
        CHECK(leido.getNombreComponente().empty(),
              "una longitud de nombre invalida deja el componente sin nombre");
    }
}

// --- Componente AudioSource ---------------------------------------------------

static void testAudioSourceSerializacion() {
    std::cout << "-- componente AudioSource: round-trip y lectura de un "
                 "archivo corrupto"
              << std::endl;

    TempPruebas::CarpetaPrueba carpeta("funshi_audio_source_serializacion");
    const fs::path ruta = carpeta.ruta() / "audio_source.bin";

    // Round-trip.
    {
        AudioSource original;
        original.setRutaClip("disparo.wav");
        original.setVolume(0.5f);
        original.setLoop(true);
        {
            std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
            original.saveComponent(&out);
        }
        AudioSource leido;
        {
            std::ifstream in(ruta, std::ios::binary);
            leido.loadComponent(&in);
        }
        CHECK(leido.getRutaClip() == "disparo.wav",
              "el round-trip conserva la ruta del clip");
        CHECK(std::fabs(leido.getVolume() - 0.5f) < 1e-5f,
              "el round-trip conserva el volumen");
        CHECK(leido.isLoop(), "el round-trip conserva el loop");
    }

    // Archivo corrupto: la longitud de la ruta del clip es absurda.
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        const size_t len = static_cast<size_t>(0xFFFFFFFFFFFFFFF0ull);
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.close();

        AudioSource leido;
        std::ifstream in(ruta, std::ios::binary);
        leido.loadComponent(&in);
        CHECK(leido.getRutaClip().empty(),
              "una longitud de clip invalida deja el componente sin clip");
        CHECK(!in.good(), "el stream queda marcado como fallido");
    }
}

// --- Componente Musica ---------------------------------------------------------

static void testMusicaSerializacion() {
    std::cout << "-- componente Musica: la lista de pistas y sus ajustes"
              << std::endl;

    TempPruebas::CarpetaPrueba carpeta("funshi_musica_serializacion");
    const fs::path ruta = carpeta.ruta() / "musica.bin";

    // Round-trip: la lista conserva el orden y los ajustes.
    {
        Musica original;
        original.setRutas({"tema.ogg", "coda.ogg"});
        original.setVolume(0.5f);
        original.setDuracionFundido(3.5f);
        original.setRepetirLista(false);
        {
            std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
            original.saveComponent(&out);
        }
        Musica leido;
        {
            std::ifstream in(ruta, std::ios::binary);
            leido.loadComponent(&in);
        }
        CHECK(leido.getRutas().size() == 2,
              "el round-trip conserva las dos pistas");
        CHECK(leido.getRutas().size() > 0 &&
                  leido.getRutas()[0] == "tema.ogg",
              "la lista conserva el orden de reproduccion");
        CHECK(leido.getRutas().size() > 1 &&
                  leido.getRutas()[1] == "coda.ogg",
              "la segunda pista va despues");
        CHECK(std::fabs(leido.getVolume() - 0.5f) < 1e-5f,
              "el round-trip conserva el volumen");
        CHECK(std::fabs(leido.getDuracionFundido() - 3.5f) < 1e-5f,
              "el round-trip conserva el fundido");
        CHECK(!leido.isRepetirLista(),
              "el round-trip conserva la repeticion de la lista");
    }

    // Una lista vacia se guarda y se lee sin pistas.
    {
        Musica original;
        {
            std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
            original.saveComponent(&out);
        }
        Musica leido;
        {
            std::ifstream in(ruta, std::ios::binary);
            leido.loadComponent(&in);
        }
        CHECK(leido.getRutas().empty(), "una lista vacia se lee vacia");
    }

    // Archivo corrupto: la cantidad de pistas es absurda.
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        const size_t cantidad = static_cast<size_t>(0xFFFFFFFFFFFFFFF0ull);
        out.write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));
        out.close();

        Musica leido;
        std::ifstream in(ruta, std::ios::binary);
        leido.loadComponent(&in);
        CHECK(leido.getRutas().empty(),
              "una cantidad invalida deja la lista de pistas vacia");
        CHECK(!in.good(), "el stream queda marcado como fallido");
    }
}

int main() {
    testLimitesDeLongitud();
    testScriptSerializacion();
    testAudioSourceSerializacion();
    testMusicaSerializacion();

    std::cout << "SerializacionTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}
