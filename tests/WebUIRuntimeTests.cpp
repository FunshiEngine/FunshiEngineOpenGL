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

// Pruebas headless del runtime de interfaces web (WebUIRuntime), del gestor de
// vistas (VistasWebUI) y del estado por interfaz (EstadoInterfaces). Corren
// enteras con el backend nulo (sin CEF ni GL):
//   - seleccion del backend por configuracion y fallback (un nombre que no
//     existe no deja al motor sin backend),
//   - ciclo de vida completo de una vista sin GPU (cargar/redimensionar/
//     actualizar/render/input/descargar),
//   - el gestor crea una vista por interfaz activa, la recarga si cambia el
//     .html o si el archivo cambia en disco (mtime), la redimensiona con el
//     contenedor y la descarga al desaparecer,
//   - el estado por interfaz arma el arbol JSON del motor y lee el que la
//     interfaz devuelve, por rutas estilo JSON Pointer,
//   - el ciclo de vistas entrega el estado pendiente de cada interfaz y olvida
//     el de la que se descarga.

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "../FunshiEngineGL/src/Objetos/Componentes/InterfaceComponent.h"
#include "../FunshiEngineGL/src/WebUI/BackendNull.h"
#include "../FunshiEngineGL/src/WebUI/EstadoInterfaces.h"
#include "../FunshiEngineGL/src/WebUI/IWebUIBackend.h"
#include "../FunshiEngineGL/src/WebUI/VistaWebUI.h"
#include "../FunshiEngineGL/src/WebUI/VistasWebUI.h"
#include "../FunshiEngineGL/src/WebUI/WebUIRuntime.h"

// El contrato de la interfaz web no tiene comportamiento por defecto (cada
// backend declara que hace al arrancar, al bombear y al cerrar) y el backend
// nulo lo implementa entero.
static_assert(std::is_abstract<IWebUIBackend>::value,
              "el contrato de la interfaz web exige implementar todos sus metodos");
static_assert(!std::is_abstract<BackendNullWebUI>::value,
              "el backend nulo implementa el contrato completo");

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

// Seleccion por configuracion y fallback: sin backend real el activo es el
// nulo; pedir uno inexistente no cambia la seleccion ni deja al motor sin
// backend.
void seleccionDeBackend() {
    CHECK(std::string(WebUIRuntime::nombreBackendActivo()) == "null",
          "sin backend real el activo es el nulo");
    CHECK(!WebUIRuntime::soporteBackendReal(),
          "no hay backend web real compilado");

    CHECK(!WebUIRuntime::seleccionarBackend("cef"),
          "un backend inexistente no se selecciona");
    CHECK(std::string(WebUIRuntime::nombreBackendActivo()) == "null",
          "la seleccion no cambia tras un nombre invalido");
    CHECK(WebUIRuntime::seleccionarBackend("null"),
          "el backend nulo siempre es seleccionable");
}

// Ciclo de vida de una vista con el backend nulo: carga, redimensiona, avanza,
// no entrega textura, acepta input sin efecto y se descarga.
void cicloDeVidaSinGL() {
    std::string error;
    VistaWebUI vista = WebUIRuntime::cargar("UI/hud.html", 800, 600, error);
    CHECK(vista.cargada, "la carga con el backend nulo tiene exito");
    CHECK(vista.valida(), "la vista cargada es valida");
    CHECK(vista.ruta == "UI/hud.html", "la vista guarda la ruta del .html");
    CHECK(vista.ancho == 800 && vista.alto == 600,
          "la vista guarda el area inicial");
    CHECK(vista.textura == 0, "el backend nulo no entrega textura");
    CHECK(vista.handleImGui == nullptr,
          "el backend nulo no entrega descriptor para ImGui");
    CHECK(error.empty(), "no hay error con el backend nulo");

    WebUIRuntime::redimensionar(vista, 1024, 768);
    CHECK(vista.ancho == 1024 && vista.alto == 768,
          "redimensionar actualiza el area");

    WebUIRuntime::actualizar(vista, 0.016f);
    CHECK(WebUIRuntime::render(vista) == 0,
          "render no produce textura con el backend nulo");

    // Input: no debe tener efecto ni romper.
    WebUIRuntime::ratonMover(vista, 10, 20);
    WebUIRuntime::ratonBoton(vista, 0, true);
    WebUIRuntime::ratonBoton(vista, 1, false);
    WebUIRuntime::ratonRueda(vista, 1.0f);
    WebUIRuntime::tecla(vista, 65, true, false);
    WebUIRuntime::foco(vista, true);
    CHECK(vista.cargada, "la vista sigue cargada tras el input");

    WebUIRuntime::descargar(vista);
    CHECK(!vista.cargada, "descargar invalida la vista");
}

