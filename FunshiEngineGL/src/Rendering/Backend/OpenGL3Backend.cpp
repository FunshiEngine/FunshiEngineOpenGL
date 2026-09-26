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
#include "OpenGL3Backend.h"

#include <cstdio>
#include <iostream>
#include <string>

#include <glm/gtc/type_ptr.hpp>

#include "../Shaders/ShaderException.h"

namespace Rendering {
namespace Backend {

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

// ---------------------------------------------------------------------------
// Inicializacion / estado
// ---------------------------------------------------------------------------

bool OpenGL3Backend::init() {
    const bool ok = GLFuncs::init();
    if (ok) fboFuncionesCargadas();
    return ok;
}

bool OpenGL3Backend::fboFuncionesCargadas() {
    if (fboCargadas_) return true;
    cargar("glGenFramebuffers", pfnGenFramebuffers);
    cargar("glDeleteFramebuffers", pfnDeleteFramebuffers);
    cargar("glBindFramebuffer", pfnBindFramebuffer);
    cargar("glFramebufferTexture2D", pfnFramebufferTexture2D);
    cargar("glGenRenderbuffers", pfnGenRenderbuffers);
    cargar("glDeleteRenderbuffers", pfnDeleteRenderbuffers);
    cargar("glBindRenderbuffer", pfnBindRenderbuffer);
    cargar("glRenderbufferStorage", pfnRenderbufferStorage);
    cargar("glFramebufferRenderbuffer", pfnFramebufferRenderbuffer);
    cargar("glCheckFramebufferStatus", pfnCheckFramebufferStatus);

    fboCargadas_ = pfnGenFramebuffers && pfnDeleteFramebuffers &&
                   pfnBindFramebuffer && pfnFramebufferTexture2D &&
                   pfnGenRenderbuffers && pfnDeleteRenderbuffers &&
                   pfnBindRenderbuffer && pfnRenderbufferStorage &&
                   pfnFramebufferRenderbuffer && pfnCheckFramebufferStatus;
    return fboCargadas_;
}

// ---------------------------------------------------------------------------
// Malla GPU
// ---------------------------------------------------------------------------

Handle OpenGL3Backend::createMesh(const MeshData& data) {
    if (data.indexCount == 0 || !data.vertices || data.vertexCount == 0)
        return kInvalidHandle;

    GLuint vao = 0;
    GLFuncs::pfnGenVertexArrays(1, &vao);
    GLFuncs::pfnBindVertexArray(vao);

    GLuint buffers[6] = {0, 0, 0, 0, 0, 0};
    GLFuncs::pfnGenBuffers(6, buffers);

    // Posicion -> atributo 0.
    if (subirBuffer(GL_ARRAY_BUFFER, buffers[0], data.vertices,
                    data.vertexCount * 3)) {
        GLFuncs::pfnEnableVertexAttribArray(0);
        GLFuncs::pfnVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0,
                                        (const void*)nullptr);
    }

    // Normal -> atributo 1 (opcional, si hay una por vertice).
    if (data.normals && data.normalCount == data.vertexCount &&
        subirBuffer(GL_ARRAY_BUFFER, buffers[1], data.normals,
                    data.normalCount * 3)) {
        GLFuncs::pfnEnableVertexAttribArray(1);
        GLFuncs::pfnVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
                                        (const void*)nullptr);
    }

