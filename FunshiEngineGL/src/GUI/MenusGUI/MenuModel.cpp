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
#include "MenuModel.h"

#include <utility>

// ============================================================================
// Implementacion de la logica pura del menu de inicio. Este archivo no
// depende de ImGui ni GLFW: el modelo debe permanecer testeable y aislado
// de la presentacion (ver MenuGUI.h y README.md del paquete).
// ============================================================================

void MenuModel::mostrarMenu() { vista = Vista::Principal; }

void MenuModel::iniciarEstudio() { vista = Vista::Ninguna; }

void MenuModel::abrirOpciones() { vista = Vista::Opciones; }

void MenuModel::abrirConfigProyecto() { vista = Vista::ConfigProyecto; }

void MenuModel::volver() {
    if (vista == Vista::Opciones || vista == Vista::ConfigProyecto) {
        vista = Vista::Principal;
    }
}

bool MenuModel::estaVisible() const noexcept { return vista != Vista::Ninguna; }

MenuModel::Vista MenuModel::getVista() const noexcept { return vista; }

const std::string& MenuModel::getNombreProyecto() const noexcept {
    return nombreProyecto;
}

void MenuModel::setNombreProyecto(const std::string& nombre) {
    nombreProyecto = nombre;
    if (onCampoCambio) onCampoCambio(Campo::Nombre);
}

const std::vector<std::string>& MenuModel::getProyectosDisponibles() const noexcept {
    return proyectosDisponibles;
}

void MenuModel::setProyectosDisponibles(const std::vector<std::string>& proyectos) {
    proyectosDisponibles = proyectos;
}

const std::string& MenuModel::getProyectoARenombrar() const noexcept {
    return proyectoARenombrar;
}

void MenuModel::setProyectoARenombrar(const std::string& nombre) {
    proyectoARenombrar = nombre;
}

void MenuModel::limpiarProyectoARenombrar() noexcept { proyectoARenombrar.clear(); }

const std::string& MenuModel::getProyectoAEliminar() const noexcept {
    return proyectoAEliminar;
}

void MenuModel::setProyectoAEliminar(const std::string& nombre) {
    proyectoAEliminar = nombre;
}

void MenuModel::limpiarProyectoAEliminar() noexcept { proyectoAEliminar.clear(); }

const std::string& MenuModel::getIdioma() const noexcept { return idioma; }

void MenuModel::setIdioma(const std::string& valor) {
    idioma = valor;
    if (onCampoCambio) onCampoCambio(Campo::Idioma);
}

const std::vector<std::string>& MenuModel::getIdiomas() const noexcept {
    return idiomasDisponibles;
}

float MenuModel::getSensibilidadCamara() const noexcept {
    return sensibilidadCamara;
}

void MenuModel::setSensibilidadCamara(float sensibilidad) {
    if (sensibilidad > 0.0f) {
        sensibilidadCamara = sensibilidad;
        if (onCampoCambio) onCampoCambio(Campo::SensibilidadCamara);
    }
}

float MenuModel::getSensibilidadMovimientoCamara() const noexcept {
    return sensibilidadMovimientoCamara;
}

void MenuModel::setSensibilidadMovimientoCamara(float sensibilidad) {
    if (sensibilidad > 0.0f) {
        sensibilidadMovimientoCamara = sensibilidad;
        if (onCampoCambio) onCampoCambio(Campo::SensibilidadMovimientoCamara);
    }
}

const Apariencia& MenuModel::getApariencia() const noexcept {
    return apariencia;
}

void MenuModel::setApariencia(const Apariencia& valor) {
    apariencia = valor;
    if (onCampoCambio) onCampoCambio(Campo::Apariencia);
}

void MenuModel::restablecerConfiguracion() {
    // Valores de fabrica de las opciones del menu. El nombre del proyecto NO se
    // toca: define la carpeta/proyecto (src<Nombre> + Memory) y un reset aqui
    // crearia otra carpeta y perderia la escena activa.
    idioma = "Espanol";
    sensibilidadCamara = 0.15f;
    sensibilidadMovimientoCamara = 1.0f;
    apariencia.restablecer();
    if (onCampoCambio) onCampoCambio(Campo::Reiniciar);
}

void MenuModel::setOnCampoCambio(OnCampoCambio cb) {
    onCampoCambio = std::move(cb);
}

std::string MenuModel::traducir(const std::string& clave) const {
    // Diccionario declarativo es/en del menu. Un clave faltante se devuelve
    // tal cual para que la interfaz nunca quede en blanco ni rompa.
    struct Entrada {
        const char* es;
        const char* en;
    };
    static const std::pair<std::string, Entrada> diccionario[] = {
        {"iniciar_estudio", {"Iniciar Estudio", "Start Studio"}},
        {"proyectos", {"Proyectos", "Projects"}},
        {"sin_proyectos",
         {"(no hay proyectos: crea uno en Config Proyect)",
          "(no projects: create one in Config Project)"}},
        {"config_proyecto", {"Config Proyect", "Config Project"}},
        {"opciones", {"Opciones", "Options"}},
        {"configuracion", {"Configuracion", "Settings"}},
        {"salir", {"Exit", "Exit"}},
        {"volver", {"Volver", "Back"}},
        {"juego", {"Juego", "Game"}},
        {"idioma", {"Idioma", "Language"}},
        {"sensibilidad_camara", {"Sensibilidad de camara", "Camera sensitivity"}},
        {"sensibilidad_movimiento",
         {"Sensibilidad de movimiento", "Movement sensitivity"}},
        {"apariencia", {"Apariencia", "Appearance"}},
        {"tema_claro", {"Tema claro de la interfaz", "Light UI theme"}},
        {"modo_bn",
         {"Modo blanco y negro (interfaz y viewport)",
          "Black & white mode (UI and viewport)"}},
        {"color_acento", {"Color de acento de la interfaz", "UI accent color"}},
        {"color_fondo", {"Color de fondo de la escena", "Scene background color"}},
        {"ayuda_bn",
         {"El modo blanco y negro desatura toda la interfaz (incluido el\n"
          "acento) y usa blanco/negro en el fondo y la grilla del viewport\n"
          "segun el tema claro u oscuro.",
          "Black & white mode desaturates the whole UI (accent included) and\n"
          "uses black/white for the background and viewport grid according\n"
          "to the light or dark theme."}},
        {"restablecer_apariencia", {"Restablecer apariencia", "Reset appearance"}},
        {"restablecer_configuracion",
         {"Restablecer configuracion", "Reset configuration"}},
        {"ayuda_reset",
         {"Reinicia idioma, apariencia, sensibilidades y estado del editor a\n"
           "los valores de fabrica.",
           "Resets language, appearance, sensitivities and editor\n"
           "state to factory defaults."}},
        {"nombre", {"Nombre", "Name"}},
        {"editar_nombre", {"Editar nombre", "Edit name"}},
        {"eliminar_proyecto", {"Eliminar proyecto", "Delete project"}},
        {"aviso_eliminar",
         {"Se eliminaran de disco la escena, los assets y la configuracion\n"
          "de este proyecto. Esta accion no se puede deshacer.",
          "The scene, assets and settings of this project will be removed\n"
          "from disk. This action cannot be undone."}},
        {"eliminar", {"Eliminar", "Delete"}},
        {"confirmar", {"Confirmar", "Confirm"}},
        {"actual", {"(actual: %s)", "(current: %s)"}},
    };
    const bool ingles = (idioma == "English");
    for (const auto& [c, t] : diccionario) {
        if (c == clave) return ingles ? t.en : t.es;
    }
    return clave;
}