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
// Pruebas headless del reproductor de musica (GestorMusica): la lista ordenada
// de pistas (por ruta), el fundido cruzado, el paso automatico a la pista
// siguiente, la deteccion y el bus.
//
// El backend de musica es FALSO (implementa IMusicaBackend sin tocar audio):
// asi se puede comprobar el comportamiento del reproductor paso a paso.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo y salida
// final "OK/FALLOS: N comprobaciones".

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Audio/GestorMusica.h"
#include "../FunshiEngineGL/src/Audio/IMusicaBackend.h"

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

// Backend de musica falso: guarda lo que el reproductor le pide, para poder
// comprobarlo despues.
class BackendFalso : public IMusicaBackend {
public:
    struct Pista {
        std::string ruta;
        float volumen = 0.0f;
        bool sonando = false;
        bool terminada = false;
        float posicion = 0.0f;
        float duracion = 120.0f;
    };

    std::map<int, Pista> pistas;
    int siguienteId = 1;
    float volumenDelBus = 1.0f;
    bool silenciado = false;
    // Pistas abiertas a la vez: sirve para detectar que una pista se queda
    // viva (y suena a la vez que la otra) cuando deberia haber cerrado.
    std::size_t abiertas() const { return pistas.size(); }

    const Pista* buscar(const std::string& ruta) const {
        for (const auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) return &pista;
        }
        return nullptr;
    }

    // Marca como terminada la pista que esta sonando con esa ruta.
    void terminar(const std::string& ruta) {
        for (auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) pista.terminada = true;
        }
    }

    void mover(const std::string& ruta, float posicion) {
        for (auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) pista.posicion = posicion;
        }
    }

    void durar(const std::string& ruta, float segundos) {
        for (auto& [id, pista] : pistas) {
            if (pista.ruta == ruta) pista.duracion = segundos;
        }
    }

    int abrirPista(const std::string& ruta) override {
        const int id = siguienteId++;
        Pista pista;
        pista.ruta = ruta;
        pistas[id] = pista;
        return id;
    }

    void cerrarPista(int id) override { pistas.erase(id); }

    bool reproducirPista(int id, float volumen) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return false;
        it->second.sonando = true;
        it->second.terminada = false;
        it->second.volumen = volumen;
        return true;
    }

    void detenerPista(int id) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return;
        it->second.sonando = false;
        it->second.volumen = 0.0f;
    }

    void volumenPista(int id, float volumen) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return;
        it->second.volumen = volumen;
    }

    void volumenBus(float volumen) override { volumenDelBus = volumen; }

    void silenciarBus(bool estado) override { silenciado = estado; }

    EstadoPista estadoPista(int id) override {
        auto it = pistas.find(id);
        if (it == pistas.end()) return EstadoPista::Detenida;
        if (it->second.terminada) return EstadoPista::Terminada;
        return it->second.sonando ? EstadoPista::Reproduciendo
                                  : EstadoPista::Detenida;
    }

    float posicionPista(int id) override {
        auto it = pistas.find(id);
        return it == pistas.end() ? 0.0f : it->second.posicion;
    }

    float duracionPista(int id) override {
        auto it = pistas.find(id);
        return it == pistas.end() ? 0.0f : it->second.duracion;
    }
};

bool casiIgual(float a, float b) { return std::fabs(a - b) < 1e-4f; }

} // namespace

// Sin lista no hay nada que reproducir.
void listaVacia() {
    BackendFalso backend;
    GestorMusica gestor(backend);

    CHECK(gestor.pistas().empty(), "la lista de pistas empieza vacia");
    CHECK(!gestor.tienePista("musica/tema.ogg"), "una pista que no esta no esta");
    CHECK(!gestor.reproducir("", 1.0f, 0.0f),
          "no se puede reproducir una pista sin ruta");
    CHECK(backend.abiertas() == 0, "no se abrio ninguna pista en el backend");
    CHECK(!gestor.sonando(), "sin pistas no hay nada sonando");
    CHECK(gestor.pistaActual().empty(), "la pista actual esta vacia");
}

