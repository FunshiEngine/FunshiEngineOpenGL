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
#ifndef SOLTARENCARPETA_H
#define SOLTARENCARPETA_H

// Logica compartida del arrastre-y-suelta del explorador: el grid de contenido
// y el arbol de carpetas son dos vistas del MISMO arbol, asi que soltar el mismo
// elemento en una o en otra tiene que hacer lo mismo. Estar duplicado era el
// motivo de que el arbol copiara mientras el grid (nuevo) movia.
//
// Header-only a proposito: son unas pocas lineas y asi el helper no obliga a
// tocar la lista de fuentes de CMakeLists.txt.

#include <string>

#include "../../Events/EditorEventBus.h"
#include "../../FileManager/FileManager.h"
#include "../../FileManager/FileSelection.h"
#include "../../Herramientas/PathUtils.h"
#include <imgui.h>

// Ctrl (o Cmd en macOS) modifica el arrastre: sin ella mueve, con ella copia.
// Se lee una vez y se pasa a la operacion, en vez de releerla dentro, para que
// el tooltip y la accion no puedan discrepar.
inline bool ctrlOCmd() {
    const ImGuiIO& io = ImGui::GetIO();
    return io.KeyCtrl || io.KeySuper;
}

// Suelta el elemento arrastrado (payload "ARCHIVO_PATH", con la ruta absoluta
// en `origen`) dentro de la carpeta `destFolder`.
//
// Semantica: por defecto MUEVE, con Ctrl copia. Es lo que espera cualquiera que
// venga de un explorador de archivos, y evita el susto de ver como desaparece
// un archivo de su carpeta. El motivo real esta en el aviso del evento, abajo.
//
// Mover cambia la ruta, asi que hay que avisar al resto del editor: se publica
// ArchivosReubicados con la ruta anterior y la nueva, que es lo que ya consume
// el gestor de proyectos para reescribir las referencias de la escena (mallas,
// texturas, fuentes de script) y persistirlas. Sin ese aviso, mover un asset
// dejaria la escena apuntando a la ruta vieja.
//
// Devuelve true si la operacion se completo. Quien la llama decide que invalidar
// de su cache: el grid relee el listado de la carpeta visible, el arbol solo
// reconstruye.
inline bool soltarEnCarpeta(FileManager* fileManager,
                            EditorEventBus* eventoArchivos,
                            const std::string& origen,
                            const std::string& destFolder,
                            bool copiar) {
    if (fileManager == nullptr || origen.empty() || destFolder.empty())
        return false;

    const std::string::size_type sep = origen.find_last_of("/\\");
    const std::string nombre =
        (sep != std::string::npos) ? origen.substr(sep + 1) : origen;
    const std::string finalDest = destFolder + PATH_SEP + nombre;
    // Soltar sobre la carpeta que ya contiene al elemento no hace nada: sin
    // este chequeo el mover intentaria renombrar un archivo sobre si mismo.
    if (finalDest == origen) return false;

    const bool esCarpeta = fileManager->esDirectorio(origen);
    const bool ok = copiar
        ? (esCarpeta ? fileManager->copiarCarpeta(origen, finalDest)
                     : fileManager->copiarArchivo(origen, finalDest))
        : fileManager->mover(origen, finalDest);
    if (!ok) return false;

    // Solo un MOVIMIENTO cambia rutas: la copia deja las dos intactas y no hay
    // nada que reescribir.
    if (!copiar && eventoArchivos != nullptr) {
        EditorEvent ev;
        ev.type = EditorEventType::ArchivosReubicados;
        ev.rutaAnterior = origen;
        ev.rutaNueva = finalDest;
        eventoArchivos->publish(ev);
    }

    // El arbol de carpetas cambia si lo que se movio es una carpeta; los
    // archivos no aparecen ahi.
    if (esCarpeta) fileManager->getSelection()->contadorCambios++;
    return true;
}

#endif
