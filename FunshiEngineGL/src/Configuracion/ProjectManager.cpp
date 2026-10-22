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
#include "ProjectManager.h"

#include <filesystem>
#include <algorithm>

namespace {

std::error_code makeEc() { return {}; }

bool crearDirs(const std::string& path, std::error_code& ec) {
    std::filesystem::create_directories(path, ec);
    return !ec;
}

bool existeDir(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

} // namespace

void ProjectManager::migrarProyectosAntiguos() {
    const std::string base = ProjectPaths::directorioBase();
    const std::string proyectsDir = ProjectPaths::directorioProyects();
    std::error_code ec;

    std::filesystem::create_directories(proyectsDir, ec);

    for (const auto& entry : std::filesystem::directory_iterator(base, ec)) {
        if (!entry.is_directory(ec)) continue;
        const std::string nombre = entry.path().filename().string();
        if (ProjectPaths::esNombreValido(nombre) && !existeDir(proyectsDir + "/" + nombre)) {
            std::filesystem::rename(entry.path(), proyectsDir + "/" + nombre, ec);
        }
    }
}

void ProjectManager::migrarConfiguracionGlobal() {
    const std::string base = ProjectPaths::directorioBase();
    const std::string oldConfig = base + "/Configuracion.json";
    const std::string newConfig = ProjectPaths::rutaConfiguracionGeneral();
    std::error_code ec;

    if (std::filesystem::exists(oldConfig, ec) && !std::filesystem::exists(newConfig, ec)) {
        std::filesystem::create_directories(ProjectPaths::directorioConfiguraciones(), ec);
        std::filesystem::copy_file(oldConfig, newConfig,
                                   std::filesystem::copy_options::overwrite_existing, ec);
    }
}

bool ProjectManager::crearEstructuraProyecto(const std::string& nombre) {
    std::error_code ec;
    const std::string mem = ProjectPaths::directorioMemory(nombre);
    const std::string src = ProjectPaths::directorioSrc(nombre);
    const std::string binScene = ProjectPaths::rutaSceneDir(nombre);

    // Migracion de proyectos legacy que guardaban el descriptor y la escena
    // directamente en la raiz de FunshiEngine-DATA/Binarios (estructura anterior a
    // Proyects/).
    const std::string base = ProjectPaths::directorioBase();
    const std::string oldBBDD = base + "/Binarios/SceneBBDDObjetos.txt";
    const std::string newBBDD = ProjectPaths::directorioBinarios(nombre) + "/SceneBBDDObjetos.txt";
    if (!std::filesystem::exists(newBBDD, ec) && std::filesystem::exists(oldBBDD, ec)) {
        std::filesystem::copy_file(oldBBDD, newBBDD,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        const std::string oldSceneDir = base + "/Binarios/Scene";
        if (std::filesystem::exists(oldSceneDir, ec)) {
            std::filesystem::copy(oldSceneDir, binScene,
                                  std::filesystem::copy_options::recursive |
                                  std::filesystem::copy_options::overwrite_existing, ec);
        }
    }

    // Solo la estructura que el motor usa: los datos del proyecto (Memory) y
    // la raiz de assets (src<nombre>). Las carpetas de assets del usuario
    // (sonidos, scripts, mods...) las crea el usuario, desde el explorador,
    // cuando las necesita.
    return crearDirs(mem + "/Binarios/Scene", ec) &&
           crearDirs(src, ec) &&
           crearDirs(binScene, ec);
}

void ProjectManager::asegurarEstructuraBase() {
    std::error_code ec;
    std::filesystem::create_directories(ProjectPaths::directorioProyects(), ec);
    std::filesystem::create_directories(ProjectPaths::directorioConfiguraciones(), ec);
    std::filesystem::create_directories(ProjectPaths::directorioExportaciones(), ec);
    // Carpeta global de mods del motor: el descubrimiento la recorre al
    // arrancar y el usuario deja ahi los .fmod que instala a mano.
    std::filesystem::create_directories(ProjectPaths::directorioMods(), ec);

    migrarProyectosAntiguos();
    migrarConfiguracionGlobal();
}

std::vector<ProjectManager::Proyecto> ProjectManager::descubrirProyectos() {
    std::vector<Proyecto> lista;
    const std::string proyectsDir = ProjectPaths::directorioProyects();
    std::error_code ec;

    if (!std::filesystem::exists(proyectsDir, ec)) return lista;

    for (const auto& entry : std::filesystem::directory_iterator(proyectsDir, ec)) {
        if (!entry.is_directory(ec)) continue;
        const std::string nombre = entry.path().filename().string();
        if (!ProjectPaths::esNombreValido(nombre)) continue;

        Proyecto p;
        p.nombre = nombre;
        p.ruta = entry.path().string();
        p.rutaSrc = ProjectPaths::directorioSrc(nombre);
        p.valido = esProyectoValido(nombre);
        lista.push_back(std::move(p));
    }
    return lista;
}

std::optional<ProjectManager::Proyecto> ProjectManager::buscarProyecto(const std::string& nombre) {
    if (!ProjectPaths::esNombreValido(nombre)) return std::nullopt;
    const std::string ruta = ProjectPaths::directorioProyecto(nombre);
    if (!existeDir(ruta)) return std::nullopt;

    Proyecto p;
    p.nombre = nombre;
    p.ruta = ruta;
    p.rutaSrc = ProjectPaths::directorioSrc(nombre);
    p.valido = esProyectoValido(nombre);
    return p;
}

bool ProjectManager::crearProyecto(const std::string& nombre) {
    if (!ProjectPaths::esNombreValido(nombre)) return false;

    const std::string ruta = ProjectPaths::directorioProyecto(nombre);
    if (!existeDir(ruta)) {
        if (!crearEstructuraProyecto(nombre)) return false;
        return true;
    }

    // Proyecto ya existe: se completa la estructura que le falta.
    return crearEstructuraProyecto(nombre);
}

bool ProjectManager::crearProyectoNuevo(const std::string& nombre,
                                        std::error_code& error) {
    namespace fs = std::filesystem;
    error.clear();
    if (!ProjectPaths::esNombreValido(nombre) || nombre == "." || nombre == "..") {
        error = std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    const fs::path raiz(ProjectPaths::directorioProyects());
    const fs::path proyecto(ProjectPaths::directorioProyecto(nombre));
    fs::create_directories(raiz, error);
    if (error) return false;

    // create_directory reserva el nombre de forma atomica: no entra a reparar ni
    // migrar el contenido si otro proyecto ya ocupa esa ruta.
    const bool reservado = fs::create_directory(proyecto, error);
    if (error || !reservado) {
        if (!error) error = std::make_error_code(std::errc::file_exists);
        return false;
    }

    // El motor solo crea sus dos carpetas: los datos del proyecto (Memory) y
    // la raiz de assets (src<nombre>). Las carpetas de assets del usuario
    // (sonidos, scripts, mods...) las crea el usuario cuando las necesita.
    const std::vector<fs::path> directorios = {
        fs::path(ProjectPaths::directorioMemory(nombre)) / "Binarios" / "Scene",
        fs::path(ProjectPaths::directorioSrc(nombre))};
    std::vector<fs::path> creados;
    for (const fs::path& directorio : directorios) {
        error.clear();
        if (!fs::create_directories(directorio, error) && error) {
            // El rollback solo retira directorios vacios, nunca contenido ajeno.
            for (auto it = creados.rbegin(); it != creados.rend(); ++it) {
                std::error_code ignorado;
                fs::remove(*it, ignorado);
            }
            std::error_code ignorado;
            fs::remove(proyecto, ignorado);
            return false;
        }
        fs::path actual = directorio;
        while (actual != proyecto && actual.has_relative_path()) {
            creados.push_back(actual);
            actual = actual.parent_path();
        }
    }
    return true;
}

bool ProjectManager::renombrarProyecto(const std::string& viejo, const std::string& nuevo) {
    if (!ProjectPaths::esNombreValido(viejo) || !ProjectPaths::esNombreValido(nuevo) || viejo == nuevo)
        return false;

    const std::string rutaViejo = ProjectPaths::directorioProyecto(viejo);
    const std::string rutaNuevo = ProjectPaths::directorioProyecto(nuevo);

    if (!existeDir(rutaViejo) || existeDir(rutaNuevo)) return false;

    std::error_code ec;
    std::filesystem::rename(rutaViejo, rutaNuevo, ec);
    if (ec) return false;

    // Renombrar src<viejo> -> src<nuevo> dentro del proyecto movido
    const std::string srcViejo = rutaNuevo + "/" + ProjectPaths::nombreRaizSrc(viejo);
    const std::string srcNuevo = rutaNuevo + "/" + ProjectPaths::nombreRaizSrc(nuevo);
    ec.clear();
    if (std::filesystem::is_directory(srcViejo, ec) && !ec) {
        ec.clear();
        std::filesystem::rename(srcViejo, srcNuevo, ec);
        if (ec) return false;
    }
    return true;
}

bool ProjectManager::eliminarProyecto(const std::string& nombre) {
    if (!ProjectPaths::esNombreValido(nombre)) return false;
    const std::string ruta = ProjectPaths::directorioProyecto(nombre);
    if (!existeDir(ruta)) return false;

    std::error_code ec;
    std::filesystem::remove_all(ruta, ec);
    return !ec;
}

bool ProjectManager::esProyectoValido(const std::string& nombre) {
    const std::string mem = ProjectPaths::directorioMemory(nombre);
    const std::string src = ProjectPaths::directorioSrc(nombre);
    return existeDir(mem) && existeDir(src);
}
