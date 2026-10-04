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
#include "DockSpaceInterface.h"
#include "../WindowNames.h"
#include <imgui.h>
#include <imgui_internal.h>

DockSpaceInterface::DockSpaceInterface(bool state)
    : GeneralUserInterface(WindowNames::EditorDockSpace, state,
                           ImGuiWindowFlags_NoDocking) {}

void DockSpaceInterface::initGUI() {
    // Setup del layout inicial (solo la primera vez; luego se persiste en imgui.ini)
    dockspaceId = ImGui::GetID(WindowNames::EditorDockSpace);
    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

        ImGuiID central = dockspaceId;
        ImGuiID left    = ImGui::DockBuilderSplitNode(central, ImGuiDir_Left,  0.20f, nullptr, &central);
        ImGuiID right   = ImGui::DockBuilderSplitNode(central, ImGuiDir_Right, 0.25f, nullptr, &central);
        ImGuiID top     = ImGui::DockBuilderSplitNode(central, ImGuiDir_Up,    0.06f, nullptr, &central);
        ImGuiID bottom  = ImGui::DockBuilderSplitNode(central, ImGuiDir_Down,  0.25f, nullptr, &central);

        // La columna izquierda se divide: arbol de objetos arriba, archivos abajo
        ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.5f, nullptr, &left);

        // Los nombres vienen de WindowNames para no divergir con los paneles:
        // antes el contenido era "Show Folder " (con espacio) escrito a
        // mano en dos sitios y un rename rompia el anclaje del dock.
        ImGui::DockBuilderDockWindow(WindowNames::SelectedObjects, left);
        ImGui::DockBuilderDockWindow(WindowNames::BrowseFile,     leftBottom);
        ImGui::DockBuilderDockWindow(WindowNames::Settings,       right);
        ImGui::DockBuilderDockWindow(WindowNames::MenuBar,        top);
        ImGui::DockBuilderDockWindow(WindowNames::ShowFolder,     bottom);
        ImGui::DockBuilderDockWindow(WindowNames::Status,       right);

        ImGui::DockBuilderFinish(dockspaceId);
    }
}

void DockSpaceInterface::contentGUI() {
    // PassthruCentralNode: el nodo central queda transparente y deja ver la escena 3D
    ImGui::DockSpaceOverViewport(dockspaceId,
                                 ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);
}

void DockSpaceInterface::endGUI() {}

void DockSpaceInterface::printGUI() {
    if (stateGUI) { initGUI(); contentGUI(); endGUI(); }
}