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
#ifndef PROPIEDADESCOMPONENTES_H
#define PROPIEDADESCOMPONENTES_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "TextoValor.h"

namespace MotorInspeccion {

// Una variable de un componente que la API de inspeccion puede leer y
// escribir. El acceso siempre pasa por el getter y el setter publicos del
// componente: los componentes del motor guardan su estado en copias
// sincronizadas por sus propios setters (por ejemplo Transform), asi que
// escribir en el dato interno desincronizaria el render.
struct DefPropiedad {
    std::string nombre;   // identificador estable ("masa")
    std::string etiqueta; // texto para mostrar ("Masa")
    TipoValor tipo = TipoValor::Flotante;
    // Valores admitidos, solo para TipoValor::Enumeracion.
    std::vector<std::string> admitidos;
    // Devuelve el valor actual en forma de texto.
    std::function<std::string(void*)> leer;
    // Comprueba que el texto se puede interpretar, sin tocar el componente.
    // Devuelve false con el motivo si no.
    std::function<bool(const std::string&, std::string&)> validar;
    // Interpreta el texto y lo escribe. Devuelve false con el motivo si el
    // motor no acepta el valor.
    std::function<bool(void*, const std::string&, std::string&)> escribir;
};

// Catalogo de las variables de cada tipo de componente. El motor no tenia
// ninguna lista de este tipo: el inspector de la GUI edita cada componente con
// su panel y su propio cast, asi que el catalogo se construye aqui una sola
// vez y es la unica fuente que consultan la API y la CLI.
class CatalogoPropiedades {
public:
    static const CatalogoPropiedades& instancia();

    // Propiedades del tipo indicado (nombre canonico del componente).
    // Devuelve nullptr cuando ese tipo no expone variables.
    const std::vector<DefPropiedad>* de(const std::string& tipo) const;

    // Busca una variable por nombre dentro de un tipo de componente. La
    // busqueda no distingue mayusculas de minusculas.
    const DefPropiedad* buscar(const std::string& tipo,
                               const std::string& variable) const;

    // Tipos de componente que exponen variables, en orden alfabetico.
    const std::vector<std::string>& tipos() const { return orden_; }

private:
    CatalogoPropiedades();

    void registrar(const std::string& tipo,
                   std::vector<DefPropiedad> propiedades);

    std::vector<std::string> orden_;
    std::unordered_map<std::string, std::vector<DefPropiedad>> mapa_;
};

}  // namespace MotorInspeccion

#endif
