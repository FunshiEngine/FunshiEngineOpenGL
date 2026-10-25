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
#include "GestorDeProyectos.h"

#include <filesystem>
#include <iostream>

#include "../GUIManager/GUIManager.h"
#include "../GUI/MenusGUI/MenuGUI.h"
#include "../Scenes/GameScene.h"
#include "../Scenes/RutasReescritura.h"

namespace fs = std::filesystem;

GestorDeProyectos::GestorDeProyectos(EditorConfig& config, GameScene* escena,
                                     GUIManager* gui, MenuGUI* menu) noexcept
    : config_(config), scene_(escena), gui_(gui), menu_(menu) {}

void GestorDeProyectos::aplicarImguiIniDelProyectoActual() {
    layout_.aplicarProyecto(proyectoActual_);
}

void GestorDeProyectos::prepararProyectoAlArrancar() {
    if (proyectoActual_.empty())
        return;
    EditorConfig::asegurarEstructuraProyecto(proyectoActual_);
    // Recargar la configuracion del proyecto resuelto: al arrancar con
    // --proyecto la config general solo trae los datos globales, y sin esta
    // recarga el editor arrancaria con la dimension, estados de ventanas y
    // gizmo de la config anterior (o con los defaults) en vez de los del
    // proyecto pedido.
    config_.cargarProyecto(proyectoActual_);
    gui_->configurarProyecto(proyectoActual_);
    // Estructura del proyecto y puntero del creador a Memory/Interfaces
    // (debe correr antes del primer frame).
    scene_->configurarProyecto(proyectoActual_);
    // La barra de menu tiene que saber que proyecto esta activo: el dialogo
    // de exportacion sale de ahi. Sin este reflejo, arrancar con --proyecto
    // dejaba el dialogo sin proyecto y la exportacion no se podia lanzar.
    reflejarProyectoEnMenuBar();
}

void GestorDeProyectos::guardarProyectoCompleto() {
    if (proyectoActual_.empty()) {
        // Sin proyecto abierto no hay nada que guardar, pero el aviso tiene que
        // llegar: antes se salia en silencio y Ctrl+S parecia no hacer nada.
        std::cerr << "[guardar] no hay proyecto abierto: nada que guardar\n";
        if (auto* status = gui_->getStatusBarGUI())
            status->mostrarMensaje("No hay proyecto abierto: nada que guardar");
        return;
    }
    // Escena completa: binarios (.db) + manifiesto de assets (JSON).
    scene_->saveScene(EditorConfig::rutaScenePrefijo(proyectoActual_));
    // Se recogen los valores actuales (menu, ventanas, gizmo) tal como se
    // hace al salir: pudieron cambiar en la sesion.
    EditorConfig::Datos& cfg = config_.datos();
    cfg.nombreProyecto = menu_->getNombreProyecto();
    cfg.idioma = menu_->getIdioma();
    cfg.sensibilidadCamara = menu_->getSensibilidadCamara();
    cfg.ventanaCamarasAbierta = scene_->getVentanaCamarasAbierta();
    cfg.sensibilidadMovimientoCamara = scene_->getSensibilidadMovimientoCamara();
    cfg.gizmoOperacion = numeroDeOperacion(scene_->getGizmoOperation());
    cfg.gizmoGlobal = scene_->isGizmoGlobal();
    cfg.camaraActivaId = scene_->getActiveCameraId();
    cfg.estadoVentanas = gui_->obtenerEstadosVentanas();
    cfg.apariencia = menu_->getApariencia();
    config_.guardarGeneral();
    if (!cfg.nombreProyecto.empty())
        config_.guardarProyecto(cfg.nombreProyecto);
    // Mostrar mensaje en la barra de estado
    if (auto* status = gui_->getStatusBarGUI())
        status->mostrarMensaje("Proyecto guardado (Ctrl+S)");
}