// Gestor de vistas: crea una por interfaz, recarga al cambiar el .html,
// redimensiona con el contenedor y descarga las que ya no estan.
void gestorDeVistas() {
    InterfaceComponent c1;
    c1.setRutaHtml("UI/a.html");
    InterfaceComponent c2;
    c2.setRutaHtml("UI/b.html");

    VistasWebUI vistas;
    std::vector<InterfaceComponent*> lista{&c1, &c2};
    vistas.sincronizar(lista, 800, 600, 0.016f);

    CHECK(vistas.buscar(&c1) != nullptr, "se crea la vista de la primera interfaz");
    CHECK(vistas.buscar(&c2) != nullptr, "se crea la vista de la segunda interfaz");
    CHECK(vistas.buscar(&c1)->ruta == "UI/a.html", "la vista usa la ruta del .html");

    // Cambio de .html: la vista se rehace con la ruta nueva.
    c1.setRutaHtml("UI/otra.html");
    vistas.sincronizar(std::vector<InterfaceComponent*>{&c1, &c2}, 800, 600,
                       0.016f);
    CHECK(vistas.buscar(&c1)->ruta == "UI/otra.html",
          "cambiar la ruta del componente recarga la vista");

    // Una interfaz sale de la lista: su vista se descarga.
    std::vector<InterfaceComponent*> soloC1{&c1};
    vistas.sincronizar(soloC1, 800, 600, 0.016f);
    CHECK(vistas.buscar(&c2) == nullptr,
          "la interfaz que sale de la lista descarga su vista");
    CHECK(vistas.buscar(&c1) != nullptr, "la que sigue activa conserva su vista");

    // Cambia el contenedor: la vista de tamano automatico se redimensiona.
    vistas.sincronizar(soloC1, 1280, 720, 0.016f);
    CHECK(vistas.buscar(&c1)->ancho == 1280 && vistas.buscar(&c1)->alto == 720,
          "el contenedor nuevo redimensiona la vista automatica");

    // Tamano fijo: la vista usa el tamano del componente, no el contenedor.
    InterfaceComponent fijo;
    fijo.setRutaHtml("UI/fijo.html");
    fijo.setTamanoAuto(false);
    fijo.setTamano(glm::vec2(200.0f, 100.0f));
    std::vector<InterfaceComponent*> conFijo{&fijo};
    vistas.sincronizar(conFijo, 1280, 720, 0.016f);
    CHECK(vistas.buscar(&fijo)->ancho == 200 && vistas.buscar(&fijo)->alto == 100,
          "la vista de tamano fijo usa el tamano del componente");

    vistas.vaciar();
    CHECK(vistas.buscar(&c1) == nullptr && vistas.buscar(&fijo) == nullptr,
          "vaciar descarga todas las vistas");
}