    // Tangente/Bitangente -> atributos 3/4 (normal mapping, opcionales).
    if (data.tangents && data.tangentCount == data.vertexCount &&
        data.bitangents && data.bitangentCount == data.vertexCount) {
        if (subirBuffer(GL_ARRAY_BUFFER, buffers[2], data.tangents,
                        data.tangentCount * 3)) {
            GLFuncs::pfnEnableVertexAttribArray(3);
            GLFuncs::pfnVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0,
                                            (const void*)nullptr);
        }
        if (subirBuffer(GL_ARRAY_BUFFER, buffers[3], data.bitangents,
                        data.bitangentCount * 3)) {
            GLFuncs::pfnEnableVertexAttribArray(4);
            GLFuncs::pfnVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0,
                                            (const void*)nullptr);
        }
    }

    // UV -> atributo 2 (opcional, si hay una por vertice).
    if (data.uvs && data.uvCount == data.vertexCount &&
        subirBuffer(GL_ARRAY_BUFFER, buffers[4], data.uvs, data.uvCount * 2)) {
        GLFuncs::pfnEnableVertexAttribArray(2);
        GLFuncs::pfnVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0,
                                        (const void*)nullptr);
    }

    // Indices -> EBO (se graban en el VAO por estar bindeados con el VAO activo).
    if (data.indices && data.indexCount > 0) {
        GLFuncs::pfnBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[5]);
        GLFuncs::pfnBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(data.indexCount * sizeof(GLuint)),
            data.indices, GL_STATIC_DRAW);
    }

    GLFuncs::pfnBindVertexArray(0);

    const Handle handle = static_cast<Handle>(vao);
    meshes_[handle] = GpuMesh{vao, {buffers[0], buffers[1], buffers[2],
                                    buffers[3], buffers[4], buffers[5]}};
    return handle;
}

void OpenGL3Backend::destroyMesh(Handle mesh) {
    if (mesh == kInvalidHandle) return;
    const auto it = meshes_.find(mesh);
    if (it == meshes_.end()) return;
    const GpuMesh& g = it->second;
    if (GLFuncs::pfnDeleteVertexArrays) {
        GLuint vao = g.vao;
        GLFuncs::pfnDeleteVertexArrays(1, &vao);
    }
    if (GLFuncs::pfnDeleteBuffers) {
        GLuint buffers[6] = {g.buffers[0], g.buffers[1], g.buffers[2],
                             g.buffers[3], g.buffers[4], g.buffers[5]};
        GLFuncs::pfnDeleteBuffers(6, buffers);
    }
    meshes_.erase(it);
}

void OpenGL3Backend::drawMesh(Handle mesh, unsigned int indexCount) {
    if (mesh == kInvalidHandle || indexCount == 0) return;
    GLFuncs::pfnBindVertexArray(static_cast<GLuint>(mesh));
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                   GL_UNSIGNED_INT, (const void*)nullptr);
    GLFuncs::pfnBindVertexArray(0);
}

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
    return static_cast<Handle>(texture);
}

void OpenGL3Backend::destroyTexture2D(Handle texture) {
    if (texture == kInvalidHandle) return;
    GLuint id = static_cast<GLuint>(texture);
    glDeleteTextures(1, &id);
}

void OpenGL3Backend::bindTexture2D(Handle texture, int unit) {
    if (texture == kInvalidHandle || !GLFuncs::pfnActiveTexture) return;
    GLFuncs::pfnActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
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
    const GpuTarget& t = it->second;
    if (pfnDeleteFramebuffers) {
        GLuint fbo = t.fbo;
        pfnDeleteFramebuffers(1, &fbo);
        targets_.erase(it);
    }
    if (pfnDeleteRenderbuffers) {
        GLuint rbo = t.rbo;
        pfnDeleteRenderbuffers(1, &rbo);
    }
    if (t.colorTex) {
        GLuint colorTex = t.colorTex;
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
// Estado del pipeline de compatibilidad / matriz de modelo
// ---------------------------------------------------------------------------

void OpenGL3Backend::setViewport(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) return;
    glViewport(x, y, width, height);
}

void OpenGL3Backend::setCompatibilityMatrices(const float* projection,
                                              const float* view) {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view);
}

