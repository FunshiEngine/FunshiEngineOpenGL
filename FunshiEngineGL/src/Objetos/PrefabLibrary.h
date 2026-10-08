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
#ifndef PREFABLIBRARY_H
#define PREFABLIBRARY_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class GameObject;
class Prefab;
class EditorController;

// Biblioteca de prefabs: gestiona la coleccion de prefabs disponibles
// (escaneando Assets/Prefabs/), permite crear, cargar, listar y borrar prefabs.
class PrefabLibrary {
public:
    explicit PrefabLibrary() = default;
    ~PrefabLibrary() = default;

    // Escanea Assets/Prefabs/ y carga todos los .prefab en memoria.
    void recargar();

    // Crea un prefab nuevo a partir de un objeto de la escena.
    // `nombre` debe ser unico (se uniciza si existe).
    Prefab* crearPrefab(const std::string& nombre, GameObject* raiz, EditorController* editor);

    // Obtiene un prefab por nombre (lo carga si no esta en cache).
    Prefab* obtener(const std::string& nombre);

    // Lista de nombres de prefabs disponibles.
    std::vector<std::string> listarNombres() const;

    // Borra el archivo .prefab y lo quita de la cache.
    bool borrar(const std::string& nombre);

    // Renombra un prefab (mueve el archivo).
    bool renombrar(const std::string& viejoNombre, const std::string& nuevoNombre);

private:
    std::unordered_map<std::string, std::unique_ptr<Prefab>> cache_;
    std::string directorioPrefabs() const;
};

#endif