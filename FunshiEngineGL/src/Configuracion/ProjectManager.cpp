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
    const std::string sonidos = ProjectPaths::directorioSonidos(nombre);
    const std::string interfaces = ProjectPaths::directorioInterfaces(nombre);
    const std::string binScene = ProjectPaths::rutaSceneDir(nombre);

    const std::string proyectoDir = ProjectPaths::directorioProyecto(nombre);
    const std::string sonidosViejo = proyectoDir + "/Sonidos";

    bool viejoExiste = std::filesystem::exists(sonidosViejo, ec);
    bool nuevoExiste = std::filesystem::exists(sonidos, ec);

    if (viejoExiste && !nuevoExiste) {
        std::filesystem::create_directories(ProjectPaths::directorioSrc(nombre), ec);
        std::filesystem::rename(sonidosViejo, sonidos, ec);
        if (ec) {
            // Fallback entre dispositivos (p. ej. Windows con volumenes
            // distintos): copiar; si la copia funciona, retirar el original.
            ec.clear();
            std::filesystem::copy(sonidosViejo, sonidos,
                                  std::filesystem::copy_options::recursive, ec);
            if (!ec) {
                std::error_code ec2;
                std::filesystem::remove_all(sonidosViejo, ec2);
            }
        }
    }

    // Migracion de proyectos legacy que guardaban el descriptor y la escena
    // directamente en la raiz de MotorGrafico/Binarios (estructura anterior a
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

    return crearDirs(mem + "/Binarios/Scene", ec) &&
           crearDirs(src, ec) &&
           crearDirs(sonidos, ec) &&
           crearDirs(interfaces, ec) &&
           crearDirs(binScene, ec);
}

void ProjectManager::asegurarEstructuraBase() {
    std::error_code ec;
    std::filesystem::create_directories(ProjectPaths::directorioProyects(), ec);
    std::filesystem::create_directories(ProjectPaths::directorioConfiguraciones(), ec);
    std::filesystem::create_directories(ProjectPaths::directorioExportaciones(), ec);

    migrarProyectosAntiguos();
    migrarConfiguracionGlobal();
}

bool ProjectManager::crearProyectoPorDefecto() {
    const std::string proyectsDir = ProjectPaths::directorioProyects();
    std::error_code ec;
    std::filesystem::create_directories(proyectsDir, ec);

    bool hayProyectos = false;
    for (const auto& entry : std::filesystem::directory_iterator(proyectsDir, ec)) {
        if (entry.is_directory(ec)) {
            hayProyectos = true;
            break;
        }
    }
    if (hayProyectos) return false;

    return crearProyecto("NuevoProyecto");
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

    // Proyecto ya existe, pero puede necesitar migración (ej. Sonidos en raiz antigua)
    return crearEstructuraProyecto(nombre);
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