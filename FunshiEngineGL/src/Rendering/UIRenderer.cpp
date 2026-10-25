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
#include "UIRenderer.h"

#include "../Assets/TextureManager.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <exception>
#include <iostream>

namespace {

// Margen interior de los widgets con texto (en pixeles).
constexpr float kMargenTexto = 2.0f;
// Redondeo de los rectangulos (en pixeles).
constexpr float kRedondeo = 2.0f;
// Cuanto se aclara el boton que tiene el raton encima.
constexpr float kClaroHover = 0.25f;

// Convierte un color 0..1 al color empaquetado de ImGui.
ImU32 colorU32(const ColorUI& color) {
    const auto canal = [](float valor) -> int {
        const float acotado = std::max(0.0f, std::min(1.0f, valor));
        return static_cast<int>(acotado * 255.0f + 0.5f);
    };
    return IM_COL32(canal(color.r), canal(color.g), canal(color.b),
                    canal(color.a));
}

// Tamano con el que se dibuja el texto de un widget (0 = el de la fuente
// activa). La fuente de ImGui es una sola (la que trae el motor) y su tamano
// se escala por widget.
float tamanoTextoEfectivo(const WidgetUI& widget) {
    if (widget.tamanoTexto > 0.0f) return widget.tamanoTexto;
    return ImGui::GetFontSize();
}

} // namespace

UIRenderer::UIRenderer(Rendering::Backend::IRenderBackend& backend) noexcept
    : backend_(backend) {}

UIRenderer::~UIRenderer() {
    // Las texturas de la interfaz se destruyen antes de que muera el backend.
    for (auto& par : texturas_) {
        if (par.second != Rendering::Backend::kInvalidHandle)
            backend_.destroyTexture2D(par.second);
    }
    texturas_.clear();
}

ImTextureID UIRenderer::texturaDe(const std::string& ruta) {
    if (ruta.empty() || textureManager_ == nullptr) return ImTextureID_Invalid;

    const auto existente = texturas_.find(ruta);
    if (existente != texturas_.end()) {
        if (existente->second == Rendering::Backend::kInvalidHandle)
            return ImTextureID_Invalid;
        const void* descriptor = backend_.imguiTextureId(existente->second);
        return descriptor ? static_cast<ImTextureID>(
                                reinterpret_cast<std::intptr_t>(descriptor))
                          : ImTextureID_Invalid;
    }

    // El cache de la escena devuelve la imagen ya decodificada en RGBA.
    std::shared_ptr<const Image> imagen;
    try {
        imagen = textureManager_->getTexture(ruta);
    } catch (const std::exception& error) {
        std::cerr << "[interfaz] no se pudo cargar la imagen '" << ruta
                  << "': " << error.what() << "\n";
        texturas_[ruta] = Rendering::Backend::kInvalidHandle;
        return ImTextureID_Invalid;
    }
    if (!imagen || imagen->isEmpty()) {
        texturas_[ruta] = Rendering::Backend::kInvalidHandle;
        return ImTextureID_Invalid;
    }

    Rendering::Backend::Image2D gpuImage;
    gpuImage.width = imagen->width;
    gpuImage.height = imagen->height;
    gpuImage.pixels = imagen->pixels.data();
    gpuImage.generateMipmaps = true;
    const Rendering::Backend::Handle textura =
        backend_.createTexture2D(gpuImage);
    texturas_[ruta] = textura;
    if (textura == Rendering::Backend::kInvalidHandle) {
        std::cerr << "[interfaz] no se pudo subir la imagen '" << ruta
                  << "'\n";
        return ImTextureID_Invalid;
    }

    const void* descriptor = backend_.imguiTextureId(textura);
    return descriptor ? static_cast<ImTextureID>(
                            reinterpret_cast<std::intptr_t>(descriptor))
                      : ImTextureID_Invalid;
}

void UIRenderer::dibujar(const LienzoUI& lienzo, const SistemaUI& sistema,
                          ImDrawList* dibujo) {
    if (dibujo == nullptr) return;

    const std::vector<WidgetUI>& widgets = lienzo.widgets();
    const std::vector<RectUI>& rectangulos = lienzo.rectangulos();
    if (rectangulos.size() != widgets.size()) return;

    ImFont* fuente = ImGui::GetFont();

    for (std::size_t i = 0; i < widgets.size(); ++i) {
        const WidgetUI& widget = widgets[i];
        if (!widget.visible) continue;

        const RectUI& rect = rectangulos[i];
        const ImVec2 minimo(rect.x, rect.y);
        const ImVec2 maximo(rect.x + rect.ancho, rect.y + rect.alto);

        switch (widget.tipo) {
        case TipoWidgetUI::Panel: {
            dibujo->AddRectFilled(minimo, maximo, colorU32(widget.color),
                                  kRedondeo);
            break;
        }
        case TipoWidgetUI::Imagen: {
            const ImTextureID textura = texturaDe(widget.rutaTextura);
            if (textura == ImTextureID_Invalid) break;
            // El loader de imagenes las deja volteadas para OpenGL (origen
            // abajo-izquierda); ImGui usa el origen arriba-izquierda, asi que
            // las coordenadas de textura van invertidas.
            dibujo->AddImage(ImTextureRef(textura), minimo, maximo,
                             ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
                             colorU32(widget.color));
            break;
        }
        case TipoWidgetUI::Etiqueta: {
            if (widget.texto.empty()) break;
            const float tamano = tamanoTextoEfectivo(widget);
            const ImVec2 posicion(rect.x + kMargenTexto,
                                  rect.y + kMargenTexto);
            dibujo->AddText(fuente, tamano, posicion, colorU32(widget.colorTexto),
                            widget.texto.c_str());
            break;
        }
        case TipoWidgetUI::Boton: {
            ColorUI fondo = widget.color;
            if (sistema.sobreIndice(i)) {
                fondo.r = std::min(1.0f, fondo.r + kClaroHover);
                fondo.g = std::min(1.0f, fondo.g + kClaroHover);
                fondo.b = std::min(1.0f, fondo.b + kClaroHover);
            }
            dibujo->AddRectFilled(minimo, maximo, colorU32(fondo), kRedondeo);
            if (widget.texto.empty()) break;
            const float tamano = tamanoTextoEfectivo(widget);
            const ImVec2 medidas =
                fuente ? fuente->CalcTextSizeA(tamano, FLT_MAX, 0.0f,
                                               widget.texto.c_str(), nullptr,
                                               nullptr)
                       : ImVec2(0.0f, 0.0f);
            const ImVec2 posicion(
                rect.x + (rect.ancho - medidas.x) * 0.5f,
                rect.y + (rect.alto - medidas.y) * 0.5f);
            dibujo->AddText(fuente, tamano, posicion, colorU32(widget.colorTexto),
                            widget.texto.c_str());
            break;
        }
        default:
            break;
        }
    }
}
