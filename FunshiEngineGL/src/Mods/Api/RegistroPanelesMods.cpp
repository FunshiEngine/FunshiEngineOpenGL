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
#include "RegistroPanelesMods.h"

namespace FunshiMods {

namespace {
// Prefijo de las claves de ventana de los paneles de mods. Con prefijo, la
// ventana de un mod nunca choca con una ventana del editor ni con la de otro
// mod (el nombre de la ventana es la clave con la que se persiste su estado
// y con la que ImGui la identifica).
constexpr const char* PREFIJO_VENTANA = "Mod:";
} // namespace

RegistroPanelesMods& RegistroPanelesMods::global() {
    static RegistroPanelesMods registro;
    return registro;
}

std::string RegistroPanelesMods::nombreVentanaDe(const std::string& modId,
                                                  const std::string& id) {
    return std::string(PREFIJO_VENTANA) + modId + ":" + id;
}

bool RegistroPanelesMods::registrar(const std::string& modId,
                                     const std::string& id,
                                     const std::string& titulo,
                                     CallbackPanelMod dibujar, void* datos) {
    if (modId.empty() || id.empty() || titulo.empty() || !dibujar) return false;

    for (const PanelMod& panel : paneles_) {
        if (panel.modId == modId && panel.id == id) return false;
    }

    PanelMod panel;
    panel.modId = modId;
    panel.id = id;
    panel.titulo = titulo;
    panel.nombreVentana = nombreVentanaDe(modId, id);
    panel.dibujar = dibujar;
    panel.datos = datos;
    paneles_.push_back(std::move(panel));
    return true;
}

bool RegistroPanelesMods::registrarDelModEnEjecucion(
    const char* id, const char* titulo, CallbackPanelMod dibujar, void* datos) {
    if (modEnEjecucion_.empty()) return false;
    if (!id || !titulo) return false;
    return registrar(modEnEjecucion_, id, titulo, dibujar, datos);
}

void RegistroPanelesMods::quitarMod(const std::string& modId) {
    for (auto iterador = paneles_.begin(); iterador != paneles_.end();) {
        if (iterador->modId == modId) {
            iterador = paneles_.erase(iterador);
        } else {
            ++iterador;
        }
    }
    if (modEnEjecucion_ == modId) modEnEjecucion_.clear();
}

void RegistroPanelesMods::limpiar() {
    paneles_.clear();
    modEnEjecucion_.clear();
}

const PanelMod* RegistroPanelesMods::buscarPorVentana(
    const std::string& nombreVentana) const {
    for (const PanelMod& panel : paneles_) {
        if (panel.nombreVentana == nombreVentana) return &panel;
    }
    return nullptr;
}

bool RegistroPanelesMods::establecerEstado(const std::string& nombreVentana,
                                            bool abierta) {
    for (PanelMod& panel : paneles_) {
        if (panel.nombreVentana == nombreVentana) {
            panel.abierta = abierta;
            return true;
        }
    }
    return false;
}

void RegistroPanelesMods::fijarModEnEjecucion(const std::string& modId) {
    modEnEjecucion_ = modId;
}

void RegistroPanelesMods::limpiarModEnEjecucion() { modEnEjecucion_.clear(); }

} // namespace FunshiMods
