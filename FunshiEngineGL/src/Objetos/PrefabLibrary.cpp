/*
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    SPDX-License-Identifier: Apache-2.0
*/
#include "PrefabLibrary.h"

#include <filesystem>
#include <iostream>

#include "Prefab.h"
#include "Configuracion/EditorConfig.h"
#include "Herramientas/PathUtils.h"

namespace fs = std::filesystem;

std::string PrefabLibrary::directorioPrefabs() const {
    fs::path base = EditorConfig::raizAssetsFijada();
    return (base / "Prefabs").string();
}

void PrefabLibrary::recargar() {
    cache_.clear();
    fs::path dir = directorioPrefabs();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
        return;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".prefab") {
            std::string nombre = entry.path().stem().string();
            cache_[nombre] = std::make_unique<Prefab>(nombre);
        }
    }
}

Prefab* PrefabLibrary::crearPrefab(
    const std::string& nombre, GameObject* raiz, EditorController* editor) {
    
    std::string nombreFinal = nombre;
    int sufijo = 1;
    while (cache_.count(nombreFinal) || fs::exists(fs::path(directorioPrefabs()) / (nombreFinal + ".prefab"))) {
        nombreFinal = nombre + " " + std::to_string(sufijo++);
    }
    
    auto prefab = std::make_unique<Prefab>(nombreFinal);
    if (prefab->guardarDesdeObjeto(raiz, editor)) {
        Prefab* ptr = prefab.get();
    cache_[nombreFinal] = std::move(prefab);
        return ptr;
    }
    return nullptr;
}

Prefab* PrefabLibrary::obtener(const std::string& nombre) {
    auto it = cache_.find(nombre);
    if (it != cache_.end()) return it->second.get();

    // Intentar cargar del disco
    fs::path archivo = fs::path(directorioPrefabs()) / (nombre + ".prefab");
    if (fs::exists(archivo)) {
        // Validar integridad del archivo antes de cachear
        if (!Prefab::validarArchivo(archivo.string())) {
            std::cerr << "[PrefabLibrary] Archivo prefab invalido o corrupto: " << archivo.string() << std::endl;
            return nullptr;
        }
        cache_[nombre] = std::make_unique<Prefab>(nombre);
        return cache_[nombre].get();
    }
    return nullptr;
}

std::vector<std::string> PrefabLibrary::listarNombres() const {
    std::vector<std::string> nombres;
    nombres.reserve(cache_.size());
    for (const auto& [nombre, _] : cache_) nombres.push_back(nombre);
    return nombres;
}

bool PrefabLibrary::borrar(const std::string& nombre) {
    fs::path archivo = fs::path(directorioPrefabs()) / (nombre + ".prefab");
    if (fs::exists(archivo)) {
        fs::remove(archivo);
    }
    cache_.erase(nombre);
    return true;
}

bool PrefabLibrary::renombrar(const std::string& viejoNombre, const std::string& nuevoNombre) {
    if (cache_.count(nuevoNombre)) return false;
    fs::path viejoArchivo = fs::path(directorioPrefabs()) / (viejoNombre + ".prefab");
    fs::path nuevoArchivo = fs::path(directorioPrefabs()) / (nuevoNombre + ".prefab");
    if (!fs::exists(viejoArchivo)) return false;
    fs::rename(viejoArchivo, nuevoArchivo);
    if (auto it = cache_.find(viejoNombre); it != cache_.end()) {
    cache_[nuevoNombre] = std::move(it->second);
    cache_.erase(it);
    }
    return true;
}