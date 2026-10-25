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
#ifndef PRECONDICIONESEXPORTACION_H
#define PRECONDICIONESEXPORTACION_H

#include <string>

// Comprobaciones que se hacen antes de lanzar una exportacion.
//
// Son reglas de dominio, no de interfaz: devuelven el motivo (el texto que ve
// el usuario) para que el dialogo pueda mostrarlo en lugar de fallar en
// silencio.
namespace PrecondicionesExportacion {

// La peticion se puede lanzar si hay un proyecto elegido y su carpeta existe.
// Devuelve false y escribe el motivo en `error` cuando no.
bool comprobar(const std::string& nombreProyecto,
               const std::string& rutaProyecto,
               std::string& error);

} // namespace PrecondicionesExportacion

#endif // PRECONDICIONESEXPORTACION_H
