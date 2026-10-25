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
#ifndef MUSICA_H
#define MUSICA_H

#include "Component.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class AudioEngine;

// Reproductor de musica como Component: se adjunta a un GameObject y lleva la
// LISTA ORDENADA de pistas (por RUTA) que sonara el motor. El usuario arrastra
// los archivos al componente (igual que con los scripts y los efectos de
// sonido) y el motor reproduce esas rutas: el archivo puede estar en cualquier
// carpeta dentro de src<proyecto>/, no hay carpeta ni catalogo de musica.
//
// Cuando la pista actual se acaba, el reproductor pasa a la siguiente de la
// lista con un fundido cruzado (si la lista se repite, al final vuelve a la
// primera). El AudioEngine lo inyecta la escena (solo en modo play).
class Musica : public Component {
private:
    std::vector<std::string> rutas; // lista ordenada de pistas (rutas)
    float volume = 1.0f;
    float duracionFundido = 2.0f;
    bool reproduccionAutomatica = true; // suena al entrar en play
    bool repetirLista = true;          // al final vuelve a la primera
    AudioEngine* motor = nullptr;      // inyectado por GameScene

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Musica() = default;

    void setMotor(AudioEngine* m) noexcept { motor = m; }
    AudioEngine* getMotor() const noexcept { return motor; }

    // La lista de pistas, en orden de reproduccion.
    const std::vector<std::string>& getRutas() const noexcept { return rutas; }
    void setRutas(std::vector<std::string> nuevas) noexcept {
        rutas = std::move(nuevas);
    }
    // Anade una pista al final de la lista.
    void anadirRuta(std::string ruta);
    // Quita la pista de la posicion dada (si la posicion existe).
    void quitarRuta(std::size_t indice);
    // Mueve una pista de una posicion a otra (si ambas existen).
    void moverRuta(std::size_t desde, std::size_t hasta);

    float getVolume() const noexcept { return volume; }
    void setVolume(float v) noexcept { volume = v; }

    // Duracion del fundido entre pistas, en segundos.
    float getDuracionFundido() const noexcept { return duracionFundido; }
    void setDuracionFundido(float d) noexcept { duracionFundido = d; }

    bool isReproduccionAutomatica() const noexcept {
        return reproduccionAutomatica;
    }
    void setReproduccionAutomatica(bool b) noexcept { reproduccionAutomatica = b; }

    bool isRepetirLista() const noexcept { return repetirLista; }
    void setRepetirLista(bool b) noexcept { repetirLista = b; }

    // Encola la lista en el motor y arranca la primera pista.
    void reproducir();
    // Detiene la musica (con el fundido de salida del componente).
    void detener();
};

#endif // MUSICA_H
