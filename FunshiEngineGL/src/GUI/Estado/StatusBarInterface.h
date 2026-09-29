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
#ifndef STATUSBARINTERFACE_H
#define STATUSBARINTERFACE_H

#include "../GeneralUserInterface.h"
#include "../../Behaviour/ScriptRuntime.h"
#include <string>
#include <vector>

class SceneRegistry;
class EditorEventBus;

// Ventana "Estado": muestra el toolchain externo (compilador C++, javac,
// libjvm, cache de artefactos) y el estado de los scripts de la escena
// como lineas de log simples.
class StatusBarInterface : public GeneralUserInterface {
private:
    SceneRegistry* scene_ = nullptr;
    // Estado de compilacion publicado por GameScene cada frame.
    bool compilando_ = false;
    std::string actual_;
    std::size_t hecha_ = 0;
    std::size_t total_ = 0;
    std::vector<ScriptRuntime::ResultadoCarga> resultados_;
    bool toolchainListo_ = false;
    ScriptRuntime::EstadoHerramientas toolchain_;
    // Mensaje temporal para feedback (p. ej. exportacion).
    std::string mensajeTemporal_;
    float temporizadorMensaje_ = 0.0f;
    // Canal de GUI interna (lo posee GUIManager; puntero NO propietario).
    EditorEventBus* busEditor = nullptr;
    // Ultimo estado publicado en el bus (evita republicar en cada frame);
    // se inicializa con el estado de fabrica para no notificar la restauracion.
    bool estadoPublicado_ = false;

    void dibujarToolchain();

public:
    explicit StatusBarInterface(bool stateGUI);

    void bindScene(SceneRegistry* scene);
    void setEditorEventBus(EditorEventBus* bus);
    // Recibe estado de compilacion desde GameScene y lo convierte a log.
    void setEstadoCompilacion(
        bool enCurso, const std::string& actual, std::size_t hecha,
        std::size_t total,
        const std::vector<ScriptRuntime::ResultadoCarga>& resultados,
        bool overlayProgreso, bool overlayResultado);
    // Muestra un mensaje temporal en la barra de estado (p. ej. resultado de exportacion).
    void mostrarMensaje(const std::string& mensaje);

    void contentGUI() override;
    void printGUI() override;
};

#endif // STATUSBARINTERFACE_H