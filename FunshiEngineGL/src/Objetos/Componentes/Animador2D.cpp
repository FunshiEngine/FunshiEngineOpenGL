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
#include "Animador2D.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "Objetos/GameObject.h"
#include "Objetos/Componentes/SpriteRenderer.h"

namespace {
// Marcador binario del payload. v1 = columnas, filas, primerFrame, ultimoFrame
// (int32), fps (float) y los flags bucle/reproduciendo (1 byte cada uno). El
// estado de ejecucion (fotograma y acumulador) no viaja al disco.
const char kAnimador2DMagicV1[4] = {'F', 'A', '2', '1'};

// Cotas de lectura: acotan el producto columnas*filas y evitan divisiones por
// cero o indices fuera de rango si el archivo viene corrupto.
constexpr int kMaxLado = 4096;
constexpr float kMaxFps = 1000.0f;

int acotar(int valor, int minimo, int maximo) {
    return std::min(std::max(valor, minimo), maximo);
}
} // namespace

Animador2D::Animador2D() = default;

void Animador2D::onUpdate(GameObject& owner, float deltaTime) {
    if (!reproduciendo_ || fps_ <= 0.0f) return;
    if (columnas_ <= 0 || filas_ <= 0) return;

    SpriteRenderer* sprite = owner.getComponent<SpriteRenderer>();
    if (!sprite) return;

    const int fin = finEfectivo();
    const int inicio = acotar(primerFrame_, 0, fin);
    if (frameActual_ < inicio || frameActual_ > fin) {
        frameActual_ = inicio;
        acumulador_ = 0.0f;
    }

    // Avance por deltaTime con tope de iteraciones: un dt grande (carga, hitch)
    // no debe recorrer miles de fotogramas de golpe.
    const float periodo = 1.0f / fps_;
    acumulador_ += deltaTime;
    int pasos = 0;
    while (acumulador_ >= periodo && pasos < 200) {
        acumulador_ -= periodo;
        ++pasos;
        if (frameActual_ >= fin) {
            if (!bucle_) {
                acumulador_ = 0.0f;
                break;
            }
            frameActual_ = inicio;
        } else {
            ++frameActual_;
        }
    }

    const float invColumnas = 1.0f / static_cast<float>(columnas_);
    const float invFilas = 1.0f / static_cast<float>(filas_);
    const int columna = frameActual_ % columnas_;
    const int fila = frameActual_ / columnas_;
    sprite->setUvRect(columna * invColumnas, fila * invFilas,
                      (columna + 1) * invColumnas, (fila + 1) * invFilas);
}

int Animador2D::finEfectivo() const {
    const int total = columnas_ * filas_;
    return ultimoFrame_ < 0 ? total - 1 : acotar(ultimoFrame_, 0, total - 1);
}

void Animador2D::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    file->write(kAnimador2DMagicV1, sizeof(kAnimador2DMagicV1));
    const std::int32_t enteros[4] = {
        static_cast<std::int32_t>(columnas_),
        static_cast<std::int32_t>(filas_),
        static_cast<std::int32_t>(primerFrame_),
        static_cast<std::int32_t>(ultimoFrame_)};
    file->write(reinterpret_cast<const char*>(enteros), sizeof(enteros));
    file->write(reinterpret_cast<const char*>(&fps_), sizeof(fps_));
    const std::uint8_t flags[2] = {static_cast<std::uint8_t>(bucle_ ? 1 : 0),
                                   static_cast<std::uint8_t>(reproduciendo_ ? 1
                                                                            : 0)};
    file->write(reinterpret_cast<const char*>(flags), sizeof(flags));
}

void Animador2D::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;

    char magic[4] = {};
    file->read(magic, sizeof(magic));
    const bool conPayload =
        file->gcount() == static_cast<std::streamsize>(sizeof(magic)) &&
        std::memcmp(magic, kAnimador2DMagicV1, sizeof(magic)) == 0;
    if (!conPayload) {
        file->seekg(-static_cast<std::streamoff>(sizeof(magic)), std::ios::cur);
        return;
    }

    std::int32_t enteros[4] = {1, 1, 0, -1};
    file->read(reinterpret_cast<char*>(enteros), sizeof(enteros));
    columnas_ = acotar(static_cast<int>(enteros[0]), 1, kMaxLado);
    filas_ = acotar(static_cast<int>(enteros[1]), 1, kMaxLado);
    primerFrame_ = acotar(static_cast<int>(enteros[2]), 0, kMaxLado * kMaxLado);
    ultimoFrame_ = static_cast<int>(enteros[3]);
    if (ultimoFrame_ >= 0)
        ultimoFrame_ = acotar(ultimoFrame_, 0, columnas_ * filas_ - 1);

    file->read(reinterpret_cast<char*>(&fps_), sizeof(fps_));
    if (!(fps_ > 0.0f) || fps_ > kMaxFps) fps_ = 8.0f;

    std::uint8_t flags[2] = {1, 1};
    file->read(reinterpret_cast<char*>(flags), sizeof(flags));
    bucle_ = flags[0] != 0;
    reproduciendo_ = flags[1] != 0;

    frameActual_ = acotar(primerFrame_, 0, columnas_ * filas_ - 1);
    acumulador_ = 0.0f;
}

