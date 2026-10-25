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
#include "PreprocesadorShaders.h"

#include "ShaderException.h"

#include <sstream>
#include <unordered_set>
#include <vector>

namespace Rendering {
namespace {

// Tope de anidamiento de includes. Un fuente con includes encadenados mas
// profundo que esto es un error de escribir, no algo que deba colgarse.
constexpr int kNidamientoMaximo = 16;

// Quita los espacios al principio y al final de una linea.
std::string recortar(const std::string& texto) {
    const std::size_t inicio = texto.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) return std::string();
    const std::size_t fin = texto.find_last_not_of(" \t\r\n");
    return texto.substr(inicio, fin - inicio + 1);
}

// Extrae el nombre de un #include "nombre". Devuelve vacio si la linea no es
// un include con comillas (o si el nombre no cierra).
std::string nombreDeInclude(const std::string& linea) {
    if (linea.compare(0, 8, "#include") != 0) return std::string();
    const std::size_t comilla = linea.find('"', 8);
    if (comilla == std::string::npos) return std::string();
    const std::size_t cierre = linea.find('"', comilla + 1);
    if (cierre == std::string::npos) return std::string();
    return linea.substr(comilla + 1, cierre - comilla - 1);
}

// Cuenta las lineas que produce el texto (para ajustar la numeracion).
std::size_t contarLineas(const std::string& texto) {
    std::size_t lineas = 0;
    for (char c : texto)
        if (c == '\n') ++lineas;
    return lineas;
}

std::string preprocesarInterno(const std::string& fuente,
                                const PreprocesadorShaders::Fragmentos& fragmentos,
                                std::unordered_set<std::string>& enCurso,
                                int profundidad, const std::string& nombreOrigen);

}  // namespace

std::string PreprocesadorShaders::preprocesar(
    const std::string& fuente, const Fragmentos& fragmentos) {
    std::unordered_set<std::string> enCurso;
    return preprocesarInterno(fuente, fragmentos, enCurso, 0, "shader");
}

namespace {

std::string preprocesarInterno(const std::string& fuente,
                                const PreprocesadorShaders::Fragmentos& fragmentos,
                                std::unordered_set<std::string>& enCurso,
                                int profundidad, const std::string& nombreOrigen) {
    if (profundidad > kNidamientoMaximo)
        throw ShaderIncludeException(
            "El shader supera el tope de anidamiento de includes (" +
            std::to_string(kNidamientoMaximo) + ") en " + nombreOrigen);

    std::istringstream entrada(fuente);
    std::string linea;
    std::ostringstream salida;
    int numeroLinea = 0;

    while (std::getline(entrada, linea)) {
        ++numeroLinea;
        // El include se reconoce con espacios delante: el GLSL lo permite.
        const std::string recortada = recortar(linea);
        const std::string nombre = nombreDeInclude(recortada);
        if (nombre.empty()) {
            salida << linea << '\n';
            continue;
        }

        const auto it = fragmentos.find(nombre);
        if (it == fragmentos.end())
            throw ShaderIncludeException(
                "El shader incluye un fragmento que no existe: " + nombre +
                " (en " + nombreOrigen + ", linea " +
                std::to_string(numeroLinea) + ")");

        if (enCurso.count(nombre) > 0)
            throw ShaderIncludeException(
                "El include de shaders es ciclico: " + nombre + " se incluye a si mismo (en " +
                nombreOrigen + ")");

        enCurso.insert(nombre);
        const std::string incluido = preprocesarInterno(
            it->second, fragmentos, enCurso, profundidad + 1, nombre);
        enCurso.erase(nombre);

        // El #line devuelve la numeracion al fragmento origen: sin el, un
        // error del driver senalaria una linea del fuente compuesto.
        salida << "#line 1 \"" << nombre << "\"\n";
        salida << incluido;
        if (incluido.empty() || incluido.back() != '\n') salida << '\n';
        salida << "#line " << (numeroLinea + 1) << " \"" << nombreOrigen
               << "\"\n";
    }

    return salida.str();
}

}  // namespace
}  // namespace Rendering
