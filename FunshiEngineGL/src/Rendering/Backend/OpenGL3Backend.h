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
#include <utility>

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
    void drawMesh(Handle mesh, unsigned int firstIndex,
                  unsigned int indexCount) override;

    // --- Batch de lineas ---
    Handle createLineBatch(const float* vertices,
                           std::size_t vertexCount) override;
    void destroyLineBatch(Handle batch) override;
    void updateLineBatch(Handle batch, const float* vertices,
                         std::size_t vertexCount) override;
    void drawLineBatch(Handle batch, unsigned int vertexCount) override;
    Handle createParticulasBatch(const float* vertices,
                                 std::size_t vertexCount) override;
    void destroyParticulasBatch(Handle batch) override;
    void updateParticulasBatch(Handle batch, const float* vertices,
                               std::size_t vertexCount) override;
    void drawParticulasBatch(Handle batch, unsigned int vertexCount) override;

    // --- Textura 2D ---
    Handle createTexture2D(const Image2D& image) override;
    void destroyTexture2D(Handle texture) override;
    void bindTexture2D(Handle texture, int unit) override;
    void updateTexture2D(Handle texture, const Image2D& image) override;

    // --- Textura Cubemap ---
    Handle createTextureCube(const ImageCube& image) override;
    void destroyTextureCube(Handle texture) override;
    void bindTextureCube(Handle texture, int unit) override;

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

    // --- Objetivo de profundidad (mapa de sombras) ---
    Handle createDepthTarget(int size) override;
    Handle createDepthTargetCubo(int size) override;
    void bindDepthTargetCuboFace(Handle fbo, int cara, Handle cubemapTex) override;
    void destroyDepthTarget(Handle target) override;
    void bindDepthTarget(Handle target) override;
    Handle depthTargetTexture(Handle target) const override;

    // --- Estado de la pasada y del framebuffer ---
    void setViewport(int x, int y, int width, int height) override;
    void clearScreen(const float color[3]) override;
    void applyBaseState() override;
    void setClearColor(const float color[3]) override;
    const char* diagnosticoGPU() const override;
    void setBlendEnabled(bool enabled) override;
    void setBlendMode(BlendMode mode) override;

    // --- Estado de profundidad (para el cielo: depth test on + depth mask off)
    void setDepthTestEnabled(bool enabled) override;
    void setDepthMask(bool enabled) override;
    void setDepthFunc(DepthFunc func) override;

    // --- Pasada a pantalla completa (sin atributos) ---
    void drawFullscreenTriangle() override;

private:
    struct GpuMesh { unsigned int vao; unsigned int buffers[6]; };
    // El batch de lineas es un unico VBO interleaved (el layout lo fija
    // LineBuilder): VAO + el VBO, sin EBO porque la expansion a quads no
    // comparte vertices.
    struct GpuLineBatch { unsigned int vao; unsigned int vbo; };
    // El batch de particulas es, igual que el de lineas, un unico VBO
    // interleaved (el layout lo fija ParticulasBuilder): VAO + el VBO, sin EBO
    // porque cada particula es un quad de dos triangulos sin vertices
    // compartidos.
    struct GpuParticulasBatch { unsigned int vao; unsigned int vbo; };
    struct GpuTarget {
        unsigned int fbo;
        unsigned int rbo;
        unsigned int colorTex;
    };
    // Mapa de sombras: FBO con una textura de profundidad (sin color) y el
    // lado del mapa (es cuadrado).
    struct GpuDepthTarget {
        unsigned int fbo;
        unsigned int depthTex;
        int size;
    };

    // Mapa de sombras de cubemap (luz puntual): un FBO compartido por las 6
    // caras y una textura de profundidad de cubemap. Antes de dibujar cada cara
    // se reconfigura el attachment de profundidad con bindDepthTargetCuboFace.
    struct GpuDepthCuboTarget {
        unsigned int fbo;
        unsigned int depthCuboTex;
        int size;
    };

    bool fboFuncionesCargadas();
    std::unordered_map<Handle, GpuMesh> meshes_;
    std::unordered_map<Handle, GpuLineBatch> lineBatches_;
    std::unordered_map<Handle, GpuParticulasBatch> particulasBatches_;
    std::unordered_map<Handle, GpuTarget> targets_;
    std::unordered_map<Handle, GpuDepthTarget> depthTargets_;
    std::unordered_map<Handle, GpuDepthCuboTarget> depthCuboTargets_;
    // Dimensiones vigentes de cada textura 2D creada: updateTexture2D las
    // consulta para decidir entre glTexSubImage2D (mismo tamano) y reasignar
    // (tamano distinto). Sin este registro no se puede saber el tamano actual
    // sin preguntarselo a GL.
    std::unordered_map<Handle, std::pair<int, int>> tamanosTextura2D_;
    bool fboCargadas_ = false;
    // Modo de mezcla vigente: lo recuerda el backend para que al volver a
    // habilitar la mezcla siga siendo el que eligio el consumidor.
    BlendMode modoBlend_ = BlendMode::Alpha;
    // Traduce el modo de mezcla a la funcion de GL que corresponde.
    void aplicarBlend();
    // VAO sin atributos de la pasada a pantalla completa (cielo degradado): se
    // crea en el primer dibujo y se reusa. Vive hasta que termina el proceso,
    // igual que el resto de los recursos GPU del backend (no hay apagado de GL
    // con contexto vivo despues del bucle principal).
    unsigned int vaoPantallaCompleta_ = 0;
};

} // namespace Backend
} // namespace Rendering

#endif
