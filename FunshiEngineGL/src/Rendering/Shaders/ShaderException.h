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
#ifndef SHADEREXCEPTION_H
#define SHADEREXCEPTION_H

#include "../../ExcepcionesCPP/RuntimeException.h"

// Jerarquia de errores del pipeline de shaders. El patron es el mismo que en
// AssetException: una excepcion explicita (con stack trace) en lugar de
// "std::cerr + continuar con estado roto". Quien construye un ShaderProgram
// sabre por que fallo (stage + log del driver) si el driver no soporta GLSL.
class ShaderUnavailableException : public RuntimeException {
public:
    ShaderUnavailableException()
        : RuntimeException("OpenGL moderna no disponible: no se pudieron "
                           "cargar las funciones de shaders/VAO (segun "
                           "GLFuncs).", "ShaderUnavailableException") {}
};

class ShaderCompileException : public RuntimeException {
public:
    ShaderCompileException(const std::string& stage,
                           const std::string& infoLog)
        : RuntimeException("Error de compilacion del shader " + stage + ":\n" +
                           infoLog, "ShaderCompileException") {}
};

class ShaderLinkException : public RuntimeException {
public:
    ShaderLinkException(const std::string& infoLog)
        : RuntimeException("Error de linkeo del programa de shaders:\n" +
                           infoLog, "ShaderLinkException") {}
};

// Error al resolver un #include del fuente GLSL: el fragmento no existe, el
// include es ciclico o se supera el tope de anidamiento.
class ShaderIncludeException : public RuntimeException {
public:
    explicit ShaderIncludeException(const std::string& mensaje)
        : RuntimeException(mensaje, "ShaderIncludeException") {}
};

#endif
