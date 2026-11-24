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
#include "TextureGL.h"

TextureGL::~TextureGL() { destroy(); }

void TextureGL::destroy() {
    if (texture_ != Rendering::Backend::kInvalidHandle) {
        backend_->destroyTexture2D(texture_);
        texture_ = Rendering::Backend::kInvalidHandle;
    }
    width_ = height_ = 0;
}

TextureGL::TextureGL(TextureGL&& other) noexcept {
    backend_ = other.backend_;
    texture_ = other.texture_;
    width_ = other.width_;
    height_ = other.height_;
    other.texture_ = Rendering::Backend::kInvalidHandle;
    other.width_ = other.height_ = 0;
}

TextureGL& TextureGL::operator=(TextureGL&& other) noexcept {
    if (this != &other) {
        destroy();
        backend_ = other.backend_;
        texture_ = other.texture_;
        width_ = other.width_;
        height_ = other.height_;
        other.texture_ = Rendering::Backend::kInvalidHandle;
        other.width_ = other.height_ = 0;
    }
    return *this;
}

void TextureGL::upload(const Image& image) {
    if (image.isEmpty() || image.width <= 0 || image.height <= 0) return;

    destroy();

    Rendering::Backend::Image2D gpuImage;
    gpuImage.width = image.width;
    gpuImage.height = image.height;
    gpuImage.pixels = image.pixels.data();

    texture_ = backend_->createTexture2D(gpuImage);
    if (texture_ == Rendering::Backend::kInvalidHandle) return;

    width_ = image.width;
    height_ = image.height;
}

void TextureGL::bindUnit(int unit) const {
    if (texture_ == Rendering::Backend::kInvalidHandle) return;
    backend_->bindTexture2D(texture_, unit);
}