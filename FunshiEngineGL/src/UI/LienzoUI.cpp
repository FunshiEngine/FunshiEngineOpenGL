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
#include "LienzoUI.h"

namespace {

// Punto de la pantalla al que se ancla cada posicion y hacia que lado crece el
// widget desde ahi (0 = la esquina superior izquierda del widget queda en el
// punto; 0.5 = el widget queda centrado en el punto; 1 = la esquina
// inferior derecha del widget queda en el punto).
struct AnclajeUI {
    float fraccionX;
    float fraccionY;
};

AnclajeUI anclajeDe(AnclaUI ancla) {
    switch (ancla) {
    case AnclaUI::ArribaIzquierda:
        return {0.0f, 0.0f};
    case AnclaUI::ArribaCentro:
        return {0.5f, 0.0f};
    case AnclaUI::ArribaDerecha:
        return {1.0f, 0.0f};
    case AnclaUI::MedioIzquierda:
        return {0.0f, 0.5f};
    case AnclaUI::Centro:
        return {0.5f, 0.5f};
    case AnclaUI::MedioDerecha:
        return {1.0f, 0.5f};
    case AnclaUI::AbajoIzquierda:
        return {0.0f, 1.0f};
    case AnclaUI::AbajoCentro:
        return {0.5f, 1.0f};
    case AnclaUI::AbajoDerecha:
        return {1.0f, 1.0f};
    default:
        return {0.0f, 0.0f};
    }
}

} // namespace

RectUI LienzoUI::colocar(AnclaUI ancla, const Vec2UI& desplazamiento,
                          const Vec2UI& tamano, float anchoPantalla,
                          float altoPantalla) {
    const AnclajeUI anclaje = anclajeDe(ancla);
    RectUI rect;
    rect.ancho = tamano.x;
    rect.alto = tamano.y;
    // El punto de anclaje es la fraccion correspondiente de la pantalla.
    const float puntoX = anchoPantalla * anclaje.fraccionX;
    const float puntoY = altoPantalla * anclaje.fraccionY;
    // El widget se alinea respecto a ese punto segun la fraccion del anclaje.
    rect.x = puntoX + desplazamiento.x - anclaje.fraccionX * tamano.x;
    rect.y = puntoY + desplazamiento.y - anclaje.fraccionY * tamano.y;
    return rect;
}

std::size_t LienzoUI::agregar(WidgetUI widget) {
    widgets_.push_back(std::move(widget));
    ++revision_;
    return widgets_.size() - 1;
}

bool LienzoUI::quitar(std::size_t indice) {
    if (indice >= widgets_.size()) return false;
    widgets_.erase(widgets_.begin() + static_cast<std::ptrdiff_t>(indice));
    ++revision_;
    return true;
}

bool LienzoUI::mover(std::size_t desde, std::size_t hasta) {
    if (desde >= widgets_.size() || hasta >= widgets_.size()) return false;
    if (desde == hasta) return true;
    WidgetUI widget = std::move(widgets_[desde]);
    widgets_.erase(widgets_.begin() + static_cast<std::ptrdiff_t>(desde));
    widgets_.insert(widgets_.begin() + static_cast<std::ptrdiff_t>(hasta),
                    std::move(widget));
    ++revision_;
    return true;
}

bool LienzoUI::actualizar(std::size_t indice, const WidgetUI& widget) {
    if (indice >= widgets_.size()) return false;
    widgets_[indice] = widget;
    ++revision_;
    return true;
}

std::size_t LienzoUI::indiceDe(const std::string& id) const {
    if (id.empty()) return kNoEncontrado;
    for (std::size_t i = 0; i < widgets_.size(); ++i) {
        if (widgets_[i].id == id) return i;
    }
    return kNoEncontrado;
}

bool LienzoUI::establecerTexto(const std::string& id,
                                const std::string& texto) {
    const std::size_t indice = indiceDe(id);
    if (indice == kNoEncontrado) return false;
    widgets_[indice].texto = texto;
    ++revision_;
    return true;
}

bool LienzoUI::establecerVisible(const std::string& id, bool visible) {
    const std::size_t indice = indiceDe(id);
    if (indice == kNoEncontrado) return false;
    widgets_[indice].visible = visible;
    ++revision_;
    return true;
}

std::string LienzoUI::textoDe(const std::string& id) const {
    const std::size_t indice = indiceDe(id);
    if (indice == kNoEncontrado) return std::string();
    return widgets_[indice].texto;
}

bool LienzoUI::visibleDe(const std::string& id) const {
    const std::size_t indice = indiceDe(id);
    if (indice == kNoEncontrado) return false;
    return widgets_[indice].visible;
}

const std::vector<RectUI>& LienzoUI::layout(float anchoPantalla,
                                            float altoPantalla) {
    // El layout se recalcula cuando cambia el tamano de la pantalla o la lista
    // de widgets (revision). Si no, se reutiliza el que ya hay.
    if (layoutValido_ && revision_ == revisionLayout_ &&
        anchoLayout_ == anchoPantalla && altoLayout_ == altoPantalla) {
        return rectangulos_;
    }

    rectangulos_.clear();
    rectangulos_.reserve(widgets_.size());
    for (const WidgetUI& widget : widgets_) {
        rectangulos_.push_back(colocar(widget.ancla, widget.desplazamiento,
                                       widget.tamano, anchoPantalla,
                                       altoPantalla));
    }
    anchoLayout_ = anchoPantalla;
    altoLayout_ = altoPantalla;
    revisionLayout_ = revision_;
    layoutValido_ = true;
    return rectangulos_;
}
