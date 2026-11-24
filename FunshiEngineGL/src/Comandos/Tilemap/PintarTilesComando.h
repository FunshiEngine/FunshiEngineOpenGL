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
#ifndef PINTAR_TILES_COMANDO_H
#define PINTAR_TILES_COMANDO_H

#include <vector>

#include "../IComando.h"
#include "../../Objetos/Componentes/Juego2D/PincelTilemap.h"

class SceneRegistry;

// Un trazo de pintado de tiles (mouse abajo -> arriba) como una sola entrada
// del historial. El trazo ya se aplico mientras se pintaba: ejecutar() lo
// vuelve a aplicar (no cambia nada la primera vez) y deshacer() devuelve cada
// celda a su valor anterior. El objeto se busca por id, asi el comando no
// guarda punteros que un borrado podria invalidar.
class PintarTilesComando : public IComando {
public:
    PintarTilesComando(SceneRegistry* escena, int idObjeto,
                       std::vector<CambioTile> cambios);

    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;

private:
    class Tilemap* tilemap() const;

    SceneRegistry* escena_ = nullptr;
    int idObjeto_ = -1;
    std::vector<CambioTile> cambios_;
};

#endif
