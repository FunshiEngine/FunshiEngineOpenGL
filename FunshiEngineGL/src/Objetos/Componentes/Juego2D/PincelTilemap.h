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
#ifndef PINCELTILEMAP_H
#define PINCELTILEMAP_H

#include <cstdint>
#include <optional>
#include <vector>

class Tilemap;

// Herramientas de pintado de un Tilemap, sin GUI ni GL (testeables headless).
// Trabajan en coordenadas LOCALES del objeto: 1 tile = 1 unidad y la celda
// (x, y) ocupa [x, x+1] en X y [-(y+1), -y] en Y (la fila 0 va arriba), que es
// la geometria que hornea Tilemap::construirGeometria.
//
// Cada operacion aplica el cambio y devuelve la lista de celdas que de verdad
// cambiaron, con su valor anterior: es lo que guarda el comando de undo.
struct CeldaTilemap {
    int x = 0;
    int y = 0;
};

struct CambioTile {
    int capa = 0;
    int x = 0;
    int y = 0;
    std::uint32_t anterior = 0;
    std::uint32_t nuevo = 0;
};

namespace PincelTilemap {

// Celda bajo el punto local (lx, ly), o nada si cae fuera de la capa.
std::optional<CeldaTilemap> celdaDesdeLocal(const Tilemap& tilemap, int capa,
                                            float lx, float ly);

// Pinta una celda (id 0 = borrar).
std::vector<CambioTile> pintar(Tilemap& tilemap, int capa, CeldaTilemap celda,
                               std::uint32_t id);

// Relleno por inundacion (4 vecinos) de la region contigua con el mismo id
// que la celda de partida.
std::vector<CambioTile> rellenar(Tilemap& tilemap, int capa, CeldaTilemap celda,
                                 std::uint32_t id);

// Rectangulo entre dos esquinas (incluidas), en cualquier orden.
std::vector<CambioTile> rectangulo(Tilemap& tilemap, int capa, CeldaTilemap a,
                                   CeldaTilemap b, std::uint32_t id);

// Vuelve las celdas a su valor anterior / las lleva al nuevo.
void deshacer(Tilemap& tilemap, const std::vector<CambioTile>& cambios);
void rehacer(Tilemap& tilemap, const std::vector<CambioTile>& cambios);

}  // namespace PincelTilemap

#endif
