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
// Pruebas headless de EditorEventBus (el canal tipado de GUI interna):
// subscribe/publish/unsubscribe con payload declarativo y varios suscriptores.
// Sin pila grafica: solo std C++17 (EditoreventBus no toca ImGui/OpenGL).

#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/Events/EditorEventBus.h"

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
    // 1. Un suscriptor recibe la publicacion con el payload tipado intacto.
    {
        EditorEventBus bus;
        Apariencia ap;
        ap.temaClaro = true;
        ap.blancoYNegro = true;
        ap.acento[0] = 0.5f;
        ap.fondoSuperior[2] = 0.75f;
        ap.fondoInferior[2] = 0.75f;

        bool recibido = false;
        EditorEventType tipoRecibido = EditorEventType::VentanaEstadoCambio;
        Apariencia capturada;
        bus.subscribe([&](const EditorEvent& ev) {
            recibido = true;
            tipoRecibido = ev.type;
            capturada = ev.apariencia;
        });

        EditorEvent ev;
        ev.type = EditorEventType::AparienciaCambio;
        ev.apariencia = ap;
        bus.publish(ev);

        CHECK(recibido, "el suscriptor recibe la publicacion");
        CHECK(tipoRecibido == EditorEventType::AparienciaCambio,
              "se conserva el tipo del evento");
        CHECK(capturada.temaClaro == ap.temaClaro, "payload temas");
        CHECK(capturada.blancoYNegro == ap.blancoYNegro, "payload B/N");
        CHECK(capturada.acento[0] == ap.acento[0], "payload acento");
        CHECK(capturada.fondoSuperior[2] == ap.fondoSuperior[2],
              "payload fondoSuperior");
        CHECK(capturada.fondoInferior[2] == ap.fondoInferior[2],
              "payload fondoInferior");
    }

    // 2. Varios suscriptores reciben todos la misma publicacion.
    {
        EditorEventBus bus;
        int a = 0;
        int b = 0;
        bus.subscribe([&](const EditorEvent&) { ++a; });
        bus.subscribe([&](const EditorEvent&) { ++b; });

        EditorEvent ev;
        ev.type = EditorEventType::AparienciaCambio;
        bus.publish(ev);

        CHECK(a == 1, "suscriptor A recibio una vez");
        CHECK(b == 1, "suscriptor B recibio una vez");
    }

    // 3. VentanaEstadoCambio transporta nombre de ventana + abierta/cerrada.
    {
        EditorEventBus bus;
        std::string nombre;
        bool abierta = true;
        bus.subscribe([&](const EditorEvent& ev) {
            if (ev.type != EditorEventType::VentanaEstadoCambio) return;
            nombre = ev.nombreVentana ? ev.nombreVentana : "";
            abierta = ev.abierta;
        });

        EditorEvent ev;
        ev.type = EditorEventType::VentanaEstadoCambio;
        ev.nombreVentana = "Estado";
        ev.abierta = false;
        bus.publish(ev);

        CHECK(nombre == "Estado", "se transporta el nombre de ventana");
        CHECK(!abierta, "se transporta el estado cerrado");
    }

    // 4. unsubscribe deja de recibir publicaciones posteriores.
    {
        EditorEventBus bus;
        int contador = 0;
        const size_t token = bus.subscribe([&](const EditorEvent&) {
            ++contador;
        });

        EditorEvent ev;
        ev.type = EditorEventType::AparienciaCambio;
        bus.publish(ev);
        CHECK(contador == 1, "recibe la primera publicacion");

        bus.unsubscribe(token);
        bus.publish(ev);
        bus.publish(ev);
        CHECK(contador == 1, "tras unsubscribe no recibe mas");
    }

    // 5. Los tokens de suscripcion son unicos y el bus no se copia.
    {
        EditorEventBus bus;
        const size_t t1 = bus.subscribe([](const EditorEvent&) {});
        const size_t t2 = bus.subscribe([](const EditorEvent&) {});
        CHECK(t1 != t2, "tokens de suscripcion unicos");
    }

    // 6. SensibilidadCambio transporta el multiplicador del mouse look.
    {
        EditorEventBus bus;
        float recibida = 0.0f;
        bus.subscribe([&](const EditorEvent& ev) {
            if (ev.type != EditorEventType::SensibilidadCambio) return;
            recibida = ev.sensibilidad;
        });

        EditorEvent ev;
        ev.type = EditorEventType::SensibilidadCambio;
        ev.sensibilidad = 2.5f;
        bus.publish(ev);
        CHECK(recibida == 2.5f, "se transporta el multiplicador de sensibilidad");
    }

    // 7. ReiniciarConfiguracion llega sin payload y no confunde tipos.
    {
        EditorEventBus bus;
        bool reinicio = false;
        bool apariencia = false;
        bus.subscribe([&](const EditorEvent& ev) {
            if (ev.type == EditorEventType::ReiniciarConfiguracion) reinicio = true;
            if (ev.type == EditorEventType::AparienciaCambio) apariencia = true;
        });

        EditorEvent ev;
        ev.type = EditorEventType::ReiniciarConfiguracion;
        bus.publish(ev);
        CHECK(reinicio, "el evento de reset llega a sus suscriptores");
        CHECK(!apariencia, "no se confunde con AparienciaCambio");
    }

    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    std::cout << (fallos == 0 ? "EDITOREVENTBUS TESTS OK"
                              : "EDITOREVENTBUS TESTS FALLO")
              << std::endl;
    return fallos == 0 ? 0 : 1;
}