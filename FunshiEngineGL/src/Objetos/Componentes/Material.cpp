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
#include "Material.h"

#include <cstdint>
#include <cstring>
#include <iostream>

#include "Configuracion/EditorConfig.h"

namespace {
// Longitud maxima de path de textura en disco. El deserializador descarta
// paths por encima (cota estilo Modelos3D) para no leer buffers gigantes de
// un archivo corrupto, omitiendo la misma cantidad de bytes para que los
// siguientes componentes sigan alineados.
const std::size_t kMaxTexturePathLength = 4096;
// Marcadores binarios al inicio del payload Material. Las escenas viejas no
// tienen ninguno (el formato legacy es solo los 5 campos float sin paths).
// v1 (FMtA) = rama de texturas original: 1 path (difusa). v2 (FMtB) = esta
// rama: los 4 slots en orden difusa/especular/normal/emision. Al leer, la
// ausencia de magic o un v1 mas antiguo se rebobina y se lee el formato
// respectivo; los slots sin datos quedan vacios para no romper escenas
// guardadas con versiones anteriores.
const char kMaterialMagicV1[4] = {'F', 'M', 't', 'A'};
const char kMaterialMagicV2[4] = {'F', 'M', 't', 'B'};

void escribirPath(std::ofstream* file, const std::string& path) {
    const std::uint32_t len = static_cast<std::uint32_t>(path.size());
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    file->write(path.data(), static_cast<std::streamsize>(len));
}
} // namespace

Material::Material() {
    ambient[0] = 0.25f; ambient[1] = 0.25f; ambient[2] = 0.25f; ambient[3] = 1.f;
    diffuse[0] = 1.f; diffuse[1] = 1.f; diffuse[2] = 1.f; diffuse[3] = 1.f;
    specular[0] = 0.5f; specular[1] = 0.5f; specular[2] = 0.5f; specular[3] = 1.f;
    emission[0] = 0.f; emission[1] = 0.f; emission[2] = 0.f; emission[3] = 1.f;
    shininess = 32.f;
}

void Material::leerPathTextura(std::ifstream* file, std::string& out) {
    out.clear();
    std::uint32_t len = 0;
    file->read(reinterpret_cast<char*>(&len), sizeof(len));
    if (file->gcount() != static_cast<std::streamsize>(sizeof(len))) return;
    const std::size_t pathLen = len;
    if (pathLen > kMaxTexturePathLength) {
        // Path demasiado largo: descartar los bytes para no desalinear a los
        // componentes siguientes y dejar el slot vacio.
        file->seekg(static_cast<std::streamoff>(pathLen), std::ios::cur);
        return;
    }
    if (pathLen == 0) return;
    out.assign(pathLen, '\0');
    file->read(&out[0], static_cast<std::streamsize>(pathLen));
    // Escenas nuevas guardan la ruta relativa a la raiz de assets del
    // proyecto; las legacy guardaban la absoluta, que se deja intacta.
    out = EditorConfig::absolutizarRuta(out);
}

void Material::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    // Magic v2: distingue este formato del v1 y del legacy (ver arriba).
    file->write(kMaterialMagicV2, sizeof(kMaterialMagicV2));
    file->write(reinterpret_cast<const char*>(ambient), sizeof(ambient));
    file->write(reinterpret_cast<const char*>(diffuse), sizeof(diffuse));
    file->write(reinterpret_cast<const char*>(specular), sizeof(specular));
    file->write(reinterpret_cast<const char*>(emission), sizeof(emission));
    file->write(reinterpret_cast<const char*>(&shininess), sizeof(float));
    // Los paths se persisten relativos a la raiz de assets del proyecto (si
    // esta fijada) para que la escena siga valida al mover/renombrar el
    // proyecto entero.
    escribirPath(file, EditorConfig::relativizarRuta(diffuseMapPath_));
    escribirPath(file, EditorConfig::relativizarRuta(specularMapPath_));
    escribirPath(file, EditorConfig::relativizarRuta(normalMapPath_));
    escribirPath(file, EditorConfig::relativizarRuta(emissionMapPath_));
}

void Material::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) {
        std::cerr << "Error: archivo invalido o no abierto para lectura (Material).\n";
        return;
    }
    char magic[4];
    file->read(magic, sizeof(magic));
    const bool esV2 =
        file->gcount() == static_cast<std::streamsize>(sizeof(magic)) &&
        std::memcmp(magic, kMaterialMagicV2, sizeof(kMaterialMagicV2)) == 0;
    const bool esV1 =
        !esV2 && file->gcount() == static_cast<std::streamsize>(sizeof(magic)) &&
        std::memcmp(magic, kMaterialMagicV1, sizeof(kMaterialMagicV1)) == 0;
    if (!esV2 && !esV1) {
        // Formato legacy (escenas viejas, sin texturas): rebobinar los 4
        // bytes leidos y leer los 5 campos como siempre.
        file->seekg(-static_cast<std::streamoff>(sizeof(magic)),
                    std::ios::cur);
    }

    file->read(reinterpret_cast<char*>(ambient), sizeof(ambient));
    file->read(reinterpret_cast<char*>(diffuse), sizeof(diffuse));
    file->read(reinterpret_cast<char*>(specular), sizeof(specular));
    file->read(reinterpret_cast<char*>(emission), sizeof(emission));
    file->read(reinterpret_cast<char*>(&shininess), sizeof(float));

    diffuseMapPath_.clear();
    specularMapPath_.clear();
    normalMapPath_.clear();
    emissionMapPath_.clear();

    if (esV2) {
        leerPathTextura(file, diffuseMapPath_);
        leerPathTextura(file, specularMapPath_);
        leerPathTextura(file, normalMapPath_);
        leerPathTextura(file, emissionMapPath_);
    } else if (esV1) {
        // v1: solo difusa al final del payload; el resto de slots vacio.
        leerPathTextura(file, diffuseMapPath_);
    }
}

void Material::saveComponent(std::ofstream* file) { serializeComponent(file); }

void Material::loadComponent(std::ifstream* file) { deserializeComponent(file); }

void Material::setAmbient(float r, float g, float b) {
    ambient[0] = r; ambient[1] = g; ambient[2] = b; ambient[3] = 1.f;
}
void Material::setDiffuse(float r, float g, float b) {
    diffuse[0] = r; diffuse[1] = g; diffuse[2] = b; diffuse[3] = 1.f;
}
void Material::setSpecular(float r, float g, float b) {
    specular[0] = r; specular[1] = g; specular[2] = b; specular[3] = 1.f;
}
void Material::setEmission(float r, float g, float b) {
    emission[0] = r; emission[1] = g; emission[2] = b; emission[3] = 1.f;
}
void Material::setShininess(float value) { shininess = value; }