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
#ifndef THROWABLE_H
#define THROWABLE_H

#include <stdexcept>
#include <string>

// Equivalente a Throwable en Java pero con runtime
class Throwable : public std::runtime_error {
protected:
    std::string message;
    std::string stackTrace;

public:
    // El mensaje se guarda tal cual lo entrega quien lanza la excepcion: la
    // clase base lo recibe y la anulacion de what() lo devuelve, de modo que
    // el texto nunca se pierde al atrapar la excepcion como std::exception.
    Throwable(const std::string& msg = "")
        : std::runtime_error(msg), message(msg) {
        // Aquí podrías capturar el stack trace (usando librerías como backward-cpp)
    }

    const char* what() const noexcept override {
        return message.c_str();
    }

    virtual const char* getStackTrace() const {
        return stackTrace.c_str();
    }

    virtual ~Throwable() = default;
};


#endif
