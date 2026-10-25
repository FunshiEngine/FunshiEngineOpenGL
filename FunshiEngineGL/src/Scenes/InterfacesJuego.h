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
#ifndef INTERFACES_JUEGO_H
#define INTERFACES_JUEGO_H

#include <vector>

#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

class GameObject;
class InterfaceComponent;
class UIComponent;

// Recoleccion de las interfaces de juego (HUD) de una escena. Se extrae de
// GameScene para poder probarla headless: es logica pura sobre la lista de
// objetos, sin dependencia de GL ni de la GUI del editor.
namespace InterfacesJuego {

// Devuelve las interfaces referenciadas por los objetos de la escena, en el
// orden en que aparecen en la lista. Recorre TODOS los objetos: una escena
// puede tener mas de una interfaz y ninguna debe quedar oculta por estar
// despues de otra. Las interfaces sin referencia (nombre vacio) se omiten.
std::vector<InterfaceComponent*> recolectarDeEscena(
    ListaDE<GameObject*>* objetos);

// Devuelve las interfaces NATIVAS activas de la escena (las que dibuja el
// motor, sin navegador web), ordenadas por z igual que las web.
std::vector<UIComponent*> recolectarUINativas(ListaDE<GameObject*>* objetos);

} // namespace InterfacesJuego

#endif // INTERFACES_JUEGO_H
