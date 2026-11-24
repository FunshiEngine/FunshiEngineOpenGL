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
#include "ModVersion.h"

#include <cctype>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace {

// Parsea un segmento numerico decimal sin signo. Rechaza vacios, no digitos y
// valores fuera del rango de int, para que el parseo no dependa de que
// std::stoi recorte silenciosamente.
bool parsearSegmento(const std::string& texto, int& salida) {
    if (texto.empty())
        return false;
    long long valor = 0;
    for (char c : texto) {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
        valor = valor * 10 + (c - '0');
        if (valor > std::numeric_limits<int>::max())
            return false;
    }
    salida = static_cast<int>(valor);
    return true;
}

// Divide un texto por espacios (una o mas separaciones). Devuelve los tokens
// no vacios en orden.
std::vector<std::string> separarPorEspacios(const std::string& texto) {
    std::vector<std::string> tokens;
    std::string actual;
    for (char c : texto) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!actual.empty()) {
                tokens.push_back(actual);
                actual.clear();
            }
        } else {
            actual.push_back(c);
        }
    }
    if (!actual.empty())
        tokens.push_back(actual);
    return tokens;
}

// Quita un prefijo ("">=" o "<") si el token empieza con el. Devuelve false si
// el token no empieza con ese prefijo.
bool quitarPrefijo(const std::string& token, const std::string& prefijo,
                   std::string& resto) {
    if (token.size() <= prefijo.size() ||
        token.compare(0, prefijo.size(), prefijo) != 0)
        return false;
    resto = token.substr(prefijo.size());
    return true;
}

} // namespace

bool ModVersion::parsear(const std::string& texto, ModVersion& salida) {
    // Exactamente tres segmentos: mayor.menor.parche.
    const std::size_t primerPunto = texto.find('.');
    if (primerPunto == std::string::npos)
        return false;
    const std::size_t segundoPunto = texto.find('.', primerPunto + 1);
    if (segundoPunto == std::string::npos)
        return false;
    if (texto.find('.', segundoPunto + 1) != std::string::npos)
        return false;

    ModVersion candidata;
    if (!parsearSegmento(texto.substr(0, primerPunto), candidata.mayor) ||
        !parsearSegmento(
            texto.substr(primerPunto + 1, segundoPunto - primerPunto - 1),
            candidata.menor) ||
        !parsearSegmento(texto.substr(segundoPunto + 1), candidata.parche))
        return false;

    candidata.valida = true;
    salida = candidata;
    return true;
}

int ModVersion::comparar(const ModVersion& otra) const {
    if (mayor != otra.mayor)
        return mayor < otra.mayor ? -1 : 1;
    if (menor != otra.menor)
        return menor < otra.menor ? -1 : 1;
    if (parche != otra.parche)
        return parche < otra.parche ? -1 : 1;
    return 0;
}

bool ModVersion::satisfaceMinimo(const ModVersion& minimo) const {
    if (!valida || !minimo.valida)
        return false;
    return comparar(minimo) >= 0;
}

std::string ModVersion::aTexto() const {
    return std::to_string(mayor) + "." + std::to_string(menor) + "." +
           std::to_string(parche);
}

bool RangoVersion::parsear(const std::string& texto, RangoVersion& salida) {
    const std::vector<std::string> tokens = separarPorEspacios(texto);
    if (tokens.empty() || tokens.size() > 2)
        return false;

    RangoVersion candidato;
    std::string versionMinimo;
    if (!quitarPrefijo(tokens[0], ">=", versionMinimo) ||
        !ModVersion::parsear(versionMinimo, candidato.minimo))
        return false;

    if (tokens.size() == 2) {
        std::string versionMaximo;
        if (!quitarPrefijo(tokens[1], "<", versionMaximo) ||
            !ModVersion::parsear(versionMaximo, candidato.maximo))
            return false;
        candidato.tieneMaximo = true;
    }

    candidato.valida = true;
    salida = candidato;
    return true;
}

bool RangoVersion::contiene(const ModVersion& version) const {
    if (!valida || !version.valida)
        return false;
    if (!version.satisfaceMinimo(minimo))
        return false;
    if (tieneMaximo && version.comparar(maximo) >= 0)
        return false;
    return true;
}

std::string RangoVersion::aTexto() const {
    std::string texto = ">=" + minimo.aTexto();
    if (tieneMaximo)
        texto += " <" + maximo.aTexto();
    return texto;
}
