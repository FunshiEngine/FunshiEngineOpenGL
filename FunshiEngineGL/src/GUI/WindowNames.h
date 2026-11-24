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

#include <string>

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
inline constexpr const char* Terminal       = "Terminal";
inline constexpr const char* CameraList     = "Camaras";
inline constexpr const char* VistaInterfaz  = "Vista de interfaz";
inline constexpr const char* AnimacionesParticulas = "Animaciones y Particulas";

// Titulo para ImGui::Begin: "<nombre visible en el idioma vigente>###<nombre>".
// ImGui arma el ID de la ventana solo con la parte desde "###", asi que el
// dock, el imgui.ini y el estado guardado siguen atados al nombre interno y
// cambiar de idioma no mueve ni cierra ninguna ventana. Un nombre sin titulo
// traducible (dock, menu de inicio, paneles de mods) se devuelve tal cual.
std::string titulo(const std::string& nombre);

// Nombre visible en el idioma vigente, o nullptr si la ventana no tiene uno.
const char* nombreVisible(const std::string& nombre);

// Nombre que ImGui usa para buscar o anclar la ventana ("###<nombre>" si
// tiene titulo traducible; el nombre tal cual si no).
std::string idImGui(const std::string& nombre);

// Parte de ID de un nombre de ventana de ImGui (lo que sigue a "###", o el
// nombre entero si no tiene).
std::string parteId(const char* nombreImGui);
} // namespace WindowNames

#endif
