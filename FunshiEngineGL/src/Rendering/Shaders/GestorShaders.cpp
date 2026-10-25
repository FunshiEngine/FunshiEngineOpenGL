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
#include "GestorShaders.h"

#include "ShaderException.h"
#include "ShaderSources.h"

#include "../Backend/IRenderBackend.h"

namespace Rendering {

GestorShaders::GestorShaders(Backend::IRenderBackend& backend)
    : backend_(backend) {
    registrarIncorporados();
}

void GestorShaders::registrarIncorporados() {
    // Los shaders que usa el motor. El nombre es el que pide el codigo que
    // dibuja: si dos dibujantes usan el mismo GLSL, comparten el programa.
    registrarShader("malla", kDefaultVertexShader, kDefaultFragmentShader);
    registrarShader("linea", kLineVertexShader, kLineFragmentShader);
    registrarShader("sprite", kSpriteVertexShader, kSpriteFragmentShader);
    registrarShader("particulas", kParticulasVertexShader, kParticulasFragmentShader);
    registrarShader("cielo", kSkyVertexShader, kSkyFragmentShader);
    registrarShader("cubemap", kCubemapVertexShader, kCubemapFragmentShader);
    // Pases de profundidad del mapa de sombras: no colorea, solo deja la
    // profundidad de la geometria vista desde la luz.
    registrarShader("profundidad", kDepthVertexShader, kDepthFragmentShader);
}

void GestorShaders::registrarFragmento(const std::string& nombre,
                                        std::string fuente) {
    fragmentos_[nombre] = std::move(fuente);
}

void GestorShaders::registrarShader(const std::string& nombre,
                                     std::string vertex, std::string fragment) {
    fuentes_[nombre] = {std::move(vertex), std::move(fragment)};
}

std::shared_ptr<ShaderProgram> GestorShaders::obtener(
    const std::string& nombre) {
    const auto existente = programas_.find(nombre);
    if (existente != programas_.end()) return existente->second;

    const auto fuente = fuentes_.find(nombre);
    if (fuente == fuentes_.end())
        throw ShaderIncludeException("No hay ningun shader con el nombre: " +
                                      nombre);

    const std::string vertex =
        PreprocesadorShaders::preprocesar(fuente->second.first, fragmentos_);
    const std::string fragment =
        PreprocesadorShaders::preprocesar(fuente->second.second, fragmentos_);

    // emplace no inserta si la construccion lanza: un shader que no compila
    // no deja una entrada a medias en el mapa.
    auto programa = std::make_shared<ShaderProgram>(
        ShaderProgram::fromSource(backend_, vertex.c_str(), fragment.c_str()));
    return programas_.emplace(nombre, std::move(programa)).first->second;
}

bool GestorShaders::tiene(const std::string& nombre) const noexcept {
    return fuentes_.count(nombre) > 0;
}

void GestorShaders::invalidar() {
    programas_.clear();
}

std::size_t GestorShaders::cantidadProgramas() const noexcept {
    return programas_.size();
}

}  // namespace Rendering
