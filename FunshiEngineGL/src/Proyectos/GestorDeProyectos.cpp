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

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>
#include <iostream>

#include "../GUIManager/GUIManager.h"
#include "../GUI/MenusGUI/MenuGUI.h"
#include "../Scenes/GameScene.h"
#include "../Scenes/RutasReescritura.h"

namespace fs = std::filesystem;

GestorDeProyectos::GestorDeProyectos(EditorConfig& config, GameScene* escena,
                                     GUIManager* gui, MenuGUI* menu) noexcept
    : config_(config), scene_(escena), gui_(gui), menu_(menu), io_(nullptr) {}

void GestorDeProyectos::aplicarImguiIniDelProyectoActual() {
    fijarImguiIni(proyectoActual_);
}

void GestorDeProyectos::prepararProyectoAlArrancar() {
    if (proyectoActual_.empty())
        return;
    EditorConfig::asegurarEstructuraProyecto(proyectoActual_);
    gui_->configurarProyecto(proyectoActual_);
    // Audio + interfaces: explora Sonidos/ y apunta el creador a
    // Memory/Interfaces del proyecto (debe correr antes del primer frame).
    scene_->configurarProyecto(proyectoActual_);
}

void GestorDeProyectos::guardarProyectoCompleto() {
    if (proyectoActual_.empty())
        return;
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
    cfg.gizmoOperacion = scene_->getGizmoOperation();
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
            guardarEstadoActivo();
            entrar(nombreMenu);
        }
    } else {
        // Limpiar registro vencido (nombre igual al activo o vacio).
        menu_->limpiarProyectoARenombrar();
    }
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
                // Olvidar el imgui.ini del proyecto borrado: ImGui no debe
                // reescribirlo (ni recrear su folder) al salir.
                fijarImguiIni("");
            }
        }
        menu_->limpiarProyectoAEliminar();
    }
}

void GestorDeProyectos::procesarCargaIniDiferida() {
    if (!pendingIniLoad_ || pendingIniPath_.empty() || !io_) return;
    // Ejecutar DESPUÉS de NewFrame(): g.Windows ya tiene ventanas del nuevo
    // proyecto con stateGUI restaurado. LoadIniSettingsFromDisk itera g.Windows
    // y re-aplica DockId correctamente.
    ImGui::LoadIniSettingsFromDisk(pendingIniPath_.c_str());
    // Fix LastFrameAlive: nodos creados desde ini empiezan en 0 -> undock.
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < g.DockContext.Nodes.Data.Size; ++n) {
        if (ImGuiDockNode* node = (ImGuiDockNode*)g.DockContext.Nodes.Data[n].val_p) {
            node->LastFrameAlive = g.FrameCount;
        }
    }
    pendingIniLoad_ = false;
    pendingIniPath_.clear();
}

void GestorDeProyectos::reflejarProyectoEnMenuBar() const {
    // Actualizar proyecto actual en el menu bar para exportación (solo si hay
    // un proyecto: sin el no se pone ningun nombre).
    if (proyectoActual_.empty())
        return;
    if (auto* menuBar = gui_->getMenuBarGUI())
        menuBar->setProyectoActual(proyectoActual_);
}