// La lista guarda el orden en que se establecio y cada pista es su ruta.
void listaDePistas() {
    BackendFalso backend;
    GestorMusica gestor(backend);

    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});

    CHECK(gestor.pistas().size() == 2, "se establecen dos pistas");
    CHECK(gestor.pistas()[0] == "musica/alfa.ogg", "el orden es el dado");
    CHECK(gestor.pistas()[1] == "musica/beta.ogg", "la segunda pista va despues");
    CHECK(gestor.tienePista("musica/alfa.ogg"), "la pista esta en la lista");
    CHECK(!gestor.tienePista("musica/gamma.ogg"), "una ruta que no esta no esta");
    CHECK(gestor.indiceDe("musica/beta.ogg") == 1, "el indice es la posicion");
    CHECK(gestor.indiceDe("musica/gamma.ogg") == -1,
          "una ruta que no esta no tiene indice");

    // Las rutas vacias no entran en la lista.
    gestor.establecerPistas({"musica/alfa.ogg", "", "musica/beta.ogg"});
    CHECK(gestor.pistas().size() == 2, "las rutas vacias se descartan");

    gestor.anadirPista("musica/gamma.ogg");
    CHECK(gestor.pistas().size() == 3, "anadir pone la pista al final");
    CHECK(gestor.pistas()[2] == "musica/gamma.ogg", "la pista anadida va al final");

    gestor.limpiarPistas();
    CHECK(gestor.pistas().empty(), "limpiar vacia la lista");
    CHECK(!gestor.tienePista("musica/alfa.ogg"), "tras limpiar no hay pistas");
}

// Reproducir una pista la abre en el backend con el volumen pedido.
void reproduccionSimple() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/tema.ogg"});

    CHECK(gestor.reproducir("musica/tema.ogg", 0.8f, 0.0f), "se reproduce la pista");
    CHECK(backend.abiertas() == 1, "el backend tiene una pista abierta");
    const BackendFalso::Pista* pista = backend.buscar("musica/tema.ogg");
    CHECK(pista != nullptr, "la pista abierta es la de la lista");
    if (pista) {
        CHECK(pista->sonando, "la pista esta sonando");
        CHECK(casiIgual(pista->volumen, 0.8f), "la pista suena al volumen pedido");
    }
    CHECK(gestor.pistaActual() == "musica/tema.ogg",
          "la pista actual es la ruta que suena");
    CHECK(gestor.sonando(), "el reproductor dice que suena");
    CHECK(!gestor.enFundido(), "sin fundido no hay fundido en curso");
}

// Se puede tocar un archivo que no esta en la lista: suena, pero al acabarse
// no hay pista siguiente.
void pistaFueraDeLaLista() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg"});
    gestor.duracionFundido(0.0f);

    CHECK(gestor.reproducir("musica/suelta.ogg", 1.0f, 0.0f),
          "se reproduce un archivo que no esta en la lista");
    CHECK(gestor.pistaActual() == "musica/suelta.ogg",
          "la pista actual es la que se pidio");

    backend.terminar("musica/suelta.ogg");
    gestor.actualizar(0.1f);
    CHECK(!gestor.sonando(),
          "al acabarse una pista que no esta en la lista no hay a cual seguir");
    CHECK(backend.abiertas() == 0, "la pista suelta se cierra");
}

// El volumen pedido se recorta al rango valido.
void volumenLimitado() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/tema.ogg"});

    gestor.reproducir("musica/tema.ogg", 5.0f, 0.0f);
    const BackendFalso::Pista* pista = backend.buscar("musica/tema.ogg");
    CHECK(pista != nullptr, "la pista se abrio");
    if (pista) CHECK(casiIgual(pista->volumen, 1.0f),
                     "un volumen mayor que uno se recorta a uno");
}

// El fundido cruzado baja la pista que suena y sube la que entra.
void fundidoCruzado() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});

    gestor.reproducir("musica/alfa.ogg", 1.0f, 0.0f);
    CHECK(gestor.reproducir("musica/beta.ogg", 1.0f, 2.0f), "se pide la pista siguiente");

    CHECK(gestor.enFundido(), "el fundido esta en curso");
    CHECK(gestor.pistaActual() == "musica/alfa.ogg",
          "durante el fundido sigue sonando la pista anterior");
    CHECK(gestor.pistaSiguiente() == "musica/beta.ogg",
          "la pista que entra es la nueva");
    CHECK(backend.abiertas() == 2, "las dos pistas estan vivas durante el fundido");

    gestor.actualizar(1.0f);
    const BackendFalso::Pista* saliente = backend.buscar("musica/alfa.ogg");
    const BackendFalso::Pista* entrante = backend.buscar("musica/beta.ogg");
    CHECK(saliente != nullptr && entrante != nullptr, "las dos pistas siguen vivas");
    if (saliente && entrante) {
        CHECK(casiIgual(saliente->volumen, 0.5f),
              "a mitad del fundido la pista que sale suena a la mitad");
        CHECK(casiIgual(entrante->volumen, 0.5f),
              "a mitad del fundido la pista que entra suena a la mitad");
    }
    CHECK(casiIgual(gestor.progresoFundido(), 0.5f), "el progreso va por la mitad");
    CHECK(gestor.enFundido(), "el fundido sigue en curso");

    gestor.actualizar(1.0f);
    CHECK(!gestor.enFundido(), "el fundido termina al agotarse el tiempo");
    CHECK(gestor.pistaActual() == "musica/beta.ogg",
          "la pista que entra pasa a ser la actual");
    CHECK(backend.buscar("musica/alfa.ogg") == nullptr,
          "la pista que salio se cierra al terminar el fundido");
    CHECK(backend.abiertas() == 1, "solo queda viva la pista que entra");
}

