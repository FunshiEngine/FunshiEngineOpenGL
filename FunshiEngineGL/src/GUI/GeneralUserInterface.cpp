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
#include "GeneralUserInterface.h"
#include "WindowNames.h"
#include <utility>

GeneralUserInterface::GeneralUserInterface(std::string name, bool state, ImGuiWindowFlags flags)
    : nameGUI(std::move(name)), stateGUI(state), flagGUI(flags) {}
void GeneralUserInterface::setNameGui(std::string name) { nameGUI = std::move(name); }
void GeneralUserInterface::setStateGui(bool state) { stateGUI = state; }
std::string GeneralUserInterface::getNameGui() const { return nameGUI; }
bool GeneralUserInterface::getStateGui() const { return stateGUI; }
ImGuiWindowFlags GeneralUserInterface::getFlagGui() const { return flagGUI; }
void GeneralUserInterface::initGUI() { ImGui::Begin(WindowNames::titulo(nameGUI).c_str(), &stateGUI, flagGUI); }
void GeneralUserInterface::endGUI() { ImGui::End(); }
