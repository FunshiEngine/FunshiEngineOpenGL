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

#include <cstddef>
#include <string>

// Textos del motor en varios idiomas, en formato posicional.
//
// Todos los textos viven en Textos.inc, en un bloque por idioma. Los idiomas
// se numeran en orden (0 = espanol, 1 = ingles, ...) y cada bloque tiene los
// mismos N textos en el mismo orden, asi que el texto i del idioma n esta en
// la posicion n * N + i de una sola tabla. El codigo nombra cada texto con un
// valor de Texto (su posicion i), nunca con un numero suelto.
//
// La configuracion guarda el CODIGO del idioma ("es", "en"), no el numero ni
// el nombre: reordenar o renombrar idiomas no rompe lo guardado.
namespace Localizacion {

// Posicion de cada texto dentro de un bloque (generado del bloque 0).
#define TEXTOS_BLOQUE 0
#define IDIOMA(numero, codigo, nombre)
#define TEXTO(nombre, texto) nombre,
enum class Texto : std::size_t {
#include "Textos.inc"
};
#undef TEXTO

// N: cantidad de textos de cada idioma (la distancia entre bloques).
#define TEXTO(nombre, texto) +1
inline constexpr std::size_t kCantidadTextos = 0
#include "Textos.inc"
    ;
#undef TEXTO
#undef IDIOMA
#undef TEXTOS_BLOQUE

struct Idioma {
    std::size_t numero; // orden del bloque en Textos.inc
    const char* codigo; // "es" (lo que se guarda en la configuracion)
    const char* nombre; // "Espanol" (lo que muestra el combo)
};

// Idioma 0: base y respaldo.
inline constexpr const char* kIdiomaBase = "es";

std::size_t cantidadIdiomas();
// Idioma por numero (un numero fuera de rango da el idioma 0).
const Idioma& idioma(std::size_t numero);

// Numero del idioma con ese codigo (0 si no se conoce).
std::size_t numeroIdioma(const std::string& codigo);

// Codigo valido a partir de un codigo o de un nombre guardado por versiones
// viejas ("Espanol", "English"...). Desconocido o vacio da el idioma base.
std::string normalizarIdioma(const std::string& valor);

// Nombre visible del idioma (el codigo si no se conoce).
std::string nombreIdioma(const std::string& codigo);

// Texto en la posicion numeroIdioma * N + texto. Si ese idioma lo deja vacio
// o el numero no existe, devuelve el del idioma 0. Nunca nullptr.
const char* texto(std::size_t numeroIdioma, Texto texto);
const char* texto(const std::string& codigoIdioma, Texto texto);

// Nombre del texto en Textos.inc ("IniciarEstudio"); sirve como ID estable de
// ImGui ("Texto##IniciarEstudio") y para mensajes de diagnostico.
const char* nombre(Texto texto);

} // namespace Localizacion

#endif
