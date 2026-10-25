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
// Pruebas headless de la recoleccion de interfaces de juego (HUD). La funcion
// InterfacesJuego::recolectarDeEscena recorre la lista de objetos de la escena
// y devuelve las interfaces activas con ruta referenciada, ordenadas por z; de
// ella depende que una escena con varias interfaces no oculte las demas por el
// simple hecho de aparecer despues en la lista. Se verifica que:
//   - una lista nula o vacia no devuelve nada,
//   - los objetos sin InterfaceComponent no aportan nada,
//   - una escena con varias interfaces las devuelve TODAS, en orden de z,
//   - las interfaces sin ruta o inactivas se omiten sin cortar el recorrido.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/InterfaceComponent.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
#include "../FunshiEngineGL/src/Scenes/InterfacesJuego.h"

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

namespace {

// Objeto con un InterfaceComponent que referencia `ruta` (vacio = sin
// referencia). El objeto queda vivo mientras el unique_ptr lo posea.
std::unique_ptr<GameObject> conInterfaz(const std::string& ruta,
                                        float z = 0.0f, bool activo = true) {
    auto obj = GameObjectFactory::createSimpleObject(nullptr);
    auto ic = std::make_unique<InterfaceComponent>();
    ic->setRutaHtml(ruta);
    ic->setZ(z);
    ic->setActivo(activo);
    obj->addComponent(std::move(ic));
    return obj;
}

std::unique_ptr<GameObject> sinInterfaz() {
    return GameObjectFactory::createSimpleObject(nullptr);
}

} // namespace

// Una lista nula o vacia no tiene interfaces que recolectar.
void listaNulaOVacia() {
    CHECK(InterfacesJuego::recolectarDeEscena(nullptr).empty(),
          "una lista nula no devuelve interfaces");

    ListaDE<GameObject*> vacia;
    CHECK(InterfacesJuego::recolectarDeEscena(&vacia).empty(),
          "una lista vacia no devuelve interfaces");
}

// Los objetos sin InterfaceComponent no aportan nada aunque la lista no este
// vacia.
void sinInterfaces() {
    auto a = sinInterfaz();
    auto b = sinInterfaz();
    ListaDE<GameObject*> objetos;
    objetos.addLast(a.get());
    objetos.addLast(b.get());

    CHECK(InterfacesJuego::recolectarDeEscena(&objetos).empty(),
          "los objetos sin InterfaceComponent no aportan interfaces");
}

// Una escena con varias interfaces las devuelve TODAS, en orden de z. Es el
// caso que el recorrido viejo rompia al cortar en el primero.
void variasInterfacesEnOrdenPorZ() {
    auto a = conInterfaz("a.html", 5.0f);
    auto b = conInterfaz("b.html", -2.0f);
    auto c = conInterfaz("c.html", 0.0f);
    ListaDE<GameObject*> objetos;
    objetos.addLast(a.get());
    objetos.addLast(b.get());
    objetos.addLast(c.get());

    std::vector<InterfaceComponent*> interfaces =
        InterfacesJuego::recolectarDeEscena(&objetos);

    CHECK(interfaces.size() == 3,
          "se recolectan las tres interfaces de la escena");
    if (interfaces.size() == 3) {
        CHECK(interfaces[0]->getRutaHtml() == "b.html",
              "primero la interfaz de menor z");
        CHECK(interfaces[1]->getRutaHtml() == "c.html",
              "luego la de z intermedia");
        CHECK(interfaces[2]->getRutaHtml() == "a.html",
              "al final la de mayor z");
    }
}

// Las interfaces sin ruta o inactivas se omiten, y su presencia no corta el
// recorrido: los objetos posteriores con interfaz activa si se recolectan.
void inactivasOSinRutaSeOmiten() {
    auto sinRuta = conInterfaz("");
    auto inactiva = conInterfaz("oculta.html", 0.0f, false);
    auto real = conInterfaz("visible.html", 1.0f, true);
    ListaDE<GameObject*> objetos;
    objetos.addLast(sinRuta.get());
    objetos.addLast(inactiva.get());
    objetos.addLast(real.get());

    std::vector<InterfaceComponent*> interfaces =
        InterfacesJuego::recolectarDeEscena(&objetos);

    CHECK(interfaces.size() == 1,
          "solo se recolecta la interfaz activa con ruta");
    if (interfaces.size() == 1) {
        CHECK(interfaces[0]->getRutaHtml() == "visible.html",
              "la interfaz recolectada es la activa con ruta");
    }
}

int main() {
    listaNulaOVacia();
    sinInterfaces();
    variasInterfacesEnOrdenPorZ();
    inactivasOSinRutaSeOmiten();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
