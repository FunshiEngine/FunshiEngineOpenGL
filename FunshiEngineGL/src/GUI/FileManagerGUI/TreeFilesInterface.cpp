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
#include "TreeFilesInterface.h"

#include <cstring>
#include <filesystem>
#include <vector>

#include "../../Herramientas/PathUtils.h"
#include "../../Events/EditorEventBus.h"
#include "../../FileManager/FileManager.h"
#include "../../FileManager/FileSelection.h"
#include "../../GestorDeArchivos/Carpeta.h"
#include "../../Herramientas/IconosGUI/IconosGUI.h"
#include <imgui.h>

namespace {
std::string rutaDe(Carpeta* c) {
    return c->getPathRoot() + PATH_SEP + c->getPathName();
}

// Ruta completa de un elemento del arbol (la raiz se trata como contenedor y
// no se dibuja, por eso su pathRoot+pathName nunca se indexa).
std::string rutaDeElemento(File* elemento) {
    return elemento->getPathRoot() + PATH_SEP + elemento->getPathName();
}

// Elimina un nodo y TODO su subarbol del ArbolEnlazado (que nunca es dueno de
// sus File*): limpia el estado de colapso por ruta, libera cada elemento y
// desvincula los nodos. Sin esto, deleteNodeInternalNode() promovia el primer
// hijo al lugar del padre y quedaban "carpetas fantasma" inexistentes en disco
// (B3).
void limpiarYLiberarSubarbol(ArbolEnlazado<File*>* arbol,
                             Position<File*>* p,
                             std::set<std::string>& openPaths) {
    if (!arbol || !p) return;
    if (p->getElement())
        openPaths.erase(rutaDeElemento(p->getElement()));
    if (arbol->isInternal(p)) {
        auto* hijos = arbol->childsOf(p);
        auto* it = hijos->first();
        while (it) {
            Position<File*>* hijo = it->getElement();
            limpiarYLiberarSubarbol(arbol, hijo, openPaths);
            it = (it != hijos->last()) ? hijos->next(it) : nullptr;
        }
        delete hijos;
    }
    // Ya sin hijos, el nodo es hoja: deleteNode la desvincula y devuelve el
    // File*, que liberamos.
    delete arbol->deleteNode(p);
}

// Se conserva la expansion de la rama renombrada: todos los paths que empiezan
// con el prefijo viejo pasan al prefijo nuevo (el nodo y sus descendientes).
void trasladarPrefijoEnPaths(std::set<std::string>& paths,
                             const std::string& prefixViejo,
                             const std::string& prefixNuevo) {
    std::vector<std::string> aEliminar;
    std::vector<std::string> aInsertar;
    for (const auto& p : paths) {
        if (p == prefixViejo) {
            aEliminar.push_back(p);
            aInsertar.push_back(prefixNuevo);
        } else if (p.size() > prefixViejo.size() &&
                   p.compare(0, prefixViejo.size(), prefixViejo) == 0 &&
                   p[prefixViejo.size()] == PATH_SEP) {
            aEliminar.push_back(p);
            aInsertar.push_back(prefixNuevo + p.substr(prefixViejo.size()));
        }
    }
    for (const auto& e : aEliminar) paths.erase(e);
    for (const auto& i : aInsertar) paths.insert(i);
}
} // namespace

TreeFilesInterface::TreeFilesInterface(bool stateGUI, FileManager* fileManager)
    : GeneralUserInterface(WindowNames::BrowseFile, stateGUI, ImGuiWindowFlags_MenuBar),
      fileManager(fileManager),
      arbolDeArchivos(fileManager->getArbol()) {
    if (arbolDeArchivos && !arbolDeArchivos->isEmpty()) {
        Position<File*>* rootPos = arbolDeArchivos->rootOfTree();
        if (rootPos && rootPos->getElement()) {
            openPaths.insert(rutaDeElemento(rootPos->getElement()));
        }
    }
}

void TreeFilesInterface::setIconosGUI(IconosGUI* iconosG) { iconosGUI = iconosG; }

void TreeFilesInterface::setEditorEventBus(EditorEventBus* bus) noexcept {
    eventoArchivos_ = bus;
}

