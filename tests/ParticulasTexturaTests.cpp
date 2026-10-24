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
// Pruebas headless del campo texturaParticula de EmisorParticulas:
//
//   - setTexturaParticula / texturaParticula guardan y devuelven la ruta.
//   - La serializacion binaria conserva la ruta (guardar y cargar).
//   - La ruta se guarda relativa a la raiz de assets (stub: pasa a traves).
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/Objetos/Componentes/EmisorParticulas.h"
#include "TempPruebas.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

void testGetterSetter() {
    EmisorParticulas emisor;
    CHECK(emisor.texturaParticula().empty(),
          "la textura empieza vacia");

    emisor.setTexturaParticula("particulas/humo.png");
    CHECK(emisor.texturaParticula() == "particulas/humo.png",
          "setTexturaParticula guarda la ruta");

    emisor.setTexturaParticula(std::string());
    CHECK(emisor.texturaParticula().empty(),
          "setTexturaParticula vacia limpia la ruta");
}

void testSerializacion() {
    TempPruebas::CarpetaPrueba carpeta("funshi_particulas_textura_tests");
    const auto ruta = carpeta.ruta() / "emisor.bin";

    EmisorParticulas original;
    original.setTexturaParticula("particulas/estrella.png");
    original.setActiva(false);

    {
        std::ofstream salida(ruta, std::ios::binary);
        CHECK(salida.is_open(), "se abre el archivo para escribir");
        original.saveComponent(&salida);
    }

    EmisorParticulas leida;
    {
        std::ifstream entrada(ruta, std::ios::binary);
        CHECK(entrada.is_open(), "se abre el archivo para leer");
        leida.loadComponent(&entrada);
    }

    CHECK(leida.texturaParticula() == "particulas/estrella.png",
          "la textura de particula se conserva tras serializar");
    CHECK(!leida.activa(), "el estado activo se conserva tras serializar");
}

}  // namespace

int main() {
    std::cout << "== PARTICULAS TEXTURA TESTS ==" << std::endl;
    testGetterSetter();
    testSerializacion();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "PARTICULAS TEXTURA TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
