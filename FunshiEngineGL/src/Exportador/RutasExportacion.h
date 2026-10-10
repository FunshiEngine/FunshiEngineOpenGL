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
#ifndef RUTASEXPORTACION_H
#define RUTASEXPORTACION_H

// Rutas del proyecto que el exportador necesita para armar el juego standalone.
// Header-only a proposito, como los helpers de la GUI del explorador: son unas
// pocas lineas de logica pura y asi se pueden probar sin lanzar una
// exportacion completa (que compila el engine entero).

#include <string>
#include <utility>
#include <vector>

#include "../Configuracion/EditorConfig.h"
#include "../Herramientas/PathUtils.h"

// Carpetas/archivos SOLO del editor que NO deben ir en la exportacion del juego.
inline bool esSoloEditor(const std::string& rutaRelativa) {
    // Memory/Interfaces/ - estado UI del editor (dock, ventanas)
    if (rutaRelativa.rfind("Memory/Interfaces/", 0) == 0) return true;
    if (rutaRelativa == "Memory/Interfaces") return true;
    // Memory/ConfiguracionProyecto.json - config del editor (ultimo proyecto, etc.)
    if (rutaRelativa == "Memory/ConfiguracionProyecto.json") return true;
    // Memory/imgui.ini - layout ImGui del editor
    if (rutaRelativa == "Memory/imgui.ini") return true;
    return false;
}

// Par (origen en el editor, destino en la distribucion del juego).
//
// El juego exportado conserva la disposicion del motor: su binario resuelve
// MotorGrafico/ junto a si mismo (ProjectPaths::directorioBase), asi que el
// proyecto viaja a <salida>/MotorGrafico/Proyects/<nombre>/ y el
// juego encuentra escenas, Memory/Binarios y src<nombre>/ igual que el editor.
// Se EXCLUYEN carpetas/archivos solo del editor (Memory/Interfaces/, imgui.ini,
// ConfiguracionProyecto.json).
inline std::vector<std::pair<std::string, std::string>> rutasDatosProyecto(
    const std::string& nombreProyecto, const std::string& directorioSalida) {
    return {
        {EditorConfig::directorioProyecto(nombreProyecto),
         directorioSalida + "/MotorGrafico/Proyects/" + nombreProyecto},
    };
}

// Carpeta con los fuentes de script del proyecto (los que el usuario compila
// para su juego, no los del engine).
inline std::string directorioScriptsProyecto(const std::string& nombreProyecto) {
    return EditorConfig::directorioScripts(nombreProyecto);
}

// El nombre del proyecto es el de su carpeta: sirve para no arrastrar la
// disposicion del proyecto por a mano cuando solo se tiene la ruta.
inline std::string nombreProyectoDesdeRuta(const std::string& rutaProyecto) {
    const std::string::size_type sep = indiceSeparadorFinal(rutaProyecto);
    if (sep == std::string::npos) return rutaProyecto;
    return rutaProyecto.substr(sep + 1);
}

#endif
