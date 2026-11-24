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
#include "PropiedadesComponentesDetalle.h"

namespace MotorInspeccion {

CatalogoPropiedades::CatalogoPropiedades() {
    // Cada familia de componentes registra sus propiedades en su unidad
    // (PropiedadesComponentes<Familia>.cpp).
    registrarEscena();
    registrarFisica();
    registrarRender();
    registrarInterfaz();


    for (auto& par : mapa_) orden_.push_back(par.first);
    std::sort(orden_.begin(), orden_.end());
}

void CatalogoPropiedades::registrar(const std::string& tipo,
                                     std::vector<DefPropiedad> propiedades) {
    if (propiedades.empty()) return;
    mapa_[tipo] = std::move(propiedades);
}

const std::vector<DefPropiedad>* CatalogoPropiedades::de(
    const std::string& tipo) const {
    const auto encontrado = mapa_.find(tipo);
    if (encontrado == mapa_.end()) return nullptr;
    return &encontrado->second;
}

const DefPropiedad* CatalogoPropiedades::buscar(const std::string& tipo,
                                                 const std::string& variable) const {
    const std::vector<DefPropiedad>* propiedades = de(tipo);
    if (!propiedades) return nullptr;
    for (const DefPropiedad& p : *propiedades) {
        if (p.nombre.size() != variable.size()) continue;
        bool iguales = true;
        for (std::size_t i = 0; i < p.nombre.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(p.nombre[i])) !=
                std::tolower(static_cast<unsigned char>(variable[i]))) {
                iguales = false;
                break;
            }
        }
        if (iguales) return &p;
    }
    return nullptr;
}

const CatalogoPropiedades& CatalogoPropiedades::instancia() {
    static const CatalogoPropiedades catalogo;
    return catalogo;
}

}  // namespace MotorInspeccion
