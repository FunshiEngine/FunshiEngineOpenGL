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
#include "ManifiestoMod.h"

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace {

using Json = nlohmann::json;

// Campo obligatorio de tipo cadena no vacia. Devuelve false y arma el mensaje
// con el nombre del campo si falta, no es cadena o esta vacio.
bool leerCadenaObligatoria(const Json& objeto, const char* clave,
                           std::string& destino, std::string& error) {
    if (!objeto.contains(clave)) {
        error = std::string("falta el campo obligatorio '") + clave + "'";
        return false;
    }
    if (!objeto[clave].is_string()) {
        error = std::string("el campo '") + clave + "' debe ser una cadena";
        return false;
    }
    destino = objeto[clave].get<std::string>();
    if (destino.empty()) {
        error = std::string("el campo '") + clave + "' no puede estar vacio";
        return false;
    }
    return true;
}

// Campo opcional de tipo cadena; si esta presente debe ser cadena (vacia esta
// permitida y se copia tal cual).
bool leerCadenaOpcional(const Json& objeto, const char* clave,
                        std::string& destino, std::string& error) {
    if (!objeto.contains(clave))
        return true;
    if (!objeto[clave].is_string()) {
        error = std::string("el campo '") + clave + "' debe ser una cadena";
        return false;
    }
    destino = objeto[clave].get<std::string>();
    return true;
}

// El formato del id es "Autor.Nombre": un unico punto y ambas partes no vacias.
// Un id estable sostiene las dependencias y los conflictos entre mods.
bool idConFormatoValido(const std::string& id) {
    const std::size_t punto = id.find('.');
    if (punto == std::string::npos || punto == 0 || punto + 1 >= id.size())
        return false;
    return id.find('.', punto + 1) == std::string::npos;
}

bool parsearOrden(const std::string& texto, DependenciaMod::Orden& salida,
                  std::string& error) {
    if (texto == "ninguno") {
        salida = DependenciaMod::Orden::Ninguno;
        return true;
    }
    if (texto == "antes") {
        salida = DependenciaMod::Orden::Antes;
        return true;
    }
    if (texto == "despues") {
        salida = DependenciaMod::Orden::Despues;
        return true;
    }
    error = "el campo 'orden' de una dependencia solo admite "
            "'antes', 'despues' o 'ninguno' (valor: '" + texto + "')";
    return false;
}

// Traduce una dependencia y valida sus campos. `indice` se usa en el mensaje
// para ubicar la que falla.
bool parsearDependencia(const Json& objeto, std::size_t indice,
                        DependenciaMod& salida, std::string& error) {
    if (!objeto.is_object()) {
        error = "la dependencia " + std::to_string(indice) + " no es un objeto";
        return false;
    }
    const std::string prefijo = "dependencias[" + std::to_string(indice) + "].";

    std::string id;
    if (!leerCadenaObligatoria(objeto, "id", id, error))
        return false;
    if (!idConFormatoValido(id)) {
        error = "el id de " + prefijo + "id no respeta el formato Autor.Nombre";
        return false;
    }
    salida.id = id;

    if (objeto.contains("versionMin")) {
        if (!objeto["versionMin"].is_string()) {
            error = prefijo + "versionMin debe ser una cadena";
            return false;
        }
        salida.versionMin = objeto["versionMin"].get<std::string>();
        ModVersion minima;
        if (!ModVersion::parsear(salida.versionMin, minima)) {
            error = prefijo + "versionMin no es una version semver (X.Y.Z)";
            return false;
        }
    }

    if (objeto.contains("requerida")) {
        if (!objeto["requerida"].is_boolean()) {
            error = prefijo + "requerida debe ser booleano";
            return false;
        }
        salida.requerida = objeto["requerida"].get<bool>();
    }

    if (objeto.contains("orden")) {
        if (!objeto["orden"].is_string()) {
            error = prefijo + "orden debe ser una cadena";
            return false;
        }
        if (!parsearOrden(objeto["orden"].get<std::string>(), salida.orden,
                          error))
            return false;
    }
    return true;
}

} // namespace

