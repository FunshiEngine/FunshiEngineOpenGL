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
#ifndef MENUGUI_H
#define MENUGUI_H

// =====================================================================
// PAQUETE: MenuGUI — interfaz de inicio (menu) del motor
// =====================================================================
// Este encabezado documenta el paquete de archivos que modulariza el menu
// de inicio y define la fachada del mismo. Siguiendo buenas practicas de
// disenio (SRP, bajo acoplamiento, alta cohesion, DIP), el paquete separa
// responsabilidades en capas con una direccion de dependencias clara:
//
//   MenuModel.h            -> Logica pura y estado (sin ImGui/GLFW). El
//                             "que": navegacion entre vistas y datos de
//                             configuracion (nombre de proyecto, idioma).
//   MenuView.h/.cpp        -> Presentacion ImGui (el "como se dibuja").
//                             Delega toda accion al modelo; no decide nada.
//   StartMenuPresenter.h   -> Puente modelo<->resto del motor (States/):
//                             responde "menu abierto/cerrado" y sincroniza
//                             cambios externos sin acoplar al motor con las
//                             clases internas del paquete.
//   MenuGUI.h/.cpp         -> Fachada del paquete: ensambla modelo + vista +
//                             presentador y expone la interfaz publica
//                             estable (ConsultarMenu / SetMenuActivo /
//                             ConsultarCierre / PedirCierre). main.cpp solo
//                             conversa con la fachada; las clases internas
//                             pueden reescribirse sin tocar el resto.
//
// Direccion de dependencias (una sola):
//   MenuGUI -> StartMenuPresenter -> MenuModel  <-  MenuView
// El modelo no sabe que existe la vista (inversion de dependencias), la
// vista no sabe que existe main, y main no conoce las clases internas.
//
// Reglas de extension (buenas practicas):
// - Nueva pantalla del menu: agregar valor a MenuModel::Vista + metodo de
//   transicion + metodo de render en MenuView (OCP: switch cerrado).
// - Nueva fuente de datos: propiedad en MenuModel + metodos de acceso +
//   sincronizacion desde el presentador (nunca desde la vista).
// - MenuModel debe permanecer sin dependencias de ImGui/GLFW (testeable).
// =====================================================================

#include "MenuModel.h"
#include "MenuView.h"
#include "StartMenuPresenter.h"

class EditorEventBus;

// Fachada del paquete: la interfaz publica con la que el resto del motor
// conversa. Encapsula las clases internas y su ensamblado.
class MenuGUI {
public:
    // Ensambla el paquete: modelo + vista (con la ventana GLFW para "Exit")
    // + presentador. La ventana llega por inyeccion de dependencias.
    MenuGUI(GLFWwindow* window);

    // ---- Interfaz publica estable (la que usa main.cpp) ----

    // "El menu esta abierto?" (per-frame; reemplaza al legacy showMenu).
    bool ConsultarMenu() const noexcept;

    // Sincroniza el estado abierto/cerrado decidido externamente (por
    // ejemplo, la transicion MenuPrincipal<->Edicion de main).
    void SetMenuActivo(bool abierto) noexcept;

    // Dibuja la pantalla activa del menu (delega en la vista interna; la
    // vista ya se auto-oculta si el modelo esta en Vista::Ninguna).
    void Renderizar();

    // "El usuario pidió cerrar el menu (Iniciar Estudio)?" — consumo unico:
    // retorna true una vez por peticion y luego se auto-limpia.
    bool ConsultarCierre() noexcept;

    // Pide el cierre del menu (lo consume ConsultarCierre).
    void PedirCierre() noexcept;

    // Configuracion global del editor configurada en la vista "Opciones"
    // (sensibilidad del mouse look de la camara). Se propaga a la escena por
    // el bus (SensibilidadCambio); main ya no relee el modelo por frame.
    float getSensibilidadCamara() const noexcept;
    void setSensibilidadCamara(float sensibilidad);

    // Sensibilidad de movimiento (WASD) de la camara del editor (vista
    // Opciones). Se propaga a la escena por SensibilidadMovimientoCambio.
    float getSensibilidadMovimientoCamara() const noexcept;
    void setSensibilidadMovimientoCamara(float sensibilidad);

    // Perfil de apariencia del editor editado en la vista Opciones. Se aplica
    // a ImGui (TemaEditor) y a la escena (fondo y grilla) por AparienciaCambio.
    const Apariencia& getApariencia() const noexcept;
    void setApariencia(const Apariencia& valor);

    // Accion global "Restablecer configuracion" (Opciones): vuelve el modelo a
    // los defaults y publica ReiniciarConfiguracion para que main reapique
    // escena/ventanas y persista.
    void reiniciarConfiguracion();

    // Datos del menu persistidos por EditorConfig (main los aplica al arrancar
    // y los recoge al salir).
    const std::string& getNombreProyecto() const noexcept;
    void setNombreProyecto(const std::string& nombre) noexcept;

    // Edicion de nombre por click derecho (passthrough al modelo, sin disco).
    // La vista registra la carpeta original; main lo consume al confirmar.
    const std::string& getProyectoARenombrar() const noexcept;
    void setProyectoARenombrar(const std::string& nombre) noexcept;
    void limpiarProyectoARenombrar() noexcept;

    // Eliminacion por click derecho (passthrough al modelo, sin disco). La
    // vista registra la carpeta confirmada; main la borra de disco y resetea
    // el estado si era el proyecto activo.
    const std::string& getProyectoAEliminar() const noexcept;
    void setProyectoAEliminar(const std::string& nombre) noexcept;
    void limpiarProyectoAEliminar() noexcept;

    // Refresca el listado de proyectos disponibles mostrado en el menu: los
    // proyectos SON las carpetas del directorio base de MotorGrafico. La
    // fachada lo lee del disco y lo vuelca al modelo (nunca la vista); main lo
    // invoca mientras el menu esta visible para reflejar carpetas creadas o
    // borradas externamente.
    void actualizarProyectos();

    const std::string& getIdioma() const noexcept;
    void setIdioma(const std::string& valor) noexcept;

    // Canal de GUI interna (bus tipado que posee GUIManager). La fachada lo
    // usa para publicar los cambios de apariencia/idioma; las ventanas
    // internas no se pasan punteros entre si.
    void setEditorEventBus(EditorEventBus* bus) noexcept;

private:
    // Orden de membresia = orden de construccion: el presentador se construye
    // ANTES que la vista porque la vista recibe su direccion (el presentador
    // solo guarda el puntero al modelo en su ctor, pero el orden correcto
    // elimina cualquier suposicion sobre orden de inicializacion).
    MenuModel model;
    StartMenuPresenter presenter;
    MenuView view;
    // Puntero NO propietario al bus de GUI interna (lo posee GUIManager).
    EditorEventBus* busEditor = nullptr;

    // Traduce un cambio del modelo (MenuModel::Campo) a un evento del bus. Un
    // unico lugar de salida: sirve tanto para los setter de la fachada como
    // para los clics del usuario en la vista (que mutan el modelo directo).
    void publicarCambio(MenuModel::Campo campo);
};

#endif