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
#include "LineBatch.h"

#include "LineBuilder.h"

using Rendering::Backend::kInvalidHandle;

LineBatch::~LineBatch() { destroy(); }

void LineBatch::destroy() {
    if (buffer_ != kInvalidHandle) {
        Rendering::Backend::activeBackend().destroyLineBatch(buffer_);
        buffer_ = kInvalidHandle;
    }
    vertexCount_ = 0;
}

LineBatch::LineBatch(LineBatch&& other) noexcept {
    buffer_ = other.buffer_;
    vertexCount_ = other.vertexCount_;
    other.buffer_ = kInvalidHandle;
    other.vertexCount_ = 0;
}

LineBatch& LineBatch::operator=(LineBatch&& other) noexcept {
    if (this != &other) {
        destroy();
        buffer_ = other.buffer_;
        vertexCount_ = other.vertexCount_;
        other.buffer_ = kInvalidHandle;
        other.vertexCount_ = 0;
    }
    return *this;
}

void LineBatch::upload(const LineBuilder& builder) {
    if (builder.vacio()) return;

    destroy();
    buffer_ = Rendering::Backend::activeBackend().createLineBatch(
        builder.vertices(), builder.cantidadVertices());
    vertexCount_ = builder.cantidadVertices();
}

void LineBatch::update(const LineBuilder& builder) {
    const std::size_t count = builder.cantidadVertices();
    if (count == 0) {
        vertexCount_ = 0;
        return;
    }

    if (buffer_ == kInvalidHandle) {
        upload(builder);
        return;
    }

    Rendering::Backend::activeBackend().updateLineBatch(
        buffer_, builder.vertices(), count);
    vertexCount_ = count;
}

void LineBatch::draw() const {
    if (buffer_ == kInvalidHandle || vertexCount_ == 0) return;
    Rendering::Backend::activeBackend().drawLineBatch(
        buffer_, static_cast<unsigned int>(vertexCount_));
}
