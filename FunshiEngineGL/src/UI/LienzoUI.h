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
#ifndef LIENZOUI_H
#define LIENZOUI_H

#include <cstddef>
#include <string>
#include <vector>

#include "TiposUI.h"
#include "WidgetUI.h"

// Lienzo de la interfaz de juego nativa: la lista de widgets y la colocacion
// de cada uno en la pantalla. Es logica pura (sin OpenGL, glm ni ImGui): el
// componente la guarda y el renderer dibuja lo que sale de aqui.
//
// La colocacion se recalcula solo cuando hace falta: cambia el tamano de la
// pantalla o se toca la lista de widgets.
class LienzoUI {
public:
    // Coloca un widget con su anclaje, desplazamiento y tamano dentro de una
    // pantalla de `anchoPantalla` x `altoPantalla`. El anclaje decide que
    // punto de la pantalla se ancla el widget y hacia que lado crece desde ahi.
    static RectUI colocar(AnclaUI ancla, const Vec2UI& desplazamiento,
                           const Vec2UI& tamano, float anchoPantalla,
                           float altoPantalla);

    std::vector<WidgetUI>& widgets() noexcept { return widgets_; }
    const std::vector<WidgetUI>& widgets() const noexcept { return widgets_; }

    // Agrega un widget al final (encima de los demas) y devuelve su indice.
    std::size_t agregar(WidgetUI widget);
    // Quita el widget del indice dado. Devuelve false si el indice no existe.
    bool quitar(std::size_t indice);
    // Mueve el widget del indice a una posicion nueva (arriba o abajo en la
    // lista de apilado). Devuelve false si alguno de los indices no existe.
    bool mover(std::size_t desde, std::size_t hasta);
    // Reemplaza un widget por su indice (lo usa el inspector al editarlo).
    // Devuelve false si el indice no existe.
    bool actualizar(std::size_t indice, const WidgetUI& widget);

    // Indice del widget con ese identificador, o `kNoEncontrado` si no hay
    // ninguno.
    static constexpr std::size_t kNoEncontrado = static_cast<std::size_t>(-1);

    std::size_t indiceDe(const std::string& id) const;
    bool existe(const std::string& id) const {
        return indiceDe(id) != kNoEncontrado;
    }

    // Cambia el texto de un widget. Devuelve false si no hay widget con ese
    // identificador.
    bool establecerTexto(const std::string& id, const std::string& texto);
    // Muestra u oculta un widget. Devuelve false si no hay widget con ese
    // identificador.
    bool establecerVisible(const std::string& id, bool visible);
    // Texto de un widget. Si no existe, devuelve una referencia vacia.
    std::string textoDe(const std::string& id) const;
    // Si el widget existe y esta visible.
    bool visibleDe(const std::string& id) const;

    // Coloca los widgets para una pantalla de ese tamano y devuelve los
    // rectangulos resultantes (uno por widget, en el orden de la lista). El
    // resultado se reutiliza mientras ni la pantalla ni la lista cambien.
    const std::vector<RectUI>& layout(float anchoPantalla, float altoPantalla);

    // Rectangulos del ultimo layout calculado, sin recalcular nada. Es lo que
    // consulta el sistema de interaccion: el tamano de la pantalla lo decide
    // quien dibuja (la escena), no quien recibe el raton.
    const std::vector<RectUI>& rectangulos() const noexcept { return rectangulos_; }

    // Numero de revision: cambia con cada modificacion de la lista. Permite a
    // quien consume el lienzo saber si su layout sigue al dia.
    std::size_t revision() const noexcept { return revision_; }

private:
    std::vector<WidgetUI> widgets_;
    std::vector<RectUI> rectangulos_;
    // Revision de la lista con la que se calculo el layout actual.
    std::size_t revision_ = 0;
    std::size_t revisionLayout_ = 0;
    // Tamano de pantalla con el que se calculo el layout actual.
    float anchoLayout_ = -1.0f;
    float altoLayout_ = -1.0f;
    bool layoutValido_ = false;
};

#endif // LIENZOUI_H
