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
#ifndef RUTA_CABECERAS_SCRIPT_H
#define RUTA_CABECERAS_SCRIPT_H

#include <filesystem>
#include <functional>
#include <string>

// Resolucion de la carpeta de cabeceras del motor que los scripts C++ necesitan
// al compilarse en caliente. `configurado` puede venir de la variable de entorno
// FUNSHI_SRC_DIR o del valor horneado por CMake. En el arbol de desarrollo y en
// los tests es una ruta absoluta al checkout; en la app instalada es el nombre
// relativo "include" junto al ejecutable, porque la ruta del workspace de build
// no existe en la maquina del usuario.
//
// Se separa de BackendCpp para poder probar la resolucion sin tocar el disco ni
// depender de un compilador: `existe` decide si una ruta es usable.
namespace RutaCabecerasScript {

inline std::string resolver(
    const std::string& configurado, const std::string& directorioEjecutable,
    const std::function<bool(const std::string&)>& existe) {
    if (configurado.empty()) return {};

    const std::filesystem::path ruta(configurado);
    // Ruta absoluta: la del checkout (desarrollo/tests). Si no existe no se
    // devuelve una ruta muerta, para que el backend avise con su propio mensaje
    // en vez de que falle el compilador con un include inexistente.
    if (ruta.is_absolute())
        return existe(configurado) ? configurado : std::string();

    // Relativa al directorio de trabajo: builds y tests que corren junto al
    // arbol de fuentes.
    if (existe(configurado)) return configurado;

    // Relativa a la carpeta del ejecutable: el paquete instalado, donde el
    // nombre vale "include".
    if (directorioEjecutable.empty()) return {};
    const std::filesystem::path juntoAlExe =
        std::filesystem::path(directorioEjecutable) / ruta;
    const std::string candidato = juntoAlExe.string();
    return existe(candidato) ? candidato : std::string();
}

}  // namespace RutaCabecerasScript

#endif
