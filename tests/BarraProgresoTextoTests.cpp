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
// Pruebas headless del formato de la barra de progreso en texto de la ventana
// "Estado" (header-only, solo std). El acotado de `hecha` a `[0, total]` y el
// caso `total == 0` (sin division por cero ni longitud negativa convertida a
// size_t) son los que la barra no puede delegar al dibujo.

#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/GUI/Estado/BarraProgresoTexto.h"

namespace {

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

} // namespace

int main() {
    using namespace BarraProgresoTexto;

    // Caso sano: 1 de 2 llena la mitad.
    CHECK(porcentaje(1, 2) == 50, "porcentaje 1/2");
    CHECK(formatear(1, 2) == "#####     ", "barra 1/2 = 5 llenos");

    // Productor desincronizado: hecha > total se acota.
    CHECK(porcentaje(2, 1) == 100, "porcentaje 2/1 acotado a 100");
    CHECK(formatear(2, 1) == "##########", "barra 2/1 acotada");
    CHECK(formatear(5, 3) == "##########", "barra 5/3 acotada");

    // Sin total no hay proporcion: cero y cadena vacia.
    CHECK(porcentaje(0, 0) == 0, "porcentaje 0/0 = 0");
    CHECK(formatear(0, 0).empty(), "barra 0/0 vacia");

    // Total valido con hecha 0: barra de solo espacios.
    CHECK(formatear(0, 3) == "          ", "barra 0/3 todo espacios");

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
