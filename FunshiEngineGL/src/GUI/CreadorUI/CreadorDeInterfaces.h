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
#ifndef CREADORDEINTERFACES_H
#define CREADORDEINTERFACES_H

#include "../GeneralUserInterface.h"

#include <string>
#include <vector>

#include "UserInterfaceCustom.h"

// Ventana del editor para CREAR interfaces de usuario (assets JSON en
// Memory/Interfaces del proyecto): listar, crear, editar widgets (etiqueta,
// boton, checkbox, slider, entrada de texto) y asignarle a cada widget un
// SONIDO del proyecto (dropdown con los clips de Sonidos/). La interfaz
// activa se le entrega al CanvasInterface para probarla en vivo.
//
// Es un GeneralUserInterface mas: lo posee GUIManager (ventana persistente por
// nombre, alternable desde el menu "Ventanas" y anclable al dock).
class CreadorDeInterfaces : public GeneralUserInterface {
private:
    // Nombres de los assets encontrados en Memory/Interfaces (sin ".json").
    std::vector<std::string> interfaces_;
    std::string directorio_;
    // Asset en edicion (fuente de verdad del area de edicion).
    UserInterfaceCustom borrador_;
    // Indice del widget que se esta editando (-1 = ninguno).
    int widgetSeleccionado_ = -1;
    // Catalogo de clips del proyecto para el dropdown de sonidos.
    std::vector<std::string> clipNames_;
    // Interfaz activa para el canvas (se persiste entre frames, no en disco).
    bool tieneActiva_ = false;
    UserInterfaceCustom interfazActiva_;
    std::string nombreActiva_;

    void refrescarLista();
    void crearNueva();

public:
    explicit CreadorDeInterfaces(bool stateGUI);

    // Configura la carpeta Memory/Interfaces del proyecto y recarga la lista.
    void configurarProyecto(const std::string& directorio);
    // Catalogo de clips (nombres) para el dropdown "Sonido".
    void setClipNames(std::vector<std::string> nombres);

    // Interfaz activa para el canvas (nullptr si no hay ninguna). El canvas la
    // refleja en vivo (estado de checkbox/slider/texto), asi que no es const.
    UserInterfaceCustom* getInterfazActiva();
    const UserInterfaceCustom* getInterfazActiva() const;
    std::string getNombreActiva() const noexcept { return nombreActiva_; }

    // Activa por nombre la interfaz que usara el juego (modo play). Carga el
    // asset de disco si no es la activa actual; nullptr si no existe.
    UserInterfaceCustom* activarInterfaz(const std::string& nombre);

    virtual void contentGUI() override;
    virtual void printGUI() override;
};

#endif // CREADORDEINTERFACES_H