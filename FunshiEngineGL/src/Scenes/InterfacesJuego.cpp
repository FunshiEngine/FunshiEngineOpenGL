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
#include "InterfacesJuego.h"

#include <algorithm>

#include "../Objetos/Componentes/InterfaceComponent.h"
#include "../Objetos/Componentes/UIComponent.h"
#include "../Objetos/GameObject.h"

namespace InterfacesJuego {

std::vector<InterfaceComponent*> recolectarDeEscena(
    ListaDE<GameObject*>* objetos) {
    std::vector<InterfaceComponent*> interfaces;
    if (!objetos || objetos->isEmpty()) return interfaces;

    Position<GameObject*>* posicion = objetos->first();
    while (posicion != nullptr) {
        GameObject* objeto = posicion->getElement();
        if (objeto != nullptr) {
            if (auto* interfaz = objeto->getComponent<InterfaceComponent>()) {
                if (interfaz->estaActiva() &&
                    !interfaz->getRutaHtml().empty()) {
                    interfaces.push_back(interfaz);
                }
            }
        }
        // next() lanza si la posicion es la ultima: se corta antes de pedirlo.
        posicion = (posicion != objetos->last()) ? objetos->next(posicion)
                                                 : nullptr;
    }

    // Orden de dibujo por z ascendente (el de mayor z queda encima). El orden de
    // escena se conserva entre interfaces con la misma z.
    std::stable_sort(interfaces.begin(), interfaces.end(),
                     [](const InterfaceComponent* a,
                        const InterfaceComponent* b) {
                         return a->getZ() < b->getZ();
                     });
    return interfaces;
}

std::vector<UIComponent*> recolectarUINativas(ListaDE<GameObject*>* objetos) {
    std::vector<UIComponent*> interfaces;
    if (!objetos || objetos->isEmpty()) return interfaces;

    Position<GameObject*>* posicion = objetos->first();
    while (posicion != nullptr) {
        GameObject* objeto = posicion->getElement();
        if (objeto != nullptr) {
            if (auto* interfaz = objeto->getComponent<UIComponent>()) {
                if (interfaz->estaActivo()) interfaces.push_back(interfaz);
            }
        }
        // next() lanza si la posicion es la ultima: se corta antes de pedirlo.
        posicion = (posicion != objetos->last()) ? objetos->next(posicion)
                                                 : nullptr;
    }

    // Orden de dibujo por z ascendente (el de mayor z queda encima). El orden de
    // escena se conserva entre interfaces con la misma z.
    std::stable_sort(interfaces.begin(), interfaces.end(),
                     [](const UIComponent* a, const UIComponent* b) {
                         return a->getZ() < b->getZ();
                     });
    return interfaces;
}

} // namespace InterfacesJuego
