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
#include "PanelesModsInterface.h"

#include "../../Mods/Api/RegistroPanelesMods.h"

PanelesModsInterface::PanelesModsInterface() { sincronizar(); }

void PanelesModsInterface::sincronizar() {
    auto& registro = FunshiMods::RegistroPanelesMods::global();

    // Quitar los envoltorios cuyo panel ya no esta en el registro (el mod se
    // descargo y sus panels se fueron con el).
    for (auto iterador = paneles_.begin(); iterador != paneles_.end();) {
        if (!registro.buscarPorVentana((*iterador)->getNameGui())) {
            iterador = paneles_.erase(iterador);
        } else {
            ++iterador;
        }
    }

    // Crear los envoltorios de los paneles que aun no tienen uno.
    for (const FunshiMods::PanelMod& panel : registro.paneles()) {
        bool existe = false;
        for (const auto& envoltorio : paneles_) {
            if (envoltorio->getNameGui() == panel.nombreVentana) {
                existe = true;
                break;
            }
        }
        if (existe) continue;
        paneles_.push_back(std::make_unique<PanelModInterface>(
            panel.nombreVentana, panel.titulo));
    }
}

void PanelesModsInterface::printGUI() {
    sincronizar();
    for (auto& panel : paneles_) {
        panel->printGUI();
    }
}

std::vector<std::pair<std::string, std::string>>
PanelesModsInterface::etiquetas() const {
    std::vector<std::pair<std::string, std::string>> resultado;
    for (const FunshiMods::PanelMod& panel :
         FunshiMods::RegistroPanelesMods::global().paneles()) {
        resultado.emplace_back(panel.nombreVentana, panel.titulo);
    }
    return resultado;
}
