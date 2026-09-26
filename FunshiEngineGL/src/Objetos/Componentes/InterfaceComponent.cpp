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
#include "InterfaceComponent.h"

void InterfaceComponent::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    // Mismo formato que AudioSource: nombre por longitud (prefijo size_t) + bytes.
    const std::size_t len = interfaz.size();
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0)
        file->write(interfaz.data(), static_cast<std::streamsize>(len));
}

void InterfaceComponent::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;
    std::size_t len = 0;
    file->read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > 0) {
        interfaz.resize(len);
        file->read(interfaz.data(), static_cast<std::streamsize>(len));
    } else {
        interfaz.clear();
    }
}

void InterfaceComponent::saveComponent(std::ofstream* file) {
    serializeComponent(file);
}

void InterfaceComponent::loadComponent(std::ifstream* file) {
    deserializeComponent(file);
}