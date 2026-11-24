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
#include "Localizacion.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace Localizacion {
namespace {

// Idiomas en el orden de sus bloques.
#define TEXTOS_BLOQUE -1
#define TEXTO(nombre, texto)
#define IDIOMA(numero, codigo, nombre) Idioma{numero, codigo, nombre},
constexpr Idioma kIdiomas[] = {
#include "Textos.inc"
};
#undef IDIOMA
#undef TEXTO

constexpr std::size_t kCantidadIdiomas = sizeof(kIdiomas) / sizeof(kIdiomas[0]);

// La tabla posicional: los bloques uno detras del otro, tal como estan en
// Textos.inc. El texto i del idioma n esta en kTextos[n * N + i].
#define IDIOMA(numero, codigo, nombre)
#define TEXTO(nombre, texto) texto,
constexpr const char* kTextos[] = {
#include "Textos.inc"
};
#undef TEXTO

// El nombre de cada TEXTO, en el mismo orden que kTextos: sirve para que el
// compilador compruebe que todos los bloques coinciden.
#define TEXTO(nombre, texto) Texto::nombre,
constexpr Texto kOrden[] = {
#include "Textos.inc"
};
#undef TEXTO
#undef IDIOMA
#undef TEXTOS_BLOQUE

// Nombres de los textos (bloque 0).
#define TEXTOS_BLOQUE 0
#define IDIOMA(numero, codigo, nombre)
#define TEXTO(nombre, texto) #nombre,
constexpr const char* kNombres[] = {
#include "Textos.inc"
};
#undef TEXTO
#undef IDIOMA
#undef TEXTOS_BLOQUE

constexpr bool idiomasEnOrden() {
    for (std::size_t n = 0; n < kCantidadIdiomas; ++n) {
        if (kIdiomas[n].numero != n) return false;
    }
    return true;
}

// Cada bloque repite los mismos textos en la misma posicion.
constexpr bool bloquesAlineados() {
    for (std::size_t p = 0; p < sizeof(kOrden) / sizeof(kOrden[0]); ++p) {
        if (static_cast<std::size_t>(kOrden[p]) != p % kCantidadTextos) return false;
    }
    return true;
}

static_assert(kCantidadIdiomas > 0, "Textos.inc no define ningun IDIOMA");
static_assert(idiomasEnOrden(),
              "Textos.inc: los IDIOMA deben numerarse 0, 1, 2... en orden");
static_assert(sizeof(kTextos) / sizeof(kTextos[0]) == kCantidadIdiomas * kCantidadTextos,
              "Textos.inc: todos los bloques deben tener la misma cantidad de textos");
static_assert(bloquesAlineados(),
              "Textos.inc: un bloque tiene los textos en otro orden que el bloque 0");

std::string minusculas(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

} // namespace

std::size_t cantidadIdiomas() { return kCantidadIdiomas; }

const Idioma& idioma(std::size_t numero) {
    return kIdiomas[numero < kCantidadIdiomas ? numero : 0];
}

std::size_t numeroIdioma(const std::string& codigo) {
    for (const Idioma& i : kIdiomas) {
        if (codigo == i.codigo) return i.numero;
    }
    return 0;
}

std::string normalizarIdioma(const std::string& valor) {
    const std::string v = minusculas(valor);
    for (const Idioma& i : kIdiomas) {
        if (v == i.codigo || v == minusculas(i.nombre)) return i.codigo;
    }
    // Nombres que pudo guardar una version vieja del editor o un usuario a
    // mano en el JSON de configuracion.
    static const std::unordered_map<std::string, std::string> alias = {
        {"espa\xc3\xb1ol", "es"}, {"spanish", "es"}, {"castellano", "es"},
        {"ingles", "en"},
    };
    auto it = alias.find(v);
    return it != alias.end() ? it->second : kIdiomaBase;
}

std::string nombreIdioma(const std::string& codigo) {
    for (const Idioma& i : kIdiomas) {
        if (codigo == i.codigo) return i.nombre;
    }
    return codigo;
}

const char* texto(std::size_t numeroIdioma, Texto texto) {
    const std::size_t i = static_cast<std::size_t>(texto);
    if (i >= kCantidadTextos) return "";
    if (numeroIdioma >= kCantidadIdiomas) numeroIdioma = 0;
    const char* t = kTextos[numeroIdioma * kCantidadTextos + i];
    return t[0] != '\0' ? t : kTextos[i];
}

const char* texto(const std::string& codigoIdioma, Texto t) {
    return texto(numeroIdioma(codigoIdioma), t);
}

const char* nombre(Texto texto) {
    const std::size_t i = static_cast<std::size_t>(texto);
    return i < kCantidadTextos ? kNombres[i] : "";
}

} // namespace Localizacion
