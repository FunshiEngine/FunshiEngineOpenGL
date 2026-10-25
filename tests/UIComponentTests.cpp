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
// Pruebas headless del componente UIComponent (interfaz de juego nativa):
// la serializacion binaria de la lista de widgets y su colocacion.
//
// Cubre:
//   - los valores de fabrica del componente,
//   - el round-trip del formato (todos los campos de los widgets vuelven
//     iguales: identificador, tipo, anclaje, desplazamiento, tamano, colores,
//     texto, ruta de textura, tamano de texto y visibilidad),
//   - que un payload que NO es de este componente no se consuma de mas (un
//     centinela escrito despues debe sobrevivir intacto),
//   - que los valores fuera de rango (tipo o anclaje desconocidos, cantidad de
//     widgets disparatada) se acoten sin desalinear la lectura.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/UIComponent.h"

namespace {

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "  [FALLO] " << msg << " (linea " << __LINE__ << ")" \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

constexpr std::uint32_t CENTINELA = 0xA5A5A5A5u;

// Compara dos colores con tolerancia (van por el binario como float).
bool colorIgual(const ColorUI& a, const ColorUI& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

void valoresPorDefecto() {
    UIComponent comp;
    CHECK(comp.estaActivo(), "el componente nace activo");
    CHECK(comp.getZ() == 0.0f, "la z nace en cero");
    CHECK(comp.lienzo().widgets().empty(), "nace sin widgets");
}

// Un widget de ejemplo con todos los campos distintos de los de por defecto.
WidgetUI widgetDePrueba() {
    WidgetUI widget;
    widget.id = "vida";
    widget.tipo = TipoWidgetUI::Boton;
    widget.ancla = AnclaUI::AbajoDerecha;
    widget.desplazamiento = {12.0f, -34.0f};
    widget.tamano = {220.0f, 48.0f};
    widget.color = {0.25f, 0.5f, 0.75f, 0.8f};
    widget.colorTexto = {1.0f, 0.9f, 0.8f, 1.0f};
    widget.texto = "Vida: 100";
    widget.rutaTextura = "assets/vida.png";
    widget.tamanoTexto = 22.0f;
    widget.visible = true;
    return widget;
}

// Round-trip: se guarda un componente con varios widgets y todos los campos
// deben volver iguales.
void roundTrip() {
    UIComponent origen;
    origen.setActivo(false);
    origen.setZ(3.5f);
    origen.lienzo().agregar(widgetDePrueba());

    WidgetUI etiqueta;
    etiqueta.id = "titulo";
    etiqueta.tipo = TipoWidgetUI::Etiqueta;
    etiqueta.ancla = AnclaUI::ArribaCentro;
    etiqueta.texto = "Puntaje";
    etiqueta.tamano = {120.0f, 24.0f};
    etiqueta.visible = false;
    origen.lienzo().agregar(etiqueta);

    const TempPruebas::CarpetaPrueba carpeta("funshi_ui_component");
    const std::filesystem::path archivo = carpeta.ruta() / "interfaz.bin";

    {
        std::ofstream out(archivo, std::ios::binary);
        origen.saveComponent(&out);
    }

    UIComponent leido;
    {
        std::ifstream in(archivo, std::ios::binary);
        leido.loadComponent(&in);
    }

    CHECK(leido.estaActivo() == false, "el flag activo vuelve igual");
    CHECK(leido.getZ() == 3.5f, "la z vuelve igual");
    CHECK(leido.lienzo().widgets().size() == 2, "vuelven los dos widgets");

    if (leido.lienzo().widgets().size() != 2) return;

    const WidgetUI& primero = leido.lienzo().widgets()[0];
    CHECK(primero.id == "vida", "el identificador vuelve igual");
    CHECK(primero.tipo == TipoWidgetUI::Boton, "el tipo vuelve igual");
    CHECK(primero.ancla == AnclaUI::AbajoDerecha, "el anclaje vuelve igual");
    CHECK(primero.desplazamiento.x == 12.0f &&
              primero.desplazamiento.y == -34.0f,
          "el desplazamiento vuelve igual");
    CHECK(primero.tamano.x == 220.0f && primero.tamano.y == 48.0f,
          "el tamano vuelve igual");
    CHECK(colorIgual(primero.color, widgetDePrueba().color),
          "el color de fondo vuelve igual");
    CHECK(colorIgual(primero.colorTexto, widgetDePrueba().colorTexto),
          "el color del texto vuelve igual");
    CHECK(primero.texto == "Vida: 100", "el texto vuelve igual");
    CHECK(primero.rutaTextura == "assets/vida.png",
          "la ruta de la textura vuelve igual");
    CHECK(primero.tamanoTexto == 22.0f, "el tamano del texto vuelve igual");
    CHECK(primero.visible, "la visibilidad vuelve igual");

    const WidgetUI& segundo = leido.lienzo().widgets()[1];
    CHECK(segundo.id == "titulo", "el identificador del segundo vuelve igual");
    CHECK(segundo.tipo == TipoWidgetUI::Etiqueta,
          "el tipo del segundo vuelve igual");
    CHECK(segundo.ancla == AnclaUI::ArribaCentro,
          "el anclaje del segundo vuelve igual");
    CHECK(segundo.visible == false,
          "un widget invisible vuelve invisible");
    CHECK(segundo.texto == "Puntaje", "el texto del segundo vuelve igual");
}

// Un payload que no lleva la marca del componente no debe consumirse: el
// centinela escrito despues tiene que sobrevivir.
void payloadAjenoNoSeConsumeDeMas() {
    const TempPruebas::CarpetaPrueba carpeta("funshi_ui_component_ajeno");
    const std::filesystem::path archivo = carpeta.ruta() / "ajeno.bin";

    {
        std::ofstream out(archivo, std::ios::binary);
        // El payload ajeno tiene el tamano de la marca que lee el componente
        // (un size_t): si no, el centinela se consumiria como parte de el.
        const std::size_t contenido = 0x12345678u;
        out.write(reinterpret_cast<const char*>(&contenido), sizeof(contenido));
        out.write(reinterpret_cast<const char*>(&CENTINELA), sizeof(CENTINELA));
    }

    UIComponent comp;
    std::uint32_t trasLectura = 0;
    {
        std::ifstream in(archivo, std::ios::binary);
        comp.loadComponent(&in);
        in.read(reinterpret_cast<char*>(&trasLectura), sizeof(trasLectura));
    }

    CHECK(comp.lienzo().widgets().empty(),
          "un payload ajeno no crea widgets");
    CHECK(trasLectura == CENTINELA,
          "el lector no consume bytes de mas tras un payload ajeno");
}

// Un tipo o un anclaje que no existen en el enumerado se acotan a los valores
// por defecto, y el resto del payload se lee bien.
void valoresFueraDeRango() {
    const TempPruebas::CarpetaPrueba carpeta("funshi_ui_component_corruto");
    const std::filesystem::path archivo = carpeta.ruta() / "corrupto.bin";

    // Tamano del bloque de widgets: la cuenta de widgets mas el widget.
    // El widget lleva: identificador (size_t + bytes), tipo, anclaje, doce
    // flotantes (desplazamiento, tamano, color y color del texto), el texto y
    // la ruta de la textura (size_t + bytes), el tamano del texto y el flag de
    // visibilidad.
    const std::size_t tamanoWidget = 8 + 1 + 1 + 1 + 12 * sizeof(float) +
                                     8 + 8 + sizeof(float) + 1;
    const std::size_t tamanoBloque = sizeof(std::size_t) + tamanoWidget;

    {
        std::ofstream out(archivo, std::ios::binary);
        const std::size_t marca = 0x55494A31u; // "UIJ1"
        out.write(reinterpret_cast<const char*>(&marca), sizeof(marca));
        const std::uint8_t version = 1;
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        const std::uint8_t activo = 1;
        out.write(reinterpret_cast<const char*>(&activo), sizeof(activo));
        const float z = 1.0f;
        out.write(reinterpret_cast<const char*>(&z), sizeof(z));
        out.write(reinterpret_cast<const char*>(&tamanoBloque),
                  sizeof(tamanoBloque));

        const std::size_t cantidad = 1;
        out.write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));

        const std::size_t id = 1;
        out.write(reinterpret_cast<const char*>(&id), sizeof(id));
        out.write("x", 1);
        const std::uint8_t tipo = 99;
        out.write(reinterpret_cast<const char*>(&tipo), sizeof(tipo));
        const std::uint8_t ancla = 42;
        out.write(reinterpret_cast<const char*>(&ancla), sizeof(ancla));
        const float ceros[12] = {0, 0, 10, 10, 0, 0, 0, 0, 0, 0, 0, 0};
        out.write(reinterpret_cast<const char*>(&ceros),
                  static_cast<std::streamsize>(sizeof(ceros)));
        const std::size_t texto = 0;
        out.write(reinterpret_cast<const char*>(&texto), sizeof(texto));
        out.write(reinterpret_cast<const char*>(&texto), sizeof(texto));
        const float tamanoTexto = 12.0f;
        out.write(reinterpret_cast<const char*>(&tamanoTexto),
                  sizeof(tamanoTexto));
        const std::uint8_t visible = 1;
        out.write(reinterpret_cast<const char*>(&visible), sizeof(visible));
    }

