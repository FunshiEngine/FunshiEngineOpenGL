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
#include <utility>
#include <vector>
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
    // Paneles que aportan los mods cargados: clave de la ventana y etiqueta
    // visible en el menu. La lista cambia con los mods que se cargan, asi que
    // se refresca cada frame.
    std::vector<std::pair<std::string, std::string>> panelesMods_;
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
    void setPanelesMods(
        const std::vector<std::pair<std::string, std::string>>& paneles);
    void setProyectoActual(const std::string& proyecto);

    // Proyecto activo que la barra refleja (lo usa el dialogo de exportacion
    // para saber que proyecto exportar).
    const std::string& proyectoActual() const noexcept {
        return proyectoActual_;
    }

    // Abre el dialogo de exportacion (lo crea si no existe). El dialogo
    // nasce con el proyecto activo: sin el, la exportacion no se puede
    // lanzar.
    void abrirDialogoExportacion();

    // Dialogo de exportacion abierto (nullptr si todavia no se abrio). Permite
    // consultar su estado (proyecto, fase) sin depender del ImGui.
    ExportDialog* dialogoExportacion() const noexcept {
        return exportDialog_.get();
    }

    virtual void initGUI() override;
    virtual void contentGUI() override;
    virtual void endGUI() override;
    virtual void printGUI() override;
};
#endif
