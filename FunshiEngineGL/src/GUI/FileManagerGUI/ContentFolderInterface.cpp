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

ContentFolderInterface::ContentFolderInterface(bool stateGUI, FileManager* fileManager)
    : GeneralUserInterface(WindowNames::ShowFolder, stateGUI, ImGuiWindowFlags_MenuBar),
      fileManager(fileManager) {}

void ContentFolderInterface::setIconosGUI(IconosGUI* iconosG) { iconosGUI = iconosG; }

void ContentFolderInterface::setEditorEventBus(EditorEventBus* bus) noexcept {
    eventoArchivos_ = bus;
}

void ContentFolderInterface::setStatusBar(StatusBarInterface* barra) noexcept {
    statusBar_ = barra;
}

void ContentFolderInterface::importarRutas(
    const std::vector<std::string>& rutas, const std::string& destino) {
    if (rutas.empty()) return;

    const ImportarArchivos::Resultado resultado =
        ImportarArchivos::pegar(fileManager, destino, rutas);

    if (resultado.copiados > 0) {
        // Invalidacion explicita del cache del grid: el mtime del directorio
        // puede no haberse actualizado todavia y el archivo nuevo no
        // apareceria hasta el siguiente cambio (mismo criterio que el resto
        // de operaciones del panel).
        invalidarCache();
    }
    // Las carpetas aparecen en el arbol de navegacion, asi que hay que
    // rescanearlo. Los archivos no: por ellos el contador no se toca, para no
    // colapsar el arbol de carpetas por un pegado.
    if (resultado.carpetasCopiadas > 0) {
        fileManager->getSelection()->contadorCambios++;
    }

    if (statusBar_ == nullptr) return;

    if (!resultado.errores.empty()) {
        std::string mensaje = "No se pudo importar ";
        if (resultado.copiados > 0) {
            mensaje = "Se importaron " + std::to_string(resultado.copiados) +
                       " elementos y ";
        }
        mensaje += resultado.errores.front();
        if (resultado.errores.size() > 1) {
            mensaje += " (y " + std::to_string(resultado.errores.size() - 1) +
                       " mas)";
        }
        statusBar_->mostrarMensaje(mensaje);
        return;
    }

    statusBar_->mostrarMensaje("Se importaron " +
                                std::to_string(resultado.copiados) +
                                " elementos en la carpeta visible.");
}

void ContentFolderInterface::atenderPegadoDelSistema(
    const std::string& destino) {
    // Con un campo de texto activo (por ejemplo el nombre de un elemento
    // nuevo), el Ctrl+V es del texto, no del portapapeles de archivos.
    if (ImGui::IsAnyItemActive()) return;
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) return;
    if (!ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_V)) return;

    IServicioPortapapeles* portapapeles = fileManager->getPortapapeles();
    if (portapapeles == nullptr) {
        if (statusBar_ != nullptr) {
            statusBar_->mostrarMensaje(
                "El portapapeles del sistema no esta disponible en este "
                "motor.");
        }
        return;
    }

    std::string motivo;
    if (!portapapeles->disponible(&motivo)) {
        if (statusBar_ != nullptr) {
            statusBar_->mostrarMensaje(
                motivo.empty()
                    ? "El portapapeles del sistema no esta disponible."
                    : motivo);
        }
        return;
    }

    const std::vector<std::string> rutas = portapapeles->archivosEnPortapapeles();
    if (rutas.empty()) {
        if (statusBar_ != nullptr) {
            statusBar_->mostrarMensaje(
                "El portapapeles del sistema no tiene archivos.");
        }
        return;
    }
    importarRutas(rutas, destino);
}

std::string ContentFolderInterface::crearNuevoElemento() {
    FileSelection* sel = fileManager->getSelection();
    if (!sel->carpetaActual)
        return "No hay ninguna carpeta seleccionada: no se puede crear el elemento.";
    if (nombreNuevo[0] == '\0')
        return "Escribe un nombre para el elemento.";

    const std::string destFolder =
        sel->carpetaActual->getPathRoot() + PATH_SEP +
        sel->carpetaActual->getPathName();

    if (creandoCarpeta) {
        const CrearCarpeta::Resultado resultado =
            CrearCarpeta::crear(fileManager, destFolder, nombreNuevo);
        if (!resultado.creada) return resultado.error;
        // El contador ya lo sube el helper: el arbol de carpetas cambia y se
        // rescanceara al detectarlo. La carpeta visible se conserva por
        // rutaVisible.
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
        if (!fileManager->crearArchivo(ruta, contenido))
            return "No se pudo crear el elemento: el nombre ya existe o la "
                   "carpeta no tiene permisos de escritura.";
    }

    creandoCarpeta = false;
    creandoScript = false;
    creandoScriptJava = false;
    memset(nombreNuevo, 0, sizeof(nombreNuevo));
    return std::string();  // sin error: el elemento quedo creado
}

