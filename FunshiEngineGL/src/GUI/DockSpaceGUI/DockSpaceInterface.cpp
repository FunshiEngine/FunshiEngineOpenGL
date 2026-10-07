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
#include <array>
#include <cstring>
#include <vector>

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
        ImGui::DockBuilderDockWindow(WindowNames::CameraList,   right);
        ImGui::DockBuilderDockWindow(WindowNames::CreadorInterfaces, right);

        ultimoDockValido_[WindowNames::SelectedObjects] = left;
        ultimoDockValido_[WindowNames::BrowseFile] = leftBottom;
        ultimoDockValido_[WindowNames::Settings] = right;
        ultimoDockValido_[WindowNames::MenuBar] = top;
        ultimoDockValido_[WindowNames::ShowFolder] = bottom;
        ultimoDockValido_[WindowNames::Status] = right;
        ultimoDockValido_[WindowNames::CameraList] = right;
        ultimoDockValido_[WindowNames::CreadorInterfaces] = right;

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
    initGUI();
    contentGUI();
    endGUI();
}

void DockSpaceInterface::repararVentanasFlotantes() {
    ImGuiContext& g = *GImGui;
    ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspaceId);
    if (!root) return;

    const auto esHojaDelDockspace = [root](ImGuiID id) {
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(id);
        if (!node || !node->IsLeafNode()) return false;
        while (node->ParentNode) node = node->ParentNode;
        return node == root;
    };

    const auto primeraHoja = [](ImGuiDockNode* nodo) {
        while (nodo && !nodo->IsLeafNode())
            nodo = nodo->ChildNodes[0] ? nodo->ChildNodes[0]
                                       : nodo->ChildNodes[1];
        return nodo;
    };
    ImGuiDockNode* hojaAlternativa =
        root->CentralNode && root->CentralNode->IsLeafNode()
            ? root->CentralNode
            : primeraHoja(root);
    if (!hojaAlternativa) return;

    const std::array<const char*, 9> nombres = {
        WindowNames::SelectedObjects, WindowNames::BrowseFile,
        WindowNames::Settings, WindowNames::MenuBar, WindowNames::ShowFolder,
        WindowNames::Status, WindowNames::CameraList,
        WindowNames::CreadorInterfaces, WindowNames::CanvasUI};
    std::vector<ImGuiWindow*> ventanas;
    for (const char* nombre : nombres) {
        if (ImGuiWindow* window = ImGui::FindWindowByName(nombre))
            ventanas.push_back(window);
    }
    for (ImGuiWindow* window : g.Windows) {
        if (std::strncmp(window->Name, "Vista previa: ", 14) == 0)
            ventanas.push_back(window);
    }

    for (ImGuiWindow* window : ventanas) {
        if (window->Flags & ImGuiWindowFlags_NoDocking) continue;
        const char* nombre = window->Name;

        ImGuiDockNode* node = window->DockNode;
        if (node && node->IsLeafNode()) {
            ImGuiDockNode* ancestor = node;
            while (ancestor->ParentNode) ancestor = ancestor->ParentNode;
            if (ancestor == root) {
                ultimoDockValido_[nombre] = node->ID;
                continue;
            }
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
            g.MovingWindow == window)
            continue;

        ImGuiID destino = hojaAlternativa->ID;
        const auto ultimo = ultimoDockValido_.find(nombre);
        if (ultimo != ultimoDockValido_.end() &&
            esHojaDelDockspace(ultimo->second))
            destino = ultimo->second;
        ImGui::DockBuilderDockWindow(nombre, destino);
        ultimoDockValido_[nombre] = destino;
    }
}