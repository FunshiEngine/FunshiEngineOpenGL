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
#include "SistemaUI.h"

void SistemaUI::actualizar(const LienzoUI& lienzo, const EntradaUI& entrada) {
    const std::vector<WidgetUI>& widgets = lienzo.widgets();
    // El layout lo calcula quien sabe el tamano de la pantalla (la escena);
    // aqui solo se usa el que ya esta.
    const std::vector<RectUI>& rectangulos = lienzo.rectangulos();

    // El estado se indexa igual que la lista de widgets: si el lienzo cambio
    // (widgets anadidos o quitados), el vector se ajusta.
    estados_.resize(widgets.size());
    ids_.resize(widgets.size());
    for (std::size_t i = 0; i < widgets.size(); ++i) {
        ids_[i] = widgets[i].id;
    }

    // El frame anterior no cuenta: el flanco de pulsacion solo vale para el
    // frame en el que ocurre.
    for (Estado& estado : estados_) {
        estado.sobre = false;
        estado.pulsado = false;
        estado.pulsaEsteFrame = false;
    }
    interactivoSobre_ = false;
    idInteractivoSobre_.clear();

    if (rectangulos.size() != widgets.size()) return;

    // Se busca de arriba hacia abajo (el ultimo de la lista es el que esta
    // encima): el primero interactivo que contiene el puntero gana.
    std::size_t indiceObjetivo = LienzoUI::kNoEncontrado;
    for (std::size_t i = widgets.size(); i-- > 0;) {
        const WidgetUI& widget = widgets[i];
        if (!widget.visible) continue;
        if (!widgetEsInteractivo(widget.tipo)) continue;
        const RectUI& rect = rectangulos[i];
        if (!rect.contiene(entrada.posicion.x, entrada.posicion.y)) continue;
        indiceObjetivo = i;
        break;
    }

    if (indiceObjetivo == LienzoUI::kNoEncontrado) return;

    Estado& estado = estados_[indiceObjetivo];
    estado.sobre = true;
    estado.pulsado = entrada.botonIzquierdo;
    estado.pulsaEsteFrame = entrada.clicIzquierdo;
    interactivoSobre_ = true;
    idInteractivoSobre_ = widgets[indiceObjetivo].id;
}

std::size_t SistemaUI::indiceDeEstado(const std::string& id) const {
    if (id.empty()) return LienzoUI::kNoEncontrado;
    for (std::size_t i = 0; i < estados_.size(); ++i) {
        // El identificador se busca en el estado: el sistema guarda el
        // estado por posicion de la lista, asi que hace falta el id del
        // widget. Se resuelve con la lista que se recibio en el ultimo
        // actualizar; si no se conoce, no hay forma de saberlo.
        if (ids_[i] == id) return i;
    }
    return LienzoUI::kNoEncontrado;
}

bool SistemaUI::estaSobre(const std::string& id) const {
    const std::size_t indice = indiceDeEstado(id);
    if (indice == LienzoUI::kNoEncontrado) return false;
    return estados_[indice].sobre;
}

bool SistemaUI::sobreIndice(std::size_t indice) const {
    if (indice >= estados_.size()) return false;
    return estados_[indice].sobre;
}

bool SistemaUI::estaPulsado(const std::string& id) const {
    const std::size_t indice = indiceDeEstado(id);
    if (indice == LienzoUI::kNoEncontrado) return false;
    return estados_[indice].pulsado;
}

bool SistemaUI::pulsadoIndice(std::size_t indice) const {
    if (indice >= estados_.size()) return false;
    return estados_[indice].pulsado;
}

bool SistemaUI::fuePulsado(const std::string& id) const {
    const std::size_t indice = indiceDeEstado(id);
    if (indice == LienzoUI::kNoEncontrado) return false;
    return estados_[indice].pulsaEsteFrame;
}

void SistemaUI::limpiarClics() {
    for (Estado& estado : estados_) {
        estado.pulsaEsteFrame = false;
    }
}
