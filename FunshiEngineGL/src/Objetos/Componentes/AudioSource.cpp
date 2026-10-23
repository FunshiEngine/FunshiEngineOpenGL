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
#include "AudioSource.h"

#include "../../Audio/AudioEngine.h"
#include "../../Configuracion/EditorConfig.h"
#include "../../Objetos/GameObject.h"
#include "Herramientas/LimitesSerializacion.h"

#include <iostream>

void AudioSource::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    // La ruta del archivo se persiste relativa a la raiz de assets del
    // proyecto (src<nombre>): asi la escena sigue valida al mover o renombrar
    // el proyecto entero.
    const std::string rutaGuardada = EditorConfig::relativizarRuta(rutaClip);
    const std::size_t len = rutaGuardada.size();
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0)
        file->write(rutaGuardada.data(), static_cast<std::streamsize>(len));
    file->write(reinterpret_cast<const char*>(&volume), sizeof(float));
    file->write(reinterpret_cast<const char*>(&loop), sizeof(bool));
    file->write(reinterpret_cast<const char*>(&reproduccionAutomatica),
                sizeof(bool));
    file->write(reinterpret_cast<const char*>(&espacial), sizeof(bool));
}

void AudioSource::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;
    std::size_t len = 0;
    // La ruta la controla el archivo: sin cota, un valor enorme hace que la
    // reserva lance `std::length_error` y el motor aborte.
    if (!LimitesSerializacion::leerLongitud(*file, len,
                                            LimitesSerializacion::kRuta, 1,
                                            0)) {
        std::cerr << "Ruta de clip de audio invalida en el binario (len "
                  << len << "); se corta la lectura del componente\n";
        file->setstate(std::ios::failbit);
        return;
    }
    if (len > 0) {
        rutaClip.resize(len);
        file->read(rutaClip.data(), static_cast<std::streamsize>(len));
    } else {
        rutaClip.clear();
    }
    // Escena nueva: relativa a la raiz de assets; sin raiz (sin proyecto) la
    // ruta se queda como esta.
    rutaClip = EditorConfig::absolutizarRuta(rutaClip);
    file->read(reinterpret_cast<char*>(&volume), sizeof(float));
    file->read(reinterpret_cast<char*>(&loop), sizeof(bool));
    file->read(reinterpret_cast<char*>(&reproduccionAutomatica), sizeof(bool));
    // `espacial` se agrego despues: las escenas viejas no lo traen, y en ese
    // caso se conserva el comportamiento historico (sonido global/2D).
    bool e = false;
    file->read(reinterpret_cast<char*>(&e), sizeof(bool));
    if (file->fail()) {
        espacial = false;
        file->clear();
    } else {
        espacial = e;
    }
}

void AudioSource::reproducir() {
    if (!motor) {
        std::cerr << "[Audio] AudioSource::reproducir: motor no inyectado; "
                     "no se encola comando para ruta '" << rutaClip << "'\n";
        return;
    }
    detener(); // no apilar probabilidades: un clip, una reproduccion
    // El sonido espacial se reposiciona en la siguiente actualizacion de
    // frame (onUpdate) sobre la posicion real del objeto.
    const int handle = motor->reproducir(rutaClip, volume, loop, nullptr);
    handleActual = handle;
    sonando = (handle >= 0);
}

void AudioSource::detener() {
    if (motor && handleActual >= 0) motor->detener(handleActual);
    handleActual = -1;
    sonando = false;
}

void AudioSource::onUpdate(GameObject& owner, float deltaTime) {
    (void)deltaTime;
    if (!motor || !sonando || !espacial || handleActual < 0) return;
    if (Transform* t = owner.getGlobalTransform()) {
        const float* p = t->getTranslatef();
        motor->setPosicionSonido(handleActual, p[0], p[1], p[2]);
    }
}