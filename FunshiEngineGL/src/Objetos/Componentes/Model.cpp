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
#include <filesystem>
#include <iostream>
#include <vector>

#include "Configuracion/EditorConfig.h"
#include "../../Assets/AssetException.h"
#include "../../Assets/AssetManager.h"
#include "../../Assets/AssimpMeshLoader.h"
#include "../../Assets/Mesh.h"
#include "../../Diagnosticos/RegistroDiagnosticos.h"
#include "../../ExcepcionesCPP/RuntimeException.h"
#include "../GameObject.h"
#include "../../Matematicas/StructVec3.h"

void Model::setPath(string path) {
#if defined(_WIN32)
    strncpy_s(this->filePath, sizeof(this->filePath), path.c_str(), _TRUNCATE);
#elif defined(__linux__)
    strncpy(this->filePath, path.c_str(), sizeof(this->filePath) - 1);
    this->filePath[sizeof(this->filePath) - 1] = '\0';
#endif
    // Cargar la milla inmediatamente al asignar el path, igual que
    // Modelos3D::setObject() hacia antes.
    cargarMalla();
}

void Model::cargarMalla() {
    mesh_.reset();
    if (std::string(filePath).empty()) return;

    try {
        // Con el AssetManager inyectado la malla se comparte: pedir el mismo
        // path en N objetos devuelve el mismo asset (1 parseo de Assimp).
        // Sin manager se usa el loader local como respaldo. Los fallos no
        // matan el editor: queda sin malla y se informa por consola.
        mesh_ = assets_ ? assets_->getMesh(std::string(filePath))
                        : AssimpMeshLoader().load(std::string(filePath));
    } catch (const RuntimeException& e) {
        if (diagnosticos_) {
            registrarExcepcionDiagnostica(
                *diagnosticos_, "Objetos/Model", e,
                NivelDiagnostico::Error, "malla:" + std::string(filePath));
        } else {
            registrarExcepcionDiagnostica(
                "Objetos/Model", e, NivelDiagnostico::Error,
                "malla:" + std::string(filePath));
        }
        std::cerr << "Model: no se pudo cargar malla '" << filePath
                  << "': " << e.what() << '\n';
        mesh_.reset();
    } catch (const std::exception& e) {
        registrarExcepcionDiagnostica("Objetos/Model", e,
                                      NivelDiagnostico::Error,
                                      "malla:" + std::string(filePath));
        std::cerr << "Model: error inesperado al cargar '" << filePath
                  << "': " << e.what() << '\n';
        mesh_.reset();
    }
}

void Model::onLoaded(GameObject& /*owner*/) {
    // Post-carga: intenta cargar la malla del path (si hay uno).
    cargarMalla();
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
        std::cout << "No hay un path en el Objeto para almacenarlo"
                  << std::endl;
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
    } else {
        filePath[0] = '\0';
    }
}

const std::vector<vec3>& Model::getVertices() const {
    // Referencia estable compartida para la colision cuando no hay malla
    // cargada: nunca se devuelve nullptr a los consumidores (MallaCollider).
    static const std::vector<vec3> verticesVacios;
    return mesh_ ? mesh_->vertices : verticesVacios;
}

bool Model::getBoundingBox(vec3& outMin, vec3& outMax) const {
    return mesh_ && mesh_->computeBounds(outMin, outMax);
}
