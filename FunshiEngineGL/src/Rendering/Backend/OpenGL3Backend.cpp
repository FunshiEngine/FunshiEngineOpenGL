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
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "[OpenGL3Backend] createMesh index buffer error: " << err
                      << " indexCount=" << data.indexCount << std::endl;
        }
    }

    GLFuncs::pfnBindVertexArray(0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[OpenGL3Backend] createMesh final error: " << err << std::endl;
    }

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

void OpenGL3Backend::drawMesh(Handle mesh, unsigned int firstIndex,
                               unsigned int indexCount) {
    if (mesh == kInvalidHandle || indexCount == 0) return;
    // Buscar la malla GPU para obtener el EBO (buffers[5]) y enlazarlo
    // explicitamente: algunos drivers no restauran el GL_ELEMENT_ARRAY_BUFFER
    // del VAO de forma fiable.
    const auto it = meshes_.find(mesh);
    if (it == meshes_.end()) {
        std::cerr << "[OpenGL3Backend] drawMesh: mesh handle no encontrado: " << mesh << std::endl;
        return;
    }
    const GpuMesh& g = it->second;
    GLFuncs::pfnBindVertexArray(g.vao);
    if (g.buffers[5] != 0) {
        GLFuncs::pfnBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.buffers[5]);
    }
    // Verificar que hay un programa de shader activo
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    if (currentProgram == 0) {
        std::cerr << "[OpenGL3Backend] drawMesh: NO HAY PROGRAMA DE SHADER ACTIVO!" << std::endl;
    }
    // El puntero del EBO se desplaza al primer indice del rango: asi una
    // region (una sub-malla con material propio) se dibuja sin tocar el resto.
    const std::size_t offset = static_cast<std::size_t>(firstIndex) *
                                sizeof(unsigned int);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                   GL_UNSIGNED_INT, (const void*)offset);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[OpenGL3Backend] drawMesh error: " << err
                  << " mesh=" << mesh << " first=" << firstIndex
                  << " count=" << indexCount << std::endl;
    }
    GLFuncs::pfnBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Batch de lineas
// ---------------------------------------------------------------------------

Handle OpenGL3Backend::createLineBatch(const float* vertices,
                                       std::size_t vertexCount) {
    if (!vertices || vertexCount == 0) return kInvalidHandle;

    GLuint vao = 0;
    GLFuncs::pfnGenVertexArrays(1, &vao);
    GLFuncs::pfnBindVertexArray(vao);

    GLuint vbo = 0;
    GLFuncs::pfnGenBuffers(1, &vbo);
    GLFuncs::pfnBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLFuncs::pfnBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertexCount * LineBuilder::kFloatsPorVertice *
                                sizeof(float)),
        vertices, GL_STATIC_DRAW);

    // Un solo VBO interleaved; el layout lo define LineBuilder (12 floats por
    // vertice) y el shader los lee de los atributos 0..4.
    const GLsizei stride =
        static_cast<GLsizei>(LineBuilder::stride());
    GLFuncs::pfnEnableVertexAttribArray(0);
    GLFuncs::pfnVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(LineBuilder::offsetInicio()));
    GLFuncs::pfnEnableVertexAttribArray(1);
    GLFuncs::pfnVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(LineBuilder::offsetFin()));
    GLFuncs::pfnEnableVertexAttribArray(2);
    GLFuncs::pfnVertexAttribPointer(
        2, 1, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(LineBuilder::offsetLado()));
    GLFuncs::pfnEnableVertexAttribArray(3);
    GLFuncs::pfnVertexAttribPointer(
        3, 1, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(LineBuilder::offsetAvance()));
    GLFuncs::pfnEnableVertexAttribArray(4);
    GLFuncs::pfnVertexAttribPointer(
        4, 4, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(LineBuilder::offsetColor()));

    GLFuncs::pfnBindVertexArray(0);

    const Handle handle = static_cast<Handle>(vao);
    lineBatches_[handle] = GpuLineBatch{vao, vbo};
    return handle;
}

void OpenGL3Backend::destroyLineBatch(Handle batch) {
    if (batch == kInvalidHandle) return;
    const auto it = lineBatches_.find(batch);
    if (it == lineBatches_.end()) return;
    if (GLFuncs::pfnDeleteVertexArrays) {
        GLuint vao = it->second.vao;
        GLFuncs::pfnDeleteVertexArrays(1, &vao);
    }
    if (GLFuncs::pfnDeleteBuffers) {
        GLuint vbo = it->second.vbo;
        GLFuncs::pfnDeleteBuffers(1, &vbo);
    }
    lineBatches_.erase(it);
}

