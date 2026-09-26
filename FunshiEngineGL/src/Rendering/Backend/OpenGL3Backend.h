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
#ifndef OPENGL3BACKEND_H
#define OPENGL3BACKEND_H

#include <unordered_map>

#include "GLFuncs.h"
#include "IRenderBackend.h"

namespace Rendering {
namespace Backend {

// Backend concreto sobre OpenGL 3.3 core (todo lo moderno se carga por puntero
// con GLFuncs). Implementa IRenderBackend: los recursos GL vivos se traducen a los
// GLuint reales. Ya no expone superficie fixed-function: no hay matrices de
// compatibilidad, ni stack de matrices, ni materiales, ni primitivas
// inmediatas. Posee el unico acceso a GLFuncs (funciones modernas) y a los FBO;
// nadie mas del engine llama a OpenGL directamente a traves de la capa de
// entidades.
class OpenGL3Backend : public IRenderBackend {
public:
    bool init() override;
    bool available() const override { return GLFuncs::available(); }

    // --- Malla GPU ---
    Handle createMesh(const MeshData& data) override;
    void destroyMesh(Handle mesh) override;
    void drawMesh(Handle mesh, unsigned int indexCount) override;

    // --- Batch de lineas ---
    Handle createLineBatch(const float* vertices,
                           std::size_t vertexCount) override;
    void destroyLineBatch(Handle batch) override;
    void updateLineBatch(Handle batch, const float* vertices,
                         std::size_t vertexCount) override;
    void drawLineBatch(Handle batch, unsigned int vertexCount) override;

    // --- Textura 2D ---
    Handle createTexture2D(const Image2D& image) override;
    void destroyTexture2D(Handle texture) override;
    void bindTexture2D(Handle texture, int unit) override;

    // --- Programa ---
    Handle createProgram(const char* vertexSource,
                         const char* fragmentSource) override;
    void destroyProgram(Handle program) override;
    void useProgram(Handle program) override;
    int uniformLocation(Handle program, const char* name) override;
    void setUniformMat4(int location, const glm::mat4& value) override;
    void setUniformMat3(int location, const glm::mat3& value) override;
    void setUniformVec3(int location, const glm::vec3& value) override;
    void setUniformVec4(int location, const float* value) override;
    void setUniformFloat(int location, float value) override;
    void setUniformInt(int location, int value) override;

    // --- Render target ---
    Handle createRenderTarget(int width, int height) override;
    void destroyRenderTarget(Handle target) override;
    void bindRenderTarget(Handle target) override;
    void bindDefaultFramebuffer() override;
    Handle renderTargetColorTexture(Handle target) const override;
    void* imguiTextureId(Handle texture) const override;

    // --- Estado de la pasada y del framebuffer ---
    void setViewport(int x, int y, int width, int height) override;
    void clearScreen(const float color[3]) override;
    void applyBaseState() override;
    void setClearColor(const float color[3]) override;
    const char* diagnosticoGPU() const override;
    void setBlendEnabled(bool enabled) override;

private:
    struct GpuMesh { unsigned int vao; unsigned int buffers[6]; };
    // El batch de lineas es un unico VBO interleaved (el layout lo fija
    // LineBuilder): VAO + el VBO, sin EBO porque la expansion a quads no
    // comparte vertices.
    struct GpuLineBatch { unsigned int vao; unsigned int vbo; };
    struct GpuTarget {
        unsigned int fbo;
        unsigned int rbo;
        unsigned int colorTex;
    };

    bool fboFuncionesCargadas();
    std::unordered_map<Handle, GpuMesh> meshes_;
    std::unordered_map<Handle, GpuLineBatch> lineBatches_;
    std::unordered_map<Handle, GpuTarget> targets_;
    bool fboCargadas_ = false;
};

} // namespace Backend
} // namespace Rendering

#endif
