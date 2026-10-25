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
#include "Tilemap.h"

#include <algorithm>
#include <cstring>
#include <fstream>

#include "../../Assets/Mesh.h"
#include "../../Configuracion/EditorConfig.h"

namespace {
// Marcador y version del payload binario. v1 = ruta del tileset, numero de
// capas y, por capa, nombre, flags (visible/colision), dimensiones y los ids.
const char kTilemapMagic[4] = {'F', 'T', 'M', '1'};
const std::uint32_t kTilemapVersion = 1;

// Cotas defensivas del deserializador: acotan memoria y desalineaciones.
const std::uint32_t kMaxCapas = 256;
const int kMaxLadoGrilla = 4096;
const std::size_t kMaxNombreCapa = 256;
const std::size_t kMaxRutaTileset = 4096;

void escribirCadena(std::ofstream* file, const std::string& texto) {
    const std::uint32_t len = static_cast<std::uint32_t>(texto.size());
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) file->write(texto.data(), static_cast<std::streamsize>(len));
}

// Lee una cadena acotada; si supera la cota la salta entera para no desalinear.
void leerCadena(std::ifstream* file, std::string& texto, std::size_t cota) {
    std::uint32_t len = 0;
    file->read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > cota) {
        file->seekg(static_cast<std::streamoff>(len), std::ios::cur);
        texto.clear();
        return;
    }
    texto.assign(len, '\0');
    if (len > 0) file->read(texto.data(), static_cast<std::streamsize>(len));
}
} // namespace

Tilemap::Tilemap() = default;

void Tilemap::setRutaTileset(const std::string& ruta) {
    rutaTileset_ = ruta;
    recargarTileset();
}

void Tilemap::recargarTileset() {
    tileset_ = Tileset{};
    ultimoError_.clear();
    if (rutaTileset_.empty()) return;
    if (!cargarTilesetDesdeJson(
            EditorConfig::absolutizarRuta(rutaTileset_), tileset_,
            ultimoError_)) {
        tileset_ = Tileset{};
    }
    ++revision_;
}

std::string Tilemap::rutaImagenTileset() const {
    if (rutaTileset_.empty()) return std::string();
    return tileset_.rutaImagenAbsoluta(
        EditorConfig::absolutizarRuta(rutaTileset_));
}

const CapaTilemap* Tilemap::capa(int indice) const {
    if (indice < 0 || indice >= cantidadCapas()) return nullptr;
    return &capas_[static_cast<std::size_t>(indice)];
}

CapaTilemap* Tilemap::capa(int indice) {
    if (indice < 0 || indice >= cantidadCapas()) return nullptr;
    return &capas_[static_cast<std::size_t>(indice)];
}

int Tilemap::agregarCapa(const std::string& nombre, int ancho, int alto,
                         bool colision) {
    if (ancho <= 0 || alto <= 0 || ancho > kMaxLadoGrilla ||
        alto > kMaxLadoGrilla)
        return -1;
    if (capas_.size() >= kMaxCapas) return -1;

    CapaTilemap nueva;
    nueva.nombre = nombre.empty() ? "Capa" : nombre;
    nueva.visible = true;
    nueva.colision = colision;
    nueva.ancho = ancho;
    nueva.alto = alto;
    nueva.tiles.assign(static_cast<std::size_t>(ancho) *
                           static_cast<std::size_t>(alto),
                       0);
    capas_.push_back(std::move(nueva));
    ++revision_;
    return cantidadCapas() - 1;
}

bool Tilemap::eliminarCapa(int indice) {
    if (indice < 0 || indice >= cantidadCapas()) return false;
    capas_.erase(capas_.begin() + indice);
    ++revision_;
    return true;
}

void Tilemap::setCapaVisible(int indice, bool visible) {
    CapaTilemap* c = capa(indice);
    if (!c || c->visible == visible) return;
    c->visible = visible;
    ++revision_;
}

void Tilemap::setCapaColision(int indice, bool colision) {
    CapaTilemap* c = capa(indice);
    if (!c || c->colision == colision) return;
    c->colision = colision;
    ++revision_;
}

void Tilemap::setCapaNombre(int indice, const std::string& nombre) {
    CapaTilemap* c = capa(indice);
    if (!c) return;
    c->nombre = nombre.empty() ? "Capa" : nombre;
}

void Tilemap::pintar(int indice, int x, int y, std::uint32_t id) {
    CapaTilemap* c = capa(indice);
    if (!c) return;
    if (x < 0 || y < 0 || x >= c->ancho || y >= c->alto) return;
    std::uint32_t& celda =
        c->tiles[static_cast<std::size_t>(y) * c->ancho + x];
    if (celda == id) return;
    celda = id;
    ++revision_;
}

std::uint32_t Tilemap::tileEn(int indice, int x, int y) const {
    const CapaTilemap* c = capa(indice);
    if (!c) return 0;
    if (x < 0 || y < 0 || x >= c->ancho || y >= c->alto) return 0;
    return c->tiles[static_cast<std::size_t>(y) * c->ancho + x];
}

