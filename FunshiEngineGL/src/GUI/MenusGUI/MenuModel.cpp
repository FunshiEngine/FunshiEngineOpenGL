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
#include "../../Localizacion/Localizacion.h"

#include <algorithm>
#include <cctype>
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
        // Al volver de ConfigProyecto se reinicia la seccion de la barra a
        // "Crear proyecto": es la seccion con la que abre la vista en cada
        // entrada, segun el diseno de F1.
        if (vista == Vista::ConfigProyecto) {
            seccionConfig = SeccionConfig::CrearProyecto;
        }
        vista = Vista::Principal;
    }
}

bool MenuModel::estaVisible() const noexcept { return vista != Vista::Ninguna; }

MenuModel::Vista MenuModel::getVista() const noexcept { return vista; }

MenuModel::SeccionConfig MenuModel::getSeccionConfig() const noexcept {
    return seccionConfig;
}

void MenuModel::setSeccionConfig(SeccionConfig seccion) {
    seccionConfig = seccion;
}

MenuModel::AccionProyecto MenuModel::getAccionProyecto() const noexcept {
    return accionProyecto;
}

void MenuModel::setAccionProyecto(AccionProyecto accion) {
    accionProyecto = accion;
}

void MenuModel::limpiarAccionProyecto() noexcept {
    accionProyecto = AccionProyecto::Ninguna;
}

DimensionProyecto MenuModel::getDimensionProyecto() const noexcept {
    return dimensionProyecto;
}

void MenuModel::setDimensionProyecto(DimensionProyecto dimension) {
    dimensionProyecto = dimension;
}

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

bool MenuModel::existeProyecto(const std::string& nombre) const {
    if (nombre.empty()) return false;
#ifdef _WIN32
    // El filesystem de Windows no distingue mayusculas: "Foo" y "foo" son la
    // misma carpeta, asi que el cotejo tambien las trata como iguales.
    auto minusculas = [](std::string s) {
        for (char& c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    const std::string objetivo = minusculas(nombre);
    for (const std::string& p : proyectosDisponibles)
        if (minusculas(p) == objetivo) return true;
    return false;
#else
    return std::find(proyectosDisponibles.begin(), proyectosDisponibles.end(),
                     nombre) != proyectosDisponibles.end();
#endif
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
    // Acepta codigos ("en") y nombres guardados por versiones viejas ("English").
    idioma = Localizacion::normalizarIdioma(valor);
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

float MenuModel::getVolumenMaestro() const noexcept { return volumenMaestro; }

void MenuModel::setVolumenMaestro(float volumen) {
    if (volumen >= 0.0f && volumen <= 1.0f) {
        volumenMaestro = volumen;
        if (onCampoCambio) onCampoCambio(Campo::VolumenMaestro);
    }
}

const Apariencia& MenuModel::getApariencia() const noexcept {
    return apariencia;
}

void MenuModel::setApariencia(const Apariencia& valor) {
    // Apariencia tiene operator== justo para esto: reaplicar el mismo perfil no
    // debe republicar ni reescribir el tema. En particular evita que el
    // setApariencia inicial del arranque se considere un cambio del usuario.
    if (apariencia == valor) return;
    apariencia = valor;
    if (onCampoCambio) onCampoCambio(Campo::Apariencia);
}

void MenuModel::restablecerConfiguracion() {
    // Valores de fabrica de las opciones del menu. El nombre del proyecto NO se
    // toca: define la carpeta/proyecto (src<Nombre> + Memory) y un reset aqui
    // crearia otra carpeta y perderia la escena activa.
    idioma = Localizacion::kIdiomaBase;
    sensibilidadCamara = 0.15f;
    sensibilidadMovimientoCamara = 1.0f;
    volumenMaestro = 1.0f;
    apariencia.restablecer();
    if (onCampoCambio) onCampoCambio(Campo::Reiniciar);
}

void MenuModel::setOnCampoCambio(OnCampoCambio cb) {
    onCampoCambio = std::move(cb);
}

std::string MenuModel::traducir(const std::string& clave) const {
    // Los textos viven en Localizacion (una tabla por idioma, con respaldo al
    // espanol y a la clave). El modelo solo aporta el idioma elegido.
    return Localizacion::texto(idioma, clave);
}