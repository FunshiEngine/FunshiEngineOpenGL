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
#include "Color.h"

#include "../../Objetos/GameObject.h"

#include <iostream>

Color::Color() {
    range[0] = f1;
    range[1] = f2;
    range[2] = f3;
    range[3] = 1.0f;
}

void Color::serializeComponent(std::ofstream* file) {
    file->write(reinterpret_cast<const char*>(&f1), sizeof(float));
    file->write(reinterpret_cast<const char*>(&f2), sizeof(float));
    file->write(reinterpret_cast<const char*>(&f3), sizeof(float));
}

void Color::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) {
        std::cerr << "Error: archivo invalido o no abierto para lectura.\n";
        return;
    }
    file->read(reinterpret_cast<char*>(&f1), sizeof(float));
    file->read(reinterpret_cast<char*>(&f2), sizeof(float));
    file->read(reinterpret_cast<char*>(&f3), sizeof(float));

    range[0] = f1;
    range[1] = f2;
    range[2] = f3;
    range[3] = 1.0f;
}

void Color::saveComponent(std::ofstream* file) { serializeComponent(file); }

void Color::loadComponent(std::ifstream* file) { deserializeComponent(file); }

void Color::setColor(const color cor) {
    f1 = cor[0];
    f2 = cor[1];
    f3 = cor[2];
    range[0] = f1;
    range[1] = f2;
    range[2] = f3;
    range[3] = 1.0f;
}

void Color::setColor(float r, float g, float b) {
    f1 = r;
    f2 = g;
    f3 = b;
    range[0] = f1;
    range[1] = f2;
    range[2] = f3;
    range[3] = 1.0f;
}

void Color::onLoaded(GameObject& owner) {
    // El color recien deserializado se refleja en el auxColor del objeto: con
    // el hook, el objeto no tiene que saber "si es Color" para ofrecerle el
    // color al inspector.
    const float* fuente = getColor();
    for (int i = 0; i < 4; ++i) owner.auxColor[i] = fuente[i];
}