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
#include "TablasTextos.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace Localizacion {
namespace {

using Tabla = std::unordered_map<std::string, std::string>;

struct IdiomaRegistrado {
    Idioma idioma;
    const std::vector<Detalle::EntradaTexto>& (*textos)();
};

// Primer elemento: el idioma base. Agregar un idioma = una fila mas.
const std::vector<IdiomaRegistrado>& registrados() {
    static const std::vector<IdiomaRegistrado> lista = {
        {{"es", "Espanol"}, &Detalle::textosEs},
        {{"en", "English"}, &Detalle::textosEn},
    };
    return lista;
}

// Tabla hash por idioma, armada una sola vez (busqueda O(1) por frame).
const Tabla* tablaDe(const std::string& codigo) {
    static const std::unordered_map<std::string, Tabla> tablas = [] {
        std::unordered_map<std::string, Tabla> t;
        for (const IdiomaRegistrado& r : registrados()) {
            Tabla& tabla = t[r.idioma.codigo];
            for (const Detalle::EntradaTexto& e : r.textos())
                tabla.emplace(e.clave, e.texto);
        }
        return t;
    }();
    auto it = tablas.find(codigo);
    return it != tablas.end() ? &it->second : nullptr;
}

std::string minusculas(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

void avisarClaveFaltante(const std::string& clave) {
#ifndef NDEBUG
    static std::mutex mutex;
    static std::unordered_set<std::string> avisadas;
    std::lock_guard<std::mutex> lock(mutex);
    if (avisadas.insert(clave).second)
        std::cerr << "[localizacion] clave sin texto: '" << clave << "'" << std::endl;
#else
    (void)clave;
#endif
}

} // namespace

const std::vector<Idioma>& idiomas() {
    static const std::vector<Idioma> lista = [] {
        std::vector<Idioma> v;
        for (const IdiomaRegistrado& r : registrados()) v.push_back(r.idioma);
        return v;
    }();
    return lista;
}

std::string normalizarIdioma(const std::string& valor) {
    const std::string v = minusculas(valor);
    for (const Idioma& i : idiomas()) {
        if (v == i.codigo || v == minusculas(i.nombre)) return i.codigo;
    }
    // Nombres que pudo guardar una version vieja del editor o un usuario a
    // mano en el JSON de configuracion.
    static const std::unordered_map<std::string, std::string> alias = {
        {"espa\xc3\xb1ol", "es"}, {"spanish", "es"}, {"castellano", "es"},
        {"ingles", "en"},          {"english", "en"},
    };
    auto it = alias.find(v);
    return it != alias.end() ? it->second : kIdiomaBase;
}

std::string nombreIdioma(const std::string& codigo) {
    for (const Idioma& i : idiomas()) {
        if (codigo == i.codigo) return i.nombre;
    }
    return codigo;
}

std::string texto(const std::string& codigo, const std::string& clave) {
    if (const Tabla* t = tablaDe(codigo)) {
        auto it = t->find(clave);
        if (it != t->end()) return it->second;
    }
    if (const Tabla* base = tablaDe(kIdiomaBase)) {
        auto it = base->find(clave);
        if (it != base->end()) return it->second;
    }
    avisarClaveFaltante(clave);
    return clave;
}

bool existeClave(const std::string& clave) {
    const Tabla* base = tablaDe(kIdiomaBase);
    return base && base->count(clave) > 0;
}

} // namespace Localizacion
