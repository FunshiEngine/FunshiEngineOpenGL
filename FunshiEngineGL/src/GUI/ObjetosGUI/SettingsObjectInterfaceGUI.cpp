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
#include "SettingsObjectInterfaceDetalle.h"
#include "../../Localizacion/Localizacion.h"
#include "../WindowNames.h"

#include <cstdio>

using Localizacion::Texto;

void SettingsObjectInterface::initGUI() {
    ImGui::Begin(WindowNames::titulo(getNameGui()).c_str(), &stateGUI, getFlagGui());
    ImGui::PushID(object);
}

void SettingsObjectInterface::contentGUI() {
    if (!object) return;

    if (tagBufferOwner_ != object) {
        tagBuffer_.fill('\0');
        const std::string& tag = object->getTag();
        std::memcpy(tagBuffer_.data(), tag.data(),
                    std::min(tag.size(), tagBuffer_.size() - 1));
        tagBufferOwner_ = object;
    }

    ImGui::TextUnformatted(Localizacion::tr(Texto::Tag));
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::InputText("##TagObject", tagBuffer_.data(),
                         tagBuffer_.size(),
                         ImGuiInputTextFlags_EnterReturnsTrue))
        object->setTag(tagBuffer_.data());
    ImGui::SameLine();
    const std::string tagActual = object->getTag();
    if (ImGui::BeginCombo("##TagsRegistrados", tagActual.c_str())) {
        for (const std::string& tag : TagRegistry::registrados()) {
            const bool seleccionado = tag == tagActual;
            if (ImGui::Selectable(tag.c_str(), seleccionado)) {
                object->setTag(tag);
                tagBuffer_.fill('\0');
                std::memcpy(tagBuffer_.data(), tag.data(),
                            std::min(tag.size(), tagBuffer_.size() - 1));
            }
            if (seleccionado) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Separator();

    // MOSTRAMOS COMPONENTES
    iterandoComponentes = true;
    componenteABorrar = nullptr;

    // Purga preventiva: elimina settings de componentes que ya no existen
    // en el objeto (evita punteros colgantes si el evento de estructura
    // no se proceso a tiempo).
    purgarSettingsHuerfanos();

    if (!listaDESettingsComponent->isEmpty()) {
        Position<SettingsComponent*>* position =
            listaDESettingsComponent->first();

        while (position != nullptr && position->getElement() != nullptr) {
            SettingsComponent* comp = position->getElement();
            if (!comp) {
                position = (position != listaDESettingsComponent->last())
                               ? listaDESettingsComponent->next(position)
                               : nullptr;
                continue;
            }
            ImGui::PushID(comp);

            // Obtener componente subyacente de forma segura
            Component* componenteReal = comp->getComponent();
            if (!componenteReal) {
                ImGui::PopID();
                position = (position != listaDESettingsComponent->last())
                               ? listaDESettingsComponent->next(position)
                               : nullptr;
                continue;
            }

            // Verificacion adicional: el componente debe seguir en el objeto
            // Usamos el tipo almacenado para no depender de punteros
            bool componenteValido = false;
            if (object) {
                ListaDE<Component*>* componentes = object->getComponents();
                if (componentes && !componentes->isEmpty()) {
                    const std::string& tipoEsperado = comp->getTipoComponente();
                    if (!tipoEsperado.empty()) {
                        Position<Component*>* pc = componentes->first();
                        while (pc != nullptr) {
                            Component* c = pc->getElement();
                            if (c) {
                                std::string tipoActual = demangle(typeid(*c).name());
                                if (tipoActual == tipoEsperado) {
                                    componenteValido = true;
                                    break;
                                }
                            }
                            pc = (pc != componentes->last())
                                     ? componentes->next(pc)
                                     : nullptr;
                        }
                    }
                }
            }
            if (!componenteValido) {
                ImGui::PopID();
                position = (position != listaDESettingsComponent->last())
                               ? listaDESettingsComponent->next(position)
                               : nullptr;
                continue;
            }

            // Nombre visible: el del catalogo en el idioma vigente (no el
            // nombre del tipo de C++). Los scripts muestran su propio nombre.
            std::string compName;
            if (Script* script = dynamic_cast<Script*>(componenteReal)) {
                std::string nombreScript = script->nombreParaMostrar();
                if (nombreScript.empty()) nombreScript = Localizacion::tr(Texto::CompScript);
                char titulo[256];
                std::snprintf(titulo, sizeof(titulo), Localizacion::tr(Texto::ScriptConNombre),
                              nombreScript.c_str());
                compName = titulo;
            } else {
                std::string tipo = comp->getTipoComponente();
                if (tipo.empty() && componenteReal) tipo = demangle(typeid(*componenteReal).name());
                const DescriptorComponente* d = CatalogoComponentes::instancia().buscar(tipo);
                compName = d ? Localizacion::tr(d->texto) : tipo;
            }
            if (compName.empty()) compName = Localizacion::tr(Texto::Componente);
            // ID fijo: plegar un componente no se pierde al cambiar de idioma.
            const std::string encabezado = compName + "###encabezado";

            // Icono del componente a la izquierda de su nombre. Se dibuja antes
            // del encabezado (y antes de SetNextItemOpen, que aplica al
            // encabezado): si se dibujara despues, el popup contextual y el
            // arrastre quedarian anclados al icono y no al nombre.
            if (resolutorIconos_) {
                const ImTextureID icono =
                    resolutorIconos_->iconoPara(comp->getTipoComponente());
                if (icono != ImTextureID_Invalid) {
                    ImGui::Image(icono, ImVec2(kTamanoIconoComponente,
                                              kTamanoIconoComponente));
                    ImGui::SameLine();
                }
            }

            SettingsScript* scriptSettings =
                dynamic_cast<SettingsScript*>(comp);
            if (scriptSettings && scriptSettings->estaEditandoNombre())
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            bool open = ImGui::CollapsingHeader(
                encabezado.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            if (ImGui::BeginPopupContextItem(
                    "ComponentContext",
                    ImGuiPopupFlags_MouseButtonRight)) {
                if (scriptSettings &&
                    ImGui::MenuItem(Localizacion::etiqueta(Texto::RenombrarComponente).c_str()))
                    scriptSettings->iniciarEdicionNombre();
                if (ImGui::MenuItem(Localizacion::etiqueta(Texto::EliminarComponente).c_str())) {
                    Component* target = comp->getComponent();
                    // Por el gestor: Ctrl+Z restaura el componente quitado.
                    if (target && editor && editor->getGestorComandos())
                        editor->getGestorComandos()->ejecutar(
                            std::make_unique<QuitarComponenteComando>(
                                editor, object,
                                demangle(typeid(*target).name()),
                                editor->getScene()));
                    else if (editor)
                        editor->removeComponent(object, target);
                    else
                        object->deleteComponent(target);
                    componenteABorrar = comp;
                    ImGui::EndPopup();
                    ImGui::PopID();
                    break;
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource(
                    ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
                ImGui::SetDragDropPayload("COMPONENT_DRAG", &comp,
                                          sizeof(SettingsComponent*));
                ImGui::Text("%s", compName.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload =
                        ImGui::AcceptDragDropPayload("COMPONENT_DRAG")) {
                    SettingsComponent* draggedComp =
                        *(SettingsComponent**)payload->Data;
                    auto posA =
                        listaDESettingsComponent->whatElementPosition(
                            draggedComp);
                    auto posB = listaDESettingsComponent->whatElementPosition(
                        comp);
                    if (posA && posB && posA != posB) {
                        listaDESettingsComponent->swapPositions(posA, posB);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            // Archivo soltado sobre la cabecera del componente: se aplica al
            // objeto inspeccionado al terminar el recorrido (agregar un componente
            // ahora invalidaria la lista que se esta iterando).
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload =
                        ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                    if (payload->Data)
                        archivoSoltado_ = static_cast<const char*>(payload->Data);
                }
                ImGui::EndDragDropTarget();
            }

            if (open) {
                comp->setMostrarVisualesDepuracion(
                    mostrarVisualesDepuracion_);
                comp->showDataComponent();
            }

            ImGui::PopID();
            position = (position != listaDESettingsComponent->last())
                           ? listaDESettingsComponent->next(position)
                           : nullptr;
        }
    }

    iterandoComponentes = false;

    // Toda la ventana es destino de archivos: los campos de cada panel tienen
    // su propio destino y, como ImGui entrega el payload al destino mas chico
    // bajo el mouse, este solo recibe lo que se suelta fuera de ellos.
    if (object) {
        ImGuiWindow* ventana = ImGui::GetCurrentWindow();
        if (ImGui::BeginDragDropTargetCustom(ventana->InnerRect,
                                             ImGui::GetID("##InspectorDropArchivo"))) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                if (payload->Data)
                    archivoSoltado_ = static_cast<const char*>(payload->Data);
            }
            ImGui::EndDragDropTarget();
        }
    }
    if (!archivoSoltado_.empty()) {
        const std::string ruta = std::move(archivoSoltado_);
        archivoSoltado_.clear();
        if (object && editor) {
            const ArrastreArchivos::Resultado r =
                ArrastreArchivos::aplicarAObjeto(*editor, *object, ruta);
            if (!r.ok)
                std::cerr << "[Inspector] " << ruta << ": " << r.mensaje << std::endl;
            else if (r.objeto && r.objeto != object)
                editor->selectObject(r.objeto);
        }
    }

    // Procesar borrado diferido fuera de la iteracion
    if (componenteABorrar) {
        // El componente ya se libero: reconciliar purga el panel huerfano y
        // crea los que falten, sin descartar el estado del resto de paneles.
        componenteABorrar = nullptr;
        reconciliarComponentes();
    }

    // Menu contextual en area vacia del inspector: agregar componente
    if (ImGui::BeginPopupContextWindow(
            "InspectorAddComponentPopup",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (object && editor) {
            // Alta por el gestor: Ctrl+Z quita el componente agregado.
            auto agregarViaComando =
                [&](std::unique_ptr<Component> componente) {
                    if (!componente) return;
                    if (GestorComandos* comandos = editor->getGestorComandos())
                        comandos->ejecutar(
                            std::make_unique<AgregarComponenteComando>(
                                editor, object, std::move(componente),
                                editor->getScene()));
                    else
                        editor->addComponent(object, std::move(componente));
                };

            // El catalogo define que componentes existen, en que dimension
            // aplican, que requisitos piden y en que submenu se agrupan. El
            // menu solo filtra y dibuja; agregar un componente nuevo no toca
            // este bloque.
            const DimensionProyecto dimension = dimensionProyecto();
            const std::vector<DescriptorComponente>& descriptores =
                CatalogoComponentes::instancia().descriptores();

            auto visible = [&](const DescriptorComponente& d) {
                if (!dimensionesIncluyen(d.dimensiones, dimension)) return false;
                if (d.prerequisito && !d.prerequisito(*object)) return false;
                return true;
            };
            auto dibujarItems = [&](const std::string& submenu) {
                for (const DescriptorComponente& d : descriptores) {
                    if (d.submenu != submenu) continue;
                    if (!visible(d)) continue;
                    if (ImGui::MenuItem(Localizacion::etiqueta(d.texto).c_str())) {
                        if (d.crear) agregarViaComando(d.crear(*object));
                    }
                }
            };

            dibujarItems(std::string());
            // Submenus en orden de aparicion, omitiendo los que no tienen
            // ningun item visible para la dimension vigente.
            std::vector<std::string> submenus;
            for (const DescriptorComponente& d : descriptores) {
                if (d.submenu.empty()) continue;
                if (std::find(submenus.begin(), submenus.end(), d.submenu) ==
                    submenus.end())
                    submenus.push_back(d.submenu);
            }
            for (const std::string& submenu : submenus) {
                bool hayItemVisible = false;
                Texto textoSubmenu = Texto::Componente;
                for (const DescriptorComponente& d : descriptores) {
                    if (d.submenu == submenu) textoSubmenu = d.textoSubmenu;
                    if (d.submenu == submenu && visible(d)) {
                        hayItemVisible = true;
                        break;
                    }
                }
                if (!hayItemVisible) continue;
                // El submenu se identifica por su clave interna, no por el
                // texto: cambiar de idioma no lo cierra.
                const std::string tituloSubmenu =
                    std::string(Localizacion::tr(textoSubmenu)) + "###" + submenu;
                if (ImGui::BeginMenu(tituloSubmenu.c_str())) {
                    dibujarItems(submenu);
                    ImGui::EndMenu();
                }
            }
        }
        ImGui::EndPopup();
    }
}

void SettingsObjectInterface::endGUI() {
    ImGui::PopID();
    ImGui::End();
}

void SettingsObjectInterface::printGUI() {
    if (stateGUI) {
        initGUI();
        contentGUI();
        endGUI();
    }
}
