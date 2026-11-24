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
// Stubs de EditorConfig para el test aislado de AudioSource.
// El test no ejercita serializeComponent/deserializeComponent, por lo que
// estas funciones solo necesitan ser passthrough sin depender de la cadena
// completa de configuracion del proyecto (ConfigPersistence, ProjectPaths,
// ProjectManager, nlohmann, etc.).
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"

std::string EditorConfig::relativizarRuta(const std::string& rutaAbsoluta) {
    return rutaAbsoluta;
}

std::string EditorConfig::absolutizarRuta(const std::string& rutaGuardada) {
    return rutaGuardada;
}

const std::string& EditorConfig::raizAssetsFijada() noexcept {
    static const std::string vacio;
    return vacio;
}