void GestorDeProyectos::sincronizarProyectoDesdeMenu() {
    // Sincroniza cambio de nombre de proyecto si se edito en Config Proyect
    // (o se eligio una carpeta en el listado del menu). Reconfigura el
    // FileManager y, si aun no habia proyecto, fija el imgui.ini del nuevo
    // proyecto en lugar de quedarse sin ini.
    // Renombre literal: solo si Confirmar vino de click derecho
    // (MenuGUI::getProyectoARenombrar). Destino existente = conmutar;
    // destino libre = renombrar la carpeta en disco y luego conmutar.
    // Elegir/Crear: la intencion (MenuGUI::getAccionProyecto) solo informa
    // mensajes; la autoridad crear-vs-abrir es la existencia de la carpeta en
    // disco. Nunca se crea una carpeta que el usuario no pidio crear, y nunca
    // se pisa la configuracion de un proyecto existente.
    const std::string nombreMenu = menu_->getNombreProyecto();
    if (!nombreMenu.empty() && nombreMenu != proyectoActual_) {
        const std::string aRenombrar = menu_->getProyectoARenombrar();
        if (!aRenombrar.empty()) {
            // Flujo de renombre (click derecho -> Editar nombre).
            const bool destinoExiste =
                fs::exists(EditorConfig::directorioProyecto(nombreMenu));
            if (!destinoExiste) {
                // Guardar el estado vigente ANTES de mover la carpeta
                // (la vieja desaparece); luego renombrar y entrar.
                if (aRenombrar == proyectoActual_)
                    guardarEstadoActivo();
                if (EditorConfig::renombrarProyecto(aRenombrar, nombreMenu)) {
                    menu_->limpiarProyectoARenombrar();
                    if (aRenombrar == proyectoActual_) {
                        entrar(nombreMenu);
                    } else {
                        // Carpeta renombrada en segundo plano: el estado del
                        // activo ya se salvo en el flujo normal de abajo al
                        // conmutar hacia el nuevo nombre.
                        guardarEstadoActivo();
                        entrar(nombreMenu);
                    }
                } else {
                    // Fallo de E/S: no renombrar, no conmutar.
                    menu_->limpiarProyectoARenombrar();
                }
            } else {
                // Destino ocupado: no tocar carpetas, solo conmutar.
                menu_->limpiarProyectoARenombrar();
                guardarEstadoActivo();
                entrar(nombreMenu);
            }
        } else {
            const bool destinoExiste =
                fs::exists(EditorConfig::directorioProyecto(nombreMenu));
            const MenuModel::AccionProyecto accion =
                menu_->getAccionProyecto();
            const bool esCrear = accion == MenuModel::AccionProyecto::Crear;
            bool proceder = true;
            bool crearNuevo = false;
            if (esCrear && destinoExiste) {
                // Defensa: el dialogo de creacion ya bloquea nombres repetidos.
                // Si una carrera creo la carpeta entre medio, no se pisa su
                // configuracion: se avisa y se abre la existente.
                std::cerr << "[proyectos] '" << nombreMenu
                          << "' ya existe: no se crea\n";
                if (auto* status = gui_->getStatusBarGUI())
                    status->mostrarMensaje(
                        "El proyecto ya existe: no se crea uno nuevo");
            } else if (!destinoExiste && !esCrear) {
                // Abrir (o un nombre sin accion explicita) hacia una carpeta
                // inexistente no debe inventar un proyecto nuevo en silencio.
                std::cerr << "[proyectos] '" << nombreMenu
                          << "' no existe: no se crea\n";
                if (auto* status = gui_->getStatusBarGUI())
                    status->mostrarMensaje(
                        "El proyecto no existe: no se va a crear");
                proceder = false;
            } else if (esCrear) {
                crearNuevo = true;
            }
            if (proceder) {
                // Guardar el estado del proyecto que se abandona ANTES de tocar
                // la configuracion en memoria (los datos son compartidos por
                // ambos proyectos).
                guardarEstadoActivo();
                if (crearNuevo) {
                    // Proyecto nuevo: partir de los defaults de un proyecto sin
                    // configuracion y persistir la dimension elegida ANTES de
                    // entrar, para que cargarProyecto no la reemplace con su
                    // default (3D).
                    config_.cargarProyecto(nombreMenu);
                    config_.datos().dimension = menu_->getDimensionProyecto();
                    config_.guardarProyecto(nombreMenu);
                }
                entrar(nombreMenu);
            }
        }
    } else {
        // Limpiar registro vencido (nombre igual al activo o vacio).
        menu_->limpiarProyectoARenombrar();
    }
    // La intencion se consume en cada sincronizacion, se convierta o no en
    // cambio de proyecto (una defensa avisa y no entra).
    menu_->limpiarAccionProyecto();
}

void GestorDeProyectos::eliminarProyectoDesdeMenu() {
    // Eliminacion de proyecto (click derecho -> Eliminar proyecto),
    // confirmada desde el modal. Consumo unico por confirmacion: se borra la
    // carpeta de disco y, si era el proyecto activo, se vuelve al estado "sin
    // proyecto" del primer arranque. La escena en memoria se descarta con la
    // proxima carga de escena y al salir no se guarda (guardia
    // `if (!proyectoActual_.empty())` de guardarProyectoCompleto); el
    // explorador queda apuntando a un src inexistente hasta que se elija otro
    // proyecto (mismo estado que sin proyecto elegido).
    if (const std::string& aEliminar = menu_->getProyectoAEliminar();
        !aEliminar.empty()) {
        const bool eraActivo = (aEliminar == proyectoActual_);
        if (EditorConfig::eliminarProyecto(aEliminar)) {
            if (eraActivo) {
                proyectoActual_.clear();
                config_.datos().nombreProyecto.clear();
                config_.guardarGeneral();
                menu_->setNombreProyecto("");
                // Sin proyecto: las rutas se guardan/cargan sin relativizar
                // (passthrough) hasta elegir uno nuevo.
                EditorConfig::limpiarRaizAssets();
                // Olvidar el layout del proyecto borrado: ImGui no debe
                // reescribirlo (ni recrear su folder) al salir.
                layout_.aplicarProyecto("");
            }
        }
        menu_->limpiarProyectoAEliminar();
    }
}

