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
// La E/S de plataforma (dialogos nativos, xdg-open/ShellExecute, listado de
// directorio) vive en FileManager; esta vista solo conversa con la fachada.
#include "ContentFolderInterface.h"

#include <algorithm>
#include <cstring>

#include "../../Herramientas/PathUtils.h"
#include "../../Events/EditorEventBus.h"
#include "../../FileManager/FileManager.h"
#include "../../FileManager/FileSelection.h"
#include "../WindowNames.h"
#include "../../Herramientas/IconosGUI/IconosGUI.h"
#include <imgui.h>

ContentFolderInterface::ContentFolderInterface(bool stateGUI, FileManager* fileManager)
    : GeneralUserInterface(WindowNames::ShowFolder, stateGUI, ImGuiWindowFlags_MenuBar),
      fileManager(fileManager) {}

void ContentFolderInterface::setIconosGUI(IconosGUI* iconosG) { iconosGUI = iconosG; }

void ContentFolderInterface::setEditorEventBus(EditorEventBus* bus) noexcept {
    eventoArchivos_ = bus;
}

void ContentFolderInterface::crearNuevoElemento() {
    FileSelection* sel = fileManager->getSelection();
    if (!sel->carpetaActual || nombreNuevo[0] == '\0') return;

    const std::string destFolder =
        sel->carpetaActual->getPathRoot() + PATH_SEP +
        sel->carpetaActual->getPathName();

    if (creandoCarpeta) {
        const std::string ruta = destFolder + PATH_SEP + nombreNuevo;
        if (fileManager->crearCarpeta(ruta)) {
            // El arbol de carpetas cambia: se rescanceara al detectar el
            // contador (R3). La carpeta visible se conserva por rutaVisible.
            sel->contadorCambios++;
        }
    } else {
        std::string nombre = nombreNuevo;
        if (creandoScript || creandoScriptJava) {
            const std::string ext = creandoScriptJava ? ".java" : ".cpp";
            if (nombre.find(ext) == std::string::npos) nombre += ext;
        }

        // Script componente usa la convencion <ClassName>.ext: la clase es el
        // nombre sin extension. La plantilla vive en FileManager.
        std::string contenido;
        if (creandoScript || creandoScriptJava) {
            std::string clase = nombre;
            const size_t dot = clase.find_last_of('.');
            if (dot != std::string::npos) clase = clase.substr(0, dot);
            contenido = fileManager->plantillaScript(clase, creandoScriptJava);
        }

        const std::string ruta = destFolder + PATH_SEP + nombre;
        // No sube el contador: los archivos no aparecen en el arbol de
        // carpetas y no merece colapsar la navegacion por un rescaneo.
        fileManager->crearArchivo(ruta, contenido);
    }

    creandoCarpeta = false;
    creandoScript = false;
    creandoScriptJava = false;
    memset(nombreNuevo, 0, sizeof(nombreNuevo));
}

// Copia un elemento soltado via drag&drop (payload "ARCHIVO_PATH") a
// destFolder. Carpetas -> copiarCarpeta + rescaneo del arbol; archivos ->
// copiarArchivo (el arbol no los lista). Ignora soltar una carpeta sobre si
// misma (finalDest == origen) y deja que copiarCarpeta falle si el origen es
// su propio ancestro (recursion sobre si misma, el error_code lo corta).
void ContentFolderInterface::copiarElementoSuelto(const std::string& origen,
                                                  const std::string& destFolder) {
    if (origen.empty() || destFolder.empty()) return;
    FileSelection* sel = fileManager->getSelection();
    const std::string::size_type sep = origen.find_last_of("/\\");
    const std::string nombre = (sep != std::string::npos)
        ? origen.substr(sep + 1) : origen;
    const std::string finalDest = destFolder + PATH_SEP + nombre;
    if (finalDest == origen) return;
    if (fileManager->esDirectorio(origen)) {
        if (fileManager->copiarCarpeta(origen, finalDest)) sel->contadorCambios++;
    } else {
        fileManager->copiarArchivo(origen, finalDest);
    }
}

