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
#ifndef MESHGPU_H
#define MESHGPU_H

#include <cstddef>

#include "Backend/IRenderBackend.h"

// Representacion GPU de una Mesh CPU: no conoce la API grafica concreta,
// posee un handle opaco al backend (VAO/VBO en OpenGL3, buffers en Vulkan) con
// RAII. La malla Mesh viene de AssetManager (compartida); el MeshGPU es un
// recurso del renderer y se cachea por identidad de la malla para no re-subir
// geometria redundante. Atributos: 0 = posicion (vec3), 1 = normal (vec3),
// 2 = UV (vec2), 3 = tangente (vec3), 4 = bitangente (vec3).
class MeshGPU {
private:
    // Backend con el que se crean y destruyen los recursos: se inyecta, no se
    // busca en un singleton.
    Rendering::Backend::IRenderBackend* backend_ = nullptr;
    Rendering::Backend::Handle buffer_ = 0;
    std::size_t indexCount_ = 0;

    void destroy();

public:
    explicit MeshGPU(Rendering::Backend::IRenderBackend& backend) noexcept
        : backend_(&backend) {}
    ~MeshGPU();
    MeshGPU(const MeshGPU&) = delete;
    MeshGPU& operator=(const MeshGPU&) = delete;
    MeshGPU(MeshGPU&& other) noexcept;
    MeshGPU& operator=(MeshGPU&& other) noexcept;

    // Sube los buffers desde la Mesh CPU. Rendicion vacia si no hay indices.
    void upload(const float* vertices, std::size_t vertexCount,
                const float* normals, std::size_t normalCount,
                const float* uvs, std::size_t uvCount,
                const float* tangents, std::size_t tangentCount,
                const float* bitangents, std::size_t bitangentCount,
                const unsigned int* indices, std::size_t indexCount);

    bool isUploaded() const { return buffer_ != 0; }
    unsigned int getIndexCount() const {
        return static_cast<unsigned int>(indexCount_);
    }

    // Dibuja la malla subida. No-op si no hay recurso/indices.
    void draw() const;
    // Dibuja solo un rango de indices (una region de la malla, con su
    // material propio). No-op si el rango no cabe en la malla.
    void draw(unsigned int firstIndex, unsigned int indexCount) const;
};

#endif