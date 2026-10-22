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
#include "SistemaParticulas.h"

#include <algorithm>
#include <cmath>

namespace {

// Producto vectorial.
void productoVectorial(const float a[3], const float b[3], float salida[3]) {
    salida[0] = a[1] * b[2] - a[2] * b[1];
    salida[1] = a[2] * b[0] - a[0] * b[2];
    salida[2] = a[0] * b[1] - a[1] * b[0];
}

float norma(const float v[3]) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void normalizar(float v[3]) {
    const float largo = norma(v);
    if (largo <= 1e-6f) return;
    v[0] /= largo;
    v[1] /= largo;
    v[2] /= largo;
}

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

SistemaParticulas::SistemaParticulas(std::size_t presupuesto)
    : presupuesto_(presupuesto), generador_(12345u) {}

void SistemaParticulas::fijarPresupuesto(std::size_t nuevo) {
    presupuesto_ = nuevo;
    while (particulas_.size() > presupuesto_) particulas_.pop_front();
}

float SistemaParticulas::uniforme(float minimo, float maximo) {
    if (maximo <= minimo) return minimo;
    // El rango es [0, 1): se divide por el maximo del generador mas uno.
    const double fraccion =
        static_cast<double>(generador_()) /
        (static_cast<double>(std::mt19937::max()) + 1.0);
    return minimo + static_cast<float>(fraccion) * (maximo - minimo);
}

void SistemaParticulas::direccionAleatoria(const float direccion[3],
                                            float dispersion, float salida[3]) {
    float eje[3] = {direccion[0], direccion[1], direccion[2]};
    if (norma(eje) <= 1e-6f) {
        // Sin direccion previa: punto aleatorio de la esfera.
        const float z = uniforme(-1.0f, 1.0f);
        const float angulo = uniforme(0.0f, 2.0f * kPi);
        const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        salida[0] = r * std::cos(angulo);
        salida[1] = r * std::sin(angulo);
        salida[2] = z;
        return;
    }
    normalizar(eje);

    // Base ortonormal del cono: 'u' es perpendicular al eje y 'v' completes
    // el triedro.
    float auxiliar[3] = {0.0f, 0.0f, 1.0f};
    if (std::fabs(eje[2]) > 0.9f) {
        auxiliar[0] = 1.0f;
        auxiliar[1] = 0.0f;
        auxiliar[2] = 0.0f;
    }
    float u[3];
    productoVectorial(eje, auxiliar, u);
    normalizar(u);
    float v[3];
    productoVectorial(eje, u, v);

    // El angulo sale del raiz del aleatorio para que el cono se llene de
    // forma uniforme (la raiz reparte parejo el area, no el angulo).
    const float dispersionAcotada = std::min(1.0f, std::max(0.0f, dispersion));
    const float anguloMaximo = dispersionAcotada * (kPi * 0.5f);
    const float angulo = anguloMaximo * std::sqrt(uniforme(0.0f, 1.0f));
    const float azimut = uniforme(0.0f, 2.0f * kPi);
    const float s = std::sin(angulo);
    const float c = std::cos(angulo);
    for (int i = 0; i < 3; ++i) {
        salida[i] = eje[i] * c +
                    (u[i] * std::cos(azimut) + v[i] * std::sin(azimut)) * s;
    }
}

void SistemaParticulas::emitir(const ParametrosEmisor& parametros,
                               const float origen[3],
                               const std::shared_ptr<const PerfilParticula>& perfil,
                               const std::string& texturaOverride) {
    if (presupuesto_ == 0) return;
    if (particulas_.size() >= presupuesto_) particulas_.pop_front();

    Particula nueva;
    for (int i = 0; i < 3; ++i) {
        nueva.posicion[i] = origen ? origen[i] : 0.0f;
        nueva.gravedad[i] = parametros.gravedad[i];
    }
    for (int i = 0; i < 4; ++i) nueva.color[i] = parametros.color[i];

    nueva.vidaMaxima = uniforme(parametros.vidaMinima, parametros.vidaMaxima);
    if (nueva.vidaMaxima <= 0.0f) nueva.vidaMaxima = 0.0f;
    nueva.vida = nueva.vidaMaxima;
    nueva.tamanoBase = uniforme(parametros.tamanoMinimo, parametros.tamanoMaximo);
    nueva.tamano = nueva.tamanoBase;
    nueva.rozamiento = parametros.rozamiento;
    nueva.perfil = perfil;
    nueva.texturaOverride = texturaOverride;
    nueva.rotacion = parametros.rotacionInicial;
    nueva.velocidadRotacion = parametros.velocidadRotacion;

    const float magnitud =
        uniforme(parametros.velocidadMinima, parametros.velocidadMaxima);
    float direccion[3];
    direccionAleatoria(parametros.direccion, parametros.dispersion, direccion);
    for (int i = 0; i < 3; ++i) nueva.velocidad[i] = direccion[i] * magnitud;

    particulas_.push_back(nueva);
}

void SistemaParticulas::actualizar(float deltaTime) {
    if (deltaTime <= 0.0f) return;
    for (Particula& particula : particulas_) {
        particula.vida -= deltaTime;
        if (particula.vida <= 0.0f) continue;
        // Rozamiento: la velocidad se divide por (1 + rozamiento * dt), que
        // aproxima una perdida proporcional sin depender del paso.
        const float factor = 1.0f / (1.0f + particula.rozamiento * deltaTime);
        for (int i = 0; i < 3; ++i) {
            particula.velocidad[i] *= factor;
            particula.velocidad[i] += particula.gravedad[i] * deltaTime;
            particula.posicion[i] += particula.velocidad[i] * deltaTime;
        }
        particula.rotacion += particula.velocidadRotacion * deltaTime;
        // Las curvas del perfil mandan sobre el tamano y el color mientras la
        // particula vive: asi un efecto crece y se apaga solo.
        if (particula.perfil) {
            const float fraccion = particula.fraccionVida();
            particula.tamano =
                particula.tamanoBase * particula.perfil->multiplicadorTamano(fraccion);
            particula.perfil->colorEn(fraccion, particula.color);
        }
    }
    particulas_.erase(
        std::remove_if(particulas_.begin(), particulas_.end(),
                       [](const Particula& particula) { return !particula.viva(); }),
        particulas_.end());
}

void SistemaParticulas::limpiar() noexcept {
    particulas_.clear();
}
