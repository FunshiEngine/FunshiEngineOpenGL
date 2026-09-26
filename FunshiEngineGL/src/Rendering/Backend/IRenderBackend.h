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
#ifndef IRENDERBACKEND_H
#define IRENDERBACKEND_H

#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>

namespace Rendering {
namespace Backend {

// Handle opaco a un recurso GPU (0 = vacio/null). El engine trabaja a traves
// de la interfaz; el backend concreto (OpenGL3Backend hoy) traduce cada handle
// a su objeto nativo (GLuint, ...). La interfaz no filtra detalles de la API.
using Handle = std::uint64_t;
constexpr Handle kInvalidHandle = 0;

// Geometria CPU a subir a GPU (misma informacion que los parametros posicionales
// de MeshGPU::upload, pero agrupada para la interfaz).
struct MeshData {
    const float* vertices = nullptr;
    std::size_t vertexCount = 0;
    const float* normals = nullptr;
    std::size_t normalCount = 0;
    const float* uvs = nullptr;
    std::size_t uvCount = 0;
    const float* tangents = nullptr;
    std::size_t tangentCount = 0;
    const float* bitangents = nullptr;
    std::size_t bitangentCount = 0;
    const unsigned int* indices = nullptr;
    std::size_t indexCount = 0;
};

// Pixels RGBA8 sin firmar de una imagen CPU (para subir texturas 2D).
struct Image2D {
    int width = 0;
    int height = 0;
    const unsigned char* pixels = nullptr;
    // Genera la cadena de mipmaps (muestreo trilinear al minificar). Si el
    // backend no puede generarlos cae a lineal (solo el nivel base).
    bool generateMipmaps = false;
};

// Contrato de backend grafico. Los recursos se crean/destruyen por handle y
// las operaciones de dibujado son el pipeline moderno completo: no queda
// superficie fixed-function (ni glBegin/glEnd, ni stack de matrices, ni
// GL_LIGHT*). Los consumidores de la capa de entidades nunca ven GL: todo pasa
// por aca (o por sus wrappers RAII).
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Carga las funciones de GPU (por puntero, "same fashion as GLFW"). Devuelve
    // false si el pipeline moderno no esta disponible.
    virtual bool init() = 0;
    virtual bool available() const = 0;

    // --- Malla GPU (VAO/VBO/EBO o su analogo nativo) -------------------------
    virtual Handle createMesh(const MeshData& data) = 0;
    virtual void destroyMesh(Handle mesh) = 0;
    virtual void drawMesh(Handle mesh, unsigned int indexCount) = 0;

    // --- Batch de lineas (VAO/VBO de la geometria expandida por LineBuilder) --
    // Los vertices vienen en el layout de LineBuilder (12 floats: inicio, fin,
    // lado, avance, rgba) y se dibujan como triangulos con glDrawArrays (la
    // expansion a quads es lo que reemplaza a GL_LINES/glLineWidth). update
    // vuelve a subir el buffer del mismo recurso, que es el caso por frame de la
    // grilla y los marcadores.
    virtual Handle createLineBatch(const float* vertices,
                                   std::size_t vertexCount) = 0;
    virtual void destroyLineBatch(Handle batch) = 0;
    virtual void updateLineBatch(Handle batch, const float* vertices,
                                 std::size_t vertexCount) = 0;
    virtual void drawLineBatch(Handle batch, unsigned int vertexCount) = 0;

    // --- Textura 2D ----------------------------------------------------------
    virtual Handle createTexture2D(const Image2D& image) = 0;
    virtual void destroyTexture2D(Handle texture) = 0;
    // Bindeo del sampleo en la unidad indicada (GL_TEXTURE0 + unit).
    virtual void bindTexture2D(Handle texture, int unit) = 0;

    // --- Programa de shaders (pipeline moderno) ------------------------------
    // createProgram puede lanzar (falla de compilacion/limite): la excepcion
    // es especifica del backend (ShaderCompile/Link/Unavailable) pero el
    // contrato no impone un tipo.
    virtual Handle createProgram(const char* vertexSource,
                                 const char* fragmentSource) = 0;
    virtual void destroyProgram(Handle program) = 0;
    virtual void useProgram(Handle program) = 0;
    virtual int uniformLocation(Handle program, const char* name) = 0;
    virtual void setUniformMat4(int location, const glm::mat4& value) = 0;
    virtual void setUniformMat3(int location, const glm::mat3& value) = 0;
    virtual void setUniformVec3(int location, const glm::vec3& value) = 0;
    virtual void setUniformVec4(int location, const float* value) = 0;
    virtual void setUniformFloat(int location, float value) = 0;
    virtual void setUniformInt(int location, int value) = 0;

    // --- Render target (FBO texturizado para viewports) ----------------------
    virtual Handle createRenderTarget(int width, int height) = 0;
    virtual void destroyRenderTarget(Handle target) = 0;
    virtual void bindRenderTarget(Handle target) = 0;
    virtual void bindDefaultFramebuffer() = 0;
    // Handle de la textura de color del target (para mostrarla como ImTextureID).
    virtual Handle renderTargetColorTexture(Handle target) const = 0;
    // Handle como descriptor opaco para la GUI (ImGui::Image): en GL vuelve el
    // GLuint como puntero. La GUI nunca ve el valor concreto, solo lo reenvia
    // como ImTextureID.
    virtual void* imguiTextureId(Handle texture) const = 0;

    // --- Estado de la pasada y del framebuffer -------------------------------
    // Viewport del framebuffer actual (glViewport): lo necesitan la pasada
    // principal y las vistas previas (cada una con su propio encuadre).
    virtual void setViewport(int x, int y, int width, int height) = 0;
    // Limpia el framebuffer actual con el fondo dado (color + profundidad).
    virtual void clearScreen(const float color[3]) = 0;
    // Estado base de un contexto recien creado: depth test y multisample. Se
    // llama una vez al arrancar, despues de crear el contexto y antes del bucle.
    virtual void applyBaseState() = 0;
    // Color de limpieza del framebuffer. clearScreen(null) limpia con este
    // color sin tocarlo.
    virtual void setClearColor(const float color[3]) = 0;
    // Info del GPU/contexto (renderer, version de GL y GLSL, perfil) como
    // texto para los logs de arranque de la ventana.
    virtual const char* diagnosticoGPU() const = 0;
    // Blend del framebuffer (GL_BLEND). Lo necesitan las lineas con alpha por
    // vertice del pipeline moderno (el difuminado de la grilla se funde con el
    // fondo). No se confunde con el suavizado de lineas: glLineSmooth no existe
    // en un perfil core y el ancho ya no se toma de glLineWidth.
    virtual void setBlendEnabled(bool enabled) = 0;
};

// Backend activo del engine (un solo contexto GL; el singleton se cambia en la
// migracion a Vulkan). El concreto vive en OpenGL3Backend.cpp.
IRenderBackend& activeBackend();

} // namespace Backend
} // namespace Rendering

#endif
