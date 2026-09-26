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
#include "Model.h"

#include <cstring>
#include <iostream>

#include "Configuracion/EditorConfig.h"

using namespace std;

void Model::setPath(string path) {
#if defined(_WIN32)
    strncpy_s(this->filePath, sizeof(this->filePath), path.c_str(), _TRUNCATE);
#elif defined(__linux__)
    strncpy(this->filePath, path.c_str(), sizeof(this->filePath) - 1);
    this->filePath[sizeof(this->filePath) - 1] = '\0';
#endif
}

void Model::serializeComponent(std::ofstream* fileNamePathContentObject) {
    // Se persiste relativo a la raiz de assets del proyecto (si esta fijada),
    // de modo que la escena siga valida al renombrar/mover el proyecto entero.
    const std::string almacenar = EditorConfig::relativizarRuta(filePath);
    size_t len = almacenar.size();
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&len),
                                     sizeof(size_t));
    if (len > 0) {
        fileNamePathContentObject->write(almacenar.c_str(), len);
    } else {
        std::cout << "No hay un path en el Objeto para almacenarlo" << std::endl;
    }
}

void Model::deserializeComponent(std::ifstream* fileNamePathContentObject) {
    size_t len = 0;
    fileNamePathContentObject->read(reinterpret_cast<char*>(&len),
                                    sizeof(size_t));

    if (len > 0) {
        // El escritor guarda el path completo, que puede superar lo que entra
        // en filePath: hay que consumir del stream exactamente `len` bytes
        // aunque se descarte el excedente. Leer de menos desalinea los
        // componentes siguientes y la carga termina leyendo basura (mismo
        // criterio que Material::leerPathTextura).
        const size_t toRead = std::min(len, sizeof(filePath) - 1);
        fileNamePathContentObject->read(
            filePath, static_cast<std::streamsize>(toRead));
        filePath[toRead] = '\0';
        if (len > toRead) {
            fileNamePathContentObject->seekg(
                static_cast<std::streamoff>(len - toRead), std::ios::cur);
        }

        // Las escenas nuevas persisten la ruta relativa a la raiz de assets;
        // las legacy guardaban la absoluta, que deste se deja intacta.
        const std::string abs =
            EditorConfig::absolutizarRuta(std::string(filePath));
        strncpy(filePath, abs.c_str(), sizeof(filePath) - 1);
        filePath[sizeof(filePath) - 1] = '\0';

        std::cout << filePath << std::endl;
    } else {
        filePath[0] = '\0';
        std::cout << "No hay un path en el binario" << std::endl;
    }
}

void Model::saveComponent(std::ofstream* fileNamePathContentObject) {
    serializeComponent(fileNamePathContentObject);
}

void Model::loadComponent(std::ifstream* fileNamePathContentObject) {
    deserializeComponent(fileNamePathContentObject);
}