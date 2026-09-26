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

// Backend concreto sobre OpenGL (compatibilidad + GL 2/3 moderno cargado por
// puntero). Implementa IRenderBackend: los recursos GL vivos se traducen a los
// GLuint reales y las operaciones immediate maps al pipeline legacy. Posee el
// unico acceso a GLFuncs (funciones modernas) y a los FBO; nadie mas del engine
// llama a OpenGL directamente a traves de la capa de entidades.
class OpenGL3Backend : public IRenderBackend {
public:
    bool init() override;
    bool available() const override { return GLFuncs::available(); }

    // --- Malla GPU ---
    Handle createMesh(const MeshData& data) override;
    void destroyMesh(Handle mesh) override;
    void drawMesh(Handle mesh, unsigned int indexCount) override;

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

    // --- Estado inmediato / matrices ---
    void setViewport(int x, int y, int width, int height) override;
    void setCompatibilityMatrices(const float* projection,
                                  const float* view) override;
    void clearScreen(const float color[3]) override;
    void setLegacyLights(const LegacyLight* lights, int lightCount,
                         const float* globalAmbient) override;
    const char* diagnosticoCompat() const override;
    void applyBaseState() override;
    void setClearColor(const float color[3]) override;
    const char* diagnosticoGPU() const override;
    void pushMatrix() override;
    void popMatrix() override;
    void multMatrix(const float mat4[16]) override;
    void applyTransform(const float translate[3], const float scale[3],
                        const float rotate4[4]) override;
    void setLightingEnabled(bool enabled) override;
    void setPolygonFill() override;
    void setSolidColor(float r, float g, float b) override;
    void setMaterial(const float ambient[4], const float diffuse[4],
                     const float specular[4], const float emission[4],
                     float shininess) override;
    void setLineWidth(float width) override;
    void setLineSmoothing(bool enabled) override;

    // --- Primitivas ---
    void drawLinePairs(const float* vertices, int vertexCount) override;
    void drawLinePairsRGBA(const float* vertices, int vertexCount) override;
    void drawIndexedLines(const float* vertices, int vertexCount,
                          const int* edgeIndices, int edgeCount) override;
    void drawLineStrip(const float* vertices, int vertexCount,
                       bool closed) override;
    void drawTriangles(const float* vertices, int vertexCount,
                       const float* normals, int normalCount,
                       const unsigned int* indices, int indexCount) override;

private:
    struct GpuMesh { unsigned int vao; unsigned int buffers[6]; };
    struct GpuTarget {
        unsigned int fbo;
        unsigned int rbo;
        unsigned int colorTex;
    };

    bool fboFuncionesCargadas();
    std::unordered_map<Handle, GpuMesh> meshes_;
    std::unordered_map<Handle, GpuTarget> targets_;
    bool fboCargadas_ = false;
    // Estado de GL_BLEND antes de que setLineSmoothing(true) lo activara para
    // el suavizado de lineas; se restaura al volver a false.
    bool blendPreviaLineaSmooth_ = false;
};

} // namespace Backend
} // namespace Rendering

#endif