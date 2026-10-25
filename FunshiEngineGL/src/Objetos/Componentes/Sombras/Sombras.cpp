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
#include "Sombras.h"

#include <algorithm>
#include <iostream>

namespace {

// Rangos con los que los ajustes se acotan. Son los mismos que usa el panel
// del inspector, para que un valor escrito a mano no pueda dejar el motor en
// un estado imposible (un mapa de sombras de 4 pixeles, un radio negativo...).
constexpr int kResolucionMinima = 256;
constexpr int kResolucionMaxima = 4096;
constexpr float kSesgoMaximo = 0.1f;
constexpr float kRadioMinimo = 0.5f;
constexpr float kRadioMaximo = 1000.f;
constexpr float kDistanciaMinima = 1.f;
constexpr float kDistanciaMaxima = 5000.f;

}  // namespace

Sombras::Sombras() = default;

void Sombras::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;

    file->write(reinterpret_cast<const char*>(&activado), sizeof(bool));
    file->write(reinterpret_cast<const char*>(&resolucion), sizeof(int));
    file->write(reinterpret_cast<const char*>(&sesgo), sizeof(float));
    file->write(reinterpret_cast<const char*>(&fuerza), sizeof(float));
    file->write(reinterpret_cast<const char*>(&radio), sizeof(float));
    file->write(reinterpret_cast<const char*>(&distancia), sizeof(float));
}

void Sombras::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) {
        std::cerr << "Sombras: archivo invalido o no abierto para lectura.\n";
        return;
    }
    file->read(reinterpret_cast<char*>(&activado), sizeof(bool));
    file->read(reinterpret_cast<char*>(&resolucion), sizeof(int));
    file->read(reinterpret_cast<char*>(&sesgo), sizeof(float));
    file->read(reinterpret_cast<char*>(&fuerza), sizeof(float));
    file->read(reinterpret_cast<char*>(&radio), sizeof(float));
    file->read(reinterpret_cast<char*>(&distancia), sizeof(float));

    // Los valores se acotan al leerlos: un archivo editado a mano (o de otra
    // version del motor) no puede dejar la sombra en un estado imposible.
    setResolucion(resolucion);
    setSesgo(sesgo);
    setFuerza(fuerza);
    setRadio(radio);
    setDistancia(distancia);
}

void Sombras::setResolucion(int valor) {
    resolucion = std::min(std::max(valor, kResolucionMinima), kResolucionMaxima);
    // Potencia de dos: la textura de profundidad se muestrea y se compara
    // mejor cuando el lado es una potencia de dos.
    int potencia = kResolucionMinima;
    while (potencia * 2 <= resolucion) potencia *= 2;
    resolucion = potencia;
}

void Sombras::setSesgo(float valor) {
    sesgo = std::min(std::max(valor, 0.f), kSesgoMaximo);
}

void Sombras::setFuerza(float valor) {
    fuerza = std::min(std::max(valor, 0.f), 1.f);
}

void Sombras::setRadio(float valor) {
    radio = std::min(std::max(valor, kRadioMinimo), kRadioMaximo);
}

void Sombras::setDistancia(float valor) {
    distancia = std::min(std::max(valor, kDistanciaMinima), kDistanciaMaxima);
}
