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
#ifndef WINDOWNAMES_H
#define WINDOWNAMES_H

// Nombres de ventana ImGui del editor. Se centralizan para que el dock
// (DockSpaceInterface) y los paneles compartan los mismos literales. Antes la
// ventana del contenido era "Show Folder " (con espacio) escrita a mano en
// dos sitios: renombrarla en uno rompia el anclaje del dock en silencio.
namespace WindowNames {
inline constexpr const char* BrowseFile     = "BrowseFile";
inline constexpr const char* ShowFolder     = "ShowFolder";
inline constexpr const char* SelectedObjects = "SelectedObjects";
inline constexpr const char* Settings       = "Settings";
inline constexpr const char* MenuBar        = "MenuBar";
inline constexpr const char* EditorDockSpace = "EditorDockSpace";
inline constexpr const char* Status         = "Estado";
// Ventanas del sistema de audio + creador de interfaces (CreadorDeInterfaces)
// y el canvas que las pinta con sonido (CanvasInterface).
inline constexpr const char* CreadorInterfaces = "Creador de Interfaces";
inline constexpr const char* CanvasUI        = "Canvas";
} // namespace WindowNames

#endif