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
#include "PrecondicionesExportacion.h"

#include <filesystem>

namespace PrecondicionesExportacion {

bool comprobar(const std::string& nombreProyecto,
               const std::string& rutaProyecto,
               std::string& error) {
    error.clear();
    if (nombreProyecto.empty()) {
        error = "No hay proyecto seleccionado: elige un proyecto antes de exportar";
        return false;
    }
    std::error_code ec;
    if (!std::filesystem::is_directory(rutaProyecto, ec)) {
        error = "El proyecto no existe: " + rutaProyecto;
        return false;
    }
    return true;
}

} // namespace PrecondicionesExportacion