TreeIG::RowResult TreeFilesInterface::drawFolderRow(File* element, bool wasOpen) {
    Carpeta* folderRoot = dynamic_cast<Carpeta*>(element);
    if (!folderRoot) return {};
    FileSelection* sel = fileManager->getSelection();

    const bool isSelected = (sel->carpetaActual == folderRoot);
    ImGuiTreeNodeFlags nodeFlags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (isSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;
    if (wasOpen) nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (iconosGUI && iconosGUI->getIconoCarpeta() != ImTextureID_Invalid) {
        ImGui::Image(iconosGUI->getIconoCarpeta(), ImVec2(22, 22));
        ImGui::SameLine();
    }

    bool nodeOpen;
    bool toggled;
    if (renombrandoInline && carpetaRenombrando == rutaDe(folderRoot)) {
        // R6: fila en modo rename -> InputText inline en lugar del nombre.
        nodeOpen = ImGui::TreeNodeEx("##renombrar_carpeta", nodeFlags, " ");
        toggled = ImGui::IsItemToggledOpen();

        ImGui::SameLine();
        const bool confirmado =
            ImGui::InputText("##input_renombrar", bufferRenombrar,
                             IM_ARRAYSIZE(bufferRenombrar),
                             ImGuiInputTextFlags_AutoSelectAll |
                             ImGuiInputTextFlags_EnterReturnsTrue);
        const bool cancelado = ImGui::IsKeyPressed(ImGuiKey_Escape);

        if (confirmado || cancelado || !renombrandoInline) {
            if (confirmado) {
                const std::string nuevoNombre = bufferRenombrar;
                if (!nuevoNombre.empty() &&
                    nuevoNombre != folderRoot->getPathName()) {
                    const std::string rutaVieja = rutaDe(folderRoot);
                    const std::string rutaNueva =
                        folderRoot->getPathRoot() + PATH_SEP + nuevoNombre;
                    if (fileManager->renombrar(rutaVieja, nuevoNombre)) {
                        // La ruta visible (si es esta carpeta o un descendiente)
                        // se actualiza ANTES del rescaneo para que refrescar()
                        // la re-resuelva con el nombre nuevo (R6).
                        if (sel->rutaVisible == rutaVieja) {
                            sel->rutaVisible = rutaNueva;
                        } else if (sel->rutaVisible.size() > rutaVieja.size() &&
                                   sel->rutaVisible.compare(0, rutaVieja.size(), rutaVieja) == 0 &&
                                   sel->rutaVisible[rutaVieja.size()] == PATH_SEP) {
                            sel->rutaVisible =
                                rutaNueva + sel->rutaVisible.substr(rutaVieja.size());
                        }
                        trasladarPrefijoEnPaths(openPaths, rutaVieja, rutaNueva);
                        // Referencias de la escena bajo la ruta vieja (mallas,
                        // texturas, scripts): main las reescribe y persiste.
                        if (eventoArchivos_ != nullptr) {
                            EditorEvent ev;
                            ev.type = EditorEventType::ArchivosReubicados;
                            ev.rutaAnterior = rutaVieja;
                            ev.rutaNueva = rutaNueva;
                            eventoArchivos_->publish(ev);
                        }
                        // Rescaneo del arbol (refleja el nombre nuevo).
                        sel->contadorCambios++;
                    }
                }
            }
            carpetaRenombrando.clear();
            renombrandoInline = false;
        }
    } else {
        nodeOpen = ImGui::TreeNodeEx(
            folderRoot->getPathName().c_str(), nodeFlags, "%s",
            folderRoot->getPathName().c_str());
        toggled = ImGui::IsItemToggledOpen();

        if (ImGui::IsItemClicked()) {
            sel->carpetaActual = folderRoot;
            sel->rutaVisible = rutaDe(folderRoot);
            sel->navegacionPendiente.clear();
        }

        // Destino de drag&drop: soltar un "ARCHIVO_PATH" (grid u otro origen)
        // sobre la fila copia el elemento a esta carpeta.
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* aceptado =
                    ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                const char* origen = static_cast<const char*>(aceptado->Data);
                if (origen) copiarElementoSuelto(origen, rutaDe(folderRoot));
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem("MenuContextualCarpeta")) {
            ImGui::Text("Carpeta: %s", folderRoot->getPathName().c_str());
            ImGui::Separator();
            const bool esRaiz = (arbolDeArchivos && !arbolDeArchivos->isEmpty() &&
                                 folderRoot == arbolDeArchivos->rootOfTree()->getElement());
            if (!esRaiz && ImGui::MenuItem("Renombrar Carpeta")) {
                // R6: se guarda la RUTA (no el puntero: un rescaneo
                // reconstruye el arbol y deja punteros colgando). Como el
                // editor se dibuja en la fila que coincide por ruta, su
                // "destino" se re-resuelve cada frame contra el arbol vigente.
                carpetaRenombrando = rutaDe(folderRoot);
                renombrandoInline = true;
                std::memset(bufferRenombrar, 0, sizeof(bufferRenombrar));
                std::strncpy(bufferRenombrar,
                             folderRoot->getPathName().c_str(),
                             sizeof(bufferRenombrar) - 1);
            }
            if (ImGui::MenuItem("Nueva Carpeta")) {
                const std::string rutaNuevaCarpeta =
                    rutaDe(folderRoot) + PATH_SEP + "Nueva Carpeta";
                if (fileManager->crearCarpeta(rutaNuevaCarpeta)) {
                    // No mutamos el arbol durante el recorrido (invalidaba
                    // iteradores, B4): el rescaneo del proximo frame lo agrega.
                    sel->contadorCambios++;
                }
            }
            if (!esRaiz && ImGui::MenuItem("Eliminar Carpeta")) {
                carpetaAConfirmar = rutaDe(folderRoot);
                confirmarEliminar = true;
            }
            ImGui::EndPopup();
        }
    }
    return {nodeOpen, toggled};
}

