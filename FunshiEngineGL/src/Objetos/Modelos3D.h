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
#ifndef MODELOS3D_H
#define MODELOS3D_H

#include <memory>
#include <string>
#include <vector>

#include "../Assets/Mesh.h"
#include "../Matematicas/StructVec3.h"
#include "../Objetos/GameObject.h"

class AssetManager;

class Modelos3D : public GameObject {
private:
    // Malla del modelo: se pide al AssetManager (compartida entre objetos
    // con el mismo archivo) o se carga localmente en el fallback. El objeto
    // conserva su Transform/Material/Color propios; lo compartido es la
    // geometria.
    std::shared_ptr<const Mesh> mesh_;
    std::string filePath_;
    // Proveedor de mallas (inyectado por GameScene via EditorController/
    // SceneSerializer). Con nullptr se usa el AssimpMeshLoader local como
    // respaldo (caso del inspector por defecto, que nunca dibuja).
    AssetManager* assets_ = nullptr;

    void setObject();

public:
    explicit Modelos3D(Entity* origin);
    Modelos3D();

    // Conecta el asset manager inyectado: los proximos setPath()/carga de
    // escena comparten mallas via el cache en lugar de parsear Assimp por
    // objeto.
    void setAssetManager(AssetManager* assets) noexcept { assets_ = assets; }

    void setPath(std::string path);
    std::string getPath();
    bool getBoundingBox(vec3& outMin, vec3& outMax) const;
    // Vertices en espacio local del modelo (para construir shapes de colision).
    const std::vector<vec3>& getVertices() const;

    // La malla compartida (nullptr si aun no se cargo o fallo).
    const Mesh* getMesh() const { return mesh_.get(); }

protected:
    void serializeEntity() override;
    void deserializeEntity() override;

public:
    void saveEntity(std::string filename) override;
    void loadEntity(std::string filename) override;
};

#endif