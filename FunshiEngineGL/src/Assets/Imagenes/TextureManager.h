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
#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "Image.h"

// Contrato de carga de una imagen desde disco (Strategy). El TextureManager
// inyecta el loader concreto; la integracion con el motor usara StbImageLoader
// (stb_image), pero las pruebas headless pueden usar uno artificial sin tocar
// la pila grafica.
class ITextureLoader {
public:
    virtual ~ITextureLoader() = default;
    // Devuelve la imagen decodificada (RGBA), o lanza TextureLoadException.
    virtual std::shared_ptr<Image> load(const std::string& path) = 0;
};

// Registro central de texturas (Flyweight/Registry) en CPU, espejo de
// AssetManager para mallas: pedir el mismo path devuelve la MISMA imagen
// compartida (el decode con stb_image se hace una sola vez por archivo). No
// expone OpenGL: el upload a GPU (TextureGL) vive en el renderer, que cachea
// por identidad de la imagen al igual que MeshGPU cachea por identidad de Mesh.
class TextureManager {
public:
    explicit TextureManager(std::unique_ptr<ITextureLoader> loader = nullptr);
    ~TextureManager() = default;

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) noexcept = default;
    TextureManager& operator=(TextureManager&&) noexcept = default;

    // Devuelve la imagen cacheada para el path (normalizado internamente).
    // Si no esta, intenta cargarla con el loader inyectado; sin loader o con
    // decode fallido lanza TextureNotFoundException / TextureLoadException.
    std::shared_ptr<const Image> getTexture(const std::string& path);

    // Inserta/sobrescribe una imagen construida en codigo (procedural, tests).
    void putTexture(const std::string& path, std::shared_ptr<Image> image);

    bool containsTexture(const std::string& path) const;
    size_t textureCount() const;
    void removeTexture(const std::string& path);

    // Evicta las entradas cuyo shared_ptr solo referencia el manager
    // (use_count == 1): no hay usuarios externos vivos de esa imagen.
    void clearUnusedTextures();

    void setLoader(std::unique_ptr<ITextureLoader> loader);
    const ITextureLoader* loader() const { return loader_.get(); }

private:
    std::unordered_map<std::string, std::shared_ptr<Image>> textures_;
    std::unique_ptr<ITextureLoader> loader_;
};

#endif