void TreeFilesInterface::initGUI() {
    // Usa dockAlive_ (siempre true) para que la ventana exista en g.Windows
    // cada frame y ImGui pueda re-aplicar su DockId al restaurar el ini.
    // stateGUI controla solo la visibilidad visual (usuario cierra con X).
    ImGui::Begin(getNameGui().c_str(), &dockAlive_, getFlagGui());

    if (confirmarEliminar && !carpetaAConfirmar.empty()) {
        ImGui::OpenPopup("ConfirmarEliminar");
        confirmarEliminar = false;
    }
    if (ImGui::BeginPopupModal("ConfirmarEliminar", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        // Ruta -> puntero SOLO contra el arbol del frame. Si la carpeta ya no
        // existe (rescaneo con el modal abierto), el modal se cierra en vez de
        // leer memoria liberada o eliminar una carpeta distinta por reuso del
        // heap.
        Carpeta* carpetaAConfirmarResuelta =
            fileManager->buscarCarpetaPorRuta(carpetaAConfirmar);
        if (!carpetaAConfirmarResuelta) {
            carpetaAConfirmar.clear();
            ImGui::CloseCurrentPopup();
        } else {
            ImGui::Text("Eliminar \"%s\" y todo su contenido?",
                        carpetaAConfirmarResuelta->getPathName().c_str());
            if (ImGui::Button("Eliminar", ImVec2(120, 0))) {
                carpetaAEliminar = carpetaAConfirmar;
                carpetaAConfirmar.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                carpetaAConfirmar.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void TreeFilesInterface::refrescarArbol() {
    fileManager->refrescar();
    arbolDeArchivos = fileManager->getArbol();
    // R4: openPaths se indexa por RUTA y sobrevive a la reconstruccion del
    // arbol; ya no se limpia aqui (las ramas borradas quedan como entradas
    // hueso que nunca se dibujan, inofensivas).
    carpetaAEliminar.clear();
    ultimoContadorVisto = fileManager->getSelection()->contadorCambios;
    // navegacionPendiente NO se limpia: es una ruta y debe aplicarse (FASE 2)
    // contra el arbol recien reconstruido; limpiarla aqui perderia el doble
    // clic que coincidio con un rescaneo (B6).
}

// Copia un elemento soltado sobre una carpeta del arbol (payload
// "ARCHIVO_PATH"). Carpetas -> copiarCarpeta + rescaneo; archivos ->
// copiarArchivo. No copiar sobre la propia carpeta (finalDest == origen).
void TreeFilesInterface::copiarElementoSuelto(const std::string& origen,
                                              const std::string& folderDest) {
    if (origen.empty() || folderDest.empty()) return;
    FileSelection* sel = fileManager->getSelection();
    std::error_code ec;
    const std::string nombre =
        std::filesystem::path(origen).filename().string();
    const std::string finalDest = folderDest + PATH_SEP + nombre;
    if (finalDest == origen) return;
    if (std::filesystem::is_directory(origen, ec)) {
        if (fileManager->copiarCarpeta(origen, finalDest)) sel->contadorCambios++;
    } else {
        fileManager->copiarArchivo(origen, finalDest);
    }
}

void TreeFilesInterface::aplicarNavegacionPendiente() {
    FileSelection* sel = fileManager->getSelection();
    if (sel->navegacionPendiente.empty()) return;
    const std::string ruta = sel->navegacionPendiente;
    Carpeta* objetivo = fileManager->buscarCarpetaPorRuta(ruta);
    sel->navegacionPendiente.clear();
    if (!objetivo) return;
    sel->carpetaActual = objetivo;
    sel->rutaVisible = ruta;

    // Expandir la jerarquia de padres para que sea visible: como openPaths
    // se indexa por ruta y sobrevive a los rescaneos, la expansion persiste.
    Position<File*>* currentPos = arbolDeArchivos->whatIsPositionOf(objetivo);
    while (currentPos) {
        File* currentElement = currentPos->getElement();
        if (currentElement && currentPos != arbolDeArchivos->rootOfTree())
            openPaths.insert(rutaDeElemento(currentElement));
        if (arbolDeArchivos->isRoot(currentPos)) break;
        currentPos = arbolDeArchivos->dadOf(currentPos);
    }
}

void TreeFilesInterface::contentGUI() {
    FileSelection* sel = fileManager->getSelection();

    // Refresco programado (actualizar), por cambio de FS detectado (el panel
    // de contenido creo/copio una carpeta y subio el contador, R3) o por
    // cambios hechos FUERA del editor (FileSystemWatcher).
    if (!arbolDeArchivos || actualizar ||
        sel->contadorCambios != ultimoContadorVisto ||
        fileManager->huboCambiosExternos()) {
        refrescarArbol();
        actualizar = false;
    }

    // FASE 2 se aplica DESPUÉS de cualquier refresco y contra el arbol
    // vigente: asi el doble clic sobrevive a un rescaneo programado (B6).
    aplicarNavegacionPendiente();

    if (!arbolDeArchivos->isEmpty()) {
        TreeIG::drawTreeKeyed(
            arbolDeArchivos, arbolDeArchivos->rootOfTree(), openPaths,
            [this](File* element, bool wasOpen) {
                return drawFolderRow(element, wasOpen);
            },
            [](File* element) -> std::string {
                return rutaDeElemento(element);
            },
            true);
    }

    // Borrado diferido (fuera del recorrido del arbol, B4). La ruta se resuelve
    // contra el arbol VIGENTE del frame: si un rescaneo reconstruyo el arbol
    // entre la confirmacion y aca, el puntero guardado habria quedado colgando.
    if (!carpetaAEliminar.empty()) {
        const std::string rutaAeliminar = carpetaAEliminar;
        carpetaAEliminar.clear();
        Carpeta* doomed = fileManager->buscarCarpetaPorRuta(rutaAeliminar);
        if (doomed) {
            if (sel->carpetaActual == doomed) {
                sel->carpetaActual = nullptr;
                sel->rutaVisible.clear();
            }
            Position<File*>* posicion = arbolDeArchivos->whatIsPositionOf(doomed);
            if (posicion && posicion != arbolDeArchivos->rootOfTree() &&
                fileManager->eliminarCarpeta(rutaAeliminar)) {
                limpiarYLiberarSubarbol(arbolDeArchivos, posicion, openPaths);
            }
        }
    }
}

void TreeFilesInterface::endGUI() { ImGui::End(); }

void TreeFilesInterface::printGUI() {
    // La ventana SIEMPRE existe en g.Windows (initGUI/endGUI cada frame)
    // para que ImGui pueda re-aplicar su DockId al restaurar el ini.
    // El contenido solo se dibuja si stateGUI (visibilidad).
    initGUI();
    if (stateGUI) {
        contentGUI();
    }
    endGUI();
}

void TreeFilesInterface::solicitarActualizacion() { actualizar = true; }