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
#ifndef SKYBOX_H
#define SKYBOX_H

#include "Component.h"
#include <string>

// Componente Skybox: gestiona un cubemap de 6 caras para el cielo de la escena.
// Se adjunta a un SimpleObject y, si visible=true, reemplaza el degradado
// del cielo (Cielo) por el cubemap. Sin limite de cantidad por escena (el
// primer Skybox visible gana, igual que Grid).
class Skybox : public Component {
private:
    // Rutas de las 6 caras del cubemap (pueden estar vacias si no se asignan).
    // Orden: +X (right), -X (left), +Y (top), -Y (bottom), +Z (front), -Z (back).
    std::string caraMasX;
    std::string caraMenosX;
    std::string caraMasY;
    std::string caraMenosY;
    std::string caraMasZ;
    std::string caraMenosZ;

    // Si el cubemap esta activo y debe renderizarse.
    bool visible = true;

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Skybox() = default;

    void saveComponent(std::ofstream* file) override { serializeComponent(file); }
    void loadComponent(std::ifstream* file) override { deserializeComponent(file); }

    // Acceso a las caras
    const std::string& getCaraMasX() const { return caraMasX; }
    void setCaraMasX(const std::string& s) { caraMasX = s; }

    const std::string& getCaraMenosX() const { return caraMenosX; }
    void setCaraMenosX(const std::string& s) { caraMenosX = s; }

    const std::string& getCaraMasY() const { return caraMasY; }
    void setCaraMasY(const std::string& s) { caraMasY = s; }

    const std::string& getCaraMenosY() const { return caraMenosY; }
    void setCaraMenosY(const std::string& s) { caraMenosY = s; }

    const std::string& getCaraMasZ() const { return caraMasZ; }
    void setCaraMasZ(const std::string& s) { caraMasZ = s; }

    const std::string& getCaraMenosZ() const { return caraMenosZ; }
    void setCaraMenosZ(const std::string& s) { caraMenosZ = s; }

    bool getVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }
};

#endif // SKYBOX_H