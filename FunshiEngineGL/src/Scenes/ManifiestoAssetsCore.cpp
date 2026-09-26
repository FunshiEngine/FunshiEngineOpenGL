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
#include "ManifiestoAssetsCore.h"

#include <fstream>
#include <iostream>

#include "Configuracion/EditorConfig.h"
#include <nlohmann/json.hpp>

namespace {

// Version del formato del manifiesto: se incrementa al cambiar el esquema.
// El campo es solo informativo; la lectura no exige una version exacta.
constexpr int VERSION_MANIFIESTO = 1;

nlohmann::json entradaAJson(const EntradaAssets& entrada) {
    nlohmann::json objeto = nlohmann::json::object();
    if (!entrada.malla.empty())
        objeto["malla"] = entrada.malla;
    nlohmann::json texturas = nlohmann::json::array();
    for (const std::string& textura : entrada.texturas)
        texturas.push_back(textura);
    objeto["texturas"] = std::move(texturas);
    if (!entrada.script.empty())
        objeto["script"] = entrada.script;
    return objeto;
}

bool jsonAEntrada(const nlohmann::json& objeto, EntradaAssets& entrada) {
    if (!objeto.is_object())
        return false;
    if (objeto.contains("malla") && objeto["malla"].is_string())
        entrada.malla = objeto["malla"].get<std::string>();
    if (objeto.contains("script") && objeto["script"].is_string())
        entrada.script = objeto["script"].get<std::string>();
    if (objeto.contains("texturas") && objeto["texturas"].is_array()) {
        // Solo los slots presentes (hasta 4); los ausentes quedan vacios.
        const std::size_t slots =
            std::min<std::size_t>(objeto["texturas"].size(), entrada.texturas.size());
        for (std::size_t i = 0; i < slots; ++i)
            if (objeto["texturas"][i].is_string())
                entrada.texturas[i] = objeto["texturas"][i].get<std::string>();
    }
    return true;
}

} // namespace

bool ManifiestoAssetsCore::escribirArchivo(
    const std::string& ruta, const std::map<int, EntradaAssets>& entradas) {
    nlohmann::json documento;
    documento["version"] = VERSION_MANIFIESTO;
    nlohmann::json assets = nlohmann::json::object();
    for (const auto& [id, entrada] : entradas)
        assets[std::to_string(id)] = entradaAJson(entrada);
    documento["assets"] = std::move(assets);

    std::ofstream archivo(ruta, std::ios::trunc);
    if (!archivo.is_open()) {
        std::cerr << "ManifiestoAssets: no se pudo escribir " << ruta << '\n';
        return false;
    }
    archivo << documento.dump(2) << '\n';
    return archivo.good();
}

bool ManifiestoAssetsCore::leerArchivo(
    const std::string& ruta, std::map<int, EntradaAssets>& entradas) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open())
        return false;

    // Tolerante: un manifiesto corrupto nunca derriba la carga de la escena;
    // se ignora y se sigue con los paths que deja el .db.
    try {
        nlohmann::json documento;
        archivo >> documento;
        if (!documento.is_object() || !documento.contains("assets"))
            return false;
        for (const auto& [clave, valor] : documento["assets"].items()) {
            int id = 0;
            try {
                id = std::stoi(clave);
            } catch (const std::exception&) {
                continue;
            }
            EntradaAssets entrada;
            if (jsonAEntrada(valor, entrada))
                entradas[id] = std::move(entrada);
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void ManifiestoAssetsCore::relativizarEntrada(EntradaAssets& entrada) {
    entrada.malla = EditorConfig::relativizarRuta(entrada.malla);
    for (std::string& textura : entrada.texturas)
        textura = EditorConfig::relativizarRuta(textura);
    entrada.script = EditorConfig::relativizarRuta(entrada.script);
}

void ManifiestoAssetsCore::absolutizarEntrada(EntradaAssets& entrada) {
    entrada.malla = EditorConfig::absolutizarRuta(entrada.malla);
    for (std::string& textura : entrada.texturas)
        textura = EditorConfig::absolutizarRuta(textura);
    entrada.script = EditorConfig::absolutizarRuta(entrada.script);
}

std::string ManifiestoAssetsCore::resolverRuta(
    const std::string& persistida, const std::string& actual) {
    return (!persistida.empty() && persistida != actual) ? persistida : actual;
}

bool ManifiestoAssetsCore::entradaVacia(const EntradaAssets& entrada) {
    return entrada.malla.empty() && entrada.script.empty() &&
           entrada.texturas[0].empty() && entrada.texturas[1].empty() &&
           entrada.texturas[2].empty() && entrada.texturas[3].empty();
}