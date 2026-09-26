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
// Pruebas headless del asset de interfaces de usuario (UserInterfaceCustom +
// WidgetUI). Sin ImGui ni pila grafica: el modelo es puro (solo std C++17 +
// nlohmann/json vendoriado), asi que se ejercita el round-trip JSON, la
// persistencia guardar/cargar en un proyecto temporal y la tolerancia a
// campos faltantes (los widgets conservan sus defaults).
//
// Casos:
//   - aJson()/desdeJson() redondean todos los campos de los 5 tipos de widget.
//   - guardar()/cargar() persisten una interfaz completa en disco.
//   - Desde JSON parcial (sin "sonido", sin controles opcionales) el modelo
//     conserva los valores por defecto.
//   - El nombre del archivo identifica la interfaz (nombre == stem).

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/GUI/CreadorUI/UserInterfaceCustom.h"

namespace fs = std::filesystem;

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

void probarRoundTripJson() {
    UserInterfaceCustom ui;
    ui.nombre = "MenuPrincipal";
    ui.titulo = "Menu principal";
    ui.ancho = 400.0f;
    ui.alto = 300.0f;

    WidgetUI boton;
    boton.tipo = TipoWidget::Boton;
    boton.nombre = "play";
    boton.etiqueta = "Jugar";
    boton.sonido = "click";

    WidgetUI slider;
    slider.tipo = TipoWidget::Slider;
    slider.nombre = "volumen";
    slider.valor = 0.5f;
    slider.minimo = 0.0f;
    slider.maximo = 1.0f;
    slider.sonido = "pickup";

    WidgetUI check;
    check.tipo = TipoWidget::Checkbox;
    check.nombre = "fullscreen";
    check.activado = true;

    WidgetUI texto;
    texto.tipo = TipoWidget::EntradaTexto;
    texto.nombre = "nombre";
    texto.texto = "Gian";
    texto.etiqueta = "Jugador";

    WidgetUI etiqueta;
    etiqueta.tipo = TipoWidget::Etiqueta;
    etiqueta.etiqueta = "Version 1.0";

    ui.widgets.push_back(boton);
    ui.widgets.push_back(slider);
    ui.widgets.push_back(check);
    ui.widgets.push_back(texto);
    ui.widgets.push_back(etiqueta);

    UserInterfaceCustom copia;
    copia.desdeJson(ui.aJson());

    CHECK(copia.nombre == ui.nombre, "roundtrip: nombre");
    CHECK(copia.titulo == ui.titulo, "roundtrip: titulo");
    CHECK(copia.ancho == ui.ancho, "roundtrip: ancho");
    CHECK(copia.alto == ui.alto, "roundtrip: alto");
    CHECK(copia.widgets.size() == 5, "roundtrip: 5 widgets");

    const WidgetUI& wb = copia.widgets[0];
    CHECK(wb.tipo == TipoWidget::Boton, "roundtrip: tipo boton");
    CHECK(wb.nombre == "play", "roundtrip: nombre widget");
    CHECK(wb.etiqueta == "Jugar", "roundtrip: etiqueta widget");
    CHECK(wb.sonido == "click", "roundtrip: sonido widget");

    const WidgetUI& ws = copia.widgets[1];
    CHECK(ws.tipo == TipoWidget::Slider, "roundtrip: tipo slider");
    CHECK(ws.valor == 0.5f, "roundtrip: valor slider");
    CHECK(ws.minimo == 0.0f && ws.maximo == 1.0f, "roundtrip: rango slider");
    CHECK(ws.sonido == "pickup", "roundtrip: sonido slider");

    const WidgetUI& wc = copia.widgets[2];
    CHECK(wc.tipo == TipoWidget::Checkbox, "roundtrip: tipo checkbox");
    CHECK(wc.activado, "roundtrip: checkbox activado");

    const WidgetUI& wt = copia.widgets[3];
    CHECK(wt.tipo == TipoWidget::EntradaTexto, "roundtrip: tipo texto");
    CHECK(wt.texto == "Gian", "roundtrip: texto de entrada");

    const WidgetUI& we = copia.widgets[4];
    CHECK(we.tipo == TipoWidget::Etiqueta, "roundtrip: tipo etiqueta");
}

void probarPersistencia() {
    // Carpeta temporal unica por proceso: crea y se limpia al salir (RAII).
    TempPruebas::CarpetaPrueba carpetaBase("funshi_userinterface_tests");
    const fs::path base = carpetaBase.ruta();

    UserInterfaceCustom ui;
    ui.nombre = "Opciones";
    ui.titulo = "Opciones del juego";
    {
        WidgetUI w;
        w.tipo = TipoWidget::Boton;
        w.nombre = "guardar";
        w.etiqueta = "Guardar";
        w.sonido = "guardar";
        ui.widgets.push_back(std::move(w));
    }

    CHECK(ui.guardar(base.string()), "guardar escribe el archivo JSON");

    UserInterfaceCustom cargo;
    cargo.nombre = "Opciones";  // el nombre identifica el archivo
    CHECK(cargo.cargar(base.string()), "cargar lee el archivo JSON");
    CHECK(cargo.titulo == "Opciones del juego", "cargar recupera el titulo");
    CHECK(cargo.ancho == ui.ancho && cargo.alto == ui.alto,
          "cargar recupera el tamano");
    CHECK(cargo.widgets.size() == 1, "cargar recupera los widgets");
    if (!cargo.widgets.empty()) {
        CHECK(cargo.widgets[0].nombre == "guardar",
              "cargar recupera el widget");
        CHECK(cargo.widgets[0].sonido == "guardar",
              "cargar recupera el sonido del widget");
    }

    // Cargar un nombre que no existe falla y no toca el modelo.
    UserInterfaceCustom inexistente;
    inexistente.nombre = "NoExiste";
    CHECK(!inexistente.cargar(base.string()),
          "cargar un nombre sin archivo falla");

    fs::remove_all(base);
}

void probarToleranciaAJsonParcial() {
    // Un widget sin "sonido" ni campos opcionales conserva sus defaults.
    nlohmann::json j = {{"nombre", "Basica"},
                        {"titulo", "Basica"},
                        {"widgets", nlohmann::json::array(
                                        {nlohmann::json::object(
                                            {{"tipo", "boton"},
                                             {"nombre", "ok"}})})}};

    UserInterfaceCustom ui;
    ui.desdeJson(j);
    CHECK(ui.nombre == "Basica", "parcial: nombre");
    CHECK(j.contains("widgets"), "parcial: widgets presentes");
    CHECK(ui.widgets.size() == 1, "parcial: un widget");
    if (!ui.widgets.empty()) {
        CHECK(ui.widgets[0].sonido.empty(),
              "parcial: sin sonido asignado (vacio == kSinSonido)");
        CHECK(ui.widgets[0].activado == false,
              "parcial: checkbox desactivado por defecto");
        CHECK(ui.widgets[0].valor == 0.0f,
              "parcial: valor por defecto");
    }

    // Un json no-objeto se rechaza sin tocar el modelo previo.
    nlohmann::json invalido = nlohmann::json::array({1, 2, 3});
    UserInterfaceCustom ui2;
    ui2.nombre = "Previo";
    ui2.desdeJson(invalido);
    CHECK(ui2.nombre == "Previo", "json no-objeto conserva el modelo");
    CHECK(ui2.widgets.empty(), "json no-objeto no agrega widgets");
}

}  // namespace

int main() {
    std::cout << "== USER INTERFACE TESTS ==" << std::endl;
    probarRoundTripJson();
    probarPersistencia();
    probarToleranciaAJsonParcial();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "USER INTERFACE TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}