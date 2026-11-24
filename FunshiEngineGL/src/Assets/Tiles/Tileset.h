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
#ifndef ASSETS_TILESET_H
#define ASSETS_TILESET_H

#include <cstdint>
#include <string>
#include <vector>

// Metadatos de un tileset para el componente Tilemap: una imagen (atlas) con una
// grilla regular de tiles y un flag de solidez por tipo de tile.
//
// Se cargan de un JSON compatible con el export JSON de Tiled (subset): campos
// "image", "imagewidth", "imageheight", "tilewidth", "tileheight", "margin",
// "spacing", "columns", "tilecount" y "tiles" (con "properties" donde el nombre
// "solid" marca el tile como colisionable). El struct no depende de OpenGL: los
// tests headless lo construyen y lo verifican sin contexto grafico.
struct Tileset {
    // Ruta de la imagen relativa al archivo del tileset (semantica de Tiled).
    std::string rutaImagen;
    int imageWidth = 0;
    int imageHeight = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    int margin = 0;
    int spacing = 0;
    int columns = 0;
    int rows = 0;
    int tileCount = 0;
    // Un byte por id de tile (0..tileCount-1): 1 si el tile es solido.
    std::vector<std::uint8_t> solido;

    bool esValido() const {
        return imageWidth > 0 && imageHeight > 0 && tileWidth > 0 &&
               tileHeight > 0 && columns > 0 && rows > 0;
    }

    bool esSolido(std::uint32_t id) const {
        return id < solido.size() && solido[id] != 0;
    }

    // Region UV [u0, v0, u1, v1] del tile con la orientacion de la textura tal
    // como la sube el motor (stb la carga espejada en vertical: v=0 es el borde
    // inferior de la imagen). Aplica un inset de medio texel para que el
    // muestreo con mipmaps no absorba el tile vecino.
    void uvDeTile(std::uint32_t id, float out[4]) const;

    // Ruta absoluta de la imagen a partir de la ruta absoluta del archivo del
    // tileset (la imagen del JSON es relativa al propio tileset).
    std::string rutaImagenAbsoluta(const std::string& rutaTilesetAbs) const;
};

// Carga un tileset desde el JSON en 'rutaAbsoluta'. Devuelve false y describe el
// motivo en 'error' si el archivo no abre o le faltan campos obligatorios.
bool cargarTilesetDesdeJson(const std::string& rutaAbsoluta, Tileset& out,
                            std::string& error);

#endif
