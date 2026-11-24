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
#include "ServiciosMods.h"

#include <cstring>
#include <string>

#include "RegistroPanelesMods.h"

namespace FunshiMods {

namespace {

// Estado inyectado por el motor. Todo es opaco para el mod: la tabla solo
// expone punteros a funcion.
const FuenteDatosEscena* g_datosEscena = nullptr;
std::string g_nombreProyecto;
void* g_contextoImGui = nullptr;

// --- registro de paneles ---

bool servicioRegistrarPanel(const char* id, const char* titulo,
                             CallbackPanelMod dibujar, void* datos) {
    return RegistroPanelesMods::global().registrarDelModEnEjecucion(
        id, titulo, dibujar, datos);
}

// --- datos de la escena ---

int servicioContarObjetos() {
    if (!g_datosEscena || !g_datosEscena->contarObjetos) return 0;
    return g_datosEscena->contarObjetos(g_datosEscena->contexto);
}

bool servicioObtenerObjeto(int indice, ApiModGameObject* salida) {
    if (!g_datosEscena || !g_datosEscena->obtenerObjeto || !salida) {
        return false;
    }
    std::memset(salida, 0, sizeof(ApiModGameObject));
    salida->escala[0] = 1.0f;
    salida->escala[1] = 1.0f;
    salida->escala[2] = 1.0f;
    return g_datosEscena->obtenerObjeto(g_datosEscena->contexto, indice,
                                        salida);
}

int servicioContarComponentes(int indice) {
    if (!g_datosEscena || !g_datosEscena->contarComponentes) return 0;
    return g_datosEscena->contarComponentes(g_datosEscena->contexto, indice);
}

bool servicioObtenerComponente(int indiceObjeto, int indiceComponente,
                                ApiModComponente* salida) {
    if (!g_datosEscena || !g_datosEscena->obtenerComponente || !salida) {
        return false;
    }
    std::memset(salida, 0, sizeof(ApiModComponente));
    return g_datosEscena->obtenerComponente(g_datosEscena->contexto,
                                             indiceObjeto, indiceComponente,
                                             salida);
}

const char* servicioNombreProyecto() { return g_nombreProyecto.c_str(); }

void* servicioContextoImGui() { return g_contextoImGui; }

} // namespace

void inyectarDatosEscena(const FuenteDatosEscena* fuente) {
    g_datosEscena = fuente;
}

void fijarNombreProyecto(const char* nombre) {
    g_nombreProyecto = nombre ? nombre : "";
}

const char* nombreProyectoActual() { return g_nombreProyecto.c_str(); }

void fijarContextoImGui(void* contexto) { g_contextoImGui = contexto; }

void* contextoImGuiActual() { return g_contextoImGui; }

const ApiModServicios* tablaServicios() {
    static const ApiModServicios servicios = {
        /* .version          = */ versionApiMod,
        /* .registrarPanel   = */ servicioRegistrarPanel,
        /* .contarObjetos    = */ servicioContarObjetos,
        /* .obtenerObjeto    = */ servicioObtenerObjeto,
        /* .contarComponentes= */ servicioContarComponentes,
        /* .obtenerComponente= */ servicioObtenerComponente,
        /* .nombreProyecto   = */ servicioNombreProyecto,
        /* .contextoImGui    = */ servicioContextoImGui,
    };
    return &servicios;
}

} // namespace FunshiMods
