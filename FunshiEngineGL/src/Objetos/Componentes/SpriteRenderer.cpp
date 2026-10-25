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
#include "SpriteRenderer.h"

#include <cstdint>
#include <cstring>

#include "Configuracion/EditorConfig.h"

namespace {
// Cota del path de textura en disco, igual que Material: el deserializador
// descarta paths por encima y omite la misma cantidad de bytes para que los
// siguientes componentes del objeto sigan alineados.
const std::size_t kMaxSpritePathLength = 4096;
// Marcador binario del payload del componente. v1 = tint (4 floats), tamano (2),
// uvRect (4) y, al final, un path de textura. La ausencia de magic (escena sin
// esta version; no existe una anterior) se rebobina y se leen solo los 10 float
// base, sin path.
const char kSpriteMagicV1[4] = {'F', 'S', 'p', '1'};

void escribirPath(std::ofstream* file, const std::string& path) {
    const std::uint32_t len = static_cast<std::uint32_t>(path.size());
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    file->write(path.data(), static_cast<std::streamsize>(len));
}

// Lee el path relativo guardado y lo absolutiza contra la raiz de assets actual
// (simetrico a EditorConfig::relativizarRuta al guardar). Un path por encima de
// la cota se omite entero para no desalinear el stream.
void leerPath(std::ifstream* file, std::string& path) {
    std::uint32_t len = 0;
    file->read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > kMaxSpritePathLength) {
        file->seekg(static_cast<std::streamoff>(len), std::ios::cur);
        path.clear();
        return;
    }
    path.assign(len, '\0');
    if (len > 0) file->read(path.data(), static_cast<std::streamsize>(len));
    path = EditorConfig::absolutizarRuta(path);
}
} // namespace

SpriteRenderer::SpriteRenderer() = default;

void SpriteRenderer::setRutaTextura(const std::string& ruta) { rutaTextura_ = ruta; }

void SpriteRenderer::setTint(float r, float g, float b, float a) {
    tint_[0] = r; tint_[1] = g; tint_[2] = b; tint_[3] = a;
}

void SpriteRenderer::setTamano(float ancho, float alto) {
    tamano_[0] = ancho; tamano_[1] = alto;
}

void SpriteRenderer::setUvRect(float u0, float v0, float u1, float v1) {
    uvRect_[0] = u0; uvRect_[1] = v0; uvRect_[2] = u1; uvRect_[3] = v1;
}

void SpriteRenderer::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    file->write(kSpriteMagicV1, sizeof(kSpriteMagicV1));
    file->write(reinterpret_cast<const char*>(tint_), sizeof(tint_));
    file->write(reinterpret_cast<const char*>(tamano_), sizeof(tamano_));
    file->write(reinterpret_cast<const char*>(uvRect_), sizeof(uvRect_));
    // El path se persiste relativo a la raiz de assets del proyecto (si esta
    // fijada) para que la escena siga valida al mover/renombrar el proyecto.
    const std::string relativa = EditorConfig::relativizarRuta(rutaTextura_);
    const std::uint32_t len = static_cast<std::uint32_t>(relativa.size());
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    file->write(relativa.data(), static_cast<std::streamsize>(len));
}

void SpriteRenderer::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;

    char magic[4] = {};
    file->read(magic, sizeof(magic));
    const bool conPath =
        file->gcount() == static_cast<std::streamsize>(sizeof(magic)) &&
        std::memcmp(magic, kSpriteMagicV1, sizeof(magic)) == 0;
    if (!conPath) file->seekg(-static_cast<std::streamoff>(sizeof(magic)), std::ios::cur);

    file->read(reinterpret_cast<char*>(tint_), sizeof(tint_));
    file->read(reinterpret_cast<char*>(tamano_), sizeof(tamano_));
    file->read(reinterpret_cast<char*>(uvRect_), sizeof(uvRect_));

    rutaTextura_.clear();
    if (conPath) leerPath(file, rutaTextura_);
}

