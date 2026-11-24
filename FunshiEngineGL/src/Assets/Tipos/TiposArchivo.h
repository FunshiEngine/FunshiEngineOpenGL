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
#ifndef TIPOSARCHIVO_H
#define TIPOSARCHIVO_H

#include <string>

// Que es un archivo del proyecto, para decidir que objeto o componente crear
// cuando se arrastra a la escena o al inspector. Es logica pura (sin ImGui ni
// GL): la decision se toma por la extension y, para los .json (que pueden ser
// un tileset de Tiled o un perfil de particulas), mirando sus claves.
enum class TipoArchivo {
    Desconocido,
    Prefab,
    Modelo3D,
    Imagen,
    Audio,
    Tileset,
    PerfilParticulas,
    InterfazHtml,
    ScriptCpp,
    ScriptJava,
    Proyecto,
};

namespace TiposArchivo {

// Extension en minusculas con el punto (".png"), o vacia.
std::string extension(const std::string& ruta);

// Clasifica por extension; un .json se abre y se decide por su contenido.
TipoArchivo clasificar(const std::string& ruta);

// Clasifica un .json ya leido (texto). Expuesto para probarlo sin archivos.
TipoArchivo clasificarJson(const std::string& texto);

// Nombre legible del tipo (para tooltips y mensajes).
const char* nombre(TipoArchivo tipo);

}  // namespace TiposArchivo

#endif
