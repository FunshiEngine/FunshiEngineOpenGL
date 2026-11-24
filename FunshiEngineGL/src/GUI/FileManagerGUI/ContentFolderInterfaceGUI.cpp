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
#include "ContentFolderInterfaceDetalle.h"
#include "../../Localizacion/Localizacion.h"

using Localizacion::Texto;

void ContentFolderInterface::initGUI() {
    FileSelection* sel = fileManager->getSelection();
    // Guard: sel puede ser nullptr si fileManager no esta inicializado
    if (!sel) {
        ImGui::Begin(getNameGui().c_str(), &dockAlive_, getFlagGui());
        ImGui::End();
        return;
    }
    // Usa dockAlive_ (siempre true) para que la ventana exista en g.Windows
    // cada frame y ImGui pueda re-aplicar su DockId al restaurar el ini.
    // stateGUI controla solo la visibilidad visual (usuario cierra con X).
    ImGui::Begin(getNameGui().c_str(), &dockAlive_, getFlagGui());

    // Nota: carpetaActual se valida y re-resuelve en FileManager::refrescar()
    // cuando cambia contadorCambios. Aquí no dereferenciamos carpetaActual
    // para evitar crashes si es un puntero colgante (p.ej. tras borrar la
    // carpeta que se estaba viendo; contentGUI() la anula antes de borrar).

    // Barra de menu: la ventana se creo con ImGuiWindowFlags_MenuBar pero nunca
    // la dibujo, y es el lugar natural para decir que carpeta se esta viendo.
    // Antes no se mostraba el nombre en ningun lado y con varias carpetas
    // abiertas no habia forma de saber donde estabas.
    if (ImGui::BeginMenuBar()) {
        if (sel->carpetaActual) {
            // Guardar ruta antes de cualquier modificación para evitar
            // desreferenciar tras poner carpetaActual a nullptr
            const std::string pathRoot = sel->carpetaActual->getPathRoot();
            const std::string pathName = sel->carpetaActual->getPathName();
            const std::string pathCompleta =
                pathRoot + PATH_SEP + pathName;

            // Botón para subir al padre (más pequeño)
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 1.0f));
            if (ImGui::SmallButton("..")) {
                fs::path padrePath(pathName);
                padrePath = padrePath.parent_path();
                if (!padrePath.empty() && padrePath != padrePath.root_name()) {
                    std::string rutaPadre = pathRoot + PATH_SEP + padrePath.string();
                    sel->rutaVisible = rutaPadre;
                } else {
                    // Si no hay padre (raiz), ir a la raiz del proyecto
                    if (fileManager) {
                        sel->rutaVisible = pathRoot + PATH_SEP + fileManager->getRootName();
                    } else {
                        sel->rutaVisible = pathRoot;
                    }
                }
                sel->carpetaActual = nullptr;
                sel->contadorCambios++;
                cacheCarpeta.clear();
                cacheMtime = std::filesystem::file_time_type{};
            }
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", Localizacion::tr(Texto::SubirACarpetaPadre));
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(pathName.empty() ? Localizacion::tr(Texto::Raiz) : pathName.c_str());
            // La ruta completa en el tooltip: dos carpetas de sitios distintos
            // pueden llamarse igual, y con varias abiertas el nombre solo no
            // dice donde estas.
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", pathCompleta.c_str());
            }
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::BeginPopupContextWindow("AddFilesPopup", ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::NewScript).c_str())) {
            // Abre dialogo para elegir tipo de script (C++ o Java)
            tipoScriptSeleccionado = 0; // default C++
            abrirPopupTipoScript = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::NewFolder).c_str())) {
            creandoCarpeta = true; creandoScript = false;
            creandoScriptJava = false;
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            abrirPopupNombre = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::NewFile).c_str())) {
            creandoCarpeta = false; creandoScript = false;
            creandoScriptJava = false;
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            abrirPopupNombre = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::CopyExistFolder).c_str()) && sel->carpetaActual) {
            std::string sourceFolder = FileManager::seleccionarCarpetaSistema();
            if (!sourceFolder.empty()) {
                const std::string destFolder =
                    sel->carpetaActual->getPathRoot() + PATH_SEP +
                    sel->carpetaActual->getPathName();
                // La barra invertida solo separa donde el sistema la trata
                // como separador; en Linux es un caracter del nombre.
                const std::string::size_type pos = indiceSeparadorFinal(sourceFolder);
                std::string folderName = (pos != std::string::npos) ? sourceFolder.substr(pos + 1) : sourceFolder;
                const std::string finalDest = destFolder + PATH_SEP + folderName;
                if (fileManager->copiarCarpeta(sourceFolder, finalDest)) {
                    // El arbol se rescancea porque aparecen carpetas nuevas.
                    sel->contadorCambios++;
                }
            }
        }
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::CopyExistFile).c_str()) && sel->carpetaActual) {
            std::string sourceFile = FileManager::seleccionarArchivoSistema();
            if (!sourceFile.empty()) {
                const std::string destFolder =
                    sel->carpetaActual->getPathRoot() + PATH_SEP +
                    sel->carpetaActual->getPathName();
                const std::string::size_type pos = indiceSeparadorFinal(sourceFile);
                std::string fileName = (pos != std::string::npos) ? sourceFile.substr(pos + 1) : sourceFile;
                const std::string finalDest = destFolder + PATH_SEP + fileName;
                // Sin contador: copiar un archivo no modifica el arbol.
                fileManager->copiarArchivo(sourceFile, finalDest);
                // Invalidacion explicita del cache del grid: el mtime del
                // directorio puede no haberse actualizado todavia, y el
                // archivo copiado tiene que aparecer en el panel (mismo
                // criterio que el resto de operaciones del panel).
                invalidarCache();
            }
        }
        ImGui::EndPopup();
    }

    if (abrirPopupNombre) {
        ImGui::OpenPopup("###Ingresar nombre");
        abrirPopupNombre = false;
        errorNuevoElemento.clear();
    }

    if (ImGui::BeginPopupModal(
            Localizacion::etiqueta(Texto::IngresarNombre, "###Ingresar nombre").c_str(),
            NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        // Frase entera por caso: "del %s" no se traduce bien a otros idiomas.
        ImGui::TextUnformatted(Localizacion::tr(
            creandoCarpeta ? Texto::EscribeElNombreDeLa
                           : (creandoScriptJava ? Texto::EscribeNombreScriptJava
                                                : (creandoScript ? Texto::EscribeNombreScriptCpp
                                                                 : Texto::EscribeNombreArchivo))));
        const bool enter = ImGui::InputText(
            "##nombreNuevo", nombreNuevo, IM_ARRAYSIZE(nombreNuevo),
            ImGuiInputTextFlags_EnterReturnsTrue);
        const bool confirmado = ImGui::Button(Localizacion::etiqueta(Texto::Crear).c_str(), ImVec2(120, 0)) || enter;
        if (confirmado) {
            // Cierra solo si el elemento quedo creado; si no, el popup sigue
            // abierto con el motivo y el texto para corregirlo.
            errorNuevoElemento = crearNuevoElemento();
            if (errorNuevoElemento.empty())
                ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        const bool cancelar = ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0)) ||
                              ImGui::IsKeyPressed(ImGuiKey_Escape);
        if (cancelar) {
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            ImGui::CloseCurrentPopup();
        }
        if (!errorNuevoElemento.empty())
            ImGui::TextColored(TemaEditor::color(TemaEditor::RolColor::Error), "%s",
                               errorNuevoElemento.c_str());
        ImGui::EndPopup();
    }

    // Renombre del elemento del grid. El modal (campo enfocado al abrir,
    // Enter confirma) y el camino de disco/aviso viven en RenombrarElemento.h,
    // compartidos con el arbol.
    const RenombrarElemento::Resultado renombre = modalRenombrar.dibujar();
    if (renombre.confirmado &&
        RenombrarElemento::ejecutar(fileManager, eventoArchivos_,
                                    renombre.ruta, renombre.nombreNuevo)) {
        // Si era carpeta, el arbol se rescancea; el cache del grid se invalida
        // solo por mtime en el proximo recorrer().
        if (renombre.esCarpeta) sel->contadorCambios++;
    }

    // Modal para seleccionar tipo de script (C++ o Java)
    if (abrirPopupTipoScript) {
        ImGui::OpenPopup("###Seleccionar Tipo de Script");
        abrirPopupTipoScript = false;
    }
    if (ImGui::BeginPopupModal(
            Localizacion::etiqueta(Texto::SeleccionarTipoDeScript, "###Seleccionar Tipo de Script").c_str(),
            NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", Localizacion::tr(Texto::SeleccionaElTipoDeScript));
        ImGui::Separator();
        if (ImGui::RadioButton("C++ (.cpp)", &tipoScriptSeleccionado, 0)) {}
        if (ImGui::RadioButton("Java (.java)", &tipoScriptSeleccionado, 1)) {}
        ImGui::Separator();
        const bool confirmado = ImGui::Button(Localizacion::etiqueta(Texto::Continuar).c_str(), ImVec2(120, 0)) ||
                                (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
        if (confirmado) {
            creandoCarpeta = false;
            creandoScript = (tipoScriptSeleccionado == 0);
            creandoScriptJava = (tipoScriptSeleccionado == 1);
            memset(nombreNuevo, 0, sizeof(nombreNuevo));
            abrirPopupNombre = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Modal de confirmacion para eliminar archivo
    if (confirmarEliminarArchivo && !archivoAEliminar.empty()) {
        ImGui::OpenPopup("###ConfirmarEliminarArchivo");
        confirmarEliminarArchivo = false;
    }
    if (ImGui::BeginPopupModal(
            Localizacion::etiqueta(Texto::ConfirmarEliminarArchivo, "###ConfirmarEliminarArchivo").c_str(),
            nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(Localizacion::tr(Texto::EliminarArchivo2), archivoAEliminar.c_str());
        ImGui::Text("%s", Localizacion::tr(Texto::EstaAccionNoSePuede));
        if (ImGui::Button(Localizacion::etiqueta(Texto::Eliminar).c_str(), ImVec2(120, 0))) {
            archivoAEliminarConfirmado = archivoAEliminar;
            archivoAEliminar.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0))) {
            archivoAEliminar.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Modal de confirmacion para eliminar carpeta desde el grid
    if (confirmarEliminarCarpetaGrid && !carpetaAEliminarGrid.empty()) {
        ImGui::OpenPopup("###ConfirmarEliminarCarpetaGrid");
        confirmarEliminarCarpetaGrid = false;
    }
    if (ImGui::BeginPopupModal(
            Localizacion::etiqueta(Texto::ConfirmarEliminarCarpetaGrid, "###ConfirmarEliminarCarpetaGrid").c_str(),
            nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(Localizacion::tr(Texto::EliminarCarpetaYTodoSu), carpetaAEliminarGrid.c_str());
        ImGui::Text("%s", Localizacion::tr(Texto::EstaAccionNoSePuede));
        if (ImGui::Button(Localizacion::etiqueta(Texto::Eliminar).c_str(), ImVec2(120, 0))) {
            carpetaAEliminarGridConfirmada = carpetaAEliminarGrid;
            carpetaAEliminarGrid.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0))) {
            carpetaAEliminarGrid.clear();
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

    // Eliminacion diferida: ejecutada ANTES de recorrer() para que el cache
    // del grid se invalide y no muestre el elemento "fantasma" en este frame.
    if (!archivoAEliminarConfirmado.empty()) {
        fileManager->eliminarArchivo(archivoAEliminarConfirmado);
        // Forzar invalidacion del cache: el mtime del directorio puede no
        // actualizarse inmediatamente en algunos FS; limpiamos el cache manualmente.
        invalidarCache();
        archivoAEliminarConfirmado.clear();
        // No sube contadorCambios: archivos no estan en el arbol de carpetas.
    }
    if (!carpetaAEliminarGridConfirmada.empty()) {
        // Si la carpeta que se va a eliminar es la que se esta viendo,
        // navegar a la carpeta padre ANTES de borrar para no quedar
        // en una vista invalida.
        if (sel->carpetaActual) {
            std::string rutaActual =
                sel->carpetaActual->getPathRoot() + PATH_SEP + sel->carpetaActual->getPathName();
            if (rutaActual == carpetaAEliminarGridConfirmada) {
                // Construir ruta del padre: pathRoot + parent path del pathName
                fs::path padrePath(sel->carpetaActual->getPathName());
                padrePath = padrePath.parent_path();
                if (!padrePath.empty() && padrePath != padrePath.root_name()) {
                    std::string rutaPadre = sel->carpetaActual->getPathRoot() + PATH_SEP + padrePath.string();
                    sel->rutaVisible = rutaPadre;  // Solo actualizar ruta; NO tocar carpetaActual
                } else {
                    // Si no hay padre (raiz), ir a la raiz del proyecto
                    sel->rutaVisible = sel->carpetaActual->getPathRoot() + PATH_SEP + fileManager->getRootName();
                }

                // Invalidar carpetaActual AHORA para que initGUI() no intente
                // acceder a un puntero colgante (la carpeta se borra del disco y
                // el arbol se reconstruira en el proximo frame via contadorCambios).
                sel->carpetaActual = nullptr;

                // Subir contador para que el arbol se refresque en el SIGUIENTE frame
                // y re-resuelva carpetaActual desde rutaVisible (FileManager::refrescar)
                sel->contadorCambios++;
            }
        }

        // AHORA sí: ejecutar borrado (la vista ya está en carpeta padre via rutaVisible)
        if (fileManager->eliminarCarpeta(carpetaAEliminarGridConfirmada)) {
            // contadorCambios ya se subio arriba
        }
        // Forzar invalidacion del cache del grid tambien para carpetas.
        invalidarCache();
        carpetaAEliminarGridConfirmada.clear();
    }

    // Archivos soltados sobre la ventana del editor desde el explorador del
    // sistema: el sistema los entrega fuera del frame de la GUI, asi que el
    // servicio los encola y aqui se drenan, cuando ya se sabe que carpeta
    // esta viendo (la visible es el destino).
    if (IServicioDrop* receptor = fileManager->getReceptorDrop()) {
        const std::vector<std::string> rutas = receptor->drenar();
        if (!rutas.empty()) importarRutas(rutas, destFolder);
    }

    // Pegar con Ctrl+V lo que el sistema tiene en el portapapeles.
    atenderPegadoDelSistema(destFolder);

    recorrer(destFolder);

    // Zona de drop del grid: mientras se arrastra un "ARCHIVO_PATH" (desde este
    // mismo grid o de otro origen del editor, p.ej. el inspector), el espacio
    // vacio bajo las celdas es destino: soltar MUEVE el elemento a la carpeta
    // visible, y con Ctrl lo copia (misma semantica que soltar sobre una celda
    // de carpeta). Solo se dibuja durante el arrastre, asi no roba clicks ni
    // crece el area desplazable: la zona cubre lo que sobra hasta abajo.
    if (const ImGuiPayload* dragPayload = ImGui::GetDragDropPayload()) {
        if (strcmp(dragPayload->DataType, "ARCHIVO_PATH") == 0) {
            const float alturaZona = ImGui::GetContentRegionAvail().y;
            if (alturaZona >= 24.0f) {
                ImGui::InvisibleButton(
                    "zonaDropArchivos",
                    ImVec2(ImGui::GetContentRegionAvail().x, alturaZona));
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localizacion::tr(ctrlOCmd()
                        ? Texto::CopiarEnLaCarpetaActual
                        : Texto::MoverALaCarpetaActual));
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* aceptado =
                            ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                        const char* origen =
                            static_cast<const char*>(aceptado->Data);
                        std::string origenCarpeta;
                        if (origen &&
                            soltarEnCarpeta(fileManager, eventoArchivos_,
                                            origen, destFolder, ctrlOCmd(),
                                            &origenCarpeta)) {
                            // Invalidar cache de la carpeta destino (la visible)
                            invalidarCache();
                            // Invalidar cache de la carpeta origen si es distinta
                            if (!origenCarpeta.empty() && origenCarpeta != cacheCarpeta) {
                                cacheCarpeta.clear();
                                cacheMtime = std::filesystem::file_time_type{};
                            }
                        }
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
    // Guard: sel puede ser nullptr si fileManager no esta inicializado
    if (!sel) return;

    // Refrescar arbol ANTES de initGUI() si contadorCambios ha cambiado.
    // initGUI() dereferencia carpetaActual (en la barra de menu), y si el
    // arbol se reconstruyo en otro panel (TreeFilesInterface) o por cambio
    // externo, carpetaActual puede ser un puntero colgante. FileManager::refrescar()
    // re-resuelve carpetaActual desde rutaVisible de forma segura.
    if (sel->contadorCambios != ultimoContadorVisto || fileManager->huboCambiosExternos()) {
        fileManager->refrescar();
        ultimoContadorVisto = sel->contadorCambios;
    }

    const bool hayCarpeta = sel->carpetaActual != nullptr;
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
