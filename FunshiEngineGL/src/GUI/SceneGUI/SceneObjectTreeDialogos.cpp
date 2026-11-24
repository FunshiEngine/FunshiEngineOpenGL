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
#include "SceneObjectTreeDetalle.h"
#include "../../Localizacion/Localizacion.h"

using Localizacion::Texto;

void SceneObjectTree::dibujarDialogosModales() {
    if (dialogoActivo == DialogoTipo::Ninguno || !objetoEnDialogo) return;

    // Dialogo Cambiar ID
    if (dialogoActivo == DialogoTipo::CambiarID) {
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("###DialogoCambiarID");
            dialogoRecienAbierto = false;
        }
        if (ImGui::BeginPopupModal(
                Localizacion::etiqueta(Texto::DialogoCambiarID, "###DialogoCambiarID").c_str(), nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(Localizacion::tr(Texto::NuevoIDPara), objetoEnDialogo->inputName);
            ImGui::InputText("##id", bufferDialogo, sizeof(bufferDialogo),
                             ImGuiInputTextFlags_CharsDecimal);
            ImGui::Separator();
            if (ImGui::Button(Localizacion::etiqueta(Texto::Aceptar).c_str(), ImVec2(120, 0))) {
                int nuevoId = std::atoi(bufferDialogo);
                const bool esRaiz = objetoEnDialogo->getParentEntity() == nullptr;
                if (nuevoId > 0 || (esRaiz && nuevoId >= 0)) {
                    objetoEnDialogo->setId(nuevoId);
                    if (events)
                        events->publish({SceneEventType::ComponentChanged,
                                         objetoEnDialogo, nullptr});
                }
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0))) {
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // Dialogo Renombrar
    if (dialogoActivo == DialogoTipo::Renombrar) {
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("###DialogoRenombrar");
        }
        if (ImGui::BeginPopupModal(
                Localizacion::etiqueta(Texto::DialogoRenombrar, "###DialogoRenombrar").c_str(), nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (dialogoRecienAbierto) {
                ImGui::SetKeyboardFocusHere();
                dialogoRecienAbierto = false;
            }
            ImGui::Text("%s", Localizacion::tr(Texto::NuevoNombre));
            bool enterPresionado = ImGui::InputText("##nombre", bufferDialogo, sizeof(bufferDialogo),
                                                     ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Separator();
            bool confirmado = ImGui::Button(Localizacion::etiqueta(Texto::Aceptar).c_str(), ImVec2(120, 0));
            ImGui::SameLine();
            bool cancelado = ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0));
            if (confirmado || cancelado || enterPresionado) {
                if ((confirmado || enterPresionado) && bufferDialogo[0] != '\0') {
                    std::snprintf(objetoEnDialogo->inputName,
                                  sizeof(objetoEnDialogo->inputName), "%s",
                                  bufferDialogo);
                    if (events)
                        events->publish({SceneEventType::ComponentChanged,
                                         objetoEnDialogo, nullptr});
                }
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // Dialogo Eliminar
    if (dialogoActivo == DialogoTipo::Eliminar) {
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("###DialogoEliminar");
            dialogoRecienAbierto = false;
        }
        if (ImGui::BeginPopupModal(
                Localizacion::etiqueta(Texto::DialogoEliminar, "###DialogoEliminar").c_str(), nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(Localizacion::tr(Texto::EliminarID),
                        objetoEnDialogo->inputName, objetoEnDialogo->getId());
            ImGui::TextColored(TemaEditor::color(TemaEditor::RolColor::Error),
                               "%s", Localizacion::tr(Texto::EstaAccionNoSePuede));
            ImGui::Separator();
            if (ImGui::Button(Localizacion::etiqueta(Texto::Eliminar).c_str(), ImVec2(120, 0))) {
                if (editor) editor->clearSelection();
                renombrando = nullptr;
                objetoAEliminar = objetoEnDialogo;
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0))) {
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // Dialogo Crear Prefab
    if (dialogoActivo == DialogoTipo::CrearPrefab) {
        if (dialogoRecienAbierto) {
            ImGui::OpenPopup("###DialogoCrearPrefab");
            dialogoRecienAbierto = false;
        }
        if (ImGui::BeginPopupModal(
                Localizacion::etiqueta(Texto::DialogoCrearPrefab, "###DialogoCrearPrefab").c_str(), nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", Localizacion::tr(Texto::NombreDelPrefab));
            ImGui::InputText("##NombrePrefab", bufferDialogo, sizeof(bufferDialogo),
                             ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Separator();
            if (ImGui::Button(Localizacion::etiqueta(Texto::Crear).c_str(), ImVec2(120, 0))) {
                if (bufferDialogo[0] != '\0' && editor && editor->getPrefabLibrary()) {
                    editor->getPrefabLibrary()->crearPrefab(bufferDialogo, objetoEnDialogo, editor);
                }
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(Localizacion::etiqueta(Texto::Cancelar).c_str(), ImVec2(120, 0))) {
                dialogoActivo = DialogoTipo::Ninguno;
                objetoEnDialogo = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void SceneObjectTree::applyDeferredOperations() {
    GestorComandos* comandos =
        editor ? editor->getGestorComandos() : nullptr;
    if (objetoAReParentar && objetoPadreNuevo && editor && comandos)
        comandos->ejecutar(std::make_unique<ReparentarComando>(
            editor, objetoAReParentar, objetoPadreNuevo, scene));
    objetoAReParentar = nullptr;
    objetoPadreNuevo = nullptr;

    // Archivo soltado sobre una fila: se aplica al objeto de esa fila.
    if (!archivoSoltado_.empty() && objetoDestinoArchivo_ && editor &&
        scene && scene->contains(objetoDestinoArchivo_)) {
        const ArrastreArchivos::Resultado r = ArrastreArchivos::aplicarAObjeto(
            *editor, *objetoDestinoArchivo_, archivoSoltado_);
        if (r.ok && r.objeto) editor->selectObject(r.objeto);
        else std::cerr << "[SceneObjectTree] " << archivoSoltado_ << ": " << r.mensaje << std::endl;
    }
    archivoSoltado_.clear();
    objetoDestinoArchivo_ = nullptr;

    if (objetoADesanidar) {
        GameObject* raiz = scene ? scene->getRoot() : nullptr;
        if (editor && comandos && raiz)
            comandos->ejecutar(std::make_unique<ReparentarComando>(
                editor, objetoADesanidar, raiz, scene));
        objetoADesanidar = nullptr;
    }

    if (objetoAEliminar) {
        GameObject* doomed = objetoAEliminar;
        objetoAEliminar = nullptr;
        openNodes.erase(static_cast<const void*>(doomed));
        if (renombrando == doomed) renombrando = nullptr;
        if (editor && comandos)
            comandos->ejecutar(
                std::make_unique<BorrarObjetoComando>(editor, doomed, scene));
    }

    if (objetoAPegar) {
        GameObject* fuente = objetoAPegar;
        objetoAPegar = nullptr;
        GameObject* destino = padreDePegado;
        padreDePegado = nullptr;
        if (editor && scene && scene->contains(fuente)) {
            if (!destino || !scene->contains(destino)) destino = nullptr;
            // Pegar sobre si mismo o sobre un descendiente colgaria el
            // duplicado de su propio subarbol: en ese caso va a la raiz.
            if (destino && (destino == fuente || esDescendiente(destino, fuente)))
                destino = nullptr;
            // Sin tocar la seleccion: el clon no se selecciona para que el
            // proximo Ctrl+V caiga en el mismo destino y nunca se anide.
            // Por el gestor: Ctrl+Z deshace el pegado.
            if (GestorComandos* comandos = editor->getGestorComandos())
                comandos->ejecutar(std::make_unique<DuplicarObjetoComando>(
                    editor, fuente, destino, scene));
            // Cortar es mover: tras pegar se borra el original y se vacia.
            // Tambien por el gestor: el corte completo se deshace en dos
            // Ctrl+Z (borrado y duplicado).
            if (cortePendiente) {
                openNodes.erase(static_cast<const void*>(fuente));
                if (renombrando == fuente) renombrando = nullptr;
                if (GestorComandos* comandos = editor->getGestorComandos())
                    comandos->ejecutar(std::make_unique<BorrarObjetoComando>(
                        editor, fuente, scene));
                objetoCopiado = nullptr;
                cortePendiente = false;
            }
        }
    }
}

bool SceneObjectTree::esDescendiente(GameObject* nodo, GameObject* ancestro) {
    for (Entity* p = nodo ? nodo->getParentEntity() : nullptr; p;
         p = p->getParentEntity()) {
        if (p == ancestro) return true;
    }
    return false;
}

void SceneObjectTree::manejarAtajos() {
    if (!editor || !scene) return;
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) return;
    // Con un campo de texto activo (renombrado), Ctrl+C/V son del texto.
    if (ImGui::IsAnyItemActive()) return;
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_C) ||
        ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_X)) {
        GameObject* seleccionado = editor->getSelectedObject();
        if (!seleccionado || seleccionado == scene->getRoot()) return;
        objetoCopiado = seleccionado;
        cortePendiente =
            ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_X);
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_V)) {
        if (!objetoCopiado || !scene->contains(objetoCopiado)) {
            objetoCopiado = nullptr;
            cortePendiente = false;
            return;
        }
        // Pegar con un objeto seleccionado entra como su hijo; sin seleccion
        // (clic en el vacio deselecciona) entra a la raiz. Diferido como el
        // resto: mutar el arbol aca invalidaria el recorrido.
        objetoAPegar = objetoCopiado;
        padreDePegado = editor->getSelectedObject();
    }
}
