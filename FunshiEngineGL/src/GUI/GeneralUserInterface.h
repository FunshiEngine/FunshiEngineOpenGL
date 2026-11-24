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
#ifndef GENERALUSERINTERFACE_H
#define GENERALUSERINTERFACE_H
#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


using namespace std;

class GeneralUserInterface {
protected:
    string nameGUI;
    bool stateGUI;
    ImGuiWindowFlags flagGUI;

public:
    GeneralUserInterface(string nameGUI, bool stateGUI, ImGuiWindowFlags flagGUI);
    virtual ~GeneralUserInterface() = default;

    void setNameGui(string nameGUI);
    void setStateGui(bool stateGUI);

    string getNameGui() const;
    bool getStateGui() const;
    ImGuiWindowFlags getFlagGui() const;

    virtual void initGUI();

    virtual void contentGUI() = 0;
    virtual void printGUI() = 0;

    virtual void endGUI();
};
#endif