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
#include "../TablasTextos.h"

// Textos en ingles. Una clave que falte aqui se muestra en espanol.
namespace Localizacion {
namespace Detalle {

const std::vector<EntradaTexto>& textosEn() {
    static const std::vector<EntradaTexto> tabla = {
        {"iniciar_estudio", "Start Studio"},
        {"proyectos", "Projects"},
        {"sin_proyectos", "(no projects: create one in Configure project)"},
        {"config_proyecto", "Configure project"},
        {"crear_proyecto", "Create project"},
        {"importar_proyecto", "Import project"},
        {"importar_no_disponible",
         "Importing a project is not available yet."},
        {"proyecto_actual", "Current project"},
        {"dimension", "Project type"},
        {"dimension_2d", "2D - Flat"},
        {"descripcion_2d",
         "For games seen from the front, like a sheet of paper: objects move "
         "across and up. Great for platformers, puzzles and arcade games."},
        {"dimension_3d", "3D - Depth"},
        {"descripcion_3d",
         "For worlds with depth, like the real world: objects move across, up "
         "and into the scene. Great for exploration, action and open worlds."},
        {"aviso_no_revertir",
         "Once created, the project type cannot be changed: you cannot switch "
         "from 2D to 3D or back. Choose carefully."},
        {"crear", "Create"},
        {"cancelar", "Cancel"},
        {"nombre_vacio", "Type a name for the project."},
        {"nombre_repetido", "A project with that name already exists."},
        {"opciones", "Options"},
        {"configuracion", "Settings"},
        {"salir", "Exit"},
        {"volver", "Back"},
        {"juego", "Game"},
        {"idioma", "Language"},
        {"radio_difuminado", "Fade radius"},
        {"sensibilidad_camara", "Camera sensitivity"},
        {"sensibilidad_movimiento", "Movement sensitivity"},
        {"apariencia", "Appearance"},
        {"audio", "Audio"},
        {"controles", "Controls"},
        {"volumen_maestro", "Master volume"},
        {"tema_claro", "Light UI theme"},
        {"modo_bn", "Black & white mode (UI and viewport)"},
        {"color_acento", "UI accent color"},
        {"color_cielo_superior", "Sky top color"},
        {"color_cielo_inferior", "Sky bottom color"},
        {"ayuda_bn",
         "Black & white mode desaturates the whole UI (accent included) and\n"
         "uses black/white for the background and viewport grid according\n"
         "to the light or dark theme."},
        {"restablecer_apariencia", "Reset appearance"},
        {"restablecer_configuracion", "Reset configuration"},
        {"ayuda_reset",
         "Resets language, appearance, sensitivities and editor\n"
         "state to factory defaults."},
        {"nombre", "Name"},
        {"editar_nombre", "Edit name"},
        {"eliminar_proyecto", "Delete project"},
        {"aviso_eliminar",
         "The scene, assets and settings of this project will be removed\n"
         "from disk. This action cannot be undone."},
        {"eliminar", "Delete"},
        {"confirmar", "Confirm"},
        {"actual", "(current: %s)"},
    };
    return tabla;
}

} // namespace Detalle
} // namespace Localizacion
