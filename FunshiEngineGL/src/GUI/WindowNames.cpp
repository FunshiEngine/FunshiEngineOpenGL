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
#include "WindowNames.h"

#include <cstring>

#include "../Localizacion/Localizacion.h"

namespace WindowNames {
namespace {

using Localizacion::Texto;

struct TituloVentana {
    const char* nombre;
    Texto texto;
};

constexpr TituloVentana kTitulos[] = {
    {BrowseFile, Texto::VentanaExplorador},
    {ShowFolder, Texto::VentanaContenido},
    {SelectedObjects, Texto::VentanaObjetos},
    {Settings, Texto::VentanaInspector},
    {MenuBar, Texto::VentanaBarraMenus},
    {Status, Texto::VentanaEstado},
    {Terminal, Texto::VentanaTerminal},
    {CameraList, Texto::VentanaCamaras},
    {VistaInterfaz, Texto::VentanaVistaInterfaz},
    {AnimacionesParticulas, Texto::VentanaAnimacionesParticulas},
};

const TituloVentana* buscar(const std::string& nombre) {
    for (const TituloVentana& t : kTitulos) {
        if (nombre == t.nombre) return &t;
    }
    return nullptr;
}

} // namespace

std::string titulo(const std::string& nombre) {
    const TituloVentana* t = buscar(nombre);
    return t ? std::string(Localizacion::tr(t->texto)) + "###" + nombre : nombre;
}

const char* nombreVisible(const std::string& nombre) {
    const TituloVentana* t = buscar(nombre);
    return t ? Localizacion::tr(t->texto) : nullptr;
}

std::string idImGui(const std::string& nombre) {
    return buscar(nombre) ? "###" + nombre : nombre;
}

std::string parteId(const char* nombreImGui) {
    const char* id = std::strstr(nombreImGui, "###");
    return id ? std::string(id + 3) : std::string(nombreImGui);
}

} // namespace WindowNames
