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
#ifndef CONTENTFOLDERINTERFACE_H
#define CONTENTFOLDERINTERFACE_H

#include <filesystem>
#include <string>
#include <vector>

#include "../../FileManager/FileManager.h"
#include "../GeneralUserInterface.h"

class IconosGUI;
class EditorEventBus;

// Panel "ShowFolder": muestra el contenido de la carpeta seleccionada en el
// arbol. Ya no se enlaza al arbol por puntero ni le pide el contenido: lee
// cada frame la seleccion compartida (FileSelection), se muestra a si mismo
// cuando hay carpeta (y se oculta si no) y notifica su navegacion por doble
// clic dejando la ruta pendiente en la seleccion (R3). Las operaciones de
// Filesystem van a la fachada FileManager (listado, dialogos nativos, abrir
// con la app del sistema, plantillas), nunca a system() o al Filesystem. (R1)
class ContentFolderInterface : public GeneralUserInterface {
private:
    FileManager* fileManager = nullptr;
    EditorEventBus* eventoArchivos_ = nullptr;
    bool abrirPopupNombre = false;
    bool creandoCarpeta = false;
    bool creandoScript = false;
    bool creandoScriptJava = false;
    char nombreNuevo[256] = "";
    IconosGUI* iconosGUI = nullptr;

    // Estado de renombrado (R6): ruta del elemento, si es carpeta (sube el
    // contador del arbol) y el buffer con el nombre a confirmar en el modal.
    std::string renombrarRuta;
    bool renombrarEsCarpeta = false;
    bool abrirPopupRenombrar = false;
    char bufferRenombrar[256] = "";

    // Cache del grid (R5): el directorio se lee en disco SOLO cuando cambia
    // la ruta mostrada o su mtime; el dibujo del grid usa este cache en vez
    // de re-scanear cada frame. Las entradas vienen de FileManager.
    std::vector<FileManager::EntradaDirectorio> cacheEntradas;

    // Bandera de vida para el sistema de dock: SIEMPRE true. Garantiza que
    // la ventana exista en g.Windows cada frame para que ImGui pueda re-aplicar
    // su DockId al restaurar el imgui.ini del proyecto (LoadIniSettingsFromDisk
    // itera solo g.Windows; si la ventana no Begin()ea ese frame, nace suelta).
    // La visibilidad visual sigue controlada por stateGUI/hayCarpeta en printGUI().
    bool dockAlive_ = true;
    std::string cacheCarpeta;
    std::filesystem::file_time_type cacheMtime{};

    void crearNuevoElemento();
    void copiarElementoSuelto(const std::string& origen, const std::string& destFolder);
    void recorrer(const std::string& path);

public:
    ContentFolderInterface(bool stateGUI, FileManager* fileManager);

    void setIconosGUI(IconosGUI* iconosG);
    // Bus de eventos del editor: notifica ArchivosReubicados tras un rename
    // exitoso del grid (lo inyecta GUIManager; opcional, default nullptr).
    void setEditorEventBus(EditorEventBus* bus) noexcept;

    virtual void initGUI() override;
    virtual void contentGUI() override;
    virtual void endGUI() override;
    virtual void printGUI() override;
};

#endif