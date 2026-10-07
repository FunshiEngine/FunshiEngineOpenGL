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
#ifndef RUTAS_LOG_H
#define RUTAS_LOG_H

#include <filesystem>
#include <string>
#include <vector>

// Carpetas candidatas donde escribir el log de arranque, en orden de
// preferencia. La primera suele ser la raiz de datos del motor, que ya cae a la
// carpeta de usuario cuando el ejecutable no admite escritura (instalado en
// Program Files sin elevar): asi siempre hay un log, y no se pierde en el catch
// de un create_directories que no puede escribir junto al .exe.
namespace RutasLog {

inline std::vector<std::string> candidatas(const std::string& raizDatos,
                                           const std::string& dirEjecutable,
                                           const std::string& dirTemp) {
    std::vector<std::string> rutas;
    auto agregar = [&rutas](const std::filesystem::path& base) {
        if (base.empty()) return;
        const std::string ruta = (base / "logs").string();
        for (const std::string& otra : rutas)
            if (otra == ruta) return;
        rutas.push_back(ruta);
    };

    if (!raizDatos.empty()) agregar(std::filesystem::path(raizDatos));
    if (!dirEjecutable.empty()) agregar(std::filesystem::path(dirEjecutable));
    if (!dirTemp.empty())
        agregar(std::filesystem::path(dirTemp) / "FunshiEngineGL");
    return rutas;
}

}  // namespace RutasLog

#endif
