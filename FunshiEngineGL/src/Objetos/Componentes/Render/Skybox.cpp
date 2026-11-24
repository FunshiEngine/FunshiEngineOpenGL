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
#include "Skybox.h"

#include <iostream>
#include <cstdint>

#include "Configuracion/EditorConfig.h"

void Skybox::serializeComponent(std::ofstream* file) {
    // Las seis caras se guardan relativas a la raiz de assets del proyecto
    // (igual que la malla de Model, las texturas de Material y el binario de
    // Script), para que la escena siga valida al renombrar o mover el proyecto
    // entero. Las que caen fuera de la raiz se guardan tal cual.
    auto escribirRuta = [file](const std::string& ruta) {
        const std::string aGuardar = EditorConfig::relativizarRuta(ruta);
        uint32_t len = static_cast<uint32_t>(aGuardar.size());
        file->write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0) file->write(aGuardar.data(), len);
    };

    escribirRuta(caraMasX);
    escribirRuta(caraMenosX);
    escribirRuta(caraMasY);
    escribirRuta(caraMenosY);
    escribirRuta(caraMasZ);
    escribirRuta(caraMenosZ);
    file->write(reinterpret_cast<const char*>(&visible), sizeof(visible));
}

void Skybox::deserializeComponent(std::ifstream* file) {
    auto leerRuta = [file](std::string& ruta) {
        uint32_t len = 0;
        file->read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string guardada;
        if (len > 0) {
            guardada.resize(len);
            file->read(&guardada[0], len);
        } else {
            guardada.clear();
        }
        // Las escenas nuevas guardan la ruta relativa a la raiz de assets; las
        // legacy guardaban la absoluta, que desde aqui se deja intacta.
        ruta = EditorConfig::absolutizarRuta(guardada);
    };

    leerRuta(caraMasX);
    leerRuta(caraMenosX);
    leerRuta(caraMasY);
    leerRuta(caraMenosY);
    leerRuta(caraMasZ);
    leerRuta(caraMenosZ);
    file->read(reinterpret_cast<char*>(&visible), sizeof(visible));
}