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
#ifndef FONDOESCENA_H
#define FONDOESCENA_H

#include "../Configuracion/Apariencia.h"
#include "Backend/IRenderBackend.h"

// Fondo del frame de la escena: el color con el que se limpia el framebuffer
// de la ventana y el estado base del pipeline.
//
// Vive en la capa de render porque es lo que el backend necesita, y lo usan
// los dos puntos de entrada del motor (el editor y el juego exportado) para
// que los dos frames empiecen igual.
namespace FondoEscena {

// Color de fondo del viewport 3D segun el perfil de apariencia.
inline void color(const Apariencia& apariencia, float out[3]) {
    AparienciaUtil::fondoEfectivo(apariencia, out);
}

// Limpia el framebuffer de la ventana (color y profundidad) con el color de
// fondo del perfil.
//
// La limpieza no es opcional: el test de profundidad compara contra el valor
// que hay en el buffer, y ese valor inicial no es 1.0 en todos los drivers.
// Sin limpiar, el cielo y la geometria se descartan y la pantalla queda en
// negro.
inline void limpiar(Rendering::Backend::IRenderBackend& backend,
                      const Apariencia& apariencia) {
    float fondo[3];
    color(apariencia, fondo);
    backend.setClearColor(fondo);
    backend.clearScreen(nullptr);
}

} // namespace FondoEscena

#endif // FONDOESCENA_H
