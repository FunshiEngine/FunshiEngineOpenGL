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
#include "TextoValor.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

namespace MotorInspeccion {

namespace {

std::string recortar(const std::string& texto) {
    std::size_t inicio = 0;
    std::size_t fin = texto.size();
    while (inicio < fin && std::isspace(static_cast<unsigned char>(texto[inicio])))
        ++inicio;
    while (fin > inicio && std::isspace(static_cast<unsigned char>(texto[fin - 1])))
        --fin;
    return texto.substr(inicio, fin - inicio);
}

std::string minusculas(const std::string& texto) {
    std::string salida = texto;
    std::transform(salida.begin(), salida.end(), salida.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return salida;
}

// Separa por espacios y comas, descartando los separadores sobrantes.
std::vector<std::string> separarNumeros(const std::string& texto) {
    std::vector<std::string> partes;
    std::string actual;
    for (char c : texto) {
        if (std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == ';') {
            if (!actual.empty()) {
                partes.push_back(actual);
                actual.clear();
            }
        } else {
            actual.push_back(c);
        }
    }
    if (!actual.empty()) partes.push_back(actual);
    return partes;
}

}  // namespace

const char* nombreDeTipoValor(TipoValor tipo) {
    switch (tipo) {
    case TipoValor::Entero: return "Entero";
    case TipoValor::Flotante: return "Flotante";
    case TipoValor::Booleano: return "Booleano";
    case TipoValor::Texto: return "Texto";
    case TipoValor::Vec2: return "Vec2";
    case TipoValor::Vec3: return "Vec3";
    case TipoValor::Vec4: return "Vec4";
    case TipoValor::Enumeracion: return "Enumeracion";
    }
    return "Desconocido";
}

std::string textoEntero(int valor) {
    return std::to_string(valor);
}

std::string textoFlotante(float valor) {
    if (std::isnan(valor)) return "nan";
    if (std::isinf(valor)) return valor > 0.0f ? "inf" : "-inf";
    char buffer[64];
    // Se prueban precisiones crecientes y se queda con la primera que
    // reproduce exactamente el mismo float al releerse.
    for (int precision = 6; precision <= 9; ++precision) {
        std::snprintf(buffer, sizeof(buffer), "%.*g", precision,
                      static_cast<double>(valor));
        if (std::strtof(buffer, nullptr) == valor) return std::string(buffer);
    }
    return std::string(buffer);
}

std::string textoBooleano(bool valor) {
    return valor ? "true" : "false";
}

std::string textoVec2(float x, float y) {
    return textoFlotante(x) + " " + textoFlotante(y);
}

std::string textoVec3(float x, float y, float z) {
    return textoFlotante(x) + " " + textoFlotante(y) + " " + textoFlotante(z);
}

std::string textoVec4(float x, float y, float z, float w) {
    return textoVec3(x, y, z) + " " + textoFlotante(w);
}

bool leerEntero(const std::string& texto, int& valor, std::string& error) {
    const std::string limpio = recortar(texto);
    if (limpio.empty()) {
        error = "el valor esta vacio";
        return false;
    }
    char* fin = nullptr;
    errno = 0;
    const long largo = std::strtol(limpio.c_str(), &fin, 10);
    if (fin == limpio.c_str() || *fin != '\0') {
        error = "'" + limpio + "' no es un numero entero";
        return false;
    }
    if (errno == ERANGE || largo < std::numeric_limits<int>::min() ||
        largo > std::numeric_limits<int>::max()) {
        error = "'" + limpio + "' esta fuera del rango de un entero";
        return false;
    }
    valor = static_cast<int>(largo);
    return true;
}

bool leerFlotante(const std::string& texto, float& valor, std::string& error) {
    const std::string limpio = recortar(texto);
    if (limpio.empty()) {
        error = "el valor esta vacio";
        return false;
    }
    char* fin = nullptr;
    const float convertido = std::strtof(limpio.c_str(), &fin);
    if (fin == limpio.c_str() || *fin != '\0') {
        error = "'" + limpio + "' no es un numero";
        return false;
    }
    // Un NaN o un infinito en la posicion de un objeto rompe la matriz de
    // transformacion (ver decomposeMatrixToTransform): se rechazan aqui.
    if (!std::isfinite(convertido)) {
        error = "'" + limpio + "' no es un numero finito";
        return false;
    }
    valor = convertido;
    return true;
}

bool leerBooleano(const std::string& texto, bool& valor, std::string& error) {
    const std::string limpio = minusculas(recortar(texto));
    if (limpio == "true" || limpio == "1") {
        valor = true;
        return true;
    }
    if (limpio == "false" || limpio == "0") {
        valor = false;
        return true;
    }
    error = "'" + recortar(texto) + "' no es un booleano (true o false)";
    return false;
}

bool leerVec2(const std::string& texto, float& x, float& y, std::string& error) {
    const std::vector<std::string> partes = separarNumeros(texto);
    if (partes.size() != 2) {
        error = "se esperaban 2 numeros separados por espacio, se recibieron " +
                std::to_string(partes.size());
        return false;
    }
    float valores[2] = {0.0f, 0.0f};
    for (std::size_t i = 0; i < 2; ++i) {
        if (!leerFlotante(partes[i], valores[i], error)) return false;
    }
    x = valores[0];
    y = valores[1];
    return true;
}

bool leerVec3(const std::string& texto, float& x, float& y, float& z,
              std::string& error) {
    const std::vector<std::string> partes = separarNumeros(texto);
    if (partes.size() != 3) {
        error = "se esperaban 3 numeros separados por espacio, se recibieron " +
                std::to_string(partes.size());
        return false;
    }
    float valores[3] = {0.0f, 0.0f, 0.0f};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!leerFlotante(partes[i], valores[i], error)) return false;
    }
    x = valores[0];
    y = valores[1];
    z = valores[2];
    return true;
}

bool leerVec4(const std::string& texto, float& x, float& y, float& z, float& w,
              std::string& error) {
    const std::vector<std::string> partes = separarNumeros(texto);
    if (partes.size() != 4) {
        error = "se esperaban 4 numeros separados por espacio, se recibieron " +
                std::to_string(partes.size());
        return false;
    }
    float valores[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (std::size_t i = 0; i < 4; ++i) {
        if (!leerFlotante(partes[i], valores[i], error)) return false;
    }
    x = valores[0];
    y = valores[1];
    z = valores[2];
    w = valores[3];
    return true;
}

}  // namespace MotorInspeccion
