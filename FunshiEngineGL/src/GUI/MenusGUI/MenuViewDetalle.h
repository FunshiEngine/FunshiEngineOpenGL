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
// Preambulo compartido por las unidades de MenuView.cpp (generado al dividirlo).
#ifndef MENUVIEWDETALLE_H
#define MENUVIEWDETALLE_H

#include "MenuView.h"
#include <algorithm>
#include <cstring>
#include <string>
#include "../../Configuracion/Apariencia.h"

namespace {

using Localizacion::Texto;

// Constantes de layout del menu: tamanio de boton y desvios verticales desde
// el centro de la ventana para cada fila del menu principal.
constexpr float kBotonAncho = 200.0f;

constexpr float kBotonAlto = 50.0f;

constexpr float kSeparacionVertical = 60.0f;

// Vista ConfigProyecto: barra izquierda de navegacion fija + panel derecho de
// la seccion activa.
constexpr float kMargen = 40.0f;

constexpr float kBarraAncho = 220.0f;

// Tarjetas de la seccion "Crear proyecto": area reservada para la imagen de
// ejemplo + titulo + descripcion accesible.
constexpr float kTarjetaAncho = 260.0f;

constexpr float kTarjetaAlto = 280.0f;

constexpr float kImagenAlto = 96.0f;

constexpr float kEspacioTarjetas = 24.0f;

// Etiqueta traducida con ID estable. ImGui deriva el ID de un widget de su
// texto, asi que pasar la traduccion directa hacia que el ID cambiara al
// cambiar de idioma (se perdia el foco/estado del widget en ese frame y los
// IDs no eran predecibles). Se agrega el sufijo oculto "##<NombreDelTexto>": el
// usuario ve solo el texto traducido y el ID queda atado al texto, no al idioma.
std::string etiqueta(const MenuModel* model, Texto texto) {
    return std::string(model->traducir(texto)) + "##" + Localizacion::nombre(texto);
}

}  // namespace

#endif
