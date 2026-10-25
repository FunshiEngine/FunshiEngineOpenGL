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
#ifndef SISTEMAUI_H
#define SISTEMAUI_H

#include <cstddef>
#include <string>
#include <vector>

#include "EntradaUI.h"
#include "LienzoUI.h"

// Estado de interaccion de los widgets de un lienzo: que widget tiene el
// raton encima, cual esta pulsado y cual se pulso en el frame actual.
//
// Es logica pura (sin OpenGL, glm ni ImGui): el motor le pasa la posicion del
// raton y el lienzo ya colocado, y el sistema responde. El estado de un frame
// (el flanco de pulsacion) se reinicia en cada llamada a `actualizar`.
class SistemaUI {
public:
    // Actualiza el estado de interaccion. `lienzo` debe tener su layout ya
    // calculado (llamar a `LienzoUI::layout` antes).
    void actualizar(const LienzoUI& lienzo, const EntradaUI& entrada);

    // Si el raton esta encima del widget con ese identificador.
    bool estaSobre(const std::string& id) const;
    // Si el raton esta encima del widget que ocupa ese indice de la lista.
    bool sobreIndice(std::size_t indice) const;
    // Si el boton izquierdo se mantiene pulsado sobre ese widget.
    bool estaPulsado(const std::string& id) const;
    // Si el boton izquierdo se mantiene pulsado sobre el widget de ese indice.
    bool pulsadoIndice(std::size_t indice) const;
    // Si ese widget recibio el clic en el frame actual (flanco de pulsacion).
    bool fuePulsado(const std::string& id) const;

    // Si el raton esta encima de algun widget interactivo. Sirve para que el
    // juego no siga girando la camara con el raton mientras esta sobre un
    // boton del HUD.
    bool hayInteractivoSobre() const { return interactivoSobre_; }

    // Identificador del widget interactivo que tiene el raton encima (vacio
    // si no hay ninguno).
    const std::string& interactivoSobre() const { return idInteractivoSobre_; }

    // Borra los flancos de pulsacion (los "pulsado en este frame").
    void limpiarClics();

private:
    // Estado de un widget: el indice en el vector es el del widget en la
    // lista del lienzo.
    struct Estado {
        bool sobre = false;
        bool pulsado = false;
        bool pulsaEsteFrame = false;
    };

    // Indice del widget con ese identificador dentro de la lista, o
    // `kNoEncontrado` si no hay ninguno.
    std::size_t indiceDeEstado(const std::string& id) const;

    std::vector<Estado> estados_;
    // Identificador de cada widget, en el mismo orden que los estados: es lo
    // que permite preguntar por un widget concreto ("vida", "botonStart") sin
    // que quien consulta conozca la posicion en la lista.
    std::vector<std::string> ids_;
    std::string idInteractivoSobre_;
    bool interactivoSobre_ = false;
};

#endif // SISTEMAUI_H
