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

#include "TiposUI.h"

// Datos de un widget del HUD. Son datos planos: el motor los serializa tal
// cual y el renderer los convierte en primitivas de dibujo.
//
// El `id` es el nombre con el que un script se refiere al widget ("vida",
// "botonStart"): tiene que ser unico dentro del lienzo. El orden de la lista
// de widgets es el orden de apilado (el ultimo se dibuja encima).
struct WidgetUI {
    // Identificador del widget dentro del lienzo. Vacio = el widget no es
    // referenciable desde un script (sirve igual para lo visual).
    std::string id;
    TipoWidgetUI tipo = TipoWidgetUI::Etiqueta;
    // Anclaje del widget respecto a la pantalla.
    AnclaUI ancla = AnclaUI::ArribaIzquierda;
    // Separacion en pixeles respecto al punto de anclaje.
    Vec2UI desplazamiento;
    // Tamano del widget en pixeles.
    Vec2UI tamano{160.0f, 32.0f};
    // Color de fondo (panel y boton) o del texto (etiqueta).
    ColorUI color;
    // Color del texto (boton y etiqueta).
    ColorUI colorTexto{1.0f, 1.0f, 1.0f, 1.0f};
    // Texto del widget (etiqueta y boton).
    std::string texto;
    // Ruta de la textura del proyecto (imagen). Vacia = sin imagen.
    std::string rutaTextura;
    // Tamano del texto en puntos. La fuente se escala a este tamano.
    float tamanoTexto = 18.0f;
    // Si el widget se dibuja. Un widget invisible no se dibuja ni recibe el
    // raton, pero sigue en la lista (su estado se conserva).
    bool visible = true;
};

#endif // WIDGETUI_H
