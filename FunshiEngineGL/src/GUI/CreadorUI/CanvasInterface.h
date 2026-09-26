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
#ifndef CANVASINTERFACE_H
#define CANVASINTERFACE_H

#include "../GeneralUserInterface.h"

#include "UserInterfaceCustom.h"

class AudioEngine;

// Pinta EN VIVO la interfaz creada en el CreadorDeInterfaces y reproduce el
// SONIDO asignado a cada widget cuando el usuario la interactua. En el editor
// es una ventana anclable para probar; en modo play se convierte en un overlay
// a pantalla completa sobre el viewport. La reproduccion la hace el
// AudioEngine (hilo de audio propio), nunca este hilo.
class CanvasInterface : public GeneralUserInterface {
private:
    UserInterfaceCustom* ui_ = nullptr;
    AudioEngine* motor_ = nullptr;
    bool modoPlay_ = false;

public:
    explicit CanvasInterface(bool stateGUI);

    // Interfaz a pintar (nullptr = mensaje "sin interfaz activa"). El canvas
    // refleja en vivo el estado de los controles (checkbox, slider, texto).
    void setUI(UserInterfaceCustom* ui) noexcept { ui_ = ui; }
    void setAudioEngine(AudioEngine* motor) noexcept { motor_ = motor; }
    // true = overlay a pantalla completa (modo play); false = ventana dockable.
    void setModoPlay(bool modo) noexcept { modoPlay_ = modo; }
    bool hasUI() const noexcept { return ui_ != nullptr; }

    virtual void initGUI() override;
    virtual void contentGUI() override;
    virtual void printGUI() override;
};

#endif // CANVASINTERFACE_H