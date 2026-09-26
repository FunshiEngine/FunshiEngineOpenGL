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
#include "UserInterfaceCustom.h"

#include "../../../External/nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace {
const char* tipoAString(TipoWidget tipo) {
    switch (tipo) {
        case TipoWidget::Etiqueta: return "etiqueta";
        case TipoWidget::Boton: return "boton";
        case TipoWidget::Checkbox: return "checkbox";
        case TipoWidget::Slider: return "slider";
        case TipoWidget::EntradaTexto: return "entrada_texto";
    }
    return "etiqueta";
}

TipoWidget stringATipo(const std::string& s) {
    if (s == "boton") return TipoWidget::Boton;
    if (s == "checkbox") return TipoWidget::Checkbox;
    if (s == "slider") return TipoWidget::Slider;
    if (s == "entrada_texto") return TipoWidget::EntradaTexto;
    return TipoWidget::Etiqueta;
}
} // namespace

nlohmann::json UserInterfaceCustom::aJson() const {
    nlohmann::json j;
    j["nombre"] = nombre;
    j["titulo"] = titulo;
    j["ancho"] = ancho;
    j["alto"] = alto;
    j["widgets"] = nlohmann::json::array();
    for (const WidgetUI& w : widgets) {
        nlohmann::json jw;
        jw["tipo"] = tipoAString(w.tipo);
        jw["nombre"] = w.nombre;
        jw["etiqueta"] = w.etiqueta;
        jw["sonido"] = w.sonido;
        jw["valor"] = w.valor;
        jw["minimo"] = w.minimo;
        jw["maximo"] = w.maximo;
        jw["activado"] = w.activado;
        jw["texto"] = w.texto;
        j["widgets"].push_back(std::move(jw));
    }
    return j;
}

void UserInterfaceCustom::desdeJson(const nlohmann::json& j) {
    if (j.contains("nombre") && j["nombre"].is_string())
        nombre = j["nombre"].get<std::string>();
    if (j.contains("titulo") && j["titulo"].is_string())
        titulo = j["titulo"].get<std::string>();
    if (j.contains("ancho") && j["ancho"].is_number())
        ancho = j["ancho"].get<float>();
    if (j.contains("alto") && j["alto"].is_number())
        alto = j["alto"].get<float>();

    widgets.clear();
    if (!j.contains("widgets") || !j["widgets"].is_array()) return;
    for (const auto& jw : j["widgets"]) {
        if (!jw.is_object()) continue;
        WidgetUI w;
        if (jw.contains("tipo") && jw["tipo"].is_string())
            w.tipo = stringATipo(jw["tipo"].get<std::string>());
        if (jw.contains("nombre") && jw["nombre"].is_string())
            w.nombre = jw["nombre"].get<std::string>();
        if (jw.contains("etiqueta") && jw["etiqueta"].is_string())
            w.etiqueta = jw["etiqueta"].get<std::string>();
        if (jw.contains("sonido") && jw["sonido"].is_string())
            w.sonido = jw["sonido"].get<std::string>();
        if (jw.contains("valor") && jw["valor"].is_number())
            w.valor = jw["valor"].get<float>();
        if (jw.contains("minimo") && jw["minimo"].is_number())
            w.minimo = jw["minimo"].get<float>();
        if (jw.contains("maximo") && jw["maximo"].is_number())
            w.maximo = jw["maximo"].get<float>();
        if (jw.contains("activado") && jw["activado"].is_boolean())
            w.activado = jw["activado"].get<bool>();
        if (jw.contains("texto") && jw["texto"].is_string())
            w.texto = jw["texto"].get<std::string>();
        widgets.push_back(std::move(w));
    }
}

bool UserInterfaceCustom::guardar(const std::string& directorio) const {
    if (nombre.empty()) return false;
    std::error_code ec;
    std::filesystem::create_directories(directorio, ec);
    std::ofstream out(directorio + "/" + nombre + ".json");
    if (!out.is_open()) return false;
    out << aJson().dump(2) << '\n';
    return out.good();
}

bool UserInterfaceCustom::cargar(const std::string& directorio) {
    std::ifstream in(directorio + "/" + nombre + ".json");
    if (!in.is_open()) return false;
    nlohmann::json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }
    if (!j.is_object()) return false;
    desdeJson(j);
    return true;
}