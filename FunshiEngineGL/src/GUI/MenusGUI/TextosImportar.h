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
#ifndef TEXTOSIMPORTAR_H
#define TEXTOSIMPORTAR_H

#include "../../Configuracion/Proyecto/ImportarProyecto.h"
#include "../../Localizacion/Localizacion.h"

// Texto que se muestra para cada problema de ImportarProyecto. Lo usan la
// vista (al analizar lo elegido) y main (al terminar la copia), asi el mismo
// problema se explica igual en los dos lados.
inline Localizacion::Texto textoProblemaImportar(ImportarProyecto::Problema problema) {
    using ImportarProyecto::Problema;
    using Localizacion::Texto;
    switch (problema) {
    case Problema::Ninguno: return Texto::ProyectoImportado;
    case Problema::NoExiste: return Texto::ImportarNoExiste;
    case Problema::NoEsProyecto: return Texto::ImportarNoEsProyecto;
    case Problema::YaEstaEnProyectos: return Texto::ImportarYaEstaEnProyectos;
    case Problema::NombreInvalido: return Texto::ImportarNombreInvalido;
    case Problema::NombreRepetido: return Texto::NombreRepetido;
    case Problema::ErrorCopia: return Texto::ImportarErrorCopia;
    }
    return Texto::ImportarErrorCopia;
}

#endif
