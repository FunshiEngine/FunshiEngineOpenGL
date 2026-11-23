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
#include "LectorPerfil.h"

#include <array>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace {

// Lee un array de 'cantidad' numeros. Devuelve false si el campo no existe o
// no tiene la forma esperada.
bool leerNumeros(const nlohmann::json& objeto, const char* campo,
                 std::size_t cantidad, std::vector<float>& salida,
                 std::string& error) {
    if (!objeto.contains(campo)) return true;  // opcional
    const nlohmann::json& valor = objeto[campo];
    if (!valor.is_array() || valor.size() != cantidad) {
        error = std::string("el campo '") + campo + "' debe ser un array de " +
                std::to_string(cantidad) + " numeros";
        return false;
    }
    salida.clear();
    for (const auto& elemento : valor) {
        if (!elemento.is_number()) {
            error = std::string("el campo '") + campo +
                    "' tiene un valor que no es un numero";
            return false;
        }
        salida.push_back(elemento.get<float>());
    }
    return true;
}

}  // namespace

bool cargarPerfilDesdeJson(const std::string& rutaAbsoluta, PerfilParticula& out,
                            std::string& error) {
    std::ifstream archivo(rutaAbsoluta);
    if (!archivo.is_open()) {
        error = "no se pudo abrir el perfil '" + rutaAbsoluta + "'";
        return false;
    }

    nlohmann::json objeto;
    try {
        archivo >> objeto;
    } catch (const std::exception& excepcion) {
        error = "el perfil '" + rutaAbsoluta + "' no es un JSON valido: " +
                excepcion.what();
        return false;
    }
    if (!objeto.is_object()) {
        error = "el perfil '" + rutaAbsoluta + "' no es un objeto JSON";
        return false;
    }

    PerfilParticula perfil;

    if (objeto.contains("nombre") && objeto["nombre"].is_string())
        perfil.nombre = objeto["nombre"].get<std::string>();
    if (objeto.contains("textura") && objeto["textura"].is_string())
        perfil.textura = objeto["textura"].get<std::string>();

    std::vector<float> numeros;
    if (!leerNumeros(objeto, "vida", 2, numeros, error)) return false;
    if (numeros.size() == 2) {
        if (numeros[0] > numeros[1]) {
            error = "en el campo 'vida' el minimo no puede ser mayor que el maximo";
            return false;
        }
        if (numeros[0] <= 0.0f) {
            error = "en el campo 'vida' el minimo tiene que ser mayor que cero";
            return false;
        }
        perfil.vidaMinima = numeros[0];
        perfil.vidaMaxima = numeros[1];
    }

    numeros.clear();
    if (!leerNumeros(objeto, "velocidad", 2, numeros, error)) return false;
    if (numeros.size() == 2) {
        if (numeros[0] > numeros[1]) {
            error = "en el campo 'velocidad' el minimo no puede ser mayor que el maximo";
            return false;
        }
        perfil.velocidadMinima = numeros[0];
        perfil.velocidadMaxima = numeros[1];
    }

    numeros.clear();
    if (!leerNumeros(objeto, "aceleracion", 3, numeros, error)) return false;
    if (numeros.size() == 3) {
        for (int i = 0; i < 3; ++i) perfil.aceleracion[i] = numeros[static_cast<std::size_t>(i)];
    }

    if (objeto.contains("rozamiento")) {
        const nlohmann::json& valor = objeto["rozamiento"];
        if (!valor.is_number()) {
            error = "el campo 'rozamiento' no es un numero";
            return false;
        }
        if (valor.get<float>() < 0.0f) {
            error = "el campo 'rozamiento' no puede ser negativo";
            return false;
        }
        perfil.rozamiento = valor.get<float>();
    }

    if (objeto.contains("tamanos")) {
        const nlohmann::json& valor = objeto["tamanos"];
        if (!valor.is_array() || valor.empty()) {
            error = "el campo 'tamanos' debe ser un array con al menos un valor";
            return false;
        }
        for (const auto& elemento : valor) {
            if (!elemento.is_number()) {
                error = "el campo 'tamanos' tiene un valor que no es un numero";
                return false;
            }
            perfil.tamanos.push_back(elemento.get<float>());
        }
    }

    if (objeto.contains("colores")) {
        const nlohmann::json& valor = objeto["colores"];
        if (!valor.is_array() || valor.empty()) {
            error = "el campo 'colores' debe ser un array con al menos un color";
            return false;
        }
        for (const auto& elemento : valor) {
            if (!elemento.is_array() || elemento.size() != 4) {
                error = "cada color debe ser un array de 4 numeros (RGBA)";
                return false;
            }
            std::array<float, 4> color{};
            for (std::size_t i = 0; i < 4; ++i) {
                if (!elemento[i].is_number()) {
                    error = "un color tiene un valor que no es un numero";
                    return false;
                }
                color[i] = elemento[i].get<float>();
            }
            perfil.colores.push_back(color);
        }
    }

    out = perfil;
    return true;
}