// Un fundido de duracion cero cambia de pista de inmediato.
void fundidoInmediato() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});

    gestor.reproducir("musica/alfa.ogg", 1.0f, 0.0f);
    gestor.reproducir("musica/beta.ogg", 0.6f, 0.0f);

    CHECK(!gestor.enFundido(), "sin duracion no hay fundido");
    CHECK(gestor.pistaActual() == "musica/beta.ogg", "la pista nueva es la actual");
    CHECK(backend.abiertas() == 1, "la pista vieja se cierra al instante");
    const BackendFalso::Pista* pista = backend.buscar("musica/beta.ogg");
    CHECK(pista != nullptr && casiIgual(pista->volumen, 0.6f),
          "la pista nueva suena con el volumen pedido");
}

// Cuando la pista actual se acaba se pasa a la siguiente de la lista.
void avanceAutomatico() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});
    gestor.duracionFundido(0.0f);

    gestor.reproducir("musica/alfa.ogg", 1.0f, 0.0f);
    backend.terminar("musica/alfa.ogg");

    gestor.actualizar(0.1f);
    CHECK(gestor.pistaActual() == "musica/beta.ogg",
          "al acabarse la pista se pasa a la siguiente de la lista");
    CHECK(backend.buscar("musica/alfa.ogg") == nullptr,
          "la pista terminada se cierra");
    CHECK(backend.abiertas() == 1, "solo queda viva la siguiente");
}

// Al final de la lista se vuelve a la primera (si la lista se repite).
void listaSeRepite() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});
    gestor.duracionFundido(0.0f);
    CHECK(gestor.repetirLista(), "por defecto la lista se repite");

    gestor.reproducir("musica/beta.ogg", 1.0f, 0.0f);
    backend.terminar("musica/beta.ogg");
    gestor.actualizar(0.1f);
    CHECK(gestor.pistaActual() == "musica/alfa.ogg",
          "al acabarse la ultima se vuelve a la primera");
}

// Si la lista no se repite, al acabarse la ultima pista la musica se detiene.
void listaSinRepetir() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});
    gestor.duracionFundido(0.0f);
    gestor.establecerRepetirLista(false);
    CHECK(!gestor.repetirLista(), "la lista se puede dejar de repetir");

    gestor.reproducir("musica/beta.ogg", 1.0f, 0.0f);
    backend.terminar("musica/beta.ogg");
    gestor.actualizar(0.1f);
    CHECK(!gestor.sonando(), "al acabarse la ultima pista la musica se detiene");
    CHECK(backend.abiertas() == 0, "no queda ninguna pista viva");
}

// El cruce empieza antes de que la pista se acabe, para que una pista entre
// mientras la otra sale.
void fundidoAnticipado() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});
    gestor.duracionFundido(2.0f);

    gestor.reproducir("musica/alfa.ogg", 1.0f, 0.0f);
    backend.durar("musica/alfa.ogg", 10.0f);
    backend.mover("musica/alfa.ogg", 9.5f);  // le queda medio segundo

    gestor.actualizar(0.1f);
    CHECK(gestor.enFundido(), "el cruce empieza antes de que la pista acabe");
    CHECK(gestor.pistaSiguiente() == "musica/beta.ogg",
          "la siguiente entra durante el cruce");
    CHECK(backend.abiertas() == 2, "las dos pistas se oyen un momento");
}