    UIComponent comp;
    {
        std::ifstream in(archivo, std::ios::binary);
        comp.loadComponent(&in);
    }

    CHECK(comp.lienzo().widgets().size() == 1,
          "el widget con valores fuera de rango se lee");
    if (comp.lienzo().widgets().empty()) return;
    const WidgetUI& widget = comp.lienzo().widgets()[0];
    CHECK(widget.tipo == TipoWidgetUI::Etiqueta,
          "un tipo desconocido se acota al valor por defecto");
    CHECK(widget.ancla == AnclaUI::ArribaIzquierda,
          "un anclaje desconocido se acota al valor por defecto");
    CHECK(widget.id == "x", "el identificador se lee bien");
}

// Una cantidad de widgets disparatada se acota y el resto del archivo queda
// alineado (el centinela sobrevive).
void cantidadDeWidgetsDisparatada() {
    const TempPruebas::CarpetaPrueba carpeta("funshi_ui_component_datos");
    const std::filesystem::path archivo = carpeta.ruta() / "cantidad.bin";

    {
        std::ofstream out(archivo, std::ios::binary);
        const std::size_t marca = 0x55494A31u;
        out.write(reinterpret_cast<const char*>(&marca), sizeof(marca));
        const std::uint8_t version = 1;
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        const std::uint8_t activo = 1;
        out.write(reinterpret_cast<const char*>(&activo), sizeof(activo));
        const float z = 0.0f;
        out.write(reinterpret_cast<const char*>(&z), sizeof(z));
        // El bloque solo contiene la cuenta de widgets (no hay widgets).
        const std::size_t tamanoBloque = sizeof(std::size_t);
        out.write(reinterpret_cast<const char*>(&tamanoBloque),
                  sizeof(tamanoBloque));
        const std::size_t cantidad = 1000000000u;
        out.write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));
        out.write(reinterpret_cast<const char*>(&CENTINELA), sizeof(CENTINELA));
    }

    UIComponent comp;
    std::uint32_t trasLectura = 0;
    {
        std::ifstream in(archivo, std::ios::binary);
        comp.loadComponent(&in);
        in.read(reinterpret_cast<char*>(&trasLectura), sizeof(trasLectura));
    }

    CHECK(comp.lienzo().widgets().empty(),
          "una cantidad disparatada no crea widgets");
    CHECK(trasLectura == CENTINELA,
          "tras acotar la cantidad el archivo sigue alineado");
}

} // namespace

int main() {
    valoresPorDefecto();
    roundTrip();
    payloadAjenoNoSeConsumeDeMas();
    valoresFueraDeRango();
    cantidadDeWidgetsDisparatada();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