void ContentFolderInterface::recorrer(const std::string& path) {
    FileSelection* sel = fileManager->getSelection();

    // Re-scanear solo si cambio la ruta mostrada o el mtime del directorio
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
    const float iconSize = 113.1f;
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

        // Menu contextual de la celda -> Renombrar / Eliminar (archivo o carpeta).
        if (ImGui::BeginPopupContextItem("PopRenombrar")) {
            if (ImGui::MenuItem(Localizacion::etiqueta(Texto::Renombrar).c_str())) {
                // El modal compartido se encarga del disco y del aviso a la
                // escena; aca solo se le pasa el elemento y su nombre actual.
                modalRenombrar.solicitar(fullPath, esCarpeta, nombre);
                ImGui::CloseCurrentPopup();
            }
            ImGui::Separator();
            if (esCarpeta) {
                if (ImGui::MenuItem(Localizacion::etiqueta(Texto::EliminarCarpeta).c_str())) {
                    carpetaAEliminarGrid = fullPath;
                    confirmarEliminarCarpetaGrid = true;
                    ImGui::CloseCurrentPopup();
                }
            } else {
                if (ImGui::MenuItem(Localizacion::etiqueta(Texto::EliminarArchivo).c_str())) {
                    archivoAEliminar = fullPath;
                    confirmarEliminarArchivo = true;
                    ImGui::CloseCurrentPopup();
                }
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
                // Solo se registra la ruta a abrir; el arbol la
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
            // Un solo payload por arrastre: ImGui guarda UNO y un segundo
            // SetDragDropPayload lo reemplaza. Cada destino decide si acepta el
            // archivo mirando su tipo (TiposArchivo::clasificar).
            ImGui::SetDragDropPayload("ARCHIVO_PATH", fullPath.c_str(), fullPath.size() + 1);
            ImGui::Text("%s", Localizacion::tr(Texto::SoltarEnZonaDeDestino));
            ImGui::Text("%s", nombre.c_str());
            ImGui::EndDragDropSource();
        }

        // Destino de arrastre sobre una CARPETA concreta de la celda. Sin esto
        // la unica forma de soltar en una carpeta era acertar el vacio de abajo,
        // que encima copiaba. Ahora la celda es destino: MUEVE, y con Ctrl
        // copia. El tooltip aparece solo mientras se arrastra, para no tapar
        // nada en reposo.
        if (esCarpeta) {
            // El tooltip va ANTES de aceptar el payload: en cuanto se acepta,
            // el arrastre termina y ya no hay nada sobre lo que hovering.
            if (const ImGuiPayload* arrastre = ImGui::GetDragDropPayload()) {
                if (strcmp(arrastre->DataType, "ARCHIVO_PATH") == 0 &&
                    ImGui::IsItemHovered()) {
                    if (ctrlOCmd())
                        ImGui::SetTooltip(Localizacion::tr(Texto::CopiarDentroDe), nombre.c_str());
                    else
                        ImGui::SetTooltip(Localizacion::tr(Texto::MoverACtrlCopiar),
                                          nombre.c_str());
                }
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* aceptado =
                        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                    const char* origen = static_cast<const char*>(aceptado->Data);
                    // El destino es la CARPETA, no su contenido.
                    // El mtime puede no haberse actualizado todavia tras el
                    // movimiento, asi que ademas de refrescar se invalida el
                    // cache a proposito para AMBAS carpetas (origen y destino).
                    std::string origenCarpeta;
                    if (origen &&
                        soltarEnCarpeta(fileManager, eventoArchivos_, origen,
                                        fullPath, ctrlOCmd(), &origenCarpeta)) {
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
                ImGui::SetTooltip(Localizacion::tr(Texto::NombreYTipo), nombre.c_str(), extension.c_str());
            else
                ImGui::SetTooltip("%s", nombre.c_str());
        }

        ImGui::PopID();
        ImGui::EndGroup();
    }
}
