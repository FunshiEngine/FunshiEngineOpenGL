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
#ifndef INTERFACECOMPONENT_H
#define INTERFACECOMPONENT_H

#include "Component.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

// Anclaje de la interfaz respecto al area de juego (las 9 posiciones: tres
// columnas por tres filas). El ancla fija contra que borde o esquina se mide
// `desplazamiento`; `tamano` (auto segun el CSS del HTML o fijo) y `z` (orden
// de apilado entre interfaces) completan la colocacion. El valor numerico es
// el que se serializa, asi que el orden de los enumeradores no debe cambiarse.
enum class AnclaInterfaz : std::uint8_t {
    ArribaIzquierda = 0,
    ArribaCentro,
    ArribaDerecha,
    MedioIzquierda,
    Centro,
    MedioDerecha,
    AbajoIzquierda,
    AbajoCentro,
    AbajoDerecha
};

// Interfaz de usuario del JUGADOR como Component: se adjunta a un GameObject y,
// al entrar en modo play, la escena muestra esa interfaz (HUD) delante de la
// camara principal. La interfaz es un archivo .html (que referencia su CSS y su
// JS por rutas relativas) del proyecto; el componente guarda su ruta y su
// colocacion. Una escena puede tener varias interfaces: se dibujan las activas
// ordenadas por `z`.
class InterfaceComponent : public Component {
private:
    std::string rutaHtml; // ruta al .html; vacio = ninguna
    bool activo = true;
    AnclaInterfaz ancla = AnclaInterfaz::Centro;
    glm::vec2 desplazamiento{0.0f, 0.0f};
    bool tamanoAuto = true;       // true: el tamano lo fija el CSS del HTML
    glm::vec2 tamano{0.0f, 0.0f}; // usado cuando tamanoAuto == false
    float z = 0.0f;               // orden de apilado entre interfaces

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    InterfaceComponent() = default;

    const std::string& getRutaHtml() const noexcept { return rutaHtml; }
    void setRutaHtml(const std::string& ruta) noexcept { rutaHtml = ruta; }

    bool estaActiva() const noexcept { return activo; }
    void setActivo(bool valor) noexcept { activo = valor; }

    AnclaInterfaz getAncla() const noexcept { return ancla; }
    void setAncla(AnclaInterfaz valor) noexcept { ancla = valor; }

    const glm::vec2& getDesplazamiento() const noexcept { return desplazamiento; }
    void setDesplazamiento(const glm::vec2& valor) noexcept {
        desplazamiento = valor;
    }

    bool usaTamanoAuto() const noexcept { return tamanoAuto; }
    void setTamanoAuto(bool valor) noexcept { tamanoAuto = valor; }

    const glm::vec2& getTamano() const noexcept { return tamano; }
    void setTamano(const glm::vec2& valor) noexcept { tamano = valor; }

    float getZ() const noexcept { return z; }
    void setZ(float valor) noexcept { z = valor; }
};

#endif // INTERFACECOMPONENT_H
