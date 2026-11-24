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
// Render target (FBO texturizado)
// ---------------------------------------------------------------------------

Handle OpenGL3Backend::createRenderTarget(int width, int height) {
    if (width <= 0 || height <= 0) return kInvalidHandle;
    if (!fboFuncionesCargadas()) {
        std::cerr << "[RenderTarget] FBO no disponible en este GPU.\n";
        return kInvalidHandle;
    }

    GLuint fbo = 0, rbo = 0, colorTex = 0;
    pfnGenFramebuffers(1, &fbo);
    pfnGenRenderbuffers(1, &rbo);
    glGenTextures(1, &colorTex);

    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    pfnBindRenderbuffer(GL_RENDERBUFFER, rbo);
    pfnRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    pfnBindRenderbuffer(GL_RENDERBUFFER, 0);

    pfnBindFramebuffer(GL_FRAMEBUFFER, fbo);
    pfnFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            colorTex, 0);
    pfnFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_RENDERBUFFER, rbo);

    const GLenum estado = pfnCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (estado != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[RenderTarget] Framebuffer incompleto: " << estado
                  << "\n";
    }
    pfnBindFramebuffer(GL_FRAMEBUFFER, 0);

    const Handle handle = static_cast<Handle>(fbo);
    targets_[handle] = GpuTarget{fbo, rbo, colorTex};
    return handle;
}

void OpenGL3Backend::destroyRenderTarget(Handle target) {
    if (target == kInvalidHandle) return;
    const auto it = targets_.find(target);
    if (it == targets_.end()) return;

    // Copiar valores ANTES de borrar: el erase destruye el GpuTarget
    // y la referencia 't' quedaría colgando (use-after-free).
    const GLuint fbo = it->second.fbo;
    const GLuint rbo = it->second.rbo;
    const GLuint colorTex = it->second.colorTex;

    targets_.erase(it);

    if (pfnDeleteFramebuffers) {
        pfnDeleteFramebuffers(1, &fbo);
    }
    if (pfnDeleteRenderbuffers) {
        pfnDeleteRenderbuffers(1, &rbo);
    }
    if (colorTex) {
        glDeleteTextures(1, &colorTex);
    }
}

void OpenGL3Backend::bindRenderTarget(Handle target) {
    if (target == kInvalidHandle || !pfnBindFramebuffer ||
        targets_.find(target) == targets_.end())
        return;
    pfnBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(target));
    // En contextos legacy el buffer destino del FBO puede quedar en GL_BACK y
    // entonces todos los draws se descartan (textura negra). Se fuerza el
    // attachment de color del FBO para que el render llegue a la textura.
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
}

void OpenGL3Backend::bindDefaultFramebuffer() {
    if (pfnBindFramebuffer) pfnBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Handle OpenGL3Backend::renderTargetColorTexture(Handle target) const {
    const auto it = targets_.find(target);
    if (it == targets_.end()) return kInvalidHandle;
    return static_cast<Handle>(it->second.colorTex);
}

void* OpenGL3Backend::imguiTextureId(Handle texture) const {
    if (texture == kInvalidHandle) return nullptr;
    // El handle es el GLuint en opaco; ImGui lo trata como ImTextureID (void*).
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(texture));
}

// ---------------------------------------------------------------------------
// Objetivo de profundidad (mapa de sombras)
// ---------------------------------------------------------------------------

Handle OpenGL3Backend::createDepthTarget(int size) {
    if (size <= 0) return kInvalidHandle;
    if (!fboFuncionesCargadas()) {
        std::cerr << "[Sombras] FBO no disponible en este GPU.\n";
        return kInvalidHandle;
    }

    GLuint fbo = 0, depthTex = 0;
    pfnGenFramebuffers(1, &fbo);
    glGenTextures(1, &depthTex);

    glBindTexture(GL_TEXTURE_2D, depthTex);
    // El modo de comparacion es lo que hace que la textura se pueda leer con
    // un sampler2DShadow: el hardware compara la profundidad del fragmento con
    // la del mapa y devuelve el porcentaje de cobertura (y con el filtrado
    // lineal hace el PCF de 2x2 solo).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                    GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Fuera del mapa no hay sombra: con el borde a 1 (el plano lejano) la
    // comparacion da "iluminado" y la sombra no se repite en el borde.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borde[4] = {1.f, 1.f, 1.f, 1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borde);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    pfnBindFramebuffer(GL_FRAMEBUFFER, fbo);
    pfnFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                            depthTex, 0);
    // El mapa de sombras no tiene color: sin esto, algunos drivers escriben en
    // el attachment 0 y el resultado es indefinido.
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum estado = pfnCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (estado != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Sombras] Framebuffer de profundidad incompleto: " << estado
                  << "\n";
        pfnBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (pfnDeleteFramebuffers) pfnDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &depthTex);
        return kInvalidHandle;
    }
    pfnBindFramebuffer(GL_FRAMEBUFFER, 0);

    const Handle handle = static_cast<Handle>(fbo);
    depthTargets_[handle] = GpuDepthTarget{fbo, depthTex, size};
    return handle;
}

