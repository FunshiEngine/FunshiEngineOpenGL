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
#include "MeshGPU.h"

using Rendering::Backend::kInvalidHandle;
using Rendering::Backend::MeshData;

MeshGPU::~MeshGPU() { destroy(); }

void MeshGPU::destroy() {
    if (buffer_ != kInvalidHandle) {
        backend_->destroyMesh(buffer_);
        buffer_ = kInvalidHandle;
    }
    indexCount_ = 0;
}

MeshGPU::MeshGPU(MeshGPU&& other) noexcept {
    backend_ = other.backend_;
    buffer_ = other.buffer_;
    indexCount_ = other.indexCount_;
    other.buffer_ = kInvalidHandle;
    other.indexCount_ = 0;
}

MeshGPU& MeshGPU::operator=(MeshGPU&& other) noexcept {
    if (this != &other) {
        destroy();
        backend_ = other.backend_;
        buffer_ = other.buffer_;
        indexCount_ = other.indexCount_;
        other.buffer_ = kInvalidHandle;
        other.indexCount_ = 0;
    }
    return *this;
}

void MeshGPU::upload(const float* vertices, std::size_t vertexCount,
                     const float* normals, std::size_t normalCount,
                     const float* uvs, std::size_t uvCount,
                     const float* tangents, std::size_t tangentCount,
                     const float* bitangents, std::size_t bitangentCount,
                     const unsigned int* indices, std::size_t indexCount) {
    if (indexCount == 0 || !vertices || vertexCount == 0) return;

    destroy();

    MeshData data;
    data.vertices = vertices;
    data.vertexCount = vertexCount;
    data.normals = normals;
    data.normalCount = normalCount;
    data.uvs = uvs;
    data.uvCount = uvCount;
    data.tangents = tangents;
    data.tangentCount = tangentCount;
    data.bitangents = bitangents;
    data.bitangentCount = bitangentCount;
    data.indices = indices;
    data.indexCount = indexCount;

    buffer_ = backend_->createMesh(data);
    indexCount_ = indexCount;
}

void MeshGPU::draw() const {
    if (buffer_ == kInvalidHandle || indexCount_ == 0) return;
    backend_->drawMesh(buffer_, 0, getIndexCount());
}

void MeshGPU::draw(unsigned int firstIndex, unsigned int indexCount) const {
    if (buffer_ == kInvalidHandle || indexCount == 0) return;
    // Un rango que no cabe en la malla se ignora: dibujar de mas leeria
    // indices que no existen.
    if (static_cast<std::size_t>(firstIndex) + indexCount > indexCount_) return;
    backend_->drawMesh(buffer_, firstIndex, indexCount);
}