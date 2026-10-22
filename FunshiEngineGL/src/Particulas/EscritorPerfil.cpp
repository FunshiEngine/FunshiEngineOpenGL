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
#include "EscritorPerfil.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>

bool guardarPerfilAJson(const std::string& rutaAbsoluta,
                        const PerfilParticula& perfil, std::string& error) {
    error.clear();

    // Las invariantes del lector se exigen aqui: el escritor es la puerta de
    // salida y no debe producir archivos que la carga rechace. Los mensajes
    // siguen el mismo estilo que los del lector para que el usuario vea la
    // misma explicacion en ambos extremos.
    if (perfil.vidaMinima <= 0.0f) {
        error = "la vida minima tiene que ser mayor que cero";
        return false;
    }
    if (perfil.vidaMinima > perfil.vidaMaxima) {
        error = "la vida minima no puede ser mayor que la maxima";
        return false;
    }
    if (perfil.velocidadMinima > perfil.velocidadMaxima) {
        error = "la velocidad minima no puede ser mayor que la maxima";
        return false;
    }
    if (perfil.rozamiento < 0.0f) {
        error = "el rozamiento no puede ser negativo";
        return false;
    }

    // Campos opcionales (textura, tamanos, colores) solo se escriben cuando
    // traen contenido: un perfil sin curvas es valido y el lector trata su
    // ausencia igual que un array vacio.
    nlohmann::json objeto;
    objeto["version"] = 1;
    objeto["nombre"] = perfil.nombre;
    if (!perfil.textura.empty()) objeto["textura"] = perfil.textura;
    objeto["vida"] = {perfil.vidaMinima, perfil.vidaMaxima};
    objeto["velocidad"] = {perfil.velocidadMinima, perfil.velocidadMaxima};
    objeto["aceleracion"] = {perfil.aceleracion[0], perfil.aceleracion[1],
                             perfil.aceleracion[2]};
    objeto["rozamiento"] = perfil.rozamiento;
    if (!perfil.tamanos.empty()) objeto["tamanos"] = perfil.tamanos;
    if (!perfil.colores.empty()) {
        nlohmann::json colores = nlohmann::json::array();
        for (const std::array<float, 4>& color : perfil.colores) {
            colores.push_back({color[0], color[1], color[2], color[3]});
        }
        objeto["colores"] = colores;
    }

    // La carpeta destino puede no existir todavia (p. ej. la primera vez que
    // el editor genera un perfil en una subcarpeta nueva del proyecto).
    std::error_code ec;
    const std::filesystem::path ruta(rutaAbsoluta);
    if (ruta.has_parent_path())
        std::filesystem::create_directories(ruta.parent_path(), ec);
    if (ec) {
        error = "no se pudo preparar la carpeta de '" + rutaAbsoluta + "'";
        return false;
    }

    std::ofstream archivo(rutaAbsoluta);
    if (!archivo.is_open()) {
        error = "no se pudo escribir el perfil '" + rutaAbsoluta + "'";
        return false;
    }
    archivo << objeto.dump(2) << "\n";
    archivo.close();
    if (archivo.fail()) {
        error = "no se pudo escribir el perfil '" + rutaAbsoluta + "'";
        return false;
    }
    return true;
}
