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
// Pruebas headless de la serializacion binaria del InterfaceComponent: la ruta
// del .html mas su colocacion (activo, ancla, desplazamiento, tamaño y z).
//
// Cubre dos cosas que el formato comparte con el resto de la escena:
//   - el round-trip del formato nuevo (todos los campos vuelven iguales),
//   - la convivencia con el formato viejo, que guardaba solo el nombre del
//     asset como size_t longitud + bytes. El payload viejo NO lleva la marca
//     del formato nuevo; el lector debe reconocerlo, cargar el nombre como
//     ruta y, sobre todo, NO consumir bytes de mas: si se pasa de largo, los
//     componentes siguientes del objeto leen basura. Se comprueba escribiendo
//     un centinela despues del payload viejo y verificando que sobrevive.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Interfaz/InterfaceComponent.h"

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

constexpr std::uint32_t CENTINELA = 0xA5A5A5A5u;

// Round-trip del formato nuevo: se guardan valores distintos de los de por
// defecto y deben volver identicos.
void roundTrip() {
    InterfaceComponent origen;
    origen.setRutaHtml("src/Juego/UI/hud.html");
    origen.setActivo(false);
    origen.setAncla(AnclaInterfaz::AbajoDerecha);
    origen.setDesplazamiento(glm::vec2(12.0f, -34.0f));
    origen.setTamanoAuto(false);
    origen.setTamano(glm::vec2(320.0f, 180.0f));
    origen.setZ(2.5f);

    const TempPruebas::CarpetaPrueba carpeta("funshi_interface_component");
    const std::filesystem::path archivo = carpeta.ruta() / "interfaz.bin";

    {
        std::ofstream out(archivo, std::ios::binary);
        origen.saveComponent(&out);
    }

    InterfaceComponent leido;
    {
        std::ifstream in(archivo, std::ios::binary);
        leido.loadComponent(&in);
    }

    CHECK(leido.getRutaHtml() == origen.getRutaHtml(),
          "la ruta .html vuelve igual");
    CHECK(leido.estaActiva() == false, "el flag activo vuelve igual");
    CHECK(leido.getAncla() == AnclaInterfaz::AbajoDerecha,
          "el ancla vuelve igual");
    CHECK(leido.getDesplazamiento() == glm::vec2(12.0f, -34.0f),
          "el desplazamiento vuelve igual");
    CHECK(leido.usaTamanoAuto() == false, "el flag tamaño auto vuelve igual");
    CHECK(leido.getTamano() == glm::vec2(320.0f, 180.0f),
          "el tamaño vuelve igual");
    CHECK(leido.getZ() == 2.5f, "la z vuelve igual");
}

// Valores de fabrica de un componente recien creado.
void valoresPorDefecto() {
    InterfaceComponent comp;
    CHECK(comp.getRutaHtml().empty(), "sin ruta por defecto");
    CHECK(comp.estaActiva(), "activa por defecto");
    CHECK(comp.getAncla() == AnclaInterfaz::Centro, "ancla Centro por defecto");
    CHECK(comp.usaTamanoAuto(), "tamaño automatico por defecto");
    CHECK(comp.getZ() == 0.0f, "z cero por defecto");
}

// Payload del formato viejo (solo el nombre: size_t longitud + bytes) seguido
// de un centinela. El lector debe cargar el nombre y dejar el centinela
// intacto para el siguiente componente.
void formatoViejoNoSePasaDeLargo() {
    const std::string nombre = "hud_vieja";
    const TempPruebas::CarpetaPrueba carpeta("funshi_interface_legacy");
    const std::filesystem::path archivo = carpeta.ruta() / "legacy.bin";

    {
        std::ofstream out(archivo, std::ios::binary);
        const std::size_t len = nombre.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(nombre.data(), static_cast<std::streamsize>(len));
        out.write(reinterpret_cast<const char*>(&CENTINELA), sizeof(CENTINELA));
    }

    InterfaceComponent comp;
    std::uint32_t trasLectura = 0;
    {
        std::ifstream in(archivo, std::ios::binary);
        comp.loadComponent(&in);
        in.read(reinterpret_cast<char*>(&trasLectura), sizeof(trasLectura));
    }

    CHECK(comp.getRutaHtml() == nombre,
          "el nombre del formato viejo se carga como ruta");
    CHECK(comp.estaActiva(), "el formato viejo deja los defaults del componente");
    CHECK(comp.getAncla() == AnclaInterfaz::Centro,
          "el formato viejo deja el ancla por defecto");
    CHECK(trasLectura == CENTINELA,
          "el lector no consume bytes de mas tras el payload viejo");
}

} // namespace

int main() {
    valoresPorDefecto();
    roundTrip();
    formatoViejoNoSePasaDeLargo();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
