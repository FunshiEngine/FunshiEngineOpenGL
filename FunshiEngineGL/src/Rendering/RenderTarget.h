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
#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "Backend/IRenderBackend.h"

// Render-to-texture para las vistas previas de camara: wrapper RAII de un
// handle opaco del backend (FBO en OpenGL, pas-swapchain en Vulkan). No ve GL;
// la creacion del FBO y la carga de funciones (glfwGetProcAddress) vive en
// OpenGL3Backend, asi funciona igual en Linux y Windows sin GLAD/glew externo.
class RenderTarget {
private:
    // Backend con el que se crean y destruyen los recursos: se inyecta, no se
    // busca en un singleton.
    Rendering::Backend::IRenderBackend* backend_ = nullptr;
    Rendering::Backend::Handle target_ = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle colorTex_ = Rendering::Backend::kInvalidHandle;
    int width = 0;
    int height = 0;

    void destroy();

public:
    explicit RenderTarget(Rendering::Backend::IRenderBackend& backend) noexcept
        : backend_(&backend) {}
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    void resize(int w, int h);
    void bind();
    void unbind();

    Rendering::Backend::Handle getColorTexture() const { return colorTex_; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};
#endif