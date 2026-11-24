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
#ifndef LOCALIZACION_LOCALIZACION_H
#define LOCALIZACION_LOCALIZACION_H

#include <string>
#include <vector>

// Textos del editor en varios idiomas.
//
// Un idioma se identifica por su CODIGO ("es", "en"), nunca por el nombre que
// se muestra: renombrar la etiqueta del combo no rompe la traduccion ni la
// configuracion guardada. Las claves son identificadores estables
// ("iniciar_estudio", "volver"...) y sus textos viven en Textos/<Idioma>.cpp.
//
// Respaldo: si el idioma pedido no tiene la clave se usa el espanol, y si
// tampoco existe se devuelve la clave tal cual (la interfaz nunca queda en
// blanco). En debug, una clave inexistente se avisa una sola vez por stderr.
namespace Localizacion {

struct Idioma {
    const char* codigo; // "es"
    const char* nombre; // "Espanol" (lo que muestra el combo)
};

// Codigo del idioma base y de respaldo.
inline constexpr const char* kIdiomaBase = "es";

// Idiomas disponibles, en el orden en que se ofrecen.
const std::vector<Idioma>& idiomas();

// Codigo valido a partir de un codigo o de un nombre viejo guardado en la
// configuracion ("Espanol", "English"...). Un valor desconocido o vacio da
// el idioma base.
std::string normalizarIdioma(const std::string& valor);

// Nombre visible del idioma (el codigo si no se conoce).
std::string nombreIdioma(const std::string& codigo);

// Texto de la clave en el idioma indicado (con respaldo, ver arriba).
std::string texto(const std::string& codigo, const std::string& clave);

// true si la clave existe en el idioma base.
bool existeClave(const std::string& clave);

} // namespace Localizacion

#endif
