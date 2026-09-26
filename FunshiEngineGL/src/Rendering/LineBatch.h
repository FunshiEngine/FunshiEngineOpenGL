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
#ifndef LINEBATCH_H
#define LINEBATCH_H

#include <cstddef>

#include "Backend/IRenderBackend.h"

class LineBuilder;

// Lado GPU de un LineBuilder: posee un handle opaco al batch del backend (VAO +
// VBO en OpenGL3) con RAII, igual que MeshGPU para las mallas. El consumidor
// (grilla, marcadores, colliders) reutiliza la misma instancia frame a frame:
// la primera vez sube la geometria y despues solo la actualiza, con un unico
// draw call por pase en vez de uno por linea.
class LineBatch {
private:
    Rendering::Backend::Handle buffer_ = Rendering::Backend::kInvalidHandle;
    std::size_t vertexCount_ = 0;

    void destroy();

public:
    LineBatch() = default;
    ~LineBatch();
    LineBatch(const LineBatch&) = delete;
    LineBatch& operator=(const LineBatch&) = delete;
    LineBatch(LineBatch&& other) noexcept;
    LineBatch& operator=(LineBatch&& other) noexcept;

    // Crea el recurso con la geometria del builder. Rendicion vacia si el
    // builder no tiene segmentos.
    void upload(const LineBuilder& builder);

    // Vuelve a subir la geometria sobre el mismo recurso. Con un builder vacio
    // no dibuja (deja la cuenta en 0) pero conserva el recurso.
    void update(const LineBuilder& builder);

    bool isUploaded() const {
        return buffer_ != Rendering::Backend::kInvalidHandle;
    }
    std::size_t getVertexCount() const { return vertexCount_; }

    // Dibuja el batch como triangulos. No-op si no hay recurso/vertices.
    void draw() const;
};

#endif
