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
#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "Mesh.h"

// Contrato de carga de una malla desde disco (Strategy). El AssetManager
// inyecta el loader concreto; la integracion con Modelos3D usara un loader
// de Assimp, pero las pruebas headless pueden usar uno artificial sin tocar
// la pila grafica. Mismo patron que IPhysicsBackend detras de PhysicsEngine.
class IMeshLoader {
public:
    virtual ~IMeshLoader() = default;
    // Devuelve la malla cargada, o lanza AssetLoadException si falla.
    virtual std::shared_ptr<Mesh> load(const std::string& path) = 0;
};

// Registro central de assets (Flyweight/Registry): la GEOMETRIA compartida
// del proyecto. Address hace que pedir el mismo path devuelva el MISMO Mesh
// (parses de Assimp de una sola vez, sin duplicar vertices por objeto). Un
// pending del roadmap era "AssetManager/Flyweight"; esta es su primera
// materializacion, pensada para que las proximas ramas agreguen texturas,
// shaders y clips de audio al mismo patron. No expone OpenGL: es CPU-only.
//
// Ownership: los assets viven en el manager (shared_ptr). Quien pide un
// asset recibe una copia del shared_ptr y puede conservarla mientras la usa;
// clearUnusedMeshes() evicta solo las entradas sin usuarios externos.
class AssetManager {
public:
    explicit AssetManager(std::unique_ptr<IMeshLoader> loader = nullptr);
    ~AssetManager() = default;

    // No copiable (posee recursos); movible.
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) noexcept = default;
    AssetManager& operator=(AssetManager&&) noexcept = default;

    // Devuelve la malla cacheada para el path (normalizado internamente).
    // Si no esta, intenta cargarla con el loader inyectado; sin loader o con
    // carga fallida lanza AssetNotFoundException/AssetLoadException.
    std::shared_ptr<const Mesh> getMesh(const std::string& path);

    // Inserta/sobrescribe una malla construida en codigo (procedural, tests).
    // util para assets que no vienen de disco (particulas, debug).
    void putMesh(const std::string& path, std::shared_ptr<Mesh> mesh);

    bool containsMesh(const std::string& path) const;
    size_t meshCount() const;
    void removeMesh(const std::string& path);

    // Evicta las entradas cuyo shared_ptr solo referencia el manager
    // (use_count == 1): no hay usuarios externos vivos de esa malla.
    void clearUnusedMeshes();

    // Fallos de carga recordados: una ruta que ya fallo al cargarse no vuelve a
    // tocar el disco en cada llamada (el render la pide por frame y por objeto);
    // getMesh sigue lanzando el mismo error. El fallo se olvida con
    // clearFailedMeshes(), reloadMesh(), removeMesh(), putMesh() o al cambiar de
    // loader, que es cuando la ruta puede intentarse otra vez.
    void clearFailedMeshes();
    bool hasFailedMesh(const std::string& path) const;
    size_t failedMeshCount() const;

    // Reemplaza la malla de un path por una version recargada SIN invalidar
    // a los usuarios actuales: se crea un Mesh nuevo y se hace swap, de modo
    // que quien conservaba el anterior sigue leyendo datos validos.
    void reloadMesh(const std::string& path);

    // Intercambia el loader de carga (util para recargar backend en tests).
    void setLoader(std::unique_ptr<IMeshLoader> loader);

    const IMeshLoader* loader() const { return loader_.get(); }

private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes_;
    // Ruta normalizada -> motivo del ultimo fallo. Vacio no es un estado valido:
    // si la clave esta, la ruta fallo.
    std::unordered_map<std::string, std::string> fallos_;
    std::unique_ptr<IMeshLoader> loader_;
};

#endif