Handle OpenGL3Backend::createDepthTargetCubo(int size) {
    if (size <= 0) return kInvalidHandle;
    if (!fboFuncionesCargadas()) {
        std::cerr << "[Sombras] FBO no disponible en este GPU.\n";
        return kInvalidHandle;
    }

    GLuint fbo = 0, cuboTex = 0;
    pfnGenFramebuffers(1, &fbo);
    glGenTextures(1, &cuboTex);

    glBindTexture(GL_TEXTURE_CUBE_MAP, cuboTex);
    for (int cara = 0; cara < 6; ++cara) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cara, 0,
                     GL_DEPTH_COMPONENT24, size, size, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    }
    // El modo de comparacion es lo que permite leer la textura con
    // samplerCubeShadow: el hardware compara la profundidad del fragmento con
    // la del mapa y devuelve el porcentaje de cobertura.
    // Sin modo de comparacion: el shader usa samplerCube y compara manualmente
    // la profundidad almacenada (normalized [0,1]) contra la distancia relativa.
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    pfnBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // Attach la cara +X para que el FBO sea completo desde el arranque; las
    // demas caras se attachan por cara en bindDepthTargetCuboFace.
    pfnFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_CUBE_MAP_POSITIVE_X, cuboTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum estado = pfnCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (estado != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Sombras] Framebuffer de profundidad cubemap incompleto: "
                  << estado << "\n";
        pfnBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (pfnDeleteFramebuffers) pfnDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &cuboTex);
        return kInvalidHandle;
    }
    pfnBindFramebuffer(GL_FRAMEBUFFER, 0);

    const Handle handle = static_cast<Handle>(fbo);
    depthCuboTargets_[handle] = GpuDepthCuboTarget{fbo, cuboTex, size};
    return handle;
}

void OpenGL3Backend::bindDepthTargetCuboFace(Handle fbo, int cara) {
    if (fbo == kInvalidHandle || !pfnBindFramebuffer) return;
    const auto it = depthCuboTargets_.find(static_cast<unsigned int>(fbo));
    if (it == depthCuboTargets_.end()) return;
    Handle cubemapTex = static_cast<Handle>(it->second.depthCuboTex);
    pfnBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(fbo));
    pfnFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_CUBE_MAP_POSITIVE_X + cara,
                            static_cast<GLuint>(cubemapTex), 0);
    glViewport(0, 0, it->second.size, it->second.size);
}

void OpenGL3Backend::destroyDepthTarget(Handle target) {
    if (target == kInvalidHandle) return;
    // Puede ser un target de profundidad 2D o un cubemap: buscamos en ambos
    // mapas.
    auto it2d = depthTargets_.find(target);
    auto itCubo = depthCuboTargets_.find(target);
    if (it2d == depthTargets_.end() && itCubo == depthCuboTargets_.end()) return;

    if (it2d != depthTargets_.end()) {
        const GLuint fbo = it2d->second.fbo;
        const GLuint depthTex = it2d->second.depthTex;
        depthTargets_.erase(it2d);
        if (pfnDeleteFramebuffers) pfnDeleteFramebuffers(1, &fbo);
        if (depthTex) glDeleteTextures(1, &depthTex);
    }
    if (itCubo != depthCuboTargets_.end()) {
        const GLuint fbo = itCubo->second.fbo;
        const GLuint cuboTex = itCubo->second.depthCuboTex;
        depthCuboTargets_.erase(itCubo);
        if (pfnDeleteFramebuffers) pfnDeleteFramebuffers(1, &fbo);
        if (cuboTex) glDeleteTextures(1, &cuboTex);
    }
}

void OpenGL3Backend::bindDepthTarget(Handle target) {
    if (target == kInvalidHandle || !pfnBindFramebuffer) return;
    const auto it = depthTargets_.find(target);
    if (it == depthTargets_.end()) return;
    pfnBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(target));
    // El mapa de sombras es cuadrado: el viewport tiene que ser el suyo, no el
    // de la pantalla.
    glViewport(0, 0, it->second.size, it->second.size);
}

Handle OpenGL3Backend::depthTargetTexture(Handle target) const {
    auto it2d = depthTargets_.find(target);
    if (it2d != depthTargets_.end()) return static_cast<Handle>(it2d->second.depthTex);
    auto itCubo = depthCuboTargets_.find(target);
    if (itCubo != depthCuboTargets_.end()) return static_cast<Handle>(itCubo->second.depthCuboTex);
    return kInvalidHandle;
}

}  // namespace Rendering::Backend