void OpenGL3Backend::clearScreen(const float color[3]) {
    if (color) glClearColor(color[0], color[1], color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGL3Backend::setLegacyLights(const LegacyLight* lights,
                                     int lightCount,
                                     const float* globalAmbient) {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    // Modelo global (GL_LIGHT_MODEL_AMBIENT): si no llega, se usa el default
    // gris tenue que usaba LightSystem.
    static const float kAmbientDefault[4] = {0.15f, 0.15f, 0.15f, 1.0f};
    const GLfloat ambientModel[4] = {
        globalAmbient ? globalAmbient[0] : kAmbientDefault[0],
        globalAmbient ? globalAmbient[1] : kAmbientDefault[1],
        globalAmbient ? globalAmbient[2] : kAmbientDefault[2], 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientModel);

    // Apagar todos los slots garantiza que luces removidas no sigan activas:
    // el backend es dueno del estado de luz, no vive sobre estado heredado.
    for (int i = 0; i < 8; ++i) glDisable(static_cast<GLenum>(GL_LIGHT0 + i));

    if (!lights || lightCount <= 0) return;
    const int n = lightCount < 8 ? lightCount : 8;
    for (int i = 0; i < n; ++i) {
        const LegacyLight& d = lights[i];
        const GLenum slot = static_cast<GLenum>(GL_LIGHT0 + i);
        const bool direccional = (d.type == 0);

        const GLfloat pos[4] = {d.worldPos[0], d.worldPos[1], d.worldPos[2],
                                direccional ? 0.f : 1.f};
        const GLfloat amb[4] = {d.ambient[0], d.ambient[1], d.ambient[2], 1.f};
        const GLfloat diff[4] = {d.diffuse[0], d.diffuse[1], d.diffuse[2], 1.f};
        const GLfloat spec[4] = {d.specular[0], d.specular[1], d.specular[2],
                                 1.f};

        glEnable(slot);
        glLightfv(slot, GL_POSITION, pos);
        glLightfv(slot, GL_AMBIENT, amb);
        glLightfv(slot, GL_DIFFUSE, diff);
        glLightfv(slot, GL_SPECULAR, spec);

        if (d.type == 1 || d.type == 2) {
            glLightf(slot, GL_CONSTANT_ATTENUATION, d.constant);
            glLightf(slot, GL_LINEAR_ATTENUATION, d.linear);
            glLightf(slot, GL_QUADRATIC_ATTENUATION, d.quadratic);
        }
        if (d.type == 2) {
            const GLfloat spotDir[3] = {d.direction[0], d.direction[1],
                                        d.direction[2]};
            glLightfv(slot, GL_SPOT_DIRECTION, spotDir);
            glLightf(slot, GL_SPOT_CUTOFF, d.spotCutoffDegrees);
            glLightf(slot, GL_SPOT_EXPONENT, 1.f);
        }
    }
}

const char* OpenGL3Backend::diagnosticoCompat() const {
    static thread_local char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "GL_LIGHTING=%s GL_LIGHT0=%s err=0x%x",
                  glIsEnabled(GL_LIGHTING) ? "on" : "off",
                  glIsEnabled(GL_LIGHT0) ? "on" : "off",
                  static_cast<unsigned int>(glGetError()));
    return buffer;
}

void OpenGL3Backend::applyBaseState() {
    // Estado del contexto recien creado (equivalente al setup historico de
    // main: depth test, normalizacion de normales y seguimiento de color por
    // material para el modo inmediato).
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_NORMALIZE);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    // Activacion explicita del framebuffer multisampleado (se pidio 4x en la
    // creacion del contexto): suaviza las lineas de la escena sin depender de
    // GL_LINE_SMOOTH.
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

void OpenGL3Backend::pushMatrix() { glPushMatrix(); }

void OpenGL3Backend::popMatrix() { glPopMatrix(); }

void OpenGL3Backend::multMatrix(const float mat4[16]) {
    if (mat4) glMultMatrixf(mat4);
}

void OpenGL3Backend::applyTransform(const float translate[3],
                                    const float scale[3],
                                    const float rotate4[4]) {
    if (translate) glTranslatef(translate[0], translate[1], translate[2]);
    if (scale) glScalef(scale[0], scale[1], scale[2]);
    if (rotate4) glRotatef(rotate4[0], rotate4[1], rotate4[2], rotate4[3]);
}

void OpenGL3Backend::setLightingEnabled(bool enabled) {
    if (enabled)
        glEnable(GL_LIGHTING);
    else
        glDisable(GL_LIGHTING);
}

void OpenGL3Backend::setPolygonFill() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void OpenGL3Backend::setSolidColor(float r, float g, float b) {
    glColor3f(r, g, b);
}

void OpenGL3Backend::setMaterial(const float ambient[4],
                                 const float diffuse[4],
                                 const float specular[4],
                                 const float emission[4], float shininess) {
    if (ambient) glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    if (diffuse) glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    if (specular) glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    if (emission) glMaterialfv(GL_FRONT, GL_EMISSION, emission);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);
}

