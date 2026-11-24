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
#ifndef HERRAMIENTATILEMAP_H
#define HERRAMIENTATILEMAP_H

#include <cstdint>
#include <vector>

#include "../../Objetos/Componentes/Juego2D/PincelTilemap.h"

class CameraComponent;
class GameObject;
class GestorComandos;
class SceneRegistry;

// Pintado de tiles clickeando en la escena.
//
// Con "Pintar en escena" activo y un objeto con Tilemap seleccionado, el clic
// izquierdo sobre la escena pinta en la capa activa (Shift + clic borra). El
// rayo del mouse se lleva al espacio local del objeto y se corta con su plano
// z = 0, asi el pintado respeta la posicion, rotacion y escala del tilemap.
// Cada trazo (mouse abajo -> arriba) es un comando: Ctrl+Z lo deshace entero.
//
// El estado (herramienta, tile, capa) lo comparten el panel del inspector y la
// escena: vive en el EditorController, que ambos conocen.
class HerramientaTilemap {
public:
    enum class Modo { Pincel, Borrador, Relleno, Rectangulo };

    bool activa() const noexcept { return activa_; }
    void setActiva(bool valor) noexcept;
    Modo modo() const noexcept { return modo_; }
    void setModo(Modo valor) noexcept { modo_ = valor; }
    std::uint32_t tile() const noexcept { return tile_; }
    void setTile(std::uint32_t valor) noexcept { tile_ = valor; }
    int capa() const noexcept { return capa_; }
    void setCapa(int valor) noexcept { capa_ = valor < 0 ? 0 : valor; }

    // Un frame de la herramienta. Devuelve true si consumio el mouse: la
    // escena no debe hacer picking ni mover el gizmo con ese clic.
    bool procesar(CameraComponent* camara, GameObject* seleccionado,
                  SceneRegistry* escena, GestorComandos* comandos);

private:
    void terminarTrazo(GameObject* objeto, SceneRegistry* escena,
                       GestorComandos* comandos);

    bool activa_ = false;
    Modo modo_ = Modo::Pincel;
    std::uint32_t tile_ = 1;
    int capa_ = 0;

    bool trazando_ = false;
    int idObjetoTrazo_ = -1;
    std::vector<CambioTile> trazo_;
    CeldaTilemap inicioRect_;
};

#endif