void GestorDeProyectos::procesarCargaIniDiferida() {
    layout_.procesarCargaDiferida();
}

void GestorDeProyectos::reflejarProyectoEnMenuBar() const {
    // Actualizar proyecto actual en el menu bar para exportación (solo si hay
    // un proyecto: sin el no se pone ningun nombre).
    if (proyectoActual_.empty())
        return;
    if (auto* menuBar = gui_->getMenuBarGUI())
        menuBar->setProyectoActual(proyectoActual_);
}

void GestorDeProyectos::manejarArchivosReubicados(
    const std::string& rutaAnterior, const std::string& rutaNueva) {
    // Archivos/carpetas movidos o renombrados en el explorador (arbol o grid):
    // se reescriben en memoria las referencias de la escena cuyo path cayo bajo
    // la ruta anterior (mallas, texturas, fuentes de script) y se persiste al
    // instante para dejar los binarios en el mismo estado. Los paneles e
    // interfaces no se tocan: se referencian por nombre, no por ruta.
    if (proyectoActual_.empty() || rutaAnterior.empty() || rutaNueva.empty())
        return;
    const int cambios = RutasReescritura::reescribirEnEscena(
        scene_->getGameObjectsScene(), rutaAnterior, rutaNueva);
    if (cambios > 0) {
        std::cout << "Referencias reescritas (" << cambios << ") por '"
                  << rutaAnterior << "' -> '" << rutaNueva << "'\n";
        scene_->saveScene(EditorConfig::rutaScenePrefijo(proyectoActual_));
    }
}

void GestorDeProyectos::guardarEstadoActivo() {
    // Guardar el estado del proyecto actual antes de cambiar (solo si ya habia
    // un proyecto cargado).
    if (proyectoActual_.empty())
        return;
    config_.datos().ventanaCamarasAbierta = scene_->getVentanaCamarasAbierta();
    config_.datos().gizmoOperacion =
        numeroDeOperacion(scene_->getGizmoOperation());
    config_.datos().gizmoGlobal = scene_->isGizmoGlobal();
    config_.datos().camaraActivaId = scene_->getActiveCameraId();
    config_.datos().estadoVentanas = gui_->obtenerEstadosVentanas();
    config_.guardarProyecto(proyectoActual_);
}

void GestorDeProyectos::entrar(const std::string& destino) {
    // Cambiar al nuevo proyecto
    proyectoActual_ = destino;
    // Contexto de rutas de la serializacion: la raiz de assets del proyecto
    // entrante (src<nombre>). Debe fijarse ANTES de loadScene: las escenas
    // nuevas guardan rutas relativas a ella.
    EditorConfig::fijarRaizAssets(EditorConfig::directorioSrc(proyectoActual_));
    EditorConfig::asegurarEstructuraProyecto(proyectoActual_);
    gui_->configurarProyecto(proyectoActual_);
    // Actualizar proyecto actual en el menu bar para exportación.
    reflejarProyectoEnMenuBar();
    // Completar la estructura del proyecto entrante.
    scene_->configurarProyecto(proyectoActual_);
    config_.datos().nombreProyecto = proyectoActual_;
    // Conmutar el layout de la interfaz: se vuelca el del proyecto que se
    // abandona y se deja preparado el del entrante (la carga se difiere al
    // frame siguiente, cuando sus ventanas ya existen).
    layout_.conmutarProyecto(proyectoActual_);
    // Cargar la configuracion del proyecto entrante ANTES de restaurar el dock:
    // necesitamos estadoVentanas (visibilidad) para que las ventanas existan
    // en g.Windows ANTES de LoadIniSettingsFromDisk, que itera g.Windows
    // para re-aplicar DockId. Si la visibilidad se restaura después, las
    // ventanas no están en g.Windows y nacen flotando (se separan del dock).
    config_.cargarProyecto(proyectoActual_);
    gui_->restaurarEstadosVentanas(config_.datos().estadoVentanas);

    // Cargar la escena del nuevo proyecto si existe. La dimension se fija ANTES
    // de cargar: la siembra de la grilla y la camara inicial dependen de ella.
    scene_->setDimension(config_.datos().dimension);
    scene_->loadScene(EditorConfig::rutaSceneBBDD(proyectoActual_),
                      EditorConfig::rutaSceneDir(proyectoActual_));

    // Aplicar resto de estado de la escena (gizmo, cámara, etc.)
    scene_->setVentanaCamarasAbierta(config_.datos().ventanaCamarasAbierta);
    scene_->setGizmoOperation(
        operacionDesdeNumero(config_.datos().gizmoOperacion));
    scene_->setGizmoGlobal(config_.datos().gizmoGlobal);
    scene_->setActiveCameraById(config_.datos().camaraActivaId);
}