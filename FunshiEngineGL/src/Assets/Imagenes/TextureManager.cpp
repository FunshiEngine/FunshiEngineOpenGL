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
#include "TextureManager.h"

#include "../AssetPath.h"
#include "TextureException.h"

TextureManager::TextureManager(std::unique_ptr<ITextureLoader> loader)
    : loader_(std::move(loader)) {}

std::shared_ptr<const Image> TextureManager::getTexture(const std::string& path) {
    const std::string key = AssetPath::normalize(path);
    const auto it = textures_.find(key);
    if (it != textures_.end()) return it->second;

    if (!loader_) throw TextureNotFoundException(key);

    std::shared_ptr<Image> image = loader_->load(key); // puede lanzar
    if (!image || image->isEmpty())
        throw TextureLoadException(key, "el loader devolvio una imagen vacia");

    return textures_.emplace(key, std::move(image)).first->second;
}

void TextureManager::putTexture(const std::string& path,
                               std::shared_ptr<Image> image) {
    if (!image || image->isEmpty())
        throw TextureLoadException(path, "no se puede registrar una imagen vacia");
    textures_[AssetPath::normalize(path)] = std::move(image);
}

bool TextureManager::containsTexture(const std::string& path) const {
    return textures_.find(AssetPath::normalize(path)) != textures_.end();
}

size_t TextureManager::textureCount() const { return textures_.size(); }

void TextureManager::removeTexture(const std::string& path) {
    textures_.erase(AssetPath::normalize(path));
}

void TextureManager::clearUnusedTextures() {
    for (auto it = textures_.begin(); it != textures_.end();) {
        if (it->second.use_count() == 1)
            it = textures_.erase(it);
        else
            ++it;
    }
}

void TextureManager::setLoader(std::unique_ptr<ITextureLoader> loader) {
    loader_ = std::move(loader);
}