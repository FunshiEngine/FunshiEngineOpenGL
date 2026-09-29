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

void Skybox::serializeComponent(std::ofstream* file) {
    auto escribirString = [file](const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        file->write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0) file->write(s.data(), len);
    };

    escribirString(caraMasX);
    escribirString(caraMenosX);
    escribirString(caraMasY);
    escribirString(caraMenosY);
    escribirString(caraMasZ);
    escribirString(caraMenosZ);
    file->write(reinterpret_cast<const char*>(&visible), sizeof(visible));
}

void Skybox::deserializeComponent(std::ifstream* file) {
    auto leerString = [file](std::string& s) {
        uint32_t len = 0;
        file->read(reinterpret_cast<char*>(&len), sizeof(len));
        if (len > 0) {
            s.resize(len);
            file->read(&s[0], len);
        } else {
            s.clear();
        }
    };

    leerString(caraMasX);
    leerString(caraMenosX);
    leerString(caraMasY);
    leerString(caraMenosY);
    leerString(caraMasZ);
    leerString(caraMenosZ);
    file->read(reinterpret_cast<char*>(&visible), sizeof(visible));
}