// Recarga en caliente: si el .html cambia en disco, la vista se rehace sin
// cambiar la ruta del componente. Con archivo ilegible no hay recarga.
void recargaPorMtime() {
    namespace fs = std::filesystem;
    const fs::path carpeta =
        fs::temp_directory_path() / "funshi_webui_mtime_tests";
    std::error_code ec;
    fs::create_directories(carpeta, ec);
    const fs::path html = carpeta / "hud.html";
    {
        std::ofstream f(html.string(), std::ios::trunc);
        f << "<html><body>v1</body></html>";
    }

    InterfaceComponent hud;
    hud.setRutaHtml(html.string());
    VistasWebUI vistas;
    std::vector<InterfaceComponent*> lista{&hud};
    vistas.sincronizar(lista, 800, 600, 0.016f);
    const VistaWebUI* vista = vistas.buscar(&hud);
    CHECK(vista != nullptr, "la vista del .html temporal se crea");
    CHECK(vista && vista->mtimeHtml.has_value(),
          "al cargar un archivo real se guarda su sello mtime");

    // Sin tocar el archivo: el sello no cambia y la vista no se rehace.
    const SelloEscrituraWebUI selloAntes =
        vista ? vista->mtimeHtml : std::nullopt;
    vistas.sincronizar(lista, 800, 600, 0.016f);
    vista = vistas.buscar(&hud);
    CHECK(vista && vista->mtimeHtml == selloAntes,
          "sin cambios en disco la vista conserva su sello");

    // Guardado externo: el sello cambia y la vista se recarga.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    {
        std::ofstream f(html.string(), std::ios::trunc);
        f << "<html><body>v2</body></html>";
    }
    vistas.sincronizar(lista, 800, 600, 0.016f);
    vista = vistas.buscar(&hud);
    CHECK(vista != nullptr && vista->cargada,
          "tras guardar el .html la vista sigue cargada");
    CHECK(vista && vista->mtimeHtml != selloAntes,
          "tras guardar el .html la vista recarga con el sello nuevo");

    // Ruta ilegible: no hay recarga por mtime (sello -1), solo por ruta.
    InterfaceComponent fantasma;
    fantasma.setRutaHtml("UI/no-existe.html");
    VistasWebUI otras;
    otras.sincronizar(std::vector<InterfaceComponent*>{&fantasma}, 800, 600,
                      0.016f);
    const VistaWebUI* vistaFantasma = otras.buscar(&fantasma);
    CHECK(vistaFantasma && !vistaFantasma->mtimeHtml.has_value(),
          "un .html ilegible deja el sello vacio");
    otras.sincronizar(std::vector<InterfaceComponent*>{&fantasma}, 800, 600,
                      0.016f);
    CHECK(otras.buscar(&fantasma) != nullptr,
          "un .html ilegible no rompe el ciclo");

    fs::remove_all(carpeta, ec);
}

