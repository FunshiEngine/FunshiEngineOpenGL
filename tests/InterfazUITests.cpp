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
// Pruebas headless de la interfaz de juego nativa: la colocacion de los widgets
// en pantalla (los nueve anclajes), la busqueda por identificador, el cambio
// de texto y de visibilidad, y el estado de interaccion con el raton (sobre
// que widget esta, cual se pulso y el flanco de pulsacion).
//
// El modulo es CPU puro (no usa OpenGL, glm ni ImGui), asi que la suite corre
// tambien en el build sin motor.

#include <iostream>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/UI/EntradaUI.h"
#include "../FunshiEngineGL/src/UI/LienzoUI.h"
#include "../FunshiEngineGL/src/UI/SistemaUI.h"

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

constexpr float kAncho = 800.0f;
constexpr float kAlto = 600.0f;

WidgetUI widgetBoton(const std::string& id, AnclaUI ancla, float x, float y) {
    WidgetUI widget;
    widget.id = id;
    widget.tipo = TipoWidgetUI::Boton;
    widget.ancla = ancla;
    widget.desplazamiento = {x, y};
    widget.tamano = {100.0f, 40.0f};
    return widget;
}

// --- Colocacion -------------------------------------------------------------

// Los nueve anclajes: cada uno coloca el widget en su punto de la pantalla y
// lo alinea respecto a ese punto.
void anclajes() {
    const RectUI arribaIzq =
        LienzoUI::colocar(AnclaUI::ArribaIzquierda, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(arribaIzq.x == 0.0f && arribaIzq.y == 0.0f,
          "arriba izquierda pegado a la esquina");

    const RectUI arribaCentro =
        LienzoUI::colocar(AnclaUI::ArribaCentro, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(arribaCentro.x == 350.0f && arribaCentro.y == 0.0f,
          "arriba centro centrado en horizontal");

    const RectUI arribaDer =
        LienzoUI::colocar(AnclaUI::ArribaDerecha, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(arribaDer.x == 700.0f && arribaDer.y == 0.0f,
          "arriba derecha pegado al borde derecho");

    const RectUI medioIzq =
        LienzoUI::colocar(AnclaUI::MedioIzquierda, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(medioIzq.x == 0.0f && medioIzq.y == 280.0f,
          "medio izquierda centrado en vertical");

    const RectUI centro =
        LienzoUI::colocar(AnclaUI::Centro, {0.0f, 0.0f}, {100.0f, 40.0f},
                          kAncho, kAlto);
    CHECK(centro.x == 350.0f && centro.y == 280.0f, "centro en el centro");

    const RectUI medioDer =
        LienzoUI::colocar(AnclaUI::MedioDerecha, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(medioDer.x == 700.0f && medioDer.y == 280.0f,
          "medio derecha pegado al borde derecho");

    const RectUI abajoIzq =
        LienzoUI::colocar(AnclaUI::AbajoIzquierda, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(abajoIzq.x == 0.0f && abajoIzq.y == 560.0f,
          "abajo izquierda pegado a la esquina inferior");

    const RectUI abajoCentro =
        LienzoUI::colocar(AnclaUI::AbajoCentro, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(abajoCentro.x == 350.0f && abajoCentro.y == 560.0f,
          "abajo centro centrado en horizontal");

    const RectUI abajoDer =
        LienzoUI::colocar(AnclaUI::AbajoDerecha, {0.0f, 0.0f},
                          {100.0f, 40.0f}, kAncho, kAlto);
    CHECK(abajoDer.x == 700.0f && abajoDer.y == 560.0f,
          "abajo derecha pegado a la esquina inferior derecha");
}

// El desplazamiento separa el widget del punto de anclaje.
void desplazamiento() {
    const RectUI rect = LienzoUI::colocar(AnclaUI::ArribaIzquierda,
                                           {25.0f, -10.0f}, {100.0f, 40.0f},
                                           kAncho, kAlto);
    CHECK(rect.x == 25.0f && rect.y == -10.0f,
          "el desplazamiento mueve el widget respecto del anclaje");
    CHECK(rect.ancho == 100.0f && rect.alto == 40.0f,
          "el tamano del widget se respeta");
}

// El layout se recalcula cuando cambia el tamano de la pantalla.
void layoutSigueLaPantalla() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("b", AnclaUI::Centro, 0.0f, 0.0f));

    const std::vector<RectUI>& primero = lienzo.layout(kAncho, kAlto);
    CHECK(primero.size() == 1, "un widget produce un rectangulo");
    CHECK(primero[0].x == 350.0f, "el boton queda centrado en 800 de ancho");

    const std::vector<RectUI>& segundo = lienzo.layout(1000.0f, kAlto);
    CHECK(segundo[0].x == 450.0f, "al cambiar el ancho se recoloca el boton");
    CHECK(&segundo == &lienzo.layout(1000.0f, kAlto),
          "con la misma pantalla el layout se reutiliza");
}

// Al tocar la lista de widgets el layout queda obsoleto y se recalcula.
void layoutSeInvalida() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("b", AnclaUI::Centro, 0.0f, 0.0f));
    lienzo.layout(kAncho, kAlto);
    const std::size_t revisionAntes = lienzo.revision();

    WidgetUI etiqueta;
    etiqueta.id = "texto";
    etiqueta.tipo = TipoWidgetUI::Etiqueta;
    etiqueta.ancla = AnclaUI::Centro;
    etiqueta.tamano = {50.0f, 20.0f};
    lienzo.agregar(etiqueta);

    CHECK(lienzo.revision() != revisionAntes, "anadir un widget cambia la revision");
    CHECK(lienzo.layout(kAncho, kAlto).size() == 2,
          "el layout tiene un rectangulo por widget");
}

// --- Lista de widgets --------------------------------------------------------

void busquedaYEdicion() {
    LienzoUI lienzo;
    WidgetUI vida;
    vida.id = "vida";
    vida.texto = "100";
    vida.tamano = {80.0f, 20.0f};
    lienzo.agregar(vida);

    CHECK(lienzo.existe("vida"), "el widget se encuentra por su identificador");
    CHECK(!lienzo.existe("inexistente"), "un identificador que no existe no aparece");
    CHECK(lienzo.indiceDe("inexistente") == LienzoUI::kNoEncontrado,
          "indiceDe devuelve kNoEncontrado si no esta");
    CHECK(lienzo.indiceDe("") == LienzoUI::kNoEncontrado,
          "un identificador vacio no se busca");

    CHECK(lienzo.establecerTexto("vida", "80"), "se cambia el texto del widget");
    CHECK(lienzo.textoDe("vida") == "80", "el texto nuevo se lee");
    CHECK(!lienzo.establecerTexto("inexistente", "x"),
          "no se cambia el texto de un widget que no existe");

    CHECK(lienzo.visibleDe("vida"), "el widget es visible por defecto");
    CHECK(lienzo.establecerVisible("vida", false), "se oculta el widget");
    CHECK(!lienzo.visibleDe("vida"), "el widget queda oculto");
    CHECK(!lienzo.establecerVisible("inexistente", false),
          "no se oculta un widget que no existe");
    CHECK(lienzo.textoDe("inexistente").empty(),
          "el texto de un widget inexistente sale vacio");
}

void agregarQuitarYMover() {
    LienzoUI lienzo;
    CHECK(lienzo.agregar(widgetBoton("a", AnclaUI::ArribaIzquierda, 0, 0)) == 0,
          "el primer widget ocupa el indice 0");
    CHECK(lienzo.agregar(widgetBoton("b", AnclaUI::ArribaIzquierda, 0, 0)) == 1,
          "el segundo widget ocupa el indice 1");

    CHECK(lienzo.mover(0, 1), "se puede mover un widget");
    CHECK(lienzo.widgets()[1].id == "a", "el widget movido queda en su sitio");
    CHECK(!lienzo.mover(0, 9), "mover a un indice que no existe se rechaza");
    CHECK(!lienzo.quitar(9), "quitar un indice que no existe se rechaza");

    CHECK(lienzo.quitar(0), "se quita un widget");
    CHECK(lienzo.widgets().size() == 1, "la lista queda con un widget");
    CHECK(lienzo.widgets()[0].id == "a", "queda el que no se quito");
}

// --- Interaccion con el raton -----------------------------------------------

// El raton solo hace hover sobre los widgets interactivos.
void deteccionDelRaton() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("b", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    WidgetUI etiqueta;
    etiqueta.id = "texto";
    etiqueta.tipo = TipoWidgetUI::Etiqueta;
    etiqueta.ancla = AnclaUI::ArribaIzquierda;
    etiqueta.tamano = {100.0f, 20.0f};
    lienzo.agregar(etiqueta);
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {50.0f, 20.0f}; // sobre el boton
    sistema.actualizar(lienzo, entrada);

    CHECK(sistema.estaSobre("b"), "el raton esta sobre el boton");
    CHECK(sistema.hayInteractivoSobre(), "hay un interactivo bajo el raton");
    CHECK(sistema.interactivoSobre() == "b", "el interactivo es el boton");
    CHECK(!sistema.estaSobre("texto"), "una etiqueta no es interactiva");

    // El mismo punto, pero ahora sobre la etiqueta: no hay nada interactivo.
    entrada.posicion = {50.0f, 45.0f};
    sistema.actualizar(lienzo, entrada);
    CHECK(!sistema.hayInteractivoSobre(),
          "sobre una etiqueta no hay interactivo");
    CHECK(!sistema.estaSobre("b"), "el boton ya no tiene el raton encima");
}

// El clic se marca solo en el frame en que ocurre.
void flancoDePulsacion() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("b", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {50.0f, 20.0f};

    entrada.botonIzquierdo = true;
    entrada.clicIzquierdo = true;
    sistema.actualizar(lienzo, entrada);
    CHECK(sistema.fuePulsado("b"), "el clic se marca en el frame del clic");
    CHECK(sistema.estaPulsado("b"), "el boton queda pulsado mientras se mantiene");

    entrada.clicIzquierdo = false;
    sistema.actualizar(lienzo, entrada);
    CHECK(!sistema.fuePulsado("b"),
          "el clic no se repite en el frame siguiente");
    CHECK(sistema.estaPulsado("b"), "el boton sigue pulsado");

    entrada.botonIzquierdo = false;
    sistema.actualizar(lienzo, entrada);
    CHECK(!sistema.estaPulsado("b"), "al soltar el boton deja de estar pulsado");
    CHECK(!sistema.fuePulsado("b"), "el clic ya no queda registrado");
}

// Un clic fuera de los widgets no activa nada.
void clicFueraDeLosWidgets() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("b", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {500.0f, 400.0f};
    entrada.botonIzquierdo = true;
    entrada.clicIzquierdo = true;
    sistema.actualizar(lienzo, entrada);

    CHECK(!sistema.fuePulsado("b"), "un clic fuera no pulsa ningun boton");
    CHECK(!sistema.hayInteractivoSobre(), "no hay interactivo bajo el raton");
}

// Un widget invisible no se dibuja ni recibe el raton.
void widgetInvisible() {
    LienzoUI lienzo;
    WidgetUI boton = widgetBoton("b", AnclaUI::ArribaIzquierda, 0.0f, 0.0f);
    boton.visible = false;
    lienzo.agregar(boton);
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {50.0f, 20.0f};
    entrada.clicIzquierdo = true;
    sistema.actualizar(lienzo, entrada);

    CHECK(!sistema.estaSobre("b"), "un widget invisible no tiene el raton encima");
    CHECK(!sistema.fuePulsado("b"), "un widget invisible no recibe el clic");
}

// El widget de encima (el ultimo de la lista) es el que recibe el raton.
void ordenDeApilado() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("abajo", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    lienzo.agregar(widgetBoton("encima", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {50.0f, 20.0f};
    entrada.clicIzquierdo = true;
    sistema.actualizar(lienzo, entrada);

    CHECK(sistema.estaSobre("encima"), "el widget de encima tiene el raton");
    CHECK(!sistema.estaSobre("abajo"), "el de debajo queda tapado");
    CHECK(sistema.fuePulsado("encima"), "el clic lo recibe el de encima");
    CHECK(!sistema.fuePulsado("abajo"), "el de debajo no recibe el clic");
}

// Si el widget de encima se oculta, el de debajo recibe el raton.
void elDeDebajoRecibeElRatonSiElDeArribaSeOculta() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("abajo", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    lienzo.agregar(widgetBoton("encima", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    lienzo.layout(kAncho, kAlto);
    lienzo.establecerVisible("encima", false);
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {50.0f, 20.0f};
    entrada.clicIzquierdo = true;
    sistema.actualizar(lienzo, entrada);

    CHECK(sistema.estaSobre("abajo"),
          "con el de encima oculto, el de debajo recibe el raton");
    CHECK(sistema.fuePulsado("abajo"), "y recibe el clic");
}

// Un widget sin identificador no se puede consultar por nombre.
void widgetSinIdentificador() {
    LienzoUI lienzo;
    WidgetUI boton = widgetBoton("", AnclaUI::ArribaIzquierda, 0.0f, 0.0f);
    lienzo.agregar(boton);
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {50.0f, 20.0f};
    entrada.botonIzquierdo = true;
    entrada.clicIzquierdo = true;
    sistema.actualizar(lienzo, entrada);

    CHECK(sistema.hayInteractivoSobre(),
          "un boton sin identificador sigue siendo interactivo");
    CHECK(!sistema.estaSobre(""), "no se consulta un identificador vacio");
    CHECK(sistema.sobreIndice(0), "por indice el estado si se ve");
    CHECK(sistema.pulsadoIndice(0), "por indice el boton pulsado tambien");
}

// El estado se conserva cuando el widget no cambia: el sistema no toca los
// que no estan bajo el raton.
void losWidgetsFueraDelRatonNoSeActivan() {
    LienzoUI lienzo;
    lienzo.agregar(widgetBoton("a", AnclaUI::ArribaIzquierda, 0.0f, 0.0f));
    lienzo.agregar(widgetBoton("b", AnclaUI::AbajoDerecha, 0.0f, 0.0f));
    lienzo.layout(kAncho, kAlto);

    SistemaUI sistema;
    EntradaUI entrada;
    entrada.posicion = {50.0f, 20.0f};
    entrada.clicIzquierdo = true;
    sistema.actualizar(lienzo, entrada);

    CHECK(sistema.estaSobre("a"), "el boton de arriba tiene el raton");
    CHECK(!sistema.estaSobre("b"), "el boton de abajo no");
    CHECK(!sistema.fuePulsado("b"), "el boton de abajo no registra el clic");
}

} // namespace

int main() {
    anclajes();
    desplazamiento();
    layoutSigueLaPantalla();
    layoutSeInvalida();
    busquedaYEdicion();
    agregarQuitarYMover();
    deteccionDelRaton();
    flancoDePulsacion();
    clicFueraDeLosWidgets();
    widgetInvisible();
    ordenDeApilado();
    elDeDebajoRecibeElRatonSiElDeArribaSeOculta();
    widgetSinIdentificador();
    losWidgetsFueraDelRatonNoSeActivan();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
