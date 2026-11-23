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
#ifndef SCENESERIALIZER_H
#define SCENESERIALIZER_H

#include <fstream>
#include <string>

class EditorController;
class SceneRegistry;
class GameObject;
class AssetManager;
class RegistroDiagnosticos;

template <typename T>
class Position;

class SceneSerializer {
private:
    SceneRegistry* scene;
    EditorController* editor;
    // Fuente de mallas compartidas: se inyecta a cada Modelos3D antes de
    // deserializar su path, para que la geometria se cachee en el
    // AssetManager en lugar de re-parsear Assimp por objeto.
    AssetManager* assets;
    RegistroDiagnosticos* diagnosticos;

    void savePreOrder(Position<GameObject*>* root,
                       std::ofstream& archive,
                       const std::string& filename);

    void loadPreOrder(std::ifstream& file,
                      GameObject* parent,
                      const std::string& semiPath,
                      bool& cargaCompleta);

public:
    SceneSerializer(SceneRegistry* value,
                    EditorController* controller,
                    AssetManager* assetsManager = nullptr,
                    RegistroDiagnosticos* registro = nullptr);

    void save(const std::string& filename);

    bool load(const std::string& pathTxt,
              const std::string& semiPath);
};

#endif