void OpenGL3Backend::updateLineBatch(Handle batch, const float* vertices,
                                     std::size_t vertexCount) {
    if (batch == kInvalidHandle || !vertices || vertexCount == 0) return;
    if (lineBatches_.find(batch) == lineBatches_.end()) return;

    // glBufferData sobre el buffer ya enlazado en el VAO: la GPU lo considera
    // una store nueva (el driver reutiliza el bloque si la store anterior ya no
    // la referencia), que es lo que se quiere para un buffer que cambia de
    // tamano cada frame (grilla y marcadores).
    GLFuncs::pfnBindVertexArray(static_cast<GLuint>(batch));
    GLFuncs::pfnBindBuffer(GL_ARRAY_BUFFER, lineBatches_[batch].vbo);
    GLFuncs::pfnBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertexCount * LineBuilder::kFloatsPorVertice *
                                sizeof(float)),
        vertices, GL_DYNAMIC_DRAW);
    GLFuncs::pfnBindVertexArray(0);
}

void OpenGL3Backend::drawLineBatch(Handle batch, unsigned int vertexCount) {
    if (batch == kInvalidHandle || vertexCount == 0) return;
    GLFuncs::pfnBindVertexArray(static_cast<GLuint>(batch));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
    GLFuncs::pfnBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Batch de particulas
// ---------------------------------------------------------------------------

Handle OpenGL3Backend::createParticulasBatch(const float* vertices,
                                              std::size_t vertexCount) {
    if (!vertices || vertexCount == 0) return kInvalidHandle;

    GLuint vao = 0;
    GLFuncs::pfnGenVertexArrays(1, &vao);
    GLFuncs::pfnBindVertexArray(vao);

    GLuint vbo = 0;
    GLFuncs::pfnGenBuffers(1, &vbo);
    GLFuncs::pfnBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLFuncs::pfnBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertexCount *
                                ParticulasBuilder::kFloatsPorVertice *
                                sizeof(float)),
        vertices, GL_STATIC_DRAW);

    // Un solo VBO interleaved; el layout lo define ParticulasBuilder (9 floats
    // por vertice) y el shader los lee de los atributos 0..2.
    const GLsizei stride = static_cast<GLsizei>(ParticulasBuilder::stride());
    GLFuncs::pfnEnableVertexAttribArray(0);
    GLFuncs::pfnVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(ParticulasBuilder::offsetPosicion()));
    GLFuncs::pfnEnableVertexAttribArray(1);
    GLFuncs::pfnVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(ParticulasBuilder::offsetUv()));
    GLFuncs::pfnEnableVertexAttribArray(2);
    GLFuncs::pfnVertexAttribPointer(
        2, 4, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(ParticulasBuilder::offsetColor()));

    GLFuncs::pfnBindVertexArray(0);

    const Handle handle = static_cast<Handle>(vao);
    particulasBatches_[handle] = GpuParticulasBatch{vao, vbo};
    return handle;
}

void OpenGL3Backend::destroyParticulasBatch(Handle batch) {
    if (batch == kInvalidHandle) return;
    const auto it = particulasBatches_.find(batch);
    if (it == particulasBatches_.end()) return;
    if (GLFuncs::pfnDeleteVertexArrays) {
        GLuint vao = it->second.vao;
        GLFuncs::pfnDeleteVertexArrays(1, &vao);
    }
    if (GLFuncs::pfnDeleteBuffers) {
        GLuint vbo = it->second.vbo;
        GLFuncs::pfnDeleteBuffers(1, &vbo);
    }
    particulasBatches_.erase(it);
}

void OpenGL3Backend::updateParticulasBatch(Handle batch, const float* vertices,
                                           std::size_t vertexCount) {
    if (batch == kInvalidHandle || !vertices || vertexCount == 0) return;
    if (particulasBatches_.find(batch) == particulasBatches_.end()) return;

    // Igual que el batch de lineas: glBufferData sobre el buffer ya enlazado
    // en el VAO, que el driver reutiliza si la store anterior ya no lo
    // referencia. El tamano del lote cambia cada frame.
    GLFuncs::pfnBindVertexArray(static_cast<GLuint>(batch));
    GLFuncs::pfnBindBuffer(GL_ARRAY_BUFFER, particulasBatches_[batch].vbo);
    GLFuncs::pfnBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertexCount *
                                ParticulasBuilder::kFloatsPorVertice *
                                sizeof(float)),
        vertices, GL_DYNAMIC_DRAW);
    GLFuncs::pfnBindVertexArray(0);
}

void OpenGL3Backend::drawParticulasBatch(Handle batch, unsigned int vertexCount) {
    if (batch == kInvalidHandle || vertexCount == 0) return;
    GLFuncs::pfnBindVertexArray(static_cast<GLuint>(batch));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
    GLFuncs::pfnBindVertexArray(0);
}

}  // namespace Rendering::Backend
