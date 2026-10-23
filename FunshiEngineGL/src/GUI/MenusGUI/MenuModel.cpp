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
    idioma = "Espanol";
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
        {"crear_proyecto", {"Crear proyecto", "Create project"}},
        {"importar_proyecto", {"Importar proyecto", "Import project"}},
        {"importar_no_disponible",
         {"Importar un proyecto todavia no esta disponible.",
          "Importing a project is not available yet."}},
        {"proyecto_actual", {"Proyecto actual", "Current project"}},
        {"dimension", {"Tipo de proyecto", "Project type"}},
        {"dimension_2d", {"2D - Plano", "2D - Flat"}},
        {"descripcion_2d",
         {"Para juegos vistos de frente, como una hoja de papel: los objetos se "
          "mueven a lo ancho y a lo alto. Ideal para plataformas, puzzles y "
          "juegos arcade.",
          "For games seen from the front, like a sheet of paper: objects move "
          "across and up. Great for platformers, puzzles and arcade games."}},
        {"dimension_3d", {"3D - Profundidad", "3D - Depth"}},
        {"descripcion_3d",
         {"Para mundos con profundidad, como el mundo real: los objetos se "
          "mueven a lo ancho, a lo alto y hacia adentro. Ideal para "
          "exploracion, accion y mundos abiertos.",
          "For worlds with depth, like the real world: objects move across, up "
          "and into the scene. Great for exploration, action and open worlds."}},
        {"aviso_no_revertir",
         {"Una vez creado, el tipo de proyecto no se puede cambiar: no se puede "
          "pasar de 2D a 3D ni al reves. Elegi con cuidado.",
          "Once created, the project type cannot be changed: you cannot switch "
          "from 2D to 3D or back. Choose carefully."}},
        {"crear", {"Crear", "Create"}},
        {"cancelar", {"Cancelar", "Cancel"}},
        {"nombre_vacio",
         {"Escribi un nombre para el proyecto.", "Type a name for the project."}},
        {"nombre_repetido",
         {"Ya existe un proyecto con ese nombre.",
          "A project with that name already exists."}},
        {"opciones", {"Opciones", "Options"}},
        {"configuracion", {"Configuracion", "Settings"}},
        {"salir", {"Exit", "Exit"}},
        {"volver", {"Volver", "Back"}},
        {"juego", {"Juego", "Game"}},
        {"idioma", {"Idioma", "Language"}},
        {"radio_difuminado", {"Radio de difuminado", "Fade radius"}},
        {"sensibilidad_camara", {"Sensibilidad de camara", "Camera sensitivity"}},
        {"sensibilidad_movimiento",
         {"Sensibilidad de movimiento", "Movement sensitivity"}},
        {"apariencia", {"Apariencia", "Appearance"}},
        {"audio", {"Audio", "Audio"}},
        {"controles", {"Controles", "Controls"}},
        {"volumen_maestro", {"Volumen maestro", "Master volume"}},
        {"tema_claro", {"Tema claro de la interfaz", "Light UI theme"}},
        {"modo_bn",
         {"Modo blanco y negro (interfaz y viewport)",
          "Black & white mode (UI and viewport)"}},
        {"color_acento", {"Color de acento de la interfaz", "UI accent color"}},
        {"color_cielo_superior",
         {"Color de la parte superior del cielo", "Sky top color"}},
        {"color_cielo_inferior",
         {"Color de la parte inferior del cielo", "Sky bottom color"}},
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