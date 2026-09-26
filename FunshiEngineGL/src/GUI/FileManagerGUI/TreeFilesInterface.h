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
#ifndef TREEFILESINTERFACE_H
#define TREEFILESINTERFACE_H

#include <set>
#include <string>

#include "../GeneralUserInterface.h"
#include "../../Herramientas/TreeGUI/TreeGUI.h"
#include "../WindowNames.h"

class File;
class Carpeta;
class FileManager;
class IconosGUI;
class EditorEventBus;

// Panel "BrowseFile": dibuja el arbol de carpetas del proyecto y es la unica
// vista que actualiza la seleccion compartida (FileSelection). Ya no posee
// modelo: conversa con la fachada FileManager (R1) y refleja los cambios del
// Filesystem comparando su ultimo contador con el de la seleccion (R3).
class TreeFilesInterface : public GeneralUserInterface {
protected:
    bool actualizar = false;
    FileManager* fileManager = nullptr;
    ArbolEnlazado<File*>* arbolDeArchivos = nullptr;
    IconosGUI* iconosGUI = nullptr;
    // Estado de colapso por RUTA (R4): sobrevive a la reconstruccion del arbol
    // (los punteros a File* quedan colgando tras un rescaneo). La clave es la
    // ruta completa de la carpeta, igual que FileSelection::rutaVisible.
    std::set<std::string> openPaths;
    // Eliminacion recursiva diferida: el menu contextual solo encola la RUTA
    // del nodo a borrar y esta se resuelve contra el arbol vigente al final
    // del frame (fuera del recorrido) para no invalidar iteradores (B4). Por
    // ruta, igual que openPaths: los punteros a File* quedan colgando cuando
    // un rescaneo reconstruye el arbol (mismo invariante que ya siguen
    // rutaVisible y navegacionPendiente).
    std::string carpetaAEliminar;
    // Confirmacion modal de "Eliminar Carpeta" (R7): ruta de la carpeta a
    // confirmar, re-resuelta a puntero SOLO dentro del modal contra el arbol
    // del frame. Un puntero guardado desde el menu quedaría colgando si un
    // rescaneo entra con el modal abierto.
    bool confirmarEliminar = false;
    std::string carpetaAConfirmar;
    // Renombrado inline de una carpeta en el arbol (R6), por RUTA igualmente.
    // El editor se dibuja en la fila cuya ruta coincide; si la carpeta deja de
    // existir tras un rescaneo el editor simplemente deja de dibujarse, sin
    // referenciar memoria liberada ni confundir filas (el heap reusa
    // la direccion del nodo viejo).
    std::string carpetaRenombrando;
    bool renombrandoInline = false;
    char bufferRenombrar[256] = "";
    // Ultimo contador de cambios que este panel ya rescaneco.
    unsigned long ultimoContadorVisto = 0;

    // Bandera de vida para el sistema de dock: SIEMPRE true. Garantiza que
    // la ventana exista en g.Windows cada frame para que ImGui pueda re-aplicar
    // su DockId al restaurar el imgui.ini del proyecto (LoadIniSettingsFromDisk
    // itera solo g.Windows; si la ventana no Begin()ea ese frame, nace suelta).
    // La visibilidad visual sigue controlada por stateGUI en printGUI().
    bool dockAlive_ = true;

public:
    TreeFilesInterface(bool stateGUI, FileManager* fileManager);
    TreeFilesInterface(const TreeFilesInterface&) = delete;
    TreeFilesInterface& operator=(const TreeFilesInterface&) = delete;

    EditorEventBus* eventoArchivos_ = nullptr;

    void setIconosGUI(IconosGUI* iconosG);
    // Bus de eventos del editor: se usa para notificar ArchivosReubicados tras
    // un rename exitoso (lo inyecta GUIManager; opcional, default nullptr).
    void setEditorEventBus(EditorEventBus* bus) noexcept;
    void solicitarActualizacion();

    virtual void initGUI() override;
    virtual void contentGUI() override;
    virtual void endGUI() override;
    virtual void printGUI() override;

private:
    void refrescarArbol();
    void aplicarNavegacionPendiente();
    void copiarElementoSuelto(const std::string& origen, const std::string& folderDest);
    TreeIG::RowResult drawFolderRow(File* element, bool wasOpen);
};

#endif