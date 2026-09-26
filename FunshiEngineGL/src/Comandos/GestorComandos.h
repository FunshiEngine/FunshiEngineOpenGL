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
#ifndef GESTOR_COMANDOS_H
#define GESTOR_COMANDOS_H

#include <memory>
#include <vector>
#include <string>

#include "IComando.h"

class GestorComandos {
private:
    std::vector<std::unique_ptr<IComando>> historialDeshacer;
    std::vector<std::unique_ptr<IComando>> historialRehacer;
    size_t maxHistorial = 50;

public:
    GestorComandos() = default;
    ~GestorComandos() = default;

    void ejecutar(std::unique_ptr<IComando> comando) {
        comando->ejecutar();
        historialDeshacer.push_back(std::move(comando));
        if (historialDeshacer.size() > maxHistorial) {
            historialDeshacer.erase(historialDeshacer.begin());
        }
        historialRehacer.clear();
    }

    // Devuelven la descripcion del comando sobre el que se aplico, o cadena
    // vacia si la pila estaba vacia: permite que la UI avise (barra de estado)
    // que cambio tomo el estado, igual que hace Ctrl+S al guardar.
    std::string deshacer() {
        if (historialDeshacer.empty()) return std::string();
        auto cmd = std::move(historialDeshacer.back());
        historialDeshacer.pop_back();
        const std::string descripcion = cmd->descripcion();
        cmd->deshacer();
        historialRehacer.push_back(std::move(cmd));
        return descripcion;
    }

    std::string rehacer() {
        if (historialRehacer.empty()) return std::string();
        auto cmd = std::move(historialRehacer.back());
        historialRehacer.pop_back();
        const std::string descripcion = cmd->descripcion();
        cmd->ejecutar();
        historialDeshacer.push_back(std::move(cmd));
        return descripcion;
    }

    bool puedeDeshacer() const noexcept { return !historialDeshacer.empty(); }
    bool puedeRehacer() const noexcept { return !historialRehacer.empty(); }

    void limpiar() {
        historialDeshacer.clear();
        historialRehacer.clear();
    }

    size_t getMaxHistorial() const noexcept { return maxHistorial; }
    void setMaxHistorial(size_t max) noexcept { maxHistorial = max; }
};

#endif