void ContentFolderInterface::recorrer(const std::string& path) {
    FileSelection* sel = fileManager->getSelection();

    // R5: re-scanear solo si cambio la ruta mostrada o el mtime del directorio
    // (mtime de un directorio sube al agregar/quitar entradas, justo lo que
    // pinta este panel; crear/renombrar/borrar dentro lo actualiza).
    const auto mtime = FileManager::mtimeDirectorio(path);
    if (path != cacheCarpeta || mtime != cacheMtime) {
        cacheCarpeta = path;
        cacheMtime = mtime;
        if (!FileManager::listarDirectorio(path, cacheEntradas))
            cacheEntradas.clear();
    }

    // Dibujo del grid desde el cache (mismo layout del grid de iconos).
    const float iconSize = 87.0f;
    const float spacing = 16.0f;
    const float stepX = iconSize + spacing;
    const float altoTexto = ImGui::GetTextLineHeightWithSpacing();
    const float altoCelda = iconSize + altoTexto;

    float xBase = ImGui::GetCursorPosX();
    float availX = ImGui::GetContentRegionAvail().x;
    int columnas = std::max(1, (int)((availX + spacing) / stepX));
    float yInicio = ImGui::GetCursorPosY();

    for (size_t i = 0; i < cacheEntradas.size(); ++i) {
        const FileManager::EntradaDirectorio& entrada = cacheEntradas[i];
        const std::string& nombre = entrada.nombre;
        const std::string& fullPath = entrada.ruta;
        const bool esCarpeta = entrada.esCarpeta;
        const std::string& extension = entrada.extension;
        size_t dot = nombre.find_last_of('.');

        std::string uniqueID = "##" + fullPath;

        int fila = (int)(i / (size_t)columnas);
        int col = (int)(i % (size_t)columnas);
        ImGui::SetCursorPosY(yInicio + fila * altoCelda);
        ImGui::SetCursorPosX(xBase + col * stepX);

        ImGui::BeginGroup();
        ImGui::PushID(uniqueID.c_str());

        ImTextureID icono = ImTextureID_Invalid;
        if (iconosGUI) {
            icono = esCarpeta ? iconosGUI->getIconoCarpeta() : iconosGUI->getIconoPorExtension(extension);
        }

        if (icono != ImTextureID_Invalid) {
            ImVec4 baseButton = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            ImGui::PushStyleColor(ImGuiCol_Button, baseButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(baseButton.x * 1.4f, baseButton.y * 1.4f, baseButton.z * 1.4f, baseButton.w));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(baseButton.x * 1.8f, baseButton.y * 1.8f, baseButton.z * 1.8f, baseButton.w));

            // Ignoramos el retorno de ImageButton (clic simple)
            ImGui::ImageButton(uniqueID.c_str(), icono, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0));

            ImGui::PopStyleColor(3);
        } else {
            const char* icon = esCarpeta ? "F" : "A";
            ImGui::Button(icon, ImVec2(iconSize, iconSize));
        }

        // R6: menu contextual de la celda -> Renombrar (archivo o carpeta).
        if (ImGui::BeginPopupContextItem("PopRenombrar")) {
            if (ImGui::MenuItem("Renombrar")) {
                renombrarRuta = fullPath;
                renombrarEsCarpeta = esCarpeta;
                memset(bufferRenombrar, 0, sizeof(bufferRenombrar));
                strncpy(bufferRenombrar, nombre.c_str(), sizeof(bufferRenombrar) - 1);
                abrirPopupRenombrar = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // --- DETECCION DE DOBLE CLIC (solo sobre el item bajo el cursor) ---
        // IsMouseDoubleClicked es un estado global por-frame: sin el check de
        // IsItemHovered, en el frame del doble clic reaccionaban TODAS las
        // celdas dibujadas (abriendo apps de varios archivos o navegando al
        // ultimo folder procesado, no al que estaba bajo el cursor).
        const bool isDoubleClicked =
            ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        if (isDoubleClicked) {
            if (esCarpeta && sel->carpetaActual) {
                // FASE 1: solo se registra la ruta a abrir; el arbol la
                // aplica al inicio de su contentGUI contra el arbol vigente.
                sel->navegacionPendiente =
                    sel->carpetaActual->getPathRoot() + PATH_SEP +
                    sel->carpetaActual->getPathName() + PATH_SEP + nombre;
            } else if (!esCarpeta) {
                // Abre el archivo con la app predeterminada del sistema (la E/S
                // nativa vive en FileManager; aqui solo se delega).
                FileManager::abrirConAppPredeterminada(fullPath);
            }
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("ARCHIVO_PATH", fullPath.c_str(), fullPath.size() + 1);
            ImGui::Text("Soltar en zona de destino");
            ImGui::Text("%s", nombre.c_str());
            ImGui::EndDragDropSource();
        }

        std::string nombreMostrado = (dot != std::string::npos && dot != 0) ? nombre.substr(0, dot) : nombre;
        bool truncado = false;
        if (ImGui::CalcTextSize(nombreMostrado.c_str()).x > iconSize) {
            while (!nombreMostrado.empty() && ImGui::CalcTextSize((nombreMostrado + "...").c_str()).x > iconSize)
                nombreMostrado.pop_back();
            nombreMostrado += "...";
            truncado = true;
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (iconSize - ImGui::CalcTextSize(nombreMostrado.c_str()).x) * 0.5f));
        ImGui::Text("%s", nombreMostrado.c_str());

        if (truncado && ImGui::IsItemHovered()) {
            if (!esCarpeta && !extension.empty())
                ImGui::SetTooltip("%s\nTipo: %s", nombre.c_str(), extension.c_str());
            else
                ImGui::SetTooltip("%s", nombre.c_str());
        }

        ImGui::PopID();
        ImGui::EndGroup();
    }
}

