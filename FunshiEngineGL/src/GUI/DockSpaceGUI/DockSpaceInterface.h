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
#ifndef DOCKSPACEINTERFACE_H
#define DOCKSPACEINTERFACE_H

#include "../GeneralUserInterface.h"
#include <imgui.h>
#include <string>
#include <unordered_map>

class DockSpaceInterface : public GeneralUserInterface {
protected:
	ImGuiID dockspaceId = 0;
	std::unordered_map<std::string, ImGuiID> ultimoDockValido_;
public:
	DockSpaceInterface(bool stateGUI);
	virtual void initGUI() override;
	virtual void contentGUI() override;
	virtual void endGUI() override;
	virtual void printGUI() override;
	void repararVentanasFlotantes();
};
#endif