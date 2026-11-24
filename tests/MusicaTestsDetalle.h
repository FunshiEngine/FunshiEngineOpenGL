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
// Preambulo compartido por las unidades de MusicaTests.cpp (generado al dividirlo).
#ifndef MUSICATESTSDETALLE_H
#define MUSICATESTSDETALLE_H

// Pruebas headless del reproductor de musica (GestorMusica): la lista ordenada
// de pistas (por ruta), el fundido cruzado, el paso automatico a la pista
// siguiente, la deteccion y el bus.
//
// El backend de musica es FALSO (implementa IMusicaBackend sin tocar audio):
// asi se puede comprobar el comportamiento del reproductor paso a paso.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo y salida
// final "OK/FALLOS: N comprobaciones".

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "../FunshiEngineGL/src/Audio/GestorMusica.h"
#include "../FunshiEngineGL/src/Audio/IMusicaBackend.h"
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

namespace MusicaTestsDetalle {

// Backend de musica falso: guarda lo que el reproductor le pide, para poder
// comprobarlo despues.
class BackendFalso : public IMusicaBackend {
public:
    struct Pista {
        std::string ruta;
        float volumen = 0.0f;
        bool sonando = false;
        bool terminada = false;
        float posicion = 0.0f;
        float duracion = 120.0f;
    };

    std::map<int, Pista> pistas;
    int siguienteId = 1;
    float volumenDelBus = 1.0f;
    bool silenciado = false;
    // Pistas abiertas a la vez: sirve para detectar que una pista se queda
    // viva (y suena a la vez que la otra) cuando deberia haber cerrado.
    std::size_t abiertas() const { return pistas.size(); }

    const Pista* buscar(const std::string& ruta) const {
        for (const auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) return &pista;
        }
        return nullptr;
    }

    // Marca como terminada la pista que esta sonando con esa ruta.
    void terminar(const std::string& ruta) {
        for (auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) pista.terminada = true;
        }
    }

    void mover(const std::string& ruta, float posicion) {
        for (auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) pista.posicion = posicion;
        }
    }

    void durar(const std::string& ruta, float segundos) {
        for (auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) pista.duracion = segundos;
        }
    }

    int abrirPista(const std::string& ruta) override {
        const int id = siguienteId++;
        Pista pista;
        pista.ruta = ruta;
        pistas[id] = pista;
        return id;
    }

    void cerrarPista(int id) override { pistas.erase(id); }

    bool reproducirPista(int id, float volumen) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return false;
        it->second.sonando = true;
        it->second.terminada = false;
        it->second.volumen = volumen;
        return true;
    }

    void detenerPista(int id) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return;
        it->second.sonando = false;
        it->second.volumen = 0.0f;
    }

    void volumenPista(int id, float volumen) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return;
        it->second.volumen = volumen;
    }

    void volumenBus(float volumen) override { volumenDelBus = volumen; }

    void silenciarBus(bool estado) override { silenciado = estado; }

    EstadoPista estadoPista(int id) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return EstadoPista::Detenida;
        if (it->second.terminada) return EstadoPista::Terminada;
        return it->second.sonando ? EstadoPista::Reproduciendo
                                  : EstadoPista::Detenida;
    }

    float posicionPista(int id) override {
        auto it = pistas.find(id);
        return it == pistas.end() ? 0.0f : it->second.posicion;
    }

    float duracionPista(int id) override {
        auto it = pistas.find(id);
        return it == pistas.end() ? 0.0f : it->second.duracion;
    }
};

}  // namespace MusicaTestsDetalle
using namespace MusicaTestsDetalle;

// Estado de la unidad original: una sola instancia (se define en MusicaTests.cpp).

namespace MusicaTestsDetalle {

extern int total;
extern int fallos;

}  // namespace MusicaTestsDetalle
using namespace MusicaTestsDetalle;

// Funciones repartidas entre las unidades.

void listaVacia();
void listaDePistas();
void reproduccionSimple();
void pistaFueraDeLaLista();
void volumenLimitado();
void fundidoCruzado();
void fundidoInmediato();
void avanceAutomatico();
void listaSeRepite();
void listaSinRepetir();
void fundidoAnticipado();
void detener();
void detenerConFundido();
void bus();
void cancelarEntrada();
void reproducirSiguiente();

#endif
