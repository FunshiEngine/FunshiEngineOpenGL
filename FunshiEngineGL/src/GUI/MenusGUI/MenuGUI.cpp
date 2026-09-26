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
#include "MenuGUI.h"
#include "../../Configuracion/EditorConfig.h"
#include "../../Events/EditorEventBus.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

MenuGUI::MenuGUI(GLFWwindow* window)
    : view(&model, &presenter, window), presenter(&model) {
    // Observer del modelo: TODA mutacion (setter de la fachada o clic del
    // usuario en la vista, que toca el modelo directo) publica su evento en el
    // bus. Sin esto, las ediciones de la vista Opciones nunca llegarian a la
    // escena/persistencia (las vistas solo mutan el modelo, no conocen el bus).
    model.setOnCampoCambio([this](MenuModel::Campo campo) {
        publicarCambio(campo);
    });
}

bool MenuGUI::ConsultarMenu() const noexcept {
    // Delegado en el presentador: main no interpreta el modelo directamente.
    return presenter.MenuEstaActivo();
}

void MenuGUI::SetMenuActivo(bool abierto) noexcept {
    if (abierto) {
        model.mostrarMenu();
    } else if (model.estaVisible()) {
        model.iniciarEstudio();
    }
}

bool MenuGUI::ConsultarCierre() noexcept {
    // Consumo unico: el presenter auto-limpia la peticion al entregarla.
    return presenter.DebeCerrar();
}

void MenuGUI::Renderizar() { view.printGUI(); }

void MenuGUI::PedirCierre() noexcept {
    presenter.PedirCierre();
}

float MenuGUI::getSensibilidadCamara() const noexcept {
    return model.getSensibilidadCamara();
}

void MenuGUI::setSensibilidadCamara(float sensibilidad) {
    model.setSensibilidadCamara(sensibilidad); // publica SensibilidadCambio
}

float MenuGUI::getSensibilidadMovimientoCamara() const noexcept {
    return model.getSensibilidadMovimientoCamara();
}

void MenuGUI::setSensibilidadMovimientoCamara(float sensibilidad) {
    model.setSensibilidadMovimientoCamara(
        sensibilidad); // publica SensibilidadMovimientoCambio
}

const Apariencia& MenuGUI::getApariencia() const noexcept {
    return model.getApariencia();
}

void MenuGUI::setApariencia(const Apariencia& valor) {
    model.setApariencia(valor); // publica AparienciaCambio
}

void MenuGUI::reiniciarConfiguracion() {
    model.restablecerConfiguracion(); // publica ReiniciarConfiguracion
}

const std::string& MenuGUI::getNombreProyecto() const noexcept {
    return model.getNombreProyecto();
}

void MenuGUI::setNombreProyecto(const std::string& nombre) noexcept {
    model.setNombreProyecto(nombre);
}

const std::string& MenuGUI::getProyectoARenombrar() const noexcept {
    return model.getProyectoARenombrar();
}

void MenuGUI::setProyectoARenombrar(const std::string& nombre) noexcept {
    model.setProyectoARenombrar(nombre);
}

void MenuGUI::limpiarProyectoARenombrar() noexcept {
    model.limpiarProyectoARenombrar();
}

const std::string& MenuGUI::getProyectoAEliminar() const noexcept {
    return model.getProyectoAEliminar();
}

void MenuGUI::setProyectoAEliminar(const std::string& nombre) noexcept {
    model.setProyectoAEliminar(nombre);
}

void MenuGUI::limpiarProyectoAEliminar() noexcept {
    model.limpiarProyectoAEliminar();
}

void MenuGUI::actualizarProyectos() {
    // Los proyectos son carpetas de MotorGrafico/Proyects/ (una por
    // proyecto: Memory + src<Nombre>). Vista unica, sin duplicar la logica;
    // los accesos fallidos se toleran silenciosamente (no hay proyectos).
    std::vector<std::string> proyectos;
    const std::string base = EditorConfig::directorioProyects();
    std::error_code ec;
    std::filesystem::directory_iterator it(base, ec);
    const std::filesystem::directory_iterator fin;
    for (; it != fin; it.increment(ec)) {
        if (ec) break;
        if (it->is_directory(ec)) {
            proyectos.push_back(it->path().filename().string());
        }
    }
    std::sort(proyectos.begin(), proyectos.end());
    model.setProyectosDisponibles(std::move(proyectos));
}

const std::string& MenuGUI::getIdioma() const noexcept {
    return model.getIdioma();
}

void MenuGUI::setIdioma(const std::string& valor) noexcept {
    model.setIdioma(valor); // publica IdiomaCambio
}

void MenuGUI::setEditorEventBus(EditorEventBus* bus) noexcept {
    busEditor = bus;
}

void MenuGUI::publicarCambio(MenuModel::Campo campo) {
    if (!busEditor) return;
    EditorEvent ev;
    switch (campo) {
    case MenuModel::Campo::Idioma:
        ev.type = EditorEventType::IdiomaCambio;
        ev.idioma = model.getIdioma();
        break;
    case MenuModel::Campo::SensibilidadCamara:
        ev.type = EditorEventType::SensibilidadCambio;
        ev.sensibilidad = model.getSensibilidadCamara();
        break;
    case MenuModel::Campo::SensibilidadMovimientoCamara:
        ev.type = EditorEventType::SensibilidadMovimientoCambio;
        ev.sensibilidadMovimiento = model.getSensibilidadMovimientoCamara();
        break;
    case MenuModel::Campo::Apariencia:
        ev.type = EditorEventType::AparienciaCambio;
        ev.apariencia = model.getApariencia();
        break;
    case MenuModel::Campo::Reiniciar:
        ev.type = EditorEventType::ReiniciarConfiguracion;
        break;
    case MenuModel::Campo::Nombre:
        // El nombre del proyecto se recoge al salir (EditorConfig); no tiene
        // consumidor en vivo en el bus.
        return;
    }
    busEditor->publish(ev);
}