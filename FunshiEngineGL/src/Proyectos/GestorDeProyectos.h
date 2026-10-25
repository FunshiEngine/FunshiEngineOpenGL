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
#ifndef GESTOR_PROYECTOS_H
#define GESTOR_PROYECTOS_H

#include <string>

#include "../Configuracion/EditorConfig.h"
#include "LayoutInterfazProyecto.h"

struct ImGuiIO;
class GameScene;
class GUIManager;
class MenuGUI;

// Encapsula el ciclo de vida del proyecto activo en el editor: conservar el
// nombre vigente, entrar a otro proyecto, guardar su estado (ventanas, gizmo,
// camara) y su escena+config (Ctrl+S), renombrarlo/conmutar y eliminarlo desde
// el menu, exportarlo y reescibir rutas de la escena al mover archivos en el
// explorador.
//
// El layout de la interfaz del proyecto (imgui.ini) no es parte de ese ciclo:
// vive en LayoutInterfazProyecto, que decide que archivo usa ImGui y carga el
// del entrante de forma diferida.
//
// Las dependencias se inyectan (mismo patron que GameScene: no se acopla el
// flujo de proyectos a la escena/GUI por includes directos): EditorConfig
// (persistencia y CRUD de proyectos), la escena, el orquestador de GUI y la
// fachada del menu de inicio. El ImGuiIO se fija despues (ImGui::CreateContext
// corre mas tarde en el arranque). La decision de QUEDARSE del usuario (que
// proyecto selecciono, confirmo renombrar, pidio eliminar) la lee el gestor de
// la fachada MenuGUI.
class GestorDeProyectos {
public:
    GestorDeProyectos(EditorConfig& config, GameScene* escena, GUIManager* gui,
                      MenuGUI* menu) noexcept;

    const std::string& proyectoActual() const noexcept { return proyectoActual_; }
    void fijarProyectoActual(const std::string& nombre) noexcept {
        proyectoActual_ = nombre;
    }
    bool hayProyecto() const noexcept { return !proyectoActual_.empty(); }

    // ImGuiIO se crea con ImGui::CreateContext() en el arranque de main, por eso
    // aqui es un setter y no va en el constructor. Antes de llamarla, las
    // operaciones que tocan el layout simplemente no lo administran.
    void fijarImguiIO(ImGuiIO* io) noexcept { layout_.fijarImguiIO(io); }

    // Aplica al layout el proyecto que main resolvio al arrancar (o lo deja
    // sin layout si no hay proyecto: primer arranque sin "Nuevo Proyecto"
    // fantasma).
    void aplicarImguiIniDelProyectoActual();

    // Arranque con proyecto: asegura su estructura de carpetas y configura GUI y
    // escena (Audio/interfaces) antes del primer frame.
    void prepararProyectoAlArrancar();

    // Ctrl+S y cierre de la app: escena (binarios + manifiesto), config general
    // y config del proyecto. Sin proyecto no toca nada.
    void guardarProyectoCompleto();

    // Orquesta el flujo del menu: si el usuario pidio renombrar una carpeta
    // (click derecho -> Editar nombre), elegir un proyecto del listado (Abrir)
    // o crearlo (Crear), guarda el estado del activo, decide crear-vs-abrir
    // segun exista la carpeta en disco, persiste la dimension de los proyectos
    // nuevos y entra al destino. La intencion del menu se consume por frame.
    void sincronizarProyectoDesdeMenu();

    // Eliminacion confirmada desde el modal del menu: borra la carpeta y, si era
    // el proyecto activo, vuelve al estado "sin proyecto" del primer arranque.
    void eliminarProyectoDesdeMenu();

    // Refleja en la barra de menu (Exportar) el proyecto activo. Sin proyecto,
    // no toca nada (el enunciado "no hay proyecto" queda vacio).
    void reflejarProyectoEnMenuBar() const;

    // Reescribe en memoria las referencias de la escena cuya ruta cayo bajo el
    // path movido (mallas/texturas/fuentes) y persiste al instante.
    void manejarArchivosReubicados(const std::string& rutaAnterior,
                                   const std::string& rutaNueva);

private:
    EditorConfig& config_;
    GameScene* scene_;
    GUIManager* gui_;
    MenuGUI* menu_;
    std::string proyectoActual_;
    // Layout de la interfaz del proyecto (imgui.ini): que archivo usa ImGui y
    // cuando se carga el del entrante.
    LayoutInterfazProyecto layout_;

    void guardarEstadoActivo();
    void entrar(const std::string& destino);
public:
    // Llamar al inicio de cada frame, DESPUÉS de ImGui::NewFrame().
    // Si hay una carga pendiente, la ejecuta y limpia la bandera.
    void procesarCargaIniDiferida();
};

#endif  // GESTOR_PROYECTOS_H