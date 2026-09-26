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
#ifndef GRID_H
#define GRID_H

#include "Component.h"

// Grilla del suelo de la escena. Es un componente mas: vive en el Transform
// de su objeto (por eso se puede mover/escalar/rotar como cualquier otro) y
// expone visible/color que la GUI edita en sus settings. Desde el rediseno,
// la grilla es INFINITA con densidad FIJA de lineas (secundarias cada 1
// unidad, principales cada 5) y difuminado en el horizonte: los campos
// tam/separacion ya NO se usan para dibujar y solo se conservan para que las
// escenas viejas serializadas sigan leyendose con el mismo layout.
class Grid : public Component {
private:
    bool visible = true;
    float color[3] = {0.88f, 0.91f, 0.89f}; // branco_gelo historico
    float tam = 70.0f;
    float separacion = 1.0f;

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Grid() = default;

    void saveComponent(std::ofstream* file) override;
    void loadComponent(std::ifstream* file) override;

    bool getVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }

    // Puntero interno RGB (lo consume GrillaRenderer / apariencia).
    const float* getColor() const { return color; }
    void setColor(float r, float g, float b);

    float getTam() const { return tam; }
    void setTam(float value) { tam = value; }

    float getSeparacion() const { return separacion; }
    void setSeparacion(float value) { separacion = value; }
};

#endif