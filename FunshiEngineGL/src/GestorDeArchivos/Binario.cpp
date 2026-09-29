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
#include "Binario.h"

#include <cstdio>
#include <iostream>
#include <utility>

Binario::Binario(std::string path)
    : path(std::move(path)), ofBin(nullptr), ifBin(nullptr) {}

Binario::~Binario() {
    if (ofBin != nullptr) {
        if (ofBin->is_open()) ofBin->close();
        delete ofBin;
    }
    if (ifBin != nullptr) {
        if (ifBin->is_open()) ifBin->close();
        delete ifBin;
    }
}

std::string Binario::getPath() { return path; }

bool Binario::ofOpenBinary() {
    // Sin std::remove() previo: std::ofstream trunca al abrir, asi que hace lo
    // mismo; y si el abrir falla (directorio inexistente, sin permisos), el
    // archivo que ya estaba en disco queda INTACTO. Antes se borraba primero,
    // con lo que un fallo de apertura ya habia destruido el archivo bueno.
    ofBin = new std::ofstream(path, std::ios::binary);
    if (!ofBin->is_open()) {
        std::cerr << "[binario] no se pudo abrir para escritura: " << path
                  << std::endl;
        return false;
    }
    return true;
}

bool Binario::ifOpenBinary() {
    ifBin = new std::ifstream(path, std::ios::binary);
    if (!ifBin->is_open()) {
        std::cerr << "[binario] no se pudo abrir para lectura: " << path
                  << std::endl;
        return false;
    }
    return true;
}

void Binario::ofCloseBinary() { if (ofBin) ofBin->close(); }
void Binario::ifCloseBinary() { if (ifBin) ifBin->close(); }
std::ofstream* Binario::getOfBinariFile() { return ofBin; }
std::ifstream* Binario::getIfBinariFile() { return ifBin; }
