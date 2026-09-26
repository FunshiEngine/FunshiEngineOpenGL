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

// Luz del pipeline de compatibilidad (semantica GL_LIGHT0..GL_LIGHT7) lista
// para el backend. Es la contraparte API-agnostica de LightData (Iluminacion):
// el renderer de la escena convierte una en la otra antes de llamar aca, para
// que la interfaz no dependa de la capa de entidades.
struct LegacyLight {
    int type = 0;                          // 0 direccional, 1 punto, 2 spot.
    float worldPos[3] = {0.f, 0.f, 0.f};   // direccion para direccional.
    float direction[3] = {0.f, 0.f, -1.f}; // forward del objeto (spot/direccional).
    float ambient[3] = {0.f, 0.f, 0.f};
    float diffuse[3] = {1.f, 1.f, 1.f};
    float specular[3] = {1.f, 1.f, 1.f};
    float constant = 1.f;
    float linear = 0.f;
    float quadratic = 0.f;
    float spotCutoffDegrees = 45.f;
};

// Contrato de backend grafico. Los recursos se crean/destruyen por handle y
// las operaciones de dibujado son inmediatas o pequenas (malla indexada,
// primitivas de linea, triangulos del fallback legacy). Los consumidores de la
// capa de entidades nunca ven GL: todo pasa por aca (o por sus wrappers RAII).
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Carga las funciones de GPU (por puntero, "same fashion as GLFW"). Devuelve
    // false si el pipeline moderno no esta disponible (degradacion al inmediato).
    virtual bool init() = 0;
    virtual bool available() const = 0;

    // --- Malla GPU (VAO/VBO/EBO o su analogo nativo) -------------------------
    virtual Handle createMesh(const MeshData& data) = 0;
    virtual void destroyMesh(Handle mesh) = 0;
    virtual void drawMesh(Handle mesh, unsigned int indexCount) = 0;

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

    // --- Estado del pipeline de compatibilidad / stack de matrices -----------
    // Operaciones heredadas del modo inmediato (grilla, wireframes, fallback
    // legacy). El backend concreto las traduce a su API nativa.
    // Viewport del framebuffer actual (glViewport): lo necesitan la pasada
    // principal y las vistas previas (cada una con su propio encuadre).
    virtual void setViewport(int x, int y, int width, int height) = 0;
    // Carga las matrices con las que dibujan las pasadas de compatibilidad
    // (glMatrixMode + glLoadMatrixf en GL).
    virtual void setCompatibilityMatrices(const float* projection,
                                          const float* view) = 0;
    // Limpia el framebuffer actual con el fondo dado (color + profundidad).
    virtual void clearScreen(const float color[3]) = 0;
    // Luces legacy (GL_LIGHT0..7): habilita iluminacion, fija el modelo global
    // y parametriza los primeros lightCount slots (apaga el resto). Reemplaza
    // el GL que vivia en LightSystem::beginFrame.
    virtual void setLegacyLights(const LegacyLight* lights, int lightCount,
                                 const float* globalAmbient) = 0;
    // Instante del estado del pipeline de compatibilidad (iluminacion, luz 0 y
    // ultimo error grabado) como cadena corta para el diag del editor. El
    // buffer es interno del backend y vale solo hasta la siguiente llamada.
    virtual const char* diagnosticoCompat() const = 0;
    // Estado base de un contexto recien creado: depth test, normalizacion de
    // normales y seguimiento de color por material (glColorMaterial). Se llama
    // una vez al arrancar, despues de crear el contexto y antes del bucle.
    virtual void applyBaseState() = 0;
    // Color de limpieza del framebuffer. clearScreen(null) limpia con este
    // color sin tocarlo.
    virtual void setClearColor(const float color[3]) = 0;
    // Info del GPU/contexto (renderer, version de GL y GLSL, perfil) como
    // texto para los logs de arranque de la ventana.
    virtual const char* diagnosticoGPU() const = 0;
    virtual void pushMatrix() = 0;
    virtual void popMatrix() = 0;
    virtual void multMatrix(const float mat4[16]) = 0;
    // translate/scale/rotate en ese orden exacto (semantica del stack legacy).
    virtual void applyTransform(const float translate[3], const float scale[3],
                                const float rotate4[4]) = 0;
    virtual void setLightingEnabled(bool enabled) = 0;
    virtual void setPolygonFill() = 0;
    virtual void setSolidColor(float r, float g, float b) = 0;
    virtual void setMaterial(const float ambient[4], const float diffuse[4],
                             const float specular[4], const float emission[4],
                             float shininess) = 0;
    virtual void setLineWidth(float width) = 0;
    virtual void setLineSmoothing(bool enabled) = 0;

    // --- Primitivas inmediatas (en el espacio local del modelo actual) -------
    // vertices planos; cada par consecutivo [0..1], [2..3], ... es un segmento.
    virtual void drawLinePairs(const float* vertices, int vertexCount) = 0;
    // Pares de lineas (GL_LINES) con COLOR POR VERTICE: cada vertice son 7
    // floats (xyz + rgba). Permite difuminados por vertice (p.ej. la grilla que
    // se funde en el horizonte por distancia a la camara). El color actual de
    // glColor queda sobrescrito por el de cada vertice.
    virtual void drawLinePairsRGBA(const float* vertices, int vertexCount) = 0;
    // Lineas por indices: edges es un arreglo de edgeCount*2 ints.
    virtual void drawIndexedLines(const float* vertices, int vertexCount,
                                  const int* edgeIndices, int edgeCount) = 0;
    // Polilinea abierta (GL_LINE_STRIP) o cerrada (GL_LINE_LOOP).
    virtual void drawLineStrip(const float* vertices, int vertexCount,
                               bool closed) = 0;
    // Triangulos indexados con normales (fallback legacy de malla).
    virtual void drawTriangles(const float* vertices, int vertexCount,
                               const float* normals, int normalCount,
                               const unsigned int* indices, int indexCount) = 0;
};

// Backend activo del engine (un solo contexto GL; el singleton se cambia en la
// migracion a Vulkan). El concreto vive en OpenGL3Backend.cpp.
IRenderBackend& activeBackend();

} // namespace Backend
} // namespace Rendering

#endif