void OpenGL3Backend::setLineWidth(float width) { glLineWidth(width); }

void OpenGL3Backend::setLineSmoothing(bool enabled) {
    if (enabled) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        // GL_LINE_SMOOTH solo suaviza de verdad si el blending esta activo:
        // sin GL_BLEND los drivers degradan a lineas escalonadas ("a dientes").
        // Se guarda el estado previo del blending para restaurarlo al cerrar.
        blendPreviaLineaSmooth_ = (glIsEnabled(GL_BLEND) == GL_TRUE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_LINE_SMOOTH);
        if (!blendPreviaLineaSmooth_) glDisable(GL_BLEND);
    }
}

// ---------------------------------------------------------------------------
// Primitivas inmediatas
// ---------------------------------------------------------------------------

void OpenGL3Backend::drawLinePairs(const float* vertices, int vertexCount) {
    if (!vertices || vertexCount < 2) return;
    glBegin(GL_LINES);
    for (int i = 0; i + 1 < vertexCount; i += 2) {
        glVertex3fv(vertices + 3 * i);
        glVertex3fv(vertices + 3 * (i + 1));
    }
    glEnd();
}

void OpenGL3Backend::drawLinePairsRGBA(const float* vertices,
                                       int vertexCount) {
    if (!vertices || vertexCount < 2) return;
    glBegin(GL_LINES);
    for (int i = 0; i + 1 < vertexCount; i += 2) {
        glColor4fv(vertices + 7 * i + 3);
        glVertex3fv(vertices + 7 * i);
        glColor4fv(vertices + 7 * (i + 1) + 3);
        glVertex3fv(vertices + 7 * (i + 1));
    }
    glEnd();
}

void OpenGL3Backend::drawIndexedLines(const float* vertices, int vertexCount,
                                      const int* edgeIndices, int edgeCount) {
    if (!vertices || vertexCount < 1 || !edgeIndices || edgeCount < 1) return;
    glBegin(GL_LINES);
    for (int i = 0; i < edgeCount; ++i) {
        const int i0 = edgeIndices[2 * i];
        const int i1 = edgeIndices[2 * i + 1];
        if (i0 < 0 || i0 >= vertexCount || i1 < 0 || i1 >= vertexCount)
            continue;
        glVertex3fv(vertices + 3 * i0);
        glVertex3fv(vertices + 3 * i1);
    }
    glEnd();
}

void OpenGL3Backend::drawLineStrip(const float* vertices, int vertexCount,
                                   bool closed) {
    if (!vertices || vertexCount < 2) return;
    glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (int i = 0; i < vertexCount; ++i) glVertex3fv(vertices + 3 * i);
    glEnd();
}

void OpenGL3Backend::drawTriangles(const float* vertices, int vertexCount,
                                   const float* normals, int normalCount,
                                   const unsigned int* indices,
                                   int indexCount) {
    if (!vertices || vertexCount == 0 || !indices || indexCount == 0) return;
    const bool dibujaNormales = normals && normalCount >= vertexCount;
    glBegin(GL_TRIANGLES);
    for (int k = 0; k < indexCount; ++k) {
        const unsigned int idx = indices[k];
        if (idx >= static_cast<unsigned int>(vertexCount)) continue;
        if (dibujaNormales) {
            const float nx = normals[3 * idx + 0];
            const float ny = normals[3 * idx + 1];
            const float nz = normals[3 * idx + 2];
            if (!(nx == 0.f && ny == 0.f && nz == 0.f)) glNormal3f(nx, ny, nz);
        }
        glVertex3fv(vertices + 3 * static_cast<std::size_t>(idx));
    }
    glEnd();
}

// ---------------------------------------------------------------------------
// Backend activo (singleton del proceso con un solo contexto de OpenGL).
// ---------------------------------------------------------------------------

IRenderBackend& activeBackend() {
    static OpenGL3Backend backend;
    return backend;
}

} // namespace Backend
} // namespace Rendering