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
#ifndef TEMAEDITOR_H
#define TEMAEDITOR_H

#include <imgui.h>

#include "../../Configuracion/Apariencia.h"

// ============================================================================
// Aplicador del perfil de apariencia a ImGui.
//
// Separa la logica "como se ve la interfaz" (esta capa, depende de ImGui) del
// dato "que prefiere el usuario" (Apariencia, puro y persistible). main llama
// a aplicarEstilo() una sola vez al arrancar y cada vez que el perfil cambia
// desde la vista Opciones.
//
// El color de acento se inyecta en los roles visuales de ImGui (botones,
// headers, solapas del dock, sliders, checks, enlaces, fondo de los campos de
// entrada, cabeceras de tabla y destino de arrastre) y los grises azulados de
// fabrica se neutralizan: NINGUN rol de la paleta conserva el azul de Dear
// ImGui, asi que la interfaz se ve del color elegido en Opciones y no a medias.
// Los widgets que necesitan el mismo acento (por ejemplo el overlay de Estado)
// lo derivan con ImGui::GetStyleColorVec4.
// En modo blanco y negro se desatura toda la paleta (incluido el acento) para
// dejar la interfaz monocroma, coherente con el fondo y la grilla del viewport.
// ============================================================================
namespace TemaEditor {

// Color de acento resuelto: en modo B/N se devuelve su version en escala de
// grises (asi toda la interfaz queda monocroma). Se devuelve opaco: el alpha
// del perfil no participa del tema (cada rol aporta el suyo).
ImVec4 acento(const Apariencia& ap);

// Aplica el tema base (claro/oscuro), el acento y un conjunto coherente de
// redondeos/espaciados. Idempotente: llamarlo con el mismo perfil es inocuo.
void aplicarEstilo(const Apariencia& ap);

// Colores con significado (mensajes de error, avisos, exito, texto
// secundario). Los paneles los piden por rol en vez de escribir un ImVec4: asi
// se leen bien en tema claro y oscuro y respetan el modo blanco y negro.
enum class RolColor { Error, Aviso, Exito, Atenuado };

// Color del rol para el perfil dado (funcion pura, testeable sin contexto).
ImVec4 color(RolColor rol, const Apariencia& ap);

// Color del rol para el ultimo perfil aplicado con aplicarEstilo().
ImVec4 color(RolColor rol);

// Atajo: texto con el color del rol (como ImGui::TextColored).
void texto(RolColor rol, const char* formato, ...);

} // namespace TemaEditor

#endif