void GestorDeProyectos::exportarProyecto() {
    // Copia la carpeta del proyecto a <directorioBase>/Exportaciones/<proyecto>
    // para distribucion junto al ejecutable. El usuario lanza el juego con:
    // FunshiEngineGL --proyecto <nombre>.
    if (proyectoActual_.empty()) {
        if (auto* status = gui_->getStatusBarGUI())
            status->mostrarMensaje("Error: no hay proyecto abierto");
        return;
    }
    const std::string base = EditorConfig::directorioBaseMotorGrafico();
    const std::string origen = EditorConfig::directorioProyecto(proyectoActual_);
    const std::string destino = base + "/Exportaciones/" + proyectoActual_;
    std::error_code ec;
    fs::create_directories(fs::path(destino).parent_path(), ec);
    fs::copy(origen, destino,
                     fs::copy_options::recursive |
                         fs::copy_options::overwrite_existing,
                     ec);
    std::string msg;
    if (ec) {
        msg = "Error exportando: " + ec.message();
        std::cerr << msg << '\n';
    } else {
        msg = "Juego exportado a: " + destino;
        std::cout << msg << '\n';
    }
    if (auto* status = gui_->getStatusBarGUI())
        status->mostrarMensaje(msg);
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

void GestorDeProyectos::fijarImguiIni(const std::string& nombreProyecto) {
    // El imgui.ini (layout de docks y geometria de ventanas) se guarda en
    // Memory del proyecto del usuario, no en el directorio actual de
    // lanzamiento. Sin proyecto se deja ImGui sin ini en disco (primer
    // arranque: no se crean carpetas de "Nuevo Proyecto" por el camino). El
    // puntero que guarda ImGui apunta a este string (miembro de la clase): el
    // gestor vive toda la app, asi que es estable.
    ini_ =
        nombreProyecto.empty() ? std::string() : EditorConfig::rutaImguiIni(nombreProyecto);
    if (io_)
        io_->IniFilename = ini_.empty() ? nullptr : ini_.c_str();
}

void GestorDeProyectos::guardarEstadoActivo() {
    // Guardar el estado del proyecto actual antes de cambiar (solo si ya habia
    // un proyecto cargado).
    if (proyectoActual_.empty())
        return;
    config_.datos().ventanaCamarasAbierta = scene_->getVentanaCamarasAbierta();
    config_.datos().gizmoOperacion = scene_->getGizmoOperation();
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
    // Re-explorar Sonidos/ e interfaces del proyecto entrante.
    scene_->configurarProyecto(proyectoActual_);
    config_.datos().nombreProyecto = proyectoActual_;
    // Antes de conmutar el ini, persistir el layout del proyecto que se
    // abandona: ImGui solo carga el ini una vez por proceso (UpdateSettings,
    // en el primer NewFrame, pese a haber cargado SettingsLoaded, y ya ser
    // true con la bandera). Su guardado diferido escribe sobre la ruta
    // ACTIVA en el momento del volcado. Sin esto el layout no viaja con el
    // proyecto: el entrante nunca se restaura (las ventanas quedan en las
    // posiciones del anterior) y el volcado en curso contamina el ini ajeno.
    if (io_ && io_->IniFilename != nullptr) {
        ImGui::SaveIniSettingsToDisk(io_->IniFilename);
    }
    fijarImguiIni(proyectoActual_);
    // Cargar la configuracion del proyecto entrante ANTES de restaurar el dock:
    // necesitamos estadoVentanas (visibilidad) para que las ventanas existan
    // en g.Windows ANTES de LoadIniSettingsFromDisk, que itera g.Windows
    // para re-aplicar DockId. Si la visibilidad se restaura después, las
    // ventanas no están en g.Windows y nacen flotando (se separan del dock).
    config_.cargarProyecto(proyectoActual_);
    gui_->restaurarEstadosVentanas(config_.datos().estadoVentanas);

    // Carga diferida: LoadIniSettingsFromDisk debe ejecutarse DESPUÉS de
    // NewFrame() del frame SIGUIENTE. En ese momento g.Windows ya contiene
    // las ventanas del nuevo proyecto (con stateGUI restaurado) y ImGui
    // puede re-aplicar correctamente los DockId.
    if (io_ && io_->IniFilename != nullptr) {
        pendingIniLoad_ = true;
        pendingIniPath_ = io_->IniFilename;
    }

    // Cargar la escena del nuevo proyecto si existe
    scene_->loadScene(EditorConfig::rutaSceneBBDD(proyectoActual_),
                      EditorConfig::rutaSceneDir(proyectoActual_));

    // Aplicar resto de estado de la escena (gizmo, cámara, etc.)
    scene_->setVentanaCamarasAbierta(config_.datos().ventanaCamarasAbierta);
    scene_->setGizmoOperation(config_.datos().gizmoOperacion);
    scene_->setGizmoGlobal(config_.datos().gizmoGlobal);
    scene_->setActiveCameraById(config_.datos().camaraActivaId);
}