bool Tilemap::construirGeometria(Mesh& out) const {
    out = Mesh{};
    if (!tileset_.esValido()) return false;

    bool hayTiles = false;
    for (const CapaTilemap& c : capas_) {
        if (!c.visible) continue;

        for (int y = 0; y < c.alto; ++y) {
            for (int x = 0; x < c.ancho; ++x) {
                const std::uint32_t id =
                    c.tiles[static_cast<std::size_t>(y) * c.ancho + x];
                if (id == 0) continue;

                float uv[4] = {0.0f, 0.0f, 1.0f, 1.0f};
                tileset_.uvDeTile(id, uv);

                // 1 tile = 1 unidad local; la fila 0 va arriba (eje Y hacia
                // arriba), por eso el centro en Y es negativo.
                const float cx = static_cast<float>(x) + 0.5f;
                const float cy = -(static_cast<float>(y) + 0.5f);

                const unsigned int base =
                    static_cast<unsigned int>(out.vertices.size());
                out.vertices.push_back(vec3(cx - 0.5f, cy - 0.5f, 0.0f));
                out.vertices.push_back(vec3(cx + 0.5f, cy - 0.5f, 0.0f));
                out.vertices.push_back(vec3(cx + 0.5f, cy + 0.5f, 0.0f));
                out.vertices.push_back(vec3(cx - 0.5f, cy + 0.5f, 0.0f));
                out.uvs.push_back(vec2(uv[0], uv[1]));
                out.uvs.push_back(vec2(uv[2], uv[1]));
                out.uvs.push_back(vec2(uv[2], uv[3]));
                out.uvs.push_back(vec2(uv[0], uv[3]));
                out.indices.push_back(base + 0);
                out.indices.push_back(base + 1);
                out.indices.push_back(base + 2);
                out.indices.push_back(base + 0);
                out.indices.push_back(base + 2);
                out.indices.push_back(base + 3);
                // El tilemap no requiere iluminacion; una normal +Z mantiene el
                // formato de vertice valido si el shader la consulta.
                out.normals.push_back(vec3(0.0f, 0.0f, 1.0f));
                out.normals.push_back(vec3(0.0f, 0.0f, 1.0f));
                out.normals.push_back(vec3(0.0f, 0.0f, 1.0f));
                out.normals.push_back(vec3(0.0f, 0.0f, 1.0f));
            }
        }
    }
    return !out.vertices.empty();
}

const Mesh& Tilemap::mallaHorneada() const {
    if (!mallaConstruida_ || mallaRevision_ != revision_) {
        construirGeometria(mallaCache_);
        mallaRevision_ = revision_;
        mallaConstruida_ = true;
    }
    return mallaCache_;
}

void Tilemap::onLoaded(GameObject& /*dueno*/) { recargarTileset(); }

void Tilemap::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    file->write(kTilemapMagic, sizeof(kTilemapMagic));
    file->write(reinterpret_cast<const char*>(&kTilemapVersion),
                sizeof(kTilemapVersion));

    const std::string relativa = EditorConfig::relativizarRuta(rutaTileset_);
    escribirCadena(file, relativa);

    const std::uint32_t total = static_cast<std::uint32_t>(capas_.size());
    file->write(reinterpret_cast<const char*>(&total), sizeof(total));
    for (const CapaTilemap& c : capas_) {
        escribirCadena(file, c.nombre);
        const std::uint8_t flags =
            static_cast<std::uint8_t>((c.visible ? 0x1 : 0x0) |
                                      (c.colision ? 0x2 : 0x0));
        file->write(reinterpret_cast<const char*>(&flags), sizeof(flags));
        const std::int32_t ancho = c.ancho;
        const std::int32_t alto = c.alto;
        file->write(reinterpret_cast<const char*>(&ancho), sizeof(ancho));
        file->write(reinterpret_cast<const char*>(&alto), sizeof(alto));
        if (!c.tiles.empty())
            file->write(reinterpret_cast<const char*>(c.tiles.data()),
                        static_cast<std::streamsize>(c.tiles.size() *
                                                     sizeof(std::uint32_t)));
    }
}

void Tilemap::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;

    char magic[4] = {};
    file->read(magic, sizeof(magic));
    if (!(file->gcount() == static_cast<std::streamsize>(sizeof(magic)) &&
          std::memcmp(magic, kTilemapMagic, sizeof(magic)) == 0)) {
        file->seekg(-static_cast<std::streamoff>(sizeof(magic)), std::ios::cur);
        return;
    }

    std::uint32_t version = 0;
    file->read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != kTilemapVersion) return;

    std::string relativa;
    leerCadena(file, relativa, kMaxRutaTileset);
    rutaTileset_ = EditorConfig::absolutizarRuta(relativa);

    capas_.clear();
    std::uint32_t total = 0;
    file->read(reinterpret_cast<char*>(&total), sizeof(total));
    total = std::min(total, kMaxCapas);

    for (std::uint32_t i = 0; i < total; ++i) {
        CapaTilemap c;
        leerCadena(file, c.nombre, kMaxNombreCapa);
        std::uint8_t flags = 0;
        file->read(reinterpret_cast<char*>(&flags), sizeof(flags));
        c.visible = (flags & 0x1) != 0;
        c.colision = (flags & 0x2) != 0;

        std::int32_t ancho = 0;
        std::int32_t alto = 0;
        file->read(reinterpret_cast<char*>(&ancho), sizeof(ancho));
        file->read(reinterpret_cast<char*>(&alto), sizeof(alto));
        ancho = std::clamp(ancho, 0, kMaxLadoGrilla);
        alto = std::clamp(alto, 0, kMaxLadoGrilla);
        c.ancho = ancho;
        c.alto = alto;

        const std::size_t celdas = static_cast<std::size_t>(ancho) *
                                   static_cast<std::size_t>(alto);
        c.tiles.assign(celdas, 0);
        if (celdas > 0)
            file->read(reinterpret_cast<char*>(c.tiles.data()),
                       static_cast<std::streamsize>(celdas *
                                                    sizeof(std::uint32_t)));
        capas_.push_back(std::move(c));
    }

    tileset_ = Tileset{};
    ++revision_;
}