// Detener para la musica y cierra la pista.
void detener() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/tema.ogg"});

    gestor.reproducir("musica/tema.ogg", 1.0f, 0.0f);
    gestor.detener();

    CHECK(!gestor.sonando(), "tras detener no suena nada");
    CHECK(gestor.pistaActual().empty(), "la pista actual se vacia");
    CHECK(backend.abiertas() == 0, "la pista se cierra");
}

// Detener con fundido baja el volumen antes de parar.
void detenerConFundido() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/tema.ogg"});

    gestor.reproducir("musica/tema.ogg", 1.0f, 0.0f);
    gestor.detener(1.0f);
    CHECK(gestor.enFundido(), "la deteccion con fundido entra en fundido");
    CHECK(backend.abiertas() == 1, "la pista sigue viva mientras baja");

    gestor.actualizar(0.5f);
    const BackendFalso::Pista* pista = backend.buscar("musica/tema.ogg");
    CHECK(pista != nullptr && casiIgual(pista->volumen, 0.5f),
          "a mitad del fundido de salida el volumen es la mitad");

    gestor.actualizar(0.5f);
    CHECK(!gestor.sonando(), "al terminar el fundido de salida no suena nada");
    CHECK(backend.abiertas() == 0, "la pista se cierra al terminar el fundido");
}

// El volumen del bus y el silencio se aplican al backend.
void bus() {
    BackendFalso backend;
    GestorMusica gestor(backend);

    gestor.volumenBus(0.5f);
    CHECK(casiIgual(backend.volumenDelBus, 0.5f), "el bus recibe el volumen");
    CHECK(casiIgual(gestor.volumenBusActual(), 0.5f),
          "el reproductor guarda el volumen del bus");

    gestor.volumenBus(3.0f);
    CHECK(casiIgual(gestor.volumenBusActual(), 1.0f),
          "el volumen del bus se recorta a uno");

    gestor.silenciarBus(true);
    CHECK(backend.silenciado, "el backend recibe el silencio");
    CHECK(gestor.busSilenciado(), "el reproductor recuerda el silencio");

    gestor.silenciarBus(false);
    CHECK(!backend.silenciado, "el silencio se puede quitar");
}

// Pedir una pista mientras hay un fundido en curso cancela la pista que
// estaba entrando: no se quedan tres pistas sonando.
void cancelarEntrada() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg",
                              "musica/gamma.ogg"});

    gestor.reproducir("musica/alfa.ogg", 1.0f, 0.0f);
    gestor.reproducir("musica/beta.ogg", 1.0f, 2.0f);
    gestor.reproducir("musica/gamma.ogg", 1.0f, 2.0f);

    CHECK(backend.buscar("musica/beta.ogg") == nullptr,
          "la pista que estaba entrando se cierra");
    CHECK(backend.abiertas() == 2, "solo quedan la que suena y la que entra");
    CHECK(gestor.pistaSiguiente() == "musica/gamma.ogg",
          "la pista que entra es la ultima");
    CHECK(gestor.pistaActual() == "musica/alfa.ogg", "la que sigue sonando no cambia");
}

// Reproducir la siguiente avanza en el orden de la lista.
void reproducirSiguiente() {
    BackendFalso backend;
    GestorMusica gestor(backend);
    gestor.establecerPistas({"musica/alfa.ogg", "musica/beta.ogg"});

    CHECK(gestor.reproducirSiguiente(1.0f, 0.0f),
          "se reproduce la siguiente sin ninguna sonando");
    CHECK(gestor.pistaActual() == "musica/alfa.ogg",
          "sin ninguna sonando empieza la primera");

    CHECK(gestor.reproducirSiguiente(1.0f, 0.0f), "se pasa a la siguiente");
    CHECK(gestor.pistaActual() == "musica/beta.ogg",
          "la siguiente de la lista es la segunda");

    // Sin lista no hay siguiente que reproducir.
    gestor.limpiarPistas();
    CHECK(!gestor.reproducirSiguiente(1.0f, 0.0f),
          "sin lista no se puede reproducir la siguiente");
}

int main() {
    listaVacia();
    listaDePistas();
    reproduccionSimple();
    pistaFueraDeLaLista();
    volumenLimitado();
    fundidoCruzado();
    fundidoInmediato();
    avanceAutomatico();
    listaSeRepite();
    listaSinRepetir();
    fundidoAnticipado();
    detener();
    detenerConFundido();
    bus();
    cancelarEntrada();
    reproducirSiguiente();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
