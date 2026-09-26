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
#ifndef RUNTIMEEXCEPTION_H
#define RUNTIMEEXCEPTION_H

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#endif

#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

class RuntimeException : public std::runtime_error {
private:
    std::vector<std::string> stackTrace;

    // Declaraci�n correcta de los m�todos privados
    std::vector<std::string> generateStackTrace();
    std::string formatStackTrace();

public:
    explicit RuntimeException(const string& msg);
    std::string getStackTrace() const;
    virtual ~RuntimeException() = default;
};

#endif