// Estado por interfaz: el motor arma el arbol que recibe la interfaz (estado) y
// lee el que esta devuelve (salida), por rutas JSON Pointer.
void estadoDeInterfaces() {
    InterfaceComponent hud;
    EstadoInterfaces estado;

    CHECK(estado.estadoPendiente(&hud) == nullptr,
          "sin cambios no hay estado pendiente");

    // Valor escalar en una ruta anidada: se crean los objetos intermedios.
    CHECK(estado.establecerValor(&hud, "/jugador/vida", "42"),
          "establecer un valor escalar por ruta");
    const std::string* pendiente = estado.estadoPendiente(&hud);
    CHECK(pendiente != nullptr, "establecer marca el estado pendiente");
    CHECK(pendiente && pendiente->find("\"jugador\"") != std::string::npos &&
              pendiente->find("\"vida\":42") != std::string::npos,
          "el estado pendiente incluye la ruta anidada");

    estado.marcarEnviado(&hud);
    CHECK(estado.estadoPendiente(&hud) == nullptr,
          "marcar enviado limpia el pendiente");

    // Clave sin barra inicial: se normaliza a JSON Pointer.
    CHECK(estado.establecerValor(&hud, "puntaje", "7"),
          "la clave sin barra inicial tambien se acepta");
    const std::string* conClave = estado.estadoPendiente(&hud);
    CHECK(conClave && conClave->find("\"puntaje\":7") != std::string::npos,
          "la clave normalizada aparece en el estado");

    // Estado completo: reemplaza el arbol anterior.
    CHECK(estado.establecerEstado(&hud, "{\"nombre\":\"Ana\",\"vidas\":3}"),
          "establecer un estado completo");
    const std::string* completo = estado.estadoPendiente(&hud);
    CHECK(completo &&
              completo->find("\"nombre\":\"Ana\"") != std::string::npos,
          "el estado completo reemplaza al anterior");

    // JSON invalido se rechaza sin tocar el estado.
    CHECK(!estado.establecerEstado(&hud, "{no es json"),
          "un estado con JSON invalido se rechaza");
    CHECK(!estado.establecerValor(&hud, "/x", "{no es json"),
          "un valor con JSON invalido se rechaza");

    // Salida: lo que la interfaz devuelve al motor.
    CHECK(estado.valor(&hud, "/boton") == nullptr,
          "sin salida la lectura devuelve nulo");
    estado.definirSalida(&hud, "{\"boton\":\"jugar\",\"nivel\":2}");
    const char* boton = estado.valor(&hud, "/boton");
    CHECK(boton && std::string(boton) == "\"jugar\"",
          "la salida entrega el valor por ruta");
    const char* nivel = estado.valor(&hud, "nivel");
    CHECK(nivel && std::string(nivel) == "2",
          "la salida tambien normaliza la ruta");
    CHECK(estado.valor(&hud, "/inexistente") == nullptr,
          "una ruta inexistente devuelve nulo");

    // Olvidar: al descargar la vista se descarta estado y salida.
    estado.olvidar(&hud);
    CHECK(estado.valor(&hud, "/boton") == nullptr,
          "olvidar descarta la salida");
    CHECK(estado.estadoPendiente(&hud) == nullptr,
          "olvidar descarta el pending");

    // Normalizacion de rutas.
    CHECK(EstadoInterfaces::normalizarRuta("a/b") == std::string("/a/b"),
          "normalizarRuta agrega la barra inicial");
    CHECK(EstadoInterfaces::normalizarRuta("/a") == std::string("/a"),
          "normalizarRuta respeta la barra inicial");
    CHECK(EstadoInterfaces::normalizarRuta("") == std::string(),
          "normalizarRuta deja la ruta raiz vacia");
}

// Integracion del ciclo de vistas con el estado: al sincronizar, el gestor
// entrega el estado pendiente de cada interfaz (y lo marca como enviado) y, al
// descargar una vista, descarta el estado de esa interfaz. Con el backend nulo
// `establecerEstado`/`leerSalida` son no-op, pero la gestion del pendiente y el
// olvido son observables.
void sincronizacionConEstado() {
    InterfaceComponent hud;
    hud.setRutaHtml("UI/hud.html");
    EstadoInterfaces estado;
    VistasWebUI vistas;

    CHECK(estado.establecerValor(&hud, "/vida", "10"),
          "se prepara un valor de estado");
    CHECK(estado.estadoPendiente(&hud) != nullptr,
          "antes de sincronizar hay estado pendiente");

    vistas.sincronizar(std::vector<InterfaceComponent*>{&hud}, 800, 600, 0.016f,
                       &estado);
    CHECK(estado.estadoPendiente(&hud) == nullptr,
          "sincronizar entrega el estado y limpia el pendiente");

    // La interfaz desaparece: el gestor olvida su estado.
    CHECK(estado.establecerValor(&hud, "/vida", "20"),
          "se vuelve a marcar estado pendiente");
    vistas.sincronizar(std::vector<InterfaceComponent*>{}, 800, 600, 0.016f,
                       &estado);
    CHECK(estado.estadoPendiente(&hud) == nullptr,
          "al descargarse la vista se olvida el estado de la interfaz");
}

} // namespace

int main() {
    seleccionDeBackend();
    cicloDeVidaSinGL();
    gestorDeVistas();
    recargaPorMtime();
    estadoDeInterfaces();
    sincronizacionConEstado();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
