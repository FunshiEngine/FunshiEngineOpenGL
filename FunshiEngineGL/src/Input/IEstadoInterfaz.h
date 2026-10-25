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
#ifndef IESTADOINTERFAZ_H
#define IESTADOINTERFAZ_H

#include <string>

// Estado de la interfaz que el modulo de input necesita para decidir si una
// tecla o un click pertenecen al editor o a un widget.
//
// La interfaz es la dueña de esa informacion: solo ella sabe si hay un campo
// de texto en edicion, un popup modal abierto o un widget con el foco. La
// consulta viaja por esta interfaz, asi el input traduce eventos de GLFW sin
// depender de como se dibuje la interfaz ni de que biblioteca la dibuje.
class IEstadoInterfaz {
public:
    virtual ~IEstadoInterfaz() = default;

    // La interfaz tiene el teclado: la tecla pertenece a un widget (campo de
    // texto en edicion, popup modal) y no al editor.
    virtual bool capturaTeclado() const = 0;

    // La interfaz tiene el mouse: el click o la rueda pertenecen a un widget.
    virtual bool capturaMouse() const = 0;

    // Motivo legible de la captura del teclado, para el log: que widget la
    // sostiene. "sin captura" cuando la interfaz no tiene el teclado.
    virtual std::string motivoCapturaTeclado() const = 0;
};

#endif // IESTADOINTERFAZ_H
