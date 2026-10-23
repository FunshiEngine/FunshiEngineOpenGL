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
#ifndef MODEL_H
#define MODEL_H

#include "Component.h"
#include <memory>
#include <string>
#include <vector>

#include "../../Matematicas/StructVec3.h"

class AssetManager;
class RegistroDiagnosticos;
struct Mesh;

class Model : public Component {
 private:
  char filePath[4096] = ""; // path completo del asset; los demas buffers de
                            // path del editor usan 4096 (ver commit "buffers de
                            // path ampliados"). El payload de drag&drop
                            // "ARCHIVO_PATH" lleva un path de longitud
                            // arbitraria.
  std::shared_ptr<const Mesh> mesh_ = nullptr; // malla cargada (compartida via
                                               // AssetManager cuando esta
                                               // inyectada)
  AssetManager* assets_ = nullptr;             // proveedor de mallas (inyectado)
  RegistroDiagnosticos* diagnosticos_ = nullptr;

  // Carga la malla del path actual (llamado por setPath y por onLoaded).
  void cargarMalla();

 public:
  void setPath(std::string path);

  std::string getPath() { return std::string(filePath); }

  // Conecta el asset manager inyectado: los proximos setPath()/carga de
  // escena comparten mallas via el cache en lugar de parsear Assimp por
  // objeto.
  void setAssetManager(AssetManager* assets) noexcept { assets_ = assets; }
  void setRegistroDiagnosticos(RegistroDiagnosticos* registro) noexcept {
    diagnosticos_ = registro;
  }

  // La malla compartida (nullptr si aun no se cargo o fallo).
  const Mesh* getMesh() const { return mesh_.get(); }

  // Vertices en espacio local del modelo (para construir shapes de colision).
  const std::vector<vec3>& getVertices() const;

  // Calcula la caja delimitadora en espacio local del modelo.
  bool getBoundingBox(vec3& outMin, vec3& outMax) const;

 protected:
  void serializeComponent(std::ofstream* fileNamePathContentObject) override;
  void deserializeComponent(std::ifstream* fileNamePathContentObject) override;

  // Post-carga: intenta cargar la malla del path (si hay uno) despues de
  // deserializar el componente.
  void onLoaded(GameObject& owner) override;

 public:
};
#endif