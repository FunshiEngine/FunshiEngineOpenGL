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
#ifndef TYPEUTILS_H
#define TYPEUTILS_H

#include <string>
#include <typeinfo>

// Color en coma flotante (RGBA). Es un tipo plano compartido, no un
// componente: vive aqui para que el dominio no tenga que incluir el
// componente `Color` solo para nombrarlo.
typedef float color[4];

// Demangling del nombre de clase RTTI. Definido en TypeUtils.cpp.
std::string demangle(const char* name);

// Nombre simple y portable de un tipo: sin el namespace y sin el prefijo de
// class-key que MSVC deja en typeid().name() ("class Transform", "struct
// Color"). Admite el nombre crudo de typeid() en cualquier plataforma y
// tambien un nombre ya demangleado o persistido en un archivo, de modo que el
// mismo nombre sirve para escribir y para leer en Windows y en Linux.
std::string nombreSimpleTipo(const std::string& nombre);

// Nombre simple del tipo de un objeto (ver nombreSimpleTipo).
std::string nombreSimpleTipo(const std::type_info& tipo);

#endif // TYPEUTILS_H