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
#ifndef MENUVIEW_H
#define MENUVIEW_H

#include "../GeneralUserInterface.h"
// GLFW se incluye solo por el botón "Exit"; la lógica de navegación y los
// datos viven en StartMenuModel, no acá. Ningun GL: solo el tipo GLFWwindow.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "MenuModel.h"
#include "StartMenuPresenter.h"

// Vista del menu de inicio: pantalla completa a traves del paquete MenuGUI.
// Es la capa de presentacion del patron MVP que define este paquete. NO
// contiene logica de navegacion ni de datos: dibuja a partir del estado del
// modelo y, ante los clics del usuario, delega la accion de vuelta al modelo
// (la clase base es la vista). Asi la interfaz se puede reemplazar o recorrer
// sin tocar la logica, y cada pantalla se reimplementa sin afectar a las demas.
class MenuView : public GeneralUserInterface {
private:
    MenuModel* model;
    // Decisiones que afectan al resto del motor (cerrar el menu = "Iniciar
    // Estudio") se notifican por el presenter, nunca directamente al modelo:
    // es el presenter quien informa a main via DebeCerrar().
    StartMenuPresenter* presenter;
    GLFWwindow* window;
    // Buffer de edicion del nombre del proyecto. Vive en la vista porque es
    // estado de UI (la vista avisa al modelo del cambio, no al reves).
    char nombreProyectoBuffer[256];
    // Indica que el buffer tiene cambios sin confirmar (el usuario escribio
    // en el InputText pero aun no presiono "Confirmar"). La creacion de la
    // carpeta y la notificacion al modelo solo ocurren al confirmar.
    bool nombreProyectoPendiente = false;
    // Modal de renombre: proyecto original en edicion y bandera que pide abrir
    // el modal ("Renombrar", con InputText propio + Confirmar/Cancelar). Igual
    // que ContentFolderInterface (abrirPopupRenombrar), se abre un frame
    // despues de pedirlo para que ImGui lo registre correctamente.
    std::string proyectoRenombrando;
    bool abrirModalRenombrar = false;
    char nombreRenombrarBuffer[256] = "";
    // Modal de eliminacion: proyecto a eliminar y bandera que pide abrir el
    // modal ("EliminarProyecto": aviso irreversible + Eliminar/Cancelar).
    std::string proyectoEliminando;
    bool abrirModalEliminar = false;

    void renderizarPrincipal();
    void renderizarListaProyectos();
    void renderizarOpciones();
    void renderizarConfigProyecto();

public:
    MenuView(MenuModel* model, StartMenuPresenter* presenter, GLFWwindow* window);

    void initGUI() override;
    void contentGUI() override;
    void endGUI() override;
    void printGUI() override;
};

#endif