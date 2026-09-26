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
#include "ConfigPersistence.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <map>

namespace {

void escribirJson(const std::string& ruta, const nlohmann::json& j) {
    std::error_code ec;
    const std::string::size_type sep = ruta.find_last_of("/\\");
    if (sep != std::string::npos)
        std::filesystem::create_directories(ruta.substr(0, sep), ec);

    // Escritura atomica: primero a un temporal y despues rename encima del
    // destino. El truncate directo dejaba el JSON cortado si el proceso moria
    // a mitad de escritura (crash/corte de luz) y la configuracion se perdia;
    // con el rename el archivo original solo se reemplaza cuando el nuevo ya
    // esta completo en disco.
    const std::string temporal = ruta + ".tmp";
    {
        std::ofstream out(temporal, std::ios::trunc);
        if (!out.is_open()) return;
        out << j.dump(2) << '\n';
        out.flush();
        if (!out.good()) {
            out.close();
            std::filesystem::remove(temporal, ec);
            return;
        }
    }
    std::filesystem::rename(temporal, ruta, ec);
    if (!ec) return;

    // Respaldo si el rename fallo (p. ej. destino bloqueado en Windows): se
    // descarta el temporal y se reintenta la escritura directa.
    ec.clear();
    std::filesystem::remove(temporal, ec);
    std::ofstream outDirecto(ruta, std::ios::trunc);
    if (!outDirecto.is_open()) return;
    outDirecto << j.dump(2) << '\n';
}

nlohmann::json leerJson(const std::string& ruta) {
    std::ifstream in(ruta);
    if (!in.is_open()) return nlohmann::json::object();
    nlohmann::json j;
    try {
        in >> j;
    } catch (...) {
        return nlohmann::json::object();
    }
    return j.is_object() ? j : nlohmann::json::object();
}

} // namespace

void ConfigPersistence::escribirJson(const std::string& ruta, const nlohmann::json& j) {
    ::escribirJson(ruta, j);
}

nlohmann::json ConfigPersistence::leerJson(const std::string& ruta) {
    return ::leerJson(ruta);
}

nlohmann::json ConfigPersistence::aparienciaToJson(const Apariencia& a) {
    nlohmann::json j;
    j["temaClaro"] = a.temaClaro;
    j["blancoYNegro"] = a.blancoYNegro;
    j["acento"] = {a.acento[0], a.acento[1], a.acento[2], a.acento[3]};
    j["fondo"] = {a.fondo[0], a.fondo[1], a.fondo[2]};
    return j;
}

Apariencia ConfigPersistence::jsonToApariencia(const nlohmann::json& j) {
    Apariencia a;
    if (j.contains("temaClaro") && j["temaClaro"].is_boolean())
        a.temaClaro = j["temaClaro"].get<bool>();
    if (j.contains("blancoYNegro") && j["blancoYNegro"].is_boolean())
        a.blancoYNegro = j["blancoYNegro"].get<bool>();
    if (j.contains("acento") && j["acento"].is_array() && j["acento"].size() == 4) {
        for (int i = 0; i < 4; ++i)
            if (j["acento"][i].is_number())
                a.acento[i] = j["acento"][i].get<float>();
    }
    if (j.contains("fondo") && j["fondo"].is_array() && j["fondo"].size() == 3) {
        for (int i = 0; i < 3; ++i)
            if (j["fondo"][i].is_number())
                a.fondo[i] = j["fondo"][i].get<float>();
    }
    return a;
}

ConfigPersistence::General ConfigPersistence::cargarGeneral(const std::string& ruta) {
    General g;
    const std::string rutaReal = ruta.empty() ? ProjectPaths::rutaConfiguracionGeneral() : ruta;
    nlohmann::json j = leerJson(rutaReal);
    if (j.empty()) return g;

    // Compatibilidad: formato viejo con seccion "menu". Se lee ANTES de las
    // claves modernas (que van abajo y la pisan) y solo cuando no existe
    // "ultimoProyecto", igual que el lector historico de EditorConfig: si no,
    // el legacy ganaba sobre lo moderno y rompia la prioridad ya probada.
    if (!j.contains("ultimoProyecto") && j.contains("menu") && j["menu"].is_object()) {
        const auto& m = j["menu"];
        if (m.contains("proyecto") && m["proyecto"].is_string())
            g.ultimoProyecto = m["proyecto"].get<std::string>();
        if (m.contains("idioma") && m["idioma"].is_string())
            g.idioma = m["idioma"].get<std::string>();
        if (m.contains("sensibilidadCamara") && m["sensibilidadCamara"].is_number())
            g.sensibilidadCamara = m["sensibilidadCamara"].get<float>();
    }

    if (j.contains("version") && j["version"].is_number_integer())
        g.version = j["version"].get<int>();
    if (j.contains("ultimoProyecto") && j["ultimoProyecto"].is_string())
        g.ultimoProyecto = j["ultimoProyecto"].get<std::string>();
    if (j.contains("idioma") && j["idioma"].is_string())
        g.idioma = j["idioma"].get<std::string>();
    if (j.contains("sensibilidadCamara") && j["sensibilidadCamara"].is_number())
        g.sensibilidadCamara = j["sensibilidadCamara"].get<float>();
    if (j.contains("sensibilidadMovimientoCamara") && j["sensibilidadMovimientoCamara"].is_number())
        g.sensibilidadMovimientoCamara = j["sensibilidadMovimientoCamara"].get<float>();
    if (j.contains("apariencia") && j["apariencia"].is_object())
        g.apariencia = jsonToApariencia(j["apariencia"]);

    return g;
}

void ConfigPersistence::guardarGeneral(const General& g, const std::string& ruta) {
    const std::string rutaReal = ruta.empty() ? ProjectPaths::rutaConfiguracionGeneral() : ruta;
    nlohmann::json j;
    j["version"] = g.version;
    if (!g.ultimoProyecto.empty()) j["ultimoProyecto"] = g.ultimoProyecto;
    j["idioma"] = g.idioma;
    j["sensibilidadCamara"] = g.sensibilidadCamara;
    j["sensibilidadMovimientoCamara"] = g.sensibilidadMovimientoCamara;
    j["apariencia"] = aparienciaToJson(g.apariencia);
    escribirJson(rutaReal, j);
}

ConfigPersistence::Proyecto ConfigPersistence::cargarProyecto(const std::string& nombreProyecto,
                                                              const std::string& ruta) {
    Proyecto p;
    const std::string rutaReal = ruta.empty() ? ProjectPaths::rutaConfiguracionProyecto(nombreProyecto) : ruta;
    nlohmann::json j = leerJson(rutaReal);

    // Fallback: Configuracion.json viejo en Memory/
    if (j.empty() && ruta.empty()) {
        j = leerJson(ProjectPaths::directorioMemory(nombreProyecto) + "/Configuracion.json");
    }
    if (j.empty()) return p;

    if (j.contains("version") && j["version"].is_number_integer())
        p.version = j["version"].get<int>();

    if (j.contains("editor") && j["editor"].is_object()) {
        const auto& ed = j["editor"];
        if (ed.contains("ventanaCamarasAbierta") && ed["ventanaCamarasAbierta"].is_boolean())
            p.ventanaCamarasAbierta = ed["ventanaCamarasAbierta"].get<bool>();
        if (ed.contains("gizmoOperacion") && ed["gizmoOperacion"].is_number_integer())
            p.gizmoOperacion = ed["gizmoOperacion"].get<int>();
        if (ed.contains("gizmoGlobal") && ed["gizmoGlobal"].is_boolean())
            p.gizmoGlobal = ed["gizmoGlobal"].get<bool>();
        if (ed.contains("camaraActivaId") && ed["camaraActivaId"].is_number_integer())
            p.camaraActivaId = ed["camaraActivaId"].get<int>();
        if (ed.contains("ventanas") && ed["ventanas"].is_object()) {
            for (auto it = ed["ventanas"].begin(); it != ed["ventanas"].end(); ++it) {
                if (it.value().is_boolean())
                    p.estadoVentanas[it.key()] = it.value().get<bool>();
            }
        }
    }
    return p;
}

void ConfigPersistence::guardarProyecto(const Proyecto& p, const std::string& nombreProyecto,
                                        const std::string& ruta) {
    const std::string rutaReal = ruta.empty() ? ProjectPaths::rutaConfiguracionProyecto(nombreProyecto) : ruta;
    nlohmann::json j;
    j["version"] = p.version;
    j["editor"]["ventanaCamarasAbierta"] = p.ventanaCamarasAbierta;
    j["editor"]["gizmoOperacion"] = p.gizmoOperacion;
    j["editor"]["gizmoGlobal"] = p.gizmoGlobal;
    j["editor"]["camaraActivaId"] = p.camaraActivaId;
    for (const auto& [nombre, abierta] : p.estadoVentanas)
        j["editor"]["ventanas"][nombre] = abierta;
    escribirJson(rutaReal, j);
}
