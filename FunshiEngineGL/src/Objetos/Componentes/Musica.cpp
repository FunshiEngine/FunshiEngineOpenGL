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
#include "Musica.h"

#include "../../Audio/AudioEngine.h"
#include "../../Configuracion/EditorConfig.h"
#include "Herramientas/LimitesSerializacion.h"

#include <iostream>

void Musica::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    // La lista de pistas se persiste como rutas relativas a la raiz de assets
    // del proyecto (src<nombre>): asi la escena sigue valida al mover o
    // renombrar el proyecto entero.
    const std::size_t cantidad = rutas.size();
    file->write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));
    for (const std::string& ruta : rutas) {
        const std::string rutaGuardada = EditorConfig::relativizarRuta(ruta);
        const std::size_t len = rutaGuardada.size();
        file->write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0)
            file->write(rutaGuardada.data(),
                        static_cast<std::streamsize>(len));
    }
    file->write(reinterpret_cast<const char*>(&volume), sizeof(float));
    file->write(reinterpret_cast<const char*>(&duracionFundido), sizeof(float));
    file->write(reinterpret_cast<const char*>(&reproduccionAutomatica),
                sizeof(bool));
    file->write(reinterpret_cast<const char*>(&repetirLista), sizeof(bool));
}

void Musica::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;
    rutas.clear();

    // La cantidad de pistas la controla el archivo: sin cota, un valor enorme
    // haria que la reserva lance `std::length_error` y el motor aborte.
    std::size_t cantidad = 0;
    if (!LimitesSerializacion::leerLongitud(*file, cantidad,
                                            LimitesSerializacion::kCantidad, 1,
                                            0)) {
        std::cerr << "Cantidad de pistas de musica invalida en el binario ("
                  << cantidad << "); se corta la lectura del componente\n";
        file->setstate(std::ios::failbit);
        return;
    }

    for (std::size_t i = 0; i < cantidad; ++i) {
        std::size_t len = 0;
        if (!LimitesSerializacion::leerLongitud(*file, len,
                                                LimitesSerializacion::kRuta, 1,
                                                0)) {
            std::cerr << "Ruta de pista de musica invalida en el binario (len "
                      << len << "); se corta la lectura del componente\n";
            file->setstate(std::ios::failbit);
            return;
        }
        std::string ruta;
        if (len > 0) {
            ruta.resize(len);
            file->read(ruta.data(), static_cast<std::streamsize>(len));
        }
        // Escena nueva: relativa a la raiz de assets; sin raiz (sin proyecto) la
        // ruta se queda como esta.
        rutas.push_back(EditorConfig::absolutizarRuta(ruta));
    }

    file->read(reinterpret_cast<char*>(&volume), sizeof(float));
    file->read(reinterpret_cast<char*>(&duracionFundido), sizeof(float));
    file->read(reinterpret_cast<char*>(&reproduccionAutomatica), sizeof(bool));
    file->read(reinterpret_cast<char*>(&repetirLista), sizeof(bool));
}

void Musica::anadirRuta(std::string ruta) {
    if (ruta.empty()) return;
    rutas.push_back(std::move(ruta));
}

void Musica::quitarRuta(std::size_t indice) {
    if (indice >= rutas.size()) return;
    rutas.erase(rutas.begin() + static_cast<std::ptrdiff_t>(indice));
}

void Musica::moverRuta(std::size_t desde, std::size_t hasta) {
    if (desde >= rutas.size() || hasta >= rutas.size()) return;
    if (desde == hasta) return;
    const std::string ruta = rutas[desde];
    rutas.erase(rutas.begin() + static_cast<std::ptrdiff_t>(desde));
    rutas.insert(rutas.begin() + static_cast<std::ptrdiff_t>(hasta),
                 std::move(ruta));
}

void Musica::reproducir() {
    if (!motor || rutas.empty()) return;
    // El motor recibe la lista entera (es la que se sigue al pasar a la pista
    // siguiente) y arranca por la primera.
    motor->establecerMusicas(rutas, repetirLista);
    motor->reproducirMusica(rutas.front(), volume, duracionFundido);
}

void Musica::detener() {
    if (!motor) return;
    motor->detenerMusica(duracionFundido);
}
