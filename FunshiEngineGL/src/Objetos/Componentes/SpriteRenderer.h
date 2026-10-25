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
#ifndef OBJETOS_SPRITERENDERER_H
#define OBJETOS_SPRITERENDERER_H

#include <string>

#include "Component.h"

// Componente de un sprite 2D: un quad texturizado centrado en el objeto. El
// dibujado lo hace Rendering::SpriteRenderer (una pasada de sprites del
// SceneRenderer), no este componente: aca solo vive el estado editable (ruta de
// textura, tinte, tamano y region de la textura) que el inspector manipula y la
// serializacion persiste.
//
// La posicion/rotacion/escala salen del Transform global del dueno; el tamano en
// unidades de mundo es un multiplicador sobre la escala del Transform. El orden
// en pantalla lo decide la componente Z del Transform (el sprite no tiene
// volumen propio).
class SpriteRenderer : public Component {
public:
    SpriteRenderer();

    void setRutaTextura(const std::string& ruta);
    const std::string& getRutaTextura() const { return rutaTextura_; }

    // RGBA del tinte. El alpha multiplica el de la textura (fade del sprite).
    const float* getTint() const { return tint_; }
    void setTint(float r, float g, float b, float a);

    // Ancho/alto del sprite en unidades de mundo (el quad base es 1x1).
    const float* getTamano() const { return tamano_; }
    void setTamano(float ancho, float alto);

    // Region de la textura (u0, v0, u1, v1). Default (0,0,1,1). Habilita el uso
    // de spritesheets sin duplicar geometria.
    const float* getUvRect() const { return uvRect_; }
    void setUvRect(float u0, float v0, float u1, float v1);

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
private:
    std::string rutaTextura_;
    float tint_[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float tamano_[2] = {1.0f, 1.0f};
    float uvRect_[4] = {0.0f, 0.0f, 1.0f, 1.0f};
};

#endif
