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
#include "PincelTilemap.h"

#include <algorithm>
#include <cmath>

#include "Tilemap.h"

namespace PincelTilemap {

namespace {

bool dentro(const CapaTilemap& c, int x, int y) {
    return x >= 0 && y >= 0 && x < c.ancho && y < c.alto;
}

void aplicarUna(Tilemap& tilemap, int capa, int x, int y, std::uint32_t id,
                std::vector<CambioTile>& cambios) {
    const std::uint32_t anterior = tilemap.tileEn(capa, x, y);
    if (anterior == id) return;
    tilemap.pintar(capa, x, y, id);
    cambios.push_back({capa, x, y, anterior, id});
}

}  // namespace

std::optional<CeldaTilemap> celdaDesdeLocal(const Tilemap& tilemap, int capa,
                                            float lx, float ly) {
    const CapaTilemap* c = tilemap.capa(capa);
    if (!c || !std::isfinite(lx) || !std::isfinite(ly)) return std::nullopt;
    const int x = static_cast<int>(std::floor(lx));
    const int y = static_cast<int>(std::floor(-ly));
    if (!dentro(*c, x, y)) return std::nullopt;
    return CeldaTilemap{x, y};
}

std::vector<CambioTile> pintar(Tilemap& tilemap, int capa, CeldaTilemap celda,
                               std::uint32_t id) {
    std::vector<CambioTile> cambios;
    const CapaTilemap* c = tilemap.capa(capa);
    if (c && dentro(*c, celda.x, celda.y))
        aplicarUna(tilemap, capa, celda.x, celda.y, id, cambios);
    return cambios;
}

std::vector<CambioTile> rellenar(Tilemap& tilemap, int capa, CeldaTilemap celda,
                                 std::uint32_t id) {
    std::vector<CambioTile> cambios;
    const CapaTilemap* c = tilemap.capa(capa);
    if (!c || !dentro(*c, celda.x, celda.y)) return cambios;
    const std::uint32_t objetivo = tilemap.tileEn(capa, celda.x, celda.y);
    if (objetivo == id) return cambios;

    const int ancho = c->ancho;
    const int alto = c->alto;
    std::vector<char> visitada(static_cast<std::size_t>(ancho) * alto, 0);
    std::vector<CeldaTilemap> pendientes{celda};
    while (!pendientes.empty()) {
        const CeldaTilemap actual = pendientes.back();
        pendientes.pop_back();
        if (actual.x < 0 || actual.y < 0 || actual.x >= ancho || actual.y >= alto) continue;
        char& marca = visitada[static_cast<std::size_t>(actual.y) * ancho + actual.x];
        if (marca) continue;
        marca = 1;
        if (tilemap.tileEn(capa, actual.x, actual.y) != objetivo) continue;
        aplicarUna(tilemap, capa, actual.x, actual.y, id, cambios);
        pendientes.push_back({actual.x + 1, actual.y});
        pendientes.push_back({actual.x - 1, actual.y});
        pendientes.push_back({actual.x, actual.y + 1});
        pendientes.push_back({actual.x, actual.y - 1});
    }
    return cambios;
}

std::vector<CambioTile> rectangulo(Tilemap& tilemap, int capa, CeldaTilemap a,
                                   CeldaTilemap b, std::uint32_t id) {
    std::vector<CambioTile> cambios;
    const CapaTilemap* c = tilemap.capa(capa);
    if (!c) return cambios;
    const int x0 = std::max(0, std::min(a.x, b.x));
    const int x1 = std::min(c->ancho - 1, std::max(a.x, b.x));
    const int y0 = std::max(0, std::min(a.y, b.y));
    const int y1 = std::min(c->alto - 1, std::max(a.y, b.y));
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) aplicarUna(tilemap, capa, x, y, id, cambios);
    return cambios;
}

void deshacer(Tilemap& tilemap, const std::vector<CambioTile>& cambios) {
    for (auto it = cambios.rbegin(); it != cambios.rend(); ++it)
        tilemap.pintar(it->capa, it->x, it->y, it->anterior);
}

void rehacer(Tilemap& tilemap, const std::vector<CambioTile>& cambios) {
    for (const CambioTile& cambio : cambios)
        tilemap.pintar(cambio.capa, cambio.x, cambio.y, cambio.nuevo);
}

}  // namespace PincelTilemap
