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

#include <string>

// Interfaz de usuario del JUGADOR como Component: se adjunta a un GameObject
// y, al entrar en modo play, la escena muestra esa interfaz a pantalla
// completa delante de la camara principal (HUD del juego). Como los clips de
// sonido, la interfaz se referencia por NOMBRE: el nombre del asset JSON en
// Memory/Interfaces/<nombre>.json del proyecto.
class InterfaceComponent : public Component {
private:
    std::string interfaz; // nombre del asset de interfaz; vacio = ninguna

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    InterfaceComponent() = default;

    void saveComponent(std::ofstream* file) override;
    void loadComponent(std::ifstream* file) override;

    const std::string& getInterfaz() const noexcept { return interfaz; }
    void setInterfaz(const std::string& nombre) noexcept { interfaz = nombre; }
};

#endif // INTERFACECOMPONENT_H