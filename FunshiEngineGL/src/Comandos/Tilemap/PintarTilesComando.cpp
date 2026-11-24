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
#include "PintarTilesComando.h"

#include <string>

#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Juego2D/Tilemap.h"
#include "../../Scenes/Registro/SceneRegistry.h"

PintarTilesComando::PintarTilesComando(SceneRegistry* escena, int idObjeto,
                                       std::vector<CambioTile> cambios)
    : escena_(escena), idObjeto_(idObjeto), cambios_(std::move(cambios)) {}

Tilemap* PintarTilesComando::tilemap() const {
    GameObject* objeto = escena_ ? escena_->getObjectByID(idObjeto_) : nullptr;
    return objeto ? objeto->getComponent<Tilemap>() : nullptr;
}

void PintarTilesComando::ejecutar() {
    if (Tilemap* t = tilemap()) PincelTilemap::rehacer(*t, cambios_);
}

void PintarTilesComando::deshacer() {
    if (Tilemap* t = tilemap()) PincelTilemap::deshacer(*t, cambios_);
}

std::string PintarTilesComando::descripcion() const {
    return "Pintar tiles (" + std::to_string(cambios_.size()) + " celdas)";
}
