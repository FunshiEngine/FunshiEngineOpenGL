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
#include "Grid.h"

#include <iostream>

void Grid::serializeComponent(std::ofstream* file) {
    file->write(reinterpret_cast<const char*>(&visible), sizeof(bool));
    file->write(reinterpret_cast<const char*>(&color), sizeof(color));
    file->write(reinterpret_cast<const char*>(&tam), sizeof(float));
    file->write(reinterpret_cast<const char*>(&separacion), sizeof(float));
}

void Grid::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) {
        std::cerr << "Grid: archivo invalido o no abierto para lectura.\n";
        return;
    }
    file->read(reinterpret_cast<char*>(&visible), sizeof(bool));
    file->read(reinterpret_cast<char*>(&color), sizeof(color));
    file->read(reinterpret_cast<char*>(&tam), sizeof(float));
    file->read(reinterpret_cast<char*>(&separacion), sizeof(float));
}

void Grid::setColor(float r, float g, float b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
}