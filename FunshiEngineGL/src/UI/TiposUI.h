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
#ifndef TIPOSUI_H
#define TIPOSUI_H

#include <cstdint>

// Tipos de la interfaz de juego nativa (el HUD que dibuja el motor, sin
// navegador web). El modulo es CPU puro: no usa OpenGL, ni glm, ni ImGui, para
// que se pueda probar entero en los targets headless.

// Tipo de widget del HUD. El valor numerico es el que se serializa, asi que el
// orden de los enumeradores no debe cambiarse.
enum class TipoWidgetUI : std::uint8_t {
    // Rectangulo relleno: sirve de fondo o de marco de otros widgets.
    Panel = 0,
    // Texto sin fondo.
    Etiqueta = 1,
    // Textura del proyecto, estirada al tamano del widget.
    Imagen = 2,
    // Rectangulo con texto y la unica clase interactiva: el motor le entrega
    // el clic a los scripts.
    Boton = 3
};

// Anclaje del widget respecto a la pantalla: las nueve posiciones (tres
// columnas por tres filas). El anclaje dice QUE PUNTO de la pantalla se ancla
// el widget; el desplazamiento lo separa de ahi y el tamano decide hacia donde
// crece desde ese punto. El valor numerico es el que se serializa.
enum class AnclaUI : std::uint8_t {
    ArribaIzquierda = 0,
    ArribaCentro = 1,
    ArribaDerecha = 2,
    MedioIzquierda = 3,
    Centro = 4,
    MedioDerecha = 5,
    AbajoIzquierda = 6,
    AbajoCentro = 7,
    AbajoDerecha = 8
};

// Cantidad de anclajes validos: el deserializador acota con este valor para no
// aceptar un enum corrupto.
constexpr int kTotalAnclasUI = 9;

// Vector de dos componentes en pixeles. Estructura propia (no glm) para que el
// modulo no dependa de las cabeceras de matematicas y se pueda compilar en el
// build sin motor.
struct Vec2UI {
    float x = 0.0f;
    float y = 0.0f;
};

// Color RGBA con componentes de 0 a 1.
struct ColorUI {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

// Rectangulo en pixeles de pantalla: esquina superior izquierda y tamano.
struct RectUI {
    float x = 0.0f;
    float y = 0.0f;
    float ancho = 0.0f;
    float alto = 0.0f;

    bool contiene(float px, float py) const noexcept {
        return px >= x && px <= x + ancho && py >= y && py <= y + alto;
    }
};

// True si el widget es interactivo (el motor le entrega el clic).
inline bool widgetEsInteractivo(TipoWidgetUI tipo) noexcept {
    return tipo == TipoWidgetUI::Boton;
}

#endif // TIPOSUI_H