void ContentFolderInterface::initGUI() {
    FileSelection* sel = fileManager->getSelection();
    // Usa dockAlive_ (siempre true) para que la ventana exista en g.Windows
    // cada frame y ImGui pueda re-aplicar su DockId al restaurar el ini.
    // stateGUI controla solo la visibilidad visual (usuario cierra con X).
    ImGui::Begin(getNameGui().c_str(), &dockAlive_, getFlagGui());

    if (ImGui::BeginPopupContextWindow("AddFilesPopup", ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("New Script")) {
            creandoCarpeta = false; creandoScript = true;
            creandoScriptJava = false;
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            abrirPopupNombre = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("New Java Script")) {
            creandoCarpeta = false; creandoScript = false;
            creandoScriptJava = true;
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            abrirPopupNombre = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("New Folder")) {
            creandoCarpeta = true; creandoScript = false;
            creandoScriptJava = false;
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            abrirPopupNombre = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("New File")) {
            creandoCarpeta = false; creandoScript = false;
            creandoScriptJava = false;
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            abrirPopupNombre = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Copy Exist Folder") && sel->carpetaActual) {
            std::string sourceFolder = FileManager::seleccionarCarpetaSistema();
            if (!sourceFolder.empty()) {
                const std::string destFolder =
                    sel->carpetaActual->getPathRoot() + PATH_SEP +
                    sel->carpetaActual->getPathName();
                size_t pos = sourceFolder.find_last_of("/\\");
                std::string folderName = (pos != std::string::npos) ? sourceFolder.substr(pos + 1) : sourceFolder;
                const std::string finalDest = destFolder + PATH_SEP + folderName;
                if (fileManager->copiarCarpeta(sourceFolder, finalDest)) {
                    // El arbol se rescancea porque aparecen carpetas nuevas.
                    sel->contadorCambios++;
                }
            }
        }
        if (ImGui::MenuItem("Copy Exist File") && sel->carpetaActual) {
            std::string sourceFile = FileManager::seleccionarArchivoSistema();
            if (!sourceFile.empty()) {
                const std::string destFolder =
                    sel->carpetaActual->getPathRoot() + PATH_SEP +
                    sel->carpetaActual->getPathName();
                size_t pos = sourceFile.find_last_of("/\\");
                std::string fileName = (pos != std::string::npos) ? sourceFile.substr(pos + 1) : sourceFile;
                const std::string finalDest = destFolder + PATH_SEP + fileName;
                // Sin contador: copiar un archivo no modifica el arbol.
                fileManager->copiarArchivo(sourceFile, finalDest);
            }
        }
        ImGui::EndPopup();
    }

    if (abrirPopupNombre) {
        ImGui::OpenPopup("Ingresar nombre");
        abrirPopupNombre = false;
    }

    if (ImGui::BeginPopupModal("Ingresar nombre", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Escribe el nombre del %s:",
                    creandoCarpeta ? "folder"
                                   : (creandoScriptJava ? "script java"
                                                        : (creandoScript ? "script C++"
                                                                         : "file")));
        ImGui::InputText("##nombreNuevo", nombreNuevo, IM_ARRAYSIZE(nombreNuevo));
        const bool confirmado = ImGui::Button("Crear", ImVec2(120, 0)) ||
                                (ImGui::IsItemFocused() &&
                                 ImGui::IsKeyPressed(ImGuiKey_Enter));
        if (confirmado) {
            crearNuevoElemento();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // R6: modal de renombrado de un elemento del grid.
    if (abrirPopupRenombrar) {
        ImGui::OpenPopup("Renombrar");
        abrirPopupRenombrar = false;
    }
    if (!renombrarRuta.empty() &&
        ImGui::BeginPopupModal("Renombrar", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Nuevo nombre del %s:",
                    renombrarEsCarpeta ? "folder" : "archivo");
        ImGui::InputText("##renombrarElemento", bufferRenombrar, IM_ARRAYSIZE(bufferRenombrar));
        const bool confirmado = ImGui::Button("Renombrar", ImVec2(120, 0)) ||
                                (ImGui::IsItemFocused() &&
                                 ImGui::IsKeyPressed(ImGuiKey_Enter));
        if (confirmado) {
            const std::string nuevo = bufferRenombrar;
            if (!nuevo.empty() &&
                fileManager->renombrar(renombrarRuta, nuevo)) {
                // Referencias de la escena bajo la ruta vieja (mallas,
                // texturas, scripts): main las reescribe y persiste.
                if (eventoArchivos_ != nullptr) {
                    const std::string::size_type sep =
                        renombrarRuta.find_last_of("/\\");
                    if (sep != std::string::npos) {
                        EditorEvent ev;
                        ev.type = EditorEventType::ArchivosReubicados;
                        ev.rutaAnterior = renombrarRuta;
                        ev.rutaNueva =
                            renombrarRuta.substr(0, sep) + PATH_SEP + nuevo;
                        eventoArchivos_->publish(ev);
                    }
                }
                // Si es carpeta, el arbol se rescancea; el cache del grid se
                // invalida solo por mtime en el proximo recorrer().
                if (renombrarEsCarpeta) sel->contadorCambios++;
            }
            renombrarRuta.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
            renombrarRuta.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ContentFolderInterface::contentGUI() {
    FileSelection* sel = fileManager->getSelection();
    if (!sel->carpetaActual) return;
    const std::string destFolder =
        sel->carpetaActual->getPathRoot() + PATH_SEP +
        sel->carpetaActual->getPathName();

    recorrer(destFolder);

    // Zona de drop del grid: mientras se arrastra un "ARCHIVO_PATH" (desde este
    // mismo grid o de otro origen del editor, p.ej. el inspector), el espacio
    // vacio bajo las celdas es destino: soltar copia el elemento a la carpeta
    // visible (como en cualquier explorador, soltar en el vacio = soltar en la
    // carpeta). Solo se dibuja durante el arrastre, asi no roba clicks ni
    // crece el area desplazable: la zona cubre lo que sobra hasta abajo.
    if (const ImGuiPayload* dragPayload = ImGui::GetDragDropPayload()) {
        if (strcmp(dragPayload->DataType, "ARCHIVO_PATH") == 0) {
            const float alturaZona = ImGui::GetContentRegionAvail().y;
            if (alturaZona >= 24.0f) {
                ImGui::InvisibleButton(
                    "zonaDropArchivos",
                    ImVec2(ImGui::GetContentRegionAvail().x, alturaZona));
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* aceptado =
                            ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                        const char* origen =
                            static_cast<const char*>(aceptado->Data);
                        if (origen) copiarElementoSuelto(origen, destFolder);
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }
    }
}

void ContentFolderInterface::endGUI() { ImGui::End(); }

void ContentFolderInterface::printGUI() {
    FileSelection* sel = fileManager->getSelection();
    const bool hayCarpeta = sel && sel->carpetaActual != nullptr;
    // La ventana SIEMPRE existe en g.Windows (initGUI/endGUI cada frame)
    // para que ImGui pueda re-aplicar su DockId al restaurar el ini.
    // El contenido solo se dibuja si hay carpeta o stateGUI (visibilidad).
    initGUI();
    if (stateGUI || hayCarpeta) {
        stateGUI = true;
        contentGUI();
    }
    endGUI();
}