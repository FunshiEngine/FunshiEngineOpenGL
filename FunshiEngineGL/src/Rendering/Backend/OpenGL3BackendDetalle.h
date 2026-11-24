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
// Preambulo compartido por las unidades de OpenGL3Backend.cpp (generado al dividirlo).
#ifndef OPENGL3BACKENDDETALLE_H
#define OPENGL3BACKENDDETALLE_H

#include "OpenGL3Backend.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include "../Lineas/LineBuilder.h"
#include "../../Particulas/ParticulasBuilder.h"
#include "../Shaders/ShaderException.h"

namespace Rendering::Backend {

// ---------------------------------------------------------------------------
// Carga de funciones de framebuffer (FBO) por puntero ("same fashion as GLFW").
// ---------------------------------------------------------------------------
namespace {

typedef void (GLAPIENTRY* FN_GenFramebuffers)(GLsizei, GLuint*);
typedef void (GLAPIENTRY* FN_DeleteFramebuffers)(GLsizei, const GLuint*);
typedef void (GLAPIENTRY* FN_BindFramebuffer)(GLenum, GLuint);
typedef void (GLAPIENTRY* FN_FramebufferTexture2D)(GLenum, GLenum, GLenum,
                                                   GLuint, GLint);
typedef void (GLAPIENTRY* FN_GenRenderbuffers)(GLsizei, GLuint*);
typedef void (GLAPIENTRY* FN_DeleteRenderbuffers)(GLsizei, const GLuint*);
typedef void (GLAPIENTRY* FN_BindRenderbuffer)(GLenum, GLuint);
typedef void (GLAPIENTRY* FN_RenderbufferStorage)(GLenum, GLenum, GLsizei,
                                                  GLsizei);
typedef void (GLAPIENTRY* FN_FramebufferRenderbuffer)(GLenum, GLenum, GLenum,
                                                      GLuint);
typedef GLenum (GLAPIENTRY* FN_CheckFramebufferStatus)(GLenum);

// Constantes de cubemap que pueden no estar definidas en headers GL modernos
// con carga por puntero. Valores de la especificación OpenGL 3.3 core.
#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_TEXTURE_WRAP_R 0x8072
#endif
typedef void (GLAPIENTRY* FN_GenerateMipmap)(GLenum);

FN_GenFramebuffers pfnGenFramebuffers = nullptr;
FN_DeleteFramebuffers pfnDeleteFramebuffers = nullptr;
FN_BindFramebuffer pfnBindFramebuffer = nullptr;
FN_FramebufferTexture2D pfnFramebufferTexture2D = nullptr;
FN_GenRenderbuffers pfnGenRenderbuffers = nullptr;
FN_DeleteRenderbuffers pfnDeleteRenderbuffers = nullptr;
FN_BindRenderbuffer pfnBindRenderbuffer = nullptr;
FN_RenderbufferStorage pfnRenderbufferStorage = nullptr;
FN_FramebufferRenderbuffer pfnFramebufferRenderbuffer = nullptr;
FN_CheckFramebufferStatus pfnCheckFramebufferStatus = nullptr;
FN_GenerateMipmap pfnGenerateMipmap = nullptr;

template <typename T>
void cargar(const char* nombre, T& destino) {
    if (!destino) destino = reinterpret_cast<T>(glfwGetProcAddress(nombre));
}

// Sube un buffer con N floats y lo enlaza como atributo si hay datos.
bool subirBuffer(GLenum target, GLuint buffer, const float* data,
                 std::size_t count) {
    if (!data || count == 0) return false;
    GLFuncs::pfnBindBuffer(target, buffer);
    GLFuncs::pfnBufferData(
        target, static_cast<GLsizeiptr>(count * sizeof(float)), data,
        GL_STATIC_DRAW);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[OpenGL3Backend] subirBuffer error: " << err
                  << " target=" << target << " count=" << count << std::endl;
        return false;
    }
    return true;
}

// Lee el log del shader/programa sin reservar mas de lo que el driver reporta.
std::string infoLog(GLuint object, void (*getiv)(GLuint, GLenum, GLint*),
                    void (*getInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*)) {
    GLint length = 0;
    getiv(object, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) return {};
    std::string log(static_cast<std::size_t>(length), '\0');
    GLsizei written = 0;
    getInfoLog(object, length, &written, log.data());
    log.resize(static_cast<std::size_t>(written));
    return log;
}

GLuint compilarEtapa(GLenum stage, const char* source) {
    const GLuint id = GLFuncs::pfnCreateShader(stage);
    if (!id) throw ShaderUnavailableException();

    GLFuncs::pfnShaderSource(id, 1, &source, nullptr);
    GLFuncs::pfnCompileShader(id);

    GLint estado = GL_FALSE;
    GLFuncs::pfnGetShaderiv(id, GL_COMPILE_STATUS, &estado);
    if (estado == GL_FALSE) {
        const std::string log = infoLog(id, GLFuncs::pfnGetShaderiv,
                                        GLFuncs::pfnGetShaderInfoLog);
        GLFuncs::pfnDeleteShader(id);
        throw ShaderCompileException(
            stage == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
    }
    return id;
}

} // namespace

}  // namespace Rendering::Backend

#endif
