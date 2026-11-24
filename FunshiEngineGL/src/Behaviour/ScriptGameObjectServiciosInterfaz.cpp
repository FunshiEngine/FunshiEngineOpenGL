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
#include "ScriptGameObjectDetalle.h"

namespace MotorScript {

namespace {

// El handle de interfaz es un puntero opaco al InterfaceComponent del objeto;
// la tabla de servicios lo trata como void* y aca se reinterpreta.
InterfaceComponent* componenteInterfaz(const void* interfaz) {
    return const_cast<InterfaceComponent*>(
        static_cast<const InterfaceComponent*>(interfaz));
}

}  // namespace

void* serviciosInterfazDeObjeto(const char* nombreObjeto) {
    if (!nombreObjeto) return nullptr;
    GameObject* objeto =
        static_cast<GameObject*>(serviciosObjetoPorNombre(nombreObjeto));
    return objeto ? objeto->getComponent<InterfaceComponent>() : nullptr;
}

bool serviciosInterfazActiva(const void* interfaz) {
    const InterfaceComponent* c =
        static_cast<const InterfaceComponent*>(interfaz);
    return c ? c->estaActiva() : false;
}

void serviciosInterfazEstablecerActiva(void* interfaz, bool activa) {
    InterfaceComponent* c = componenteInterfaz(interfaz);
    if (!c) return;
    c->setActivo(activa);
}

bool serviciosInterfazEstablecerEstado(void* interfaz, const char* jsonEstado) {
    if (!g_interfaces || !jsonEstado) return false;
    return g_interfaces->establecerEstado(componenteInterfaz(interfaz),
                                          jsonEstado);
}

bool serviciosInterfazEstablecerValor(void* interfaz, const char* clave,
                                      const char* valorJson) {
    if (!g_interfaces || !clave || !valorJson) return false;
    return g_interfaces->establecerValor(componenteInterfaz(interfaz), clave,
                                         valorJson);
}

const char* serviciosInterfazValor(void* interfaz, const char* rutaJson) {
    if (!g_interfaces || !rutaJson) return nullptr;
    return g_interfaces->valor(componenteInterfaz(interfaz), rutaJson);
}

namespace {

// --- Interfaz de juego NATIVA (HUD que dibuja el motor) --------------------
// Mismas reglas que la interfaz web: el handle es opaco (un puntero al
// UIComponent del objeto) y toda funcion tolera un handle nulo.
UIComponent* componenteInterfazNativa(const void* interfaz) {
    return const_cast<UIComponent*>(
        static_cast<const UIComponent*>(interfaz));
}

}  // namespace

void* serviciosInterfazNativaDeObjeto(const char* nombreObjeto) {
    if (!nombreObjeto) return nullptr;
    GameObject* objeto =
        static_cast<GameObject*>(serviciosObjetoPorNombre(nombreObjeto));
    return objeto ? objeto->getComponent<UIComponent>() : nullptr;
}

bool serviciosInterfazNativaVisible(const void* interfaz) {
    const UIComponent* c = static_cast<const UIComponent*>(interfaz);
    return c ? c->estaActivo() : false;
}

void serviciosInterfazNativaEstablecerVisible(void* interfaz, bool visible) {
    UIComponent* c = componenteInterfazNativa(interfaz);
    if (!c) return;
    c->setActivo(visible);
}

bool serviciosInterfazNativaEstablecerTexto(void* interfaz,
                                              const char* idWidget,
                                              const char* texto) {
    UIComponent* c = componenteInterfazNativa(interfaz);
    if (!c || !idWidget || !texto) return false;
    return c->lienzo().establecerTexto(idWidget, texto);
}

bool serviciosInterfazNativaEstablecerVisibleWidget(void* interfaz,
                                                      const char* idWidget,
                                                      bool visible) {
    UIComponent* c = componenteInterfazNativa(interfaz);
    if (!c || !idWidget) return false;
    return c->lienzo().establecerVisible(idWidget, visible);
}

bool serviciosInterfazNativaBotonPulsado(void* interfaz,
                                          const char* idWidget) {
    UIComponent* c = componenteInterfazNativa(interfaz);
    if (!c || !idWidget) return false;
    return c->sistema().fuePulsado(idWidget);
}

}  // namespace MotorScript
