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
// Estado de la pasada y del framebuffer
// ---------------------------------------------------------------------------

void OpenGL3Backend::setViewport(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) return;
    glViewport(x, y, width, height);
}

void OpenGL3Backend::clearScreen(const float color[3]) {
    if (color) glClearColor(color[0], color[1], color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGL3Backend::applyBaseState() {
    // Estado del contexto recien creado: solo lo que el pipeline moderno
    // necesita (profundidad y multisample del framebuffer de la ventana).
    // La iluminacion viaja como uniforms y la transformacion como matriz
    // modelo, asi que no hay estado fijo que dejar activo.
    glEnable(GL_DEPTH_TEST);
    setDepthFunc(kDepthFuncLess);
    // Activacion explicita del framebuffer multisampleado (se pidio 4x en la
    // creacion del contexto): suaviza los bordes de la geometria, incluidas las
    // lineas expandidas a quads, sin depender de glLineSmooth (que no existe en
    // un perfil core).
    glEnable(GL_MULTISAMPLE);
}

void OpenGL3Backend::setClearColor(const float color[3]) {
    glClearColor(color ? color[0] : 0.f, color ? color[1] : 0.f,
                 color ? color[2] : 0.f, 1.0f);
}

const char* OpenGL3Backend::diagnosticoGPU() const {
#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B30
#endif
#ifndef GL_CONTEXT_PROFILE_MASK
#define GL_CONTEXT_PROFILE_MASK 0x9126
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif
    static thread_local char buffer[576];
    GLint perfil = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &perfil);
    std::snprintf(
        buffer, sizeof(buffer),
        "[GPU] %s\n[GL_VERSION] %s\n[GLSL] %s\n[Perfil GL] %d (1=core, "
        "2=compatibilidad, 0=desconocido/legacy)",
        glGetString(GL_RENDERER)
            ? reinterpret_cast<const char*>(glGetString(GL_RENDERER))
            : "(no disponible)",
        glGetString(GL_VERSION)
            ? reinterpret_cast<const char*>(glGetString(GL_VERSION))
            : "(no disponible)",
        glGetString(GL_SHADING_LANGUAGE_VERSION)
            ? reinterpret_cast<const char*>(
                  glGetString(GL_SHADING_LANGUAGE_VERSION))
            : "(no disponible)",
        static_cast<int>(perfil));
    return buffer;
}

void OpenGL3Backend::setBlendEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_BLEND);
        aplicarBlend();
    } else {
        glDisable(GL_BLEND);
    }
}

void OpenGL3Backend::setBlendMode(BlendMode mode) {
    modoBlend_ = mode;
    aplicarBlend();
}

void OpenGL3Backend::aplicarBlend() {
    // glBlendFunc es GL 1.1: esta en el gl.h de Windows y de Linux, asi que
    // no hace falta cargarla por puntero como las funciones del perfil core.
    if (modoBlend_ == BlendMode::Additive) {
        // Sumar el color: el fuego y las chispas brillan sobre el fondo.
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void OpenGL3Backend::setDepthTestEnabled(bool enabled) {
    if (enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
}

void OpenGL3Backend::setDepthMask(bool enabled) {
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void OpenGL3Backend::setDepthFunc(DepthFunc func) {
    glDepthFunc(func == DepthFunc::LessEqual ? GL_LEQUAL : GL_LESS);
}

// ---------------------------------------------------------------------------
// Pasada a pantalla completa (sin atributos)
// ---------------------------------------------------------------------------

void OpenGL3Backend::drawFullscreenTriangle() {
    // El VAO se crea una sola vez y queda vacio a proposito: el shader de la
    // pasada no declara atributos (arma los tres vertices con gl_VertexID), pero
    // un contexto core exige un VAO ligado para CUALQUIER dibujo. Sin el, el
    // glDrawArrays se rechaza con GL_INVALID_OPERATION y no dibuja nada, que es
    // lo que dejaba el viewport con el color de limpieza del framebuffer.
    if (vaoPantallaCompleta_ == 0) {
        if (!GLFuncs::pfnGenVertexArrays) return;
        GLFuncs::pfnGenVertexArrays(1, &vaoPantallaCompleta_);
        if (vaoPantallaCompleta_ == 0) return;
    }
    GLFuncs::pfnBindVertexArray(vaoPantallaCompleta_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    GLFuncs::pfnBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Backend activo (singleton del proceso con un solo contexto de OpenGL).
// ---------------------------------------------------------------------------

IRenderBackend& activeBackend() {
    static OpenGL3Backend backend;
    return backend;
}

}  // namespace Rendering::Backend
