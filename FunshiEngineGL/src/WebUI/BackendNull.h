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
#ifndef BACKENDNULLWEBUI_H
#define BACKENDNULLWEBUI_H

#include "IWebUIBackend.h"

// Backend de interfaz web sin motor (no-op): permite montar y probar todo el
// ciclo de una vista (cargar/actualizar/render/input/descargar) sin CEF ni GL
// (tests headless, CI) y sin romper la app cuando no hay backend real.
//
// cargar() tiene exito y deja la vista "cargada" pero SIN textura: render()
// devuelve 0, handleImGui queda nulo y el overlay no la dibuja. El resto del
// motor no distingue este caso del de un frame que todavia no esta listo.
class BackendNullWebUI : public IWebUIBackend {
public:
    const char* nombre() const override { return "null"; }

    // Sin motor web: no hay subproceso, no se arranca nada, no hay bucle que
    // bombear y no hay motor web que apagar.
    int ejecutarSubproceso(int, char**) override { return -1; }
    bool inicializar(int, char**) override { return true; }
    void bombear() override {}
    void apagar() override {}

    bool cargar(VistaWebUI& vista, const std::string& rutaHtml, int ancho,
                int alto, std::string& error) override {
        vista.ruta = rutaHtml;
        vista.backend = nombre();
        vista.manejador = nullptr;
        vista.textura = 0;
        vista.handleImGui = nullptr;
        vista.ancho = ancho;
        vista.alto = alto;
        vista.cargada = true;
        vista.error.clear();
        error.clear();
        return true;
    }

    void redimensionar(VistaWebUI& vista, int ancho, int alto) override {
        vista.ancho = ancho;
        vista.alto = alto;
    }

    void actualizar(VistaWebUI&, float) override {}

    void establecerEstado(VistaWebUI&, const std::string&) override {}
    std::string leerSalida(VistaWebUI&) override { return std::string(); }

    std::uint64_t render(VistaWebUI&) override { return 0; }

    void ratonMover(VistaWebUI&, int, int) override {}
    void ratonBoton(VistaWebUI&, BotonRatonWebUI, bool) override {}
    void ratonRueda(VistaWebUI&, float) override {}
    void tecla(VistaWebUI&, int, bool, bool) override {}
    void foco(VistaWebUI&, bool) override {}

    void descargar(VistaWebUI& vista) override { vista = VistaWebUI{}; }
};

#endif // BACKENDNULLWEBUI_H