bool ManifiestoMod::parsear(const std::string& json, ManifiestoMod& salida,
                            std::string& error) {
    error.clear();

    Json documento;
    try {
        documento = Json::parse(json);
    } catch (const Json::parse_error& ex) {
        error = std::string("JSON invalido: ") + ex.what();
        return false;
    }
    if (!documento.is_object()) {
        error = "la raiz del manifiesto debe ser un objeto JSON";
        return false;
    }

    // Cortafuegos del esquema: se exige y se compara antes que nada, para no
    // leer con reglas viejas un formato que no es el que esta build entiende.
    if (!documento.contains("esquemaVersion")) {
        error = "falta el campo obligatorio 'esquemaVersion'";
        return false;
    }
    if (!documento["esquemaVersion"].is_number_integer()) {
        error = "el campo 'esquemaVersion' debe ser un entero";
        return false;
    }
    const int esquema = documento["esquemaVersion"].get<int>();
    if (esquema != ESQUEMA_VERSION_ACTUAL) {
        error = "esquemaVersion " + std::to_string(esquema) +
                " desconocido (esta build entiende " +
                std::to_string(ESQUEMA_VERSION_ACTUAL) + ")";
        return false;
    }

    ManifiestoMod candidato;
    candidato.esquemaVersion = esquema;

    std::string campo;
    if (!leerCadenaObligatoria(documento, "id", campo, error))
        return false;
    if (!idConFormatoValido(campo)) {
        error = "el campo 'id' debe respetar el formato Autor.Nombre";
        return false;
    }
    candidato.id = campo;

    if (!leerCadenaObligatoria(documento, "nombre", candidato.nombre, error))
        return false;

    if (!leerCadenaObligatoria(documento, "version", candidato.version,
                               error))
        return false;
    ModVersion versionMod;
    if (!ModVersion::parsear(candidato.version, versionMod)) {
        error = "el campo 'version' no es una version semver (X.Y.Z)";
        return false;
    }

    if (!leerCadenaObligatoria(documento, "tipo", candidato.tipo, error))
        return false;
    if (candidato.tipo != "nativo") {
        error = "el campo 'tipo' solo admite \"nativo\" en la v1 (valor: '" +
                candidato.tipo + "')";
        return false;
    }

    if (!leerCadenaObligatoria(documento, "puntoEntrada",
                               candidato.puntoEntrada, error))
        return false;

    if (!leerCadenaOpcional(documento, "descripcion", candidato.descripcion,
                            error))
        return false;
    if (!leerCadenaOpcional(documento, "autor", candidato.autor, error))
        return false;
    if (!leerCadenaOpcional(documento, "licencia", candidato.licencia, error))
        return false;
    if (!leerCadenaOpcional(documento, "icono", candidato.icono, error))
        return false;

    if (!leerCadenaOpcional(documento, "versionMotor", candidato.versionMotor,
                            error))
        return false;
    if (!candidato.versionMotor.empty()) {
        RangoVersion rango;
        if (!RangoVersion::parsear(candidato.versionMotor, rango)) {
            error = "el campo 'versionMotor' no es un rango semver valido "
                    "(se espera \">=X.Y.Z\" o \">=X.Y.Z <X.Y.Z\")";
            return false;
        }
    }

    if (documento.contains("dependencias")) {
        if (!documento["dependencias"].is_array()) {
            error = "el campo 'dependencias' debe ser un arreglo";
            return false;
        }
        std::size_t indice = 0;
        for (const auto& dependencia : documento["dependencias"]) {
            DependenciaMod leida;
            if (!parsearDependencia(dependencia, indice, leida, error))
                return false;
            candidato.dependencias.push_back(std::move(leida));
            ++indice;
        }
    }

    if (documento.contains("conflictos")) {
        if (!documento["conflictos"].is_array()) {
            error = "el campo 'conflictos' debe ser un arreglo";
            return false;
        }
        for (const auto& conflicto : documento["conflictos"]) {
            if (!conflicto.is_string()) {
                error = "cada entrada de 'conflictos' debe ser un id (cadena)";
                return false;
            }
            candidato.conflictos.push_back(conflicto.get<std::string>());
        }
    }

    salida = std::move(candidato);
    return true;
}

bool ManifiestoMod::leerDeArchivo(const std::string& ruta, ManifiestoMod& salida,
                                  std::string& error) {
    error.clear();
    std::ifstream archivo(ruta, std::ios::binary);
    if (!archivo) {
        error = "no se pudo abrir el manifiesto: " + ruta;
        return false;
    }
    std::ostringstream contenido;
    contenido << archivo.rdbuf();
    return parsear(contenido.str(), salida, error);
}
