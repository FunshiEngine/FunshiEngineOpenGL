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
#ifndef WIDGETUI_H
#define WIDGETUI_H

#include <string>

// Un control visual de una interfaz de usuario creada en el editor. El "sonido"
// es el NOMBRE de un clip del proyecto (AudioEngine reusa el mismo catalogo que
// los AudioSource): al activar el control, el CanvasInterface lo reproduce. El
// nombre "(ninguno)" significa sin sonido.
enum class TipoWidget {
    Etiqueta,
    Boton,
    Checkbox,
    Slider,
    EntradaTexto
};

struct WidgetUI {
    TipoWidget tipo = TipoWidget::Etiqueta;
    std::string nombre;    // id unico dentro de la interfaz
    std::string etiqueta;  // texto visible
    std::string sonido;    // nombre de clip en Sonidos/ del proyecto
    float valor = 0.0f;    // slider/entrada numerica
    float minimo = 0.0f;
    float maximo = 1.0f;
    bool activado = false; // checkbox
    std::string texto;     // EntradaTexto
};

// Nombre reservado para un widget sin sonido asignado.
inline const char* kSinSonido = "(ninguno)";

#endif // WIDGETUI_H