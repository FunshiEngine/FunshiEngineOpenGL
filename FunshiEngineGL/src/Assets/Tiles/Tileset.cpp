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
#include "Tileset.h"
#include "../../Diagnosticos/RegistroDiagnosticos.h"

#include <filesystem>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace {
// El tipo de los campos numericos del JSON de Tiled es entero. Una entrada de
// otro tipo o ausente se resuelve con el valor por defecto (0 = "recalcular").
int leerEntero(const nlohmann::json& objeto, const char* campo, int porDefecto) {
    if (!objeto.contains(campo)) return porDefecto;
    const nlohmann::json& valor = objeto[campo];
    if (!valor.is_number_integer()) return porDefecto;
    return valor.get<int>();
}
} // namespace

void Tileset::uvDeTile(std::uint32_t id, float out[4]) const {
    out[0] = 0.0f; out[1] = 0.0f; out[2] = 1.0f; out[3] = 1.0f;
    if (!esValido()) return;

    const std::uint32_t col = id % static_cast<std::uint32_t>(columns);
    const std::uint32_t fila = id / static_cast<std::uint32_t>(columns);
    const float invW = 1.0f / static_cast<float>(imageWidth);
    const float invH = 1.0f / static_cast<float>(imageHeight);
    const float pasoX = static_cast<float>(tileWidth + spacing);
    const float pasoY = static_cast<float>(tileHeight + spacing);
    const float margen = static_cast<float>(margin);

    const float x0 = margen + static_cast<float>(col) * pasoX;
    const float y0 = margen + static_cast<float>(fila) * pasoY;

    float u0 = x0 * invW;
    float u1 = (x0 + static_cast<float>(tileWidth)) * invW;
    // La imagen se sube invertida en vertical (flip on load), por eso el borde
    // inferior del tile en la imagen corresponde a una v menor.
    float v1 = 1.0f - (y0 * invH);
    float v0 = 1.0f - ((y0 + static_cast<float>(tileHeight)) * invH);

    const float insetU = 0.5f * invW;
    const float insetV = 0.5f * invH;
    if (u1 - u0 > 2.0f * insetU) { u0 += insetU; u1 -= insetU; }
    if (v1 - v0 > 2.0f * insetV) { v0 += insetV; v1 -= insetV; }

    out[0] = u0; out[1] = v0; out[2] = u1; out[3] = v1;
}

std::string Tileset::rutaImagenAbsoluta(const std::string& rutaTilesetAbs) const {
    if (rutaImagen.empty()) return std::string();
    const std::filesystem::path base =
        std::filesystem::path(rutaTilesetAbs).parent_path();
    return (base / rutaImagen).string();
}

bool cargarTilesetDesdeJson(const std::string& rutaAbsoluta, Tileset& out,
                            std::string& error) {
    error.clear();
    std::ifstream archivo(rutaAbsoluta, std::ios::binary);
    if (!archivo.is_open()) {
        error = "no se pudo abrir el tileset";
        return false;
    }

    nlohmann::json raiz;
    try {
        archivo >> raiz;
    } catch (const std::exception& e) {
        error = std::string("JSON invalido: ") + e.what();
        registrarExcepcionDiagnostica(
            "Assets/Tileset", e, NivelDiagnostico::Advertencia,
            "tileset:" + rutaAbsoluta);
        return false;
    }
    if (!raiz.is_object()) {
        error = "el tileset no es un objeto JSON";
        return false;
    }

    Tileset t;
    if (raiz.contains("image") && raiz["image"].is_string())
        t.rutaImagen = raiz["image"].get<std::string>();
    if (t.rutaImagen.empty()) {
        error = "el tileset no declara 'image'";
        return false;
    }

    t.imageWidth = leerEntero(raiz, "imagewidth", 0);
    t.imageHeight = leerEntero(raiz, "imageheight", 0);
    t.tileWidth = leerEntero(raiz, "tilewidth", 0);
    t.tileHeight = leerEntero(raiz, "tileheight", 0);
    t.margin = leerEntero(raiz, "margin", 0);
    t.spacing = leerEntero(raiz, "spacing", 0);

    if (t.imageWidth <= 0 || t.imageHeight <= 0) {
        error = "faltan 'imagewidth'/'imageheight' positivos";
        return false;
    }
    if (t.tileWidth <= 0 || t.tileHeight <= 0) {
        error = "faltan 'tilewidth'/'tileheight' positivos";
        return false;
    }
    if (t.margin < 0 || t.spacing < 0) {
        error = "'margin'/'spacing' no pueden ser negativos";
        return false;
    }

    const int pasoX = t.tileWidth + t.spacing;
    const int pasoY = t.tileHeight + t.spacing;
    int columnas = leerEntero(raiz, "columns", 0);
    if (columnas <= 0)
        columnas = (t.imageWidth - 2 * t.margin + t.spacing) / pasoX;
    int filas = (t.imageHeight - 2 * t.margin + t.spacing) / pasoY;
    if (columnas <= 0 || filas <= 0) {
        error = "la grilla del tileset no tiene celdas (revisar margen/spacing)";
        return false;
    }
    t.columns = columnas;
    t.rows = filas;

    int total = leerEntero(raiz, "tilecount", 0);
    if (total <= 0 || total > columnas * filas) total = columnas * filas;
    t.tileCount = total;
    t.solido.assign(static_cast<std::size_t>(total), 0);

    if (raiz.contains("tiles") && raiz["tiles"].is_array()) {
        for (const nlohmann::json& tile : raiz["tiles"]) {
            if (!tile.is_object() || !tile.contains("id")) continue;
            const int id = leerEntero(tile, "id", -1);
            if (id < 0 || id >= total) continue;

            bool solido = false;
            if (tile.contains("properties") && tile["properties"].is_array()) {
                for (const nlohmann::json& prop : tile["properties"]) {
                    if (!prop.is_object() || !prop.contains("name")) continue;
                    if (!prop["name"].is_string()) continue;
                    if (prop["name"].get<std::string>() != "solid") continue;
                    if (!prop.contains("value")) continue;
                    const nlohmann::json& v = prop["value"];
                    if (v.is_boolean())
                        solido = v.get<bool>();
                    else if (v.is_number())
                        solido = v.get<double>() != 0.0;
                    else if (v.is_string())
                        solido = (v.get<std::string>() == "true");
                }
            }
            t.solido[static_cast<std::size_t>(id)] = solido ? 1 : 0;
        }
    }

    out = std::move(t);
    return true;
}
