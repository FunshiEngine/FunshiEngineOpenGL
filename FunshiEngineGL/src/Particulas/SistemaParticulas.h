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
#ifndef SISTEMAPARTICULAS_H
#define SISTEMAPARTICULAS_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <random>
#include <vector>

#include "ParametrosEmisor.h"
#include "Particula.h"

// Simulacion de particulas.
//
// Es CPU pura a proposito: no incluye OpenGL, ni glm, ni el motor. Asi el
// sistema entero (emision, integracion, muerte y presupuesto) se puede
// probar en los targets headless, igual que LineBuilder.
//
// El presupuesto de particulas vivas esta acotado: cuando se llega al limite,
// la particula mas vieja se descarta y entra la nueva. Asi el coste por frame
// no depende de lo que haga el usuario en el inspector.
class SistemaParticulas {
public:
    // Cuantas particulas vivas se permiten por defecto.
    static constexpr std::size_t kPresupuestoPorDefecto = 4000;

    explicit SistemaParticulas(std::size_t presupuesto = kPresupuestoPorDefecto);

    // Suelta una particula con los parametros dados en el punto 'origen'.
    // Si el presupuesto esta lleno, se descarta la particula mas vieja.
    //
    // 'perfil' es opcional: es el efecto del que sale la particula y el que
    // manda sobre su tamano y su color a lo largo de la vida.
    // 'texturaOverride', si no esta vacia, se guarda en cada particula y el
    // renderer la usa en vez de la textura del perfil.
    void emitir(const ParametrosEmisor& parametros, const float origen[3],
                const std::shared_ptr<const PerfilParticula>& perfil = nullptr,
                const std::string& texturaOverride = std::string());

    // Avanza la simulacion: aplica el rozamiento y la gravedad, integra la
    // posicion y retira las particulas que se quedaron sin vida.
    void actualizar(float deltaTime);

    // Descarta todas las particulas.
    void limpiar() noexcept;

    const std::deque<Particula>& particulas() const noexcept { return particulas_; }
    std::size_t cantidad() const noexcept { return particulas_.size(); }
    std::size_t presupuesto() const noexcept { return presupuesto_; }

    // Cambia el presupuesto. Si baja por debajo de las particulas vivas, se
    // descartan las mas viejas hasta llegar al limite.
    void fijarPresupuesto(std::size_t nuevo);

    // Semilla del generador de numeros aleatorios: la simulacion es
    // reproducible, y las pruebas lo necesitan.
    void fijarSemilla(std::uint32_t semilla) { generador_.seed(semilla); }

private:
    // Numero real en [minimo, maximo). El generador se divide por su maximo
    // en lugar de usar uniform_real_distribution: esa distribucion no esta
    // especificada y da valores distintos en cada implementacion de la
    // biblioteca estandar, y la simulacion tiene que ser igual en todos.
    float uniforme(float minimo, float maximo);
    // Direccion aleatoria dentro del cono alrededor de 'direccion'.
    void direccionAleatoria(const float direccion[3], float dispersion,
                             float salida[3]);

    std::deque<Particula> particulas_;
    std::size_t presupuesto_;
    std::mt19937 generador_;
};

#endif
