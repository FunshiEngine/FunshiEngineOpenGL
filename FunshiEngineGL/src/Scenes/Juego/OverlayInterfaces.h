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
#ifndef OVERLAYINTERFACES_H
#define OVERLAYINTERFACES_H

#include <vector>

#include "../../WebUI/ColocacionInterfaz.h"
#include "../../WebUI/VistasWebUI.h"

class EstadoInterfaces;
class GameObject;
class InterfaceComponent;
template <typename T>
class ListaDE;

// Interfaces web (HTML) de la escena sobre la pantalla del juego y su vista
// previa en el editor.
//
// HUD y vista previa usan conjuntos de vistas SEPARADOS: VistasWebUI descarga
// toda vista que no este en la lista que recibe, asi que compartir un conjunto
// hacia que cada uno descargara y recargara las vistas del otro en cada frame.
//
// El mouse se reenvia a la vista de mayor z que esta debajo del cursor (en
// coordenadas locales de la vista) y esa vista toma el foco del teclado. Las
// vistas sin tamano fijo ocupan toda la pantalla.
class OverlayInterfaces {
public:
    // HUD: recolecta las interfaces activas, las sincroniza con el estado que
    // dejan los scripts, las coloca segun su ancla, las dibuja y les pasa el
    // input. 'recibeInput' en false (por ejemplo, con el mouse sobre una
    // ventana del editor) solo dibuja.
    void pintarHud(ListaDE<GameObject*>* objetos, EstadoInterfaces* estado,
                   float deltaTime, bool recibeInput);

    // Vista previa de la interfaz del objeto seleccionado en la ventana
    // dockable 'nombreVentana'. 'estado' puede ser nulo (sin intercambio).
    void pintarVistaPrevia(GameObject* seleccionado, EstadoInterfaces* estado,
                           float deltaTime, const char* nombreVentana);

    // Tecla GLFW (action: 0 suelta, 1 presiona, 2 repite) para la vista con foco.
    void tecla(int codigo, int action);

    // Descarga solo las vistas del HUD (al salir del modo juego en el editor).
    void descargarHud();

    // Descarga todo (cambio de escena, cierre).
    void vaciar();

private:
    void enrutarRaton(const std::vector<InterfaceComponent*>& interfaces);
    void soltarFoco();

    VistasWebUI hud_;
    VistasWebUI vistaPrevia_;
    std::vector<RectInterfaz> rects_;
    InterfaceComponent* foco_ = nullptr;
    VistaWebUI* vistaFoco_ = nullptr;
    bool previaConFoco_ = false;
};

#endif
