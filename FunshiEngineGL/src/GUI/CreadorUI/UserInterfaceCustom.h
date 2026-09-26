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
#ifndef USERINTERFACECUSTOM_H
#define USERINTERFACECUSTOM_H

#include <string>
#include <vector>

#include "../../../External/nlohmann/json.hpp"

#include "WidgetUI.h"

// Interfaz de usuario creada por el usuario (asset). Se guarda como un archivo
// JSON por interfaz en Memory/Interfaces/<nombre>.json del proyecto. El modelo
// es puro (sin ImGui): el CreadorDeInterfaces la edita y el CanvasInterface la
// pinta. Es la contraparte "asset de UI" que, como los clips de sonido, se
// referencia por su NOMBRE (el nombre DEL ARCHIVO de la interfaz).
struct UserInterfaceCustom {
    std::string nombre;    // == nombre del archivo (sin ".json")
    std::string titulo;    // titulo de la ventana en el canvas
    float ancho = 320.0f;  // tamano del canvas (px)
    float alto = 220.0f;
    std::vector<WidgetUI> widgets;

    // Serializacion JSON (nlohmann, vendoriado en External/). Idempotente:
    // campos faltantes conservan el default del widget.
    nlohmann::json aJson() const;
    void desdeJson(const nlohmann::json& j);

    // Persistencia en disco: guarda/carga bajo un directorio de interfaces.
    bool guardar(const std::string& directorio) const;
    bool cargar(const std::string& directorio);
};

#endif // USERINTERFACECUSTOM_H