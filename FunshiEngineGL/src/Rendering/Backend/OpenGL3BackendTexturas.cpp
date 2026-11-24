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
#include "OpenGL3BackendDetalle.h"

namespace Rendering::Backend {

// ---------------------------------------------------------------------------
// Textura 2D
// ---------------------------------------------------------------------------

Handle OpenGL3Backend::createTexture2D(const Image2D& image) {
    if (!image.pixels || image.width <= 0 || image.height <= 0)
        return kInvalidHandle;

    // Los mipmaps son GL 3.0+; se cargan por puntero como los FBO y solo se
    // usan si llegan. Si no, se cae al nivel base con filtrado lineal.
    const bool conMipmaps =
        image.generateMipmaps && (cargar("glGenerateMipmap", pfnGenerateMipmap),
                                  pfnGenerateMipmap != nullptr);

    // glGenTextures/glBindTexture/glTexParameteri/glTexImage2D son GL 1.1,
    // estan en gl.h de cualquier plataforma (a diferencia de glActiveTexture).
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    conMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    conMipmaps ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    conMipmaps ? GL_CLAMP_TO_EDGE : GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);

    if (conMipmaps) pfnGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    tamanosTextura2D_[static_cast<Handle>(texture)] = {image.width, image.height};
    return static_cast<Handle>(texture);
}

void OpenGL3Backend::destroyTexture2D(Handle texture) {
    if (texture == kInvalidHandle) return;
    tamanosTextura2D_.erase(texture);
    GLuint id = static_cast<GLuint>(texture);
    glDeleteTextures(1, &id);
}

void OpenGL3Backend::bindTexture2D(Handle texture, int unit) {
    if (!GLFuncs::pfnActiveTexture) return;
    GLFuncs::pfnActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, texture != kInvalidHandle ? static_cast<GLuint>(texture) : 0);
}

void OpenGL3Backend::updateTexture2D(Handle texture, const Image2D& image) {
    if (texture == kInvalidHandle || !image.pixels || image.width <= 0 ||
        image.height <= 0)
        return;

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Mismo tamano: se reescribe la region completa con glTexSubImage2D, que no
    // reasigna el almacenamiento (el caso por frame de una vista offscreen). Si
    // el tamano cambio (p. ej. resize de la ventana), se reasigna con
    // glTexImage2D y se actualiza el registro.
    auto it = tamanosTextura2D_.find(texture);
    const bool mismoTamano =
        it != tamanosTextura2D_.end() && it->second.first == image.width &&
        it->second.second == image.height;
    if (mismoTamano) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width, image.height,
                        GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
        tamanosTextura2D_[texture] = {image.width, image.height};
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

// --- Textura Cubemap ---------------------------------------------------------

Handle OpenGL3Backend::createTextureCube(const ImageCube& image) {
    if (image.width <= 0 || image.height <= 0) return kInvalidHandle;
    for (int i = 0; i < 6; ++i) {
        if (!image.faces[i]) return kInvalidHandle;
    }

    const bool conMipmaps =
        image.generateMipmaps && (cargar("glGenerateMipmap", pfnGenerateMipmap),
                                  pfnGenerateMipmap != nullptr);

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    conMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Orden de caras: +X, -X, +Y, -Y, +Z, -Z
    static const GLenum cubeFaces[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    for (int i = 0; i < 6; ++i) {
        glTexImage2D(cubeFaces[i], 0, GL_RGBA8, image.width, image.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, image.faces[i]);
    }

    if (conMipmaps) pfnGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return static_cast<Handle>(texture);
}

void OpenGL3Backend::destroyTextureCube(Handle texture) {
    if (texture == kInvalidHandle) return;
    GLuint id = static_cast<GLuint>(texture);
    glDeleteTextures(1, &id);
}

void OpenGL3Backend::bindTextureCube(Handle texture, int unit) {
    if (!GLFuncs::pfnActiveTexture) return;
    GLFuncs::pfnActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture != kInvalidHandle ? static_cast<GLuint>(texture) : 0);
}

// ---------------------------------------------------------------------------
// Programa de shaders
// ---------------------------------------------------------------------------

Handle OpenGL3Backend::createProgram(const char* vertexSource,
                                     const char* fragmentSource) {
    if (!GLFuncs::available()) throw ShaderUnavailableException();

    const GLuint vs = compilarEtapa(GL_VERTEX_SHADER, vertexSource);
    const GLuint fs = compilarEtapa(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = GLFuncs::pfnCreateProgram();
    if (!program) {
        GLFuncs::pfnDeleteShader(vs);
        GLFuncs::pfnDeleteShader(fs);
        throw ShaderUnavailableException();
    }

    GLFuncs::pfnAttachShader(program, vs);
    GLFuncs::pfnAttachShader(program, fs);
    GLFuncs::pfnLinkProgram(program);

    GLFuncs::pfnDeleteShader(vs);
    GLFuncs::pfnDeleteShader(fs);

    GLint estado = GL_FALSE;
    GLFuncs::pfnGetProgramiv(program, GL_LINK_STATUS, &estado);
    if (estado == GL_FALSE) {
        const std::string log = infoLog(program, GLFuncs::pfnGetProgramiv,
                                        GLFuncs::pfnGetProgramInfoLog);
        GLFuncs::pfnDeleteProgram(program);
        throw ShaderLinkException(log);
    }
    return static_cast<Handle>(program);
}

void OpenGL3Backend::destroyProgram(Handle program) {
    if (program != kInvalidHandle && GLFuncs::pfnDeleteProgram)
        GLFuncs::pfnDeleteProgram(static_cast<GLuint>(program));
}

void OpenGL3Backend::useProgram(Handle program) {
    if (GLFuncs::pfnUseProgram)
        GLFuncs::pfnUseProgram(static_cast<GLuint>(program));
}

int OpenGL3Backend::uniformLocation(Handle program, const char* name) {
    if (program == kInvalidHandle || !GLFuncs::pfnGetUniformLocation) return -1;
    return GLFuncs::pfnGetUniformLocation(static_cast<GLuint>(program), name);
}

void OpenGL3Backend::setUniformMat4(int location, const glm::mat4& value) {
    if (location < 0) return;
    GLFuncs::pfnUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGL3Backend::setUniformMat3(int location, const glm::mat3& value) {
    if (location < 0) return;
    GLFuncs::pfnUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGL3Backend::setUniformVec3(int location, const glm::vec3& value) {
    if (location < 0) return;
    GLFuncs::pfnUniform3fv(location, 1, glm::value_ptr(value));
}

void OpenGL3Backend::setUniformVec4(int location, const float* value) {
    if (location < 0 || !value) return;
    GLFuncs::pfnUniform4fv(location, 1, value);
}

void OpenGL3Backend::setUniformFloat(int location, float value) {
    if (location < 0) return;
    GLFuncs::pfnUniform1f(location, value);
}

void OpenGL3Backend::setUniformInt(int location, int value) {
    if (location < 0) return;
    GLFuncs::pfnUniform1i(location, value);
}

}  // namespace Rendering::Backend
