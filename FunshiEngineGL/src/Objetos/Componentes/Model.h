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
#include <string>

class Model : public Component {
 private:
  char filePath[4096] = ""; // path completo del asset; los demas buffers de
                            // path del editor usan 4096 (ver commit "buffers de
                            // path ampliados"). El payload de drag&drop
                            // "ARCHIVO_PATH" lleva un path de longitud
                            // arbitraria.

 public:
  void setPath(std::string path);

  std::string getPath() { return std::string(filePath); }

 protected:
  void serializeComponent(std::ofstream* fileNamePathContentObject) override;
  void deserializeComponent(std::ifstream* fileNamePathContentObject) override;

 public:
  void saveComponent(std::ofstream* fileNamePathContentObject) override;
  void loadComponent(std::ifstream* fileNamePathContentObject) override;
};
#endif