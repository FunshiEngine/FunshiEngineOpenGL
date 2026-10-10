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
#ifndef SCENEMENUBARINTERFACE_H
#define SCENEMENUBARINTERFACE_H
#include <iostream>
#include <functional>
#include <map>
#include <string>
#include <memory>
#include "../ObjetosGUI/SettingsObjectInterface.h"
#include "../../Events/EditorEventBus.h"
#include "../Export/ExportDialog.h"
#include <imgui.h>

using namespace std;

class IconosGUI;

class SceneMenuBarInterface : public GeneralUserInterface {
public:
    enum class AccionSimulacion {
        IniciarDepuracion,
        IniciarJuego,
        Pausa,
        Reset,
        Terminar
    };

protected:
    bool* toggleBool = nullptr;
    bool* pausada_ = nullptr;
    bool* modoJuego_ = nullptr;
    bool* gizmoGlobal = nullptr;
    std::function<void(AccionSimulacion)> accionSimulacion_;
    IconosGUI* iconosGUI_ = nullptr;
    EditorEventBus* busEditor = nullptr;
    std::map<std::string, bool> ventanas_;
    std::unique_ptr<ExportDialog> exportDialog_;
    bool mostrarExportDialog_ = false;
    std::string proyectoActual_;

public:
    SceneMenuBarInterface(bool stateGUI);
    void setActivador(bool* targetBool);
    void setPausa(bool* pausada) noexcept;
    void setModoJuego(bool* modoJuego) noexcept;
    bool* getActivador();
    void setAccionSimulacion(
        std::function<void(AccionSimulacion)> accion);
    void setIconosGUI(IconosGUI* iconos) noexcept;
    void setGizmoGlobal(bool* target);
    void setEditorEventBus(EditorEventBus* bus);
    void setVentanas(const std::map<std::string, bool>& estados);
    void setProyectoActual(const std::string& proyecto);
    virtual void initGUI() override;
    virtual void contentGUI() override;
    virtual void endGUI() override;
    virtual void printGUI() override;
};
#endif
