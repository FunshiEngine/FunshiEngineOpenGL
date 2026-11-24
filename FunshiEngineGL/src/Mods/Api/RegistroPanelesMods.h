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

// Registro de los paneles de GUI que aportan los mods cargados.
//
// Un mod registra un panel desde su init() (o desde un update) con la tabla de
// servicios; el motor lo guarda aqui y la capa de GUI lo dibuja como una
// ventana propia. Este modulo no depende de ImGui ni de las clases del motor:
// es solo el almacen de los paneles y su estado, para que se pueda probar
// headless.
//
// La pertenencia de un panel a su mod se resuelve con el "mod en ejecucion":
// el motor lo fija antes de invocar al mod (init/update/dibujar) y lo limpia
// al salir. Como las llamadas del mod al motor son sincronas y anidadas (el
// motor llama al mod, el mod llama al motor), un solo puntero basta.

#ifndef REGISTRO_PANELES_MODS_H
#define REGISTRO_PANELES_MODS_H

#include <string>
#include <vector>

#include "ApiMod.h"

namespace FunshiMods {

// Panel registrado por un mod.
struct PanelMod {
    // Id del mod propietario (el del manifiesto).
    std::string modId;
    // Id del panel dentro del mod.
    std::string id;
    // Titulo visible de la ventana.
    std::string titulo;
    // Clave de la ventana: nombre unico y estable con el que se persiste el
    // estado (abierto/cerrado) junto al de las demas ventanas del editor.
    std::string nombreVentana;
    // Callback de dibujo que invoca el motor por frame.
    CallbackPanelMod dibujar = nullptr;
    // Puntero opaco que el motor devuelve tal cual al callback.
    void* datos = nullptr;
    // Estado de la ventana (abierta/cerrada). Lo escribe la capa de GUI cada
    // frame con lo que ImGui reporte (el usuario la puede cerrar con la 'X').
    // Un panel nuevo se abre: el mod que lo registra espera que se vea. El
    // estado se persiste con el proyecto, asi que si el usuario lo cierra,
    // sigue cerrado al reiniciar el editor.
    bool abierta = true;
};

class RegistroPanelesMods {
public:
    // Registro global: el motor, la tabla de servicios y la capa de GUI
    // comparten esta misma instancia.
    static RegistroPanelesMods& global();

    // Registra un panel de un mod concreto. Devuelve false si falta el id o
    // el titulo, si no hay callback de dibujo, si el id ya esta registrado en
    // ese mod o si el modId esta vacio.
    bool registrar(const std::string& modId, const std::string& id,
                   const std::string& titulo, CallbackPanelMod dibujar,
                   void* datos);

    // Registra un panel del mod en ejecucion (el que esta llamando al motor
    // ahora mismo). Devuelve false si no hay ningun mod en ejecucion.
    bool registrarDelModEnEjecucion(const char* id, const char* titulo,
                                    CallbackPanelMod dibujar, void* datos);

    // Quita todos los paneles de un mod (al descargar el mod).
    void quitarMod(const std::string& modId);

    // Quita todos los paneles.
    void limpiar();

    const std::vector<PanelMod>& paneles() const { return paneles_; }

    // Panel por clave de ventana (nullptr si no existe).
    const PanelMod* buscarPorVentana(const std::string& nombreVentana) const;

    // Abre o cierra un panel por su clave de ventana. Devuelve false si no
    // existe ese panel.
    bool establecerEstado(const std::string& nombreVentana, bool abierta);

    // Mod en ejecucion: el al que se atribuyen los panels que se registran
    // desde una llamada del mod al motor.
    void fijarModEnEjecucion(const std::string& modId);
    void limpiarModEnEjecucion();
    const std::string& modEnEjecucion() const { return modEnEjecucion_; }

    // Clave de ventana de un panel: unica por mod y panel, y estable entre
    // arranques (es lo que se persiste).
    static std::string nombreVentanaDe(const std::string& modId,
                                       const std::string& id);

private:
    RegistroPanelesMods() = default;

    std::vector<PanelMod> paneles_;
    std::string modEnEjecucion_;
};

} // namespace FunshiMods

#endif // REGISTRO_PANELES_MODS_H
