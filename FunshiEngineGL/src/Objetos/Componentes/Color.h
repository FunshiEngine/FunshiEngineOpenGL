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
#ifndef COLOR_H
#define COLOR_H
#include "Component.h"

typedef float color[4];

class Color : public Component {
private:
    float f1 = 0.0f;
    float f2 = 0.0f;
    float f3 = 0.0f;
    color range = { 0.0f, 0.0f, 0.0f, 1.0f };

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Color();

    void saveComponent(std::ofstream* file) override;
    void loadComponent(std::ifstream* file) override;

    void setColor(const color cor);
    void setColor(float r, float g, float b);

    // Post-carga: refleja el color leido del binario en el auxColor del dueno
    // (el buffer que usa el inspector). Reemplaza el dispatch por nombre.
    void onLoaded(GameObject& owner) override;

    // Devuelve directamente el puntero interno al arreglo RGBA
    const float* getColor() const {
        return range;
    }
};

// Constantes de color globales (una copia por TU, como historico).
static color rojo = { 2.0,0.0,0.0 };
static color verde = { 0.0,1.0,0.0 };
static color aguaMarina = { 0.0,1.0,1.0 };
static color celeste = { 0.0,0.0,1.0 };
static color vermelho = { 0.85, 0.12, 0.0 };
static color azul = { 0.0, 0.15,0.35 };
static color preto = { 0.0, 0.0, 0.0 };
static color branco = { 1.0, 1.0, 1.0 };
static color branco_gelo = { 0.88,0.91,0.89 };
static color amarelo = { 1.0, 1.0, 0.0 };
static color violeta = { 0.54, 0.17, 0.88 };
static color cinza = { 0.8, 0.8, 0.8 };
static color cinza_escuro = { 0.67,0.67,0.67 };
static color laranja = { 1.0, 0.6, 0.2 };
#endif