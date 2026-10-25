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
#ifndef OBJETOS_ANIMADOR2D_H
#define OBJETOS_ANIMADOR2D_H

#include "Component.h"

// Animador de una planilla de sprites: divide la textura del SpriteRenderer
// hermano en una grilla columnas x filas y recorre los fotogramas a fps fijos,
// escribiendo la region UV del sprite en cada uno. El componente no dibuja: solo
// mueve el estado (uvRect) que el SpriteRenderer ya sabe exponer al renderer.
//
// El fotograma actual y el acumulador de tiempo son estado de ejecucion y no se
// serializan: al cargar, la animacion arranca en el primer fotograma.
class Animador2D : public Component {
public:
    Animador2D();

    int getColumnas() const { return columnas_; }
    void setColumnas(int columnas) { columnas_ = columnas; }

    int getFilas() const { return filas_; }
    void setFilas(int filas) { filas_ = filas; }

    // Rango de fotogramas dentro de la grilla (indices 0..columnas*filas-1).
    // ultimoFrame < 0 significa "hasta el ultimo fotograma de la grilla".
    int getPrimerFrame() const { return primerFrame_; }
    void setPrimerFrame(int frame) { primerFrame_ = frame; }
    int getUltimoFrame() const { return ultimoFrame_; }
    void setUltimoFrame(int frame) { ultimoFrame_ = frame; }

    float getFps() const { return fps_; }
    void setFps(float fps) { fps_ = fps; }

    bool getBucle() const { return bucle_; }
    void setBucle(bool bucle) { bucle_ = bucle; }

    bool getReproduciendo() const { return reproduciendo_; }
    void setReproduciendo(bool reproduciendo) {
        reproduciendo_ = reproduciendo;
    }

    // Fotograma vigente y total de la grilla, para el inspector.
    int getFrameActual() const { return frameActual_; }
    int getTotalFrames() const { return columnas_ * filas_; }

    void reiniciar() {
        frameActual_ = primerFrame_ < 0 ? 0 : primerFrame_;
        acumulador_ = 0.0f;
    }

    void onUpdate(GameObject& owner, float deltaTime) override;

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
private:
    // Ultimo fotograma valido de la grilla (ultimoFrame_ < 0 = el ultimo).
    int finEfectivo() const;

    int columnas_ = 1;
    int filas_ = 1;
    int primerFrame_ = 0;
    int ultimoFrame_ = -1;
    float fps_ = 8.0f;
    bool bucle_ = true;
    bool reproduciendo_ = true;

    // Estado de ejecucion (no serializado).
    int frameActual_ = 0;
    float acumulador_ = 0.0f;
};

#endif
