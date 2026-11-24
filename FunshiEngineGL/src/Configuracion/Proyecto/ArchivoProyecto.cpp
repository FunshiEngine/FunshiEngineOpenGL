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
#include "ArchivoProyecto.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "../ProjectPaths.h"

#ifndef FUNSHI_VERSION_STR
#define FUNSHI_VERSION_STR "0.0.0"
#endif

namespace fs = std::filesystem;

namespace ArchivoProyecto {

namespace {

// Un .fnep es chico: uno mas grande que esto no es un .fnep.
constexpr std::uintmax_t kTamanoMaximo = 64 * 1024;
constexpr const char* kExtension = ".fnep";

bool mismaRuta(const fs::path& a, const fs::path& b) {
    std::error_code ec1, ec2;
    const fs::path ca = fs::weakly_canonical(a, ec1);
    const fs::path cb = fs::weakly_canonical(b, ec2);
    if (ec1 || ec2) return a.lexically_normal() == b.lexically_normal();
    return ca == cb;
}

}  // namespace

std::string rutaDe(const std::string& nombreProyecto) {
    return (fs::path(ProjectPaths::directorioProyecto(nombreProyecto)) /
            (nombreProyecto + kExtension)).string();
}

bool escribir(const std::string& ruta, const Datos& datos, std::string& error) {
    nlohmann::json j;
    j["formato"] = "fnep";
    j["version"] = kVersionFormato;
    j["nombre"] = datos.nombre;
    j["motor"] = datos.motor.empty() ? std::string(FUNSHI_VERSION_STR) : datos.motor;

    const fs::path destino(ruta);
    const fs::path temporal = destino.string() + ".tmp";
    {
        std::ofstream salida(temporal, std::ios::binary | std::ios::trunc);
        if (!salida) {
            error = "no se pudo escribir " + temporal.string();
            return false;
        }
        salida << j.dump(2) << '\n';
        if (!salida) {
            error = "fallo la escritura de " + temporal.string();
            return false;
        }
    }
    std::error_code ec;
    fs::rename(temporal, destino, ec);
    if (ec) {
        fs::remove(temporal, ec);
        error = "no se pudo reemplazar " + destino.string();
        return false;
    }
    return true;
}

std::optional<Datos> leer(const std::string& ruta, std::string& error) {
    std::error_code ec;
    if (!fs::is_regular_file(ruta, ec)) {
        error = "no existe el archivo " + ruta;
        return std::nullopt;
    }
    if (fs::file_size(ruta, ec) > kTamanoMaximo || ec) {
        error = "el archivo es demasiado grande para ser un .fnep";
        return std::nullopt;
    }
    std::ifstream entrada(ruta, std::ios::binary);
    std::stringstream texto;
    texto << entrada.rdbuf();
    const nlohmann::json j = nlohmann::json::parse(texto.str(), nullptr, false);
    if (j.is_discarded() || !j.is_object()) {
        error = "el archivo no es JSON valido";
        return std::nullopt;
    }
    if (j.value("formato", std::string()) != "fnep") {
        error = "el archivo no es un proyecto de FunshiEngine (.fnep)";
        return std::nullopt;
    }
    Datos datos;
    datos.version = j.value("version", 0);
    if (datos.version < 1 || datos.version > kVersionFormato) {
        error = "version de .fnep no soportada (" + std::to_string(datos.version) +
                "): actualiza FunshiEngine";
        return std::nullopt;
    }
    datos.nombre = j.value("nombre", std::string());
    datos.motor = j.value("motor", std::string());
    if (datos.nombre.empty() || !ProjectPaths::esNombreValido(datos.nombre)) {
        error = "el .fnep no tiene un nombre de proyecto valido";
        return std::nullopt;
    }
    return datos;
}

bool asegurar(const std::string& nombreProyecto, std::string& error) {
    const std::string ruta = rutaDe(nombreProyecto);
    std::error_code ec;
    if (fs::exists(ruta, ec)) return true;
    if (!fs::is_directory(ProjectPaths::directorioProyecto(nombreProyecto), ec)) {
        error = "no existe la carpeta del proyecto";
        return false;
    }
    Datos datos;
    datos.nombre = nombreProyecto;
    return escribir(ruta, datos, error);
}

std::optional<std::string> resolverApertura(const std::string& rutaFnep,
                                            std::string& error) {
    const std::optional<Datos> datos = leer(rutaFnep, error);
    if (!datos) return std::nullopt;
    // El .fnep vive en la carpeta de su proyecto: si se copio a otro lado, la
    // carpeta en la que esta no es la del proyecto y no se abre otro por error.
    const fs::path carpeta = fs::path(rutaFnep).parent_path();
    if (!mismaRuta(carpeta, ProjectPaths::directorioProyecto(datos->nombre))) {
        error = "el .fnep de '" + datos->nombre +
                "' no esta en la carpeta del proyecto (" +
                ProjectPaths::directorioProyecto(datos->nombre) + ")";
        return std::nullopt;
    }
    return datos->nombre;
}

}  // namespace ArchivoProyecto
