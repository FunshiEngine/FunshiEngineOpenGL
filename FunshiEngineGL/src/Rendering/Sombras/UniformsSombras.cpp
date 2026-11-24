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
#include "UniformsSombras.h"

#include <algorithm>
#include <cstdio>

#include "../Shaders/ShaderProgram.h"

namespace Rendering {
namespace UniformsSombras {

namespace {

int cantidadSlots(const std::vector<ProyectorSombra>& proyectores) {
    return std::min(kMaxProyectoresSombra, static_cast<int>(proyectores.size()));
}

void nombre(char* destino, std::size_t tam, const char* formato, int indice) {
    std::snprintf(destino, tam, formato, indice);
}

}  // namespace

void configurarSamplers(ShaderProgram& shader) {
    shader.use();
    char texto[32];
    for (int s = 0; s < kMaxProyectoresSombra; ++s) {
        nombre(texto, sizeof(texto), "uShadowMap%d", s);
        shader.setInt(texto, kUnidadSombra2D + s);
        nombre(texto, sizeof(texto), "uShadowCube%d", s);
        shader.setInt(texto, kUnidadSombraCubo + s);
    }
}

void subir(ShaderProgram& shader, const std::vector<ProyectorSombra>& proyectores) {
    // Los uniforms se suben con el programa activo: sin use() se descartan.
    shader.use();
    const int cantidad = cantidadSlots(proyectores);
    shader.setInt("uShadowCount", cantidad);

    char texto[40];
    for (int s = 0; s < cantidad; ++s) {
        const ProyectorSombra& p = proyectores[static_cast<std::size_t>(s)];
        nombre(texto, sizeof(texto), "uShadowMatrix[%d]", s);
        shader.setMat4(texto, p.matrizLuz);
        nombre(texto, sizeof(texto), "uShadowEsCubo[%d]", s);
        shader.setInt(texto, p.esCubemap ? 1 : 0);
        nombre(texto, sizeof(texto), "uShadowLightIndex[%d]", s);
        shader.setInt(texto, p.indiceLuz);
        nombre(texto, sizeof(texto), "uShadowBias[%d]", s);
        shader.setFloat(texto, p.sesgo);
        nombre(texto, sizeof(texto), "uShadowStrength[%d]", s);
        shader.setFloat(texto, p.fuerza);
        nombre(texto, sizeof(texto), "uShadowTexel[%d]", s);
        shader.setFloat(texto, 1.f / static_cast<float>(p.resolucion > 0 ? p.resolucion : 1));
        nombre(texto, sizeof(texto), "uShadowTexelMundo[%d]", s);
        shader.setFloat(texto, p.tamanoTexelMundo);
        nombre(texto, sizeof(texto), "uShadowPosLuz[%d]", s);
        shader.setVec3(texto, p.posicionLuz);
        nombre(texto, sizeof(texto), "uShadowDistancia[%d]", s);
        shader.setFloat(texto, p.distancia > 0.f ? p.distancia : 1.f);
    }
}

void enlazarTexturas(Backend::IRenderBackend& backend,
                     const std::vector<ProyectorSombra>& proyectores) {
    const int cantidad = cantidadSlots(proyectores);
    for (int s = 0; s < cantidad; ++s) {
        const ProyectorSombra& p = proyectores[static_cast<std::size_t>(s)];
        if (p.textura == Backend::kInvalidHandle) continue;
        if (p.esCubemap)
            backend.bindTextureCube(p.textura, kUnidadSombraCubo + s);
        else
            backend.bindTexture2D(p.textura, kUnidadSombra2D + s);
    }
}

}  // namespace UniformsSombras
}  // namespace Rendering
