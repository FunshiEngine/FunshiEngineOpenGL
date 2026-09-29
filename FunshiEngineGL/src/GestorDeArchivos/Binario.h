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
#ifndef BINARIO_H
#define BINARIO_H

#include <fstream>
#include <string>

class Binario {
private:
    std::string path;
    std::ofstream* ofBin;
    std::ifstream* ifBin;

public:
    explicit Binario(std::string path);
    ~Binario();

    Binario(const Binario&) = delete;
    Binario& operator=(const Binario&) = delete;

    std::string getPath();
    // Devuelven false si el archivo no se pudo abrir (sin directorio, sin
    // permisos...). En ese caso NO se escribe ni se lee nada y el fallo queda
    // logueado: antes los dos devolvian void y cada write()/read() fallaba en
    // silencio.
    bool ofOpenBinary();
    bool ifOpenBinary();
    void ofCloseBinary();
    void ifCloseBinary();
    std::ofstream* getOfBinariFile();
    std::ifstream* getIfBinariFile();
};

#endif
