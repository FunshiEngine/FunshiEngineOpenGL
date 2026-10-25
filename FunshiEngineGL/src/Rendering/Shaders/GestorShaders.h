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
#ifndef GESTORSHADERS_H
#define GESTORSHADERS_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "PreprocesadorShaders.h"
#include "ShaderProgram.h"

namespace Rendering {
namespace Backend {
class IRenderBackend;
}

// Dueno de los programas de shader del motor.
//
// Un programa se identifica por un nombre: el gestor lo compila la primera vez
// que se pide y lo comparte con todos los que lo piden despues. Asi el motor
// no compila el mismo GLSL una vez por renderer (el shader de sprite lo
// usaban el renderer de sprites y el de tilemaps, y cada uno compilaba el
// suyo) y el codigo que dibuja no necesita saber de donde sale el GLSL.
//
// El GLSL va embebido en el binario (no en archivos): el ejecutable exportado
// no puede depender de assets extra. Un shader puede usar #include "nombre":
// el preprocesador lo resuelve en CPU (OpenGL 3.3 no tiene
// ARB_shading_language_include) contra los fragmentos registrados aqui.
//
// El backend se inyecta: el gestor no busca el backend global.
class GestorShaders {
public:
    explicit GestorShaders(Backend::IRenderBackend& backend);

    // Registra un fragmento de GLSL que se puede incluir con #include.
    void registrarFragmento(const std::string& nombre, std::string fuente);

    // Registra un shader completo por nombre (vertex + fragment).
    void registrarShader(const std::string& nombre, std::string vertex,
                         std::string fragment);

    // Programa con ese nombre. Lo compila la primera vez y lo comparte con
    // los que lo piden despues. Lanza ShaderException si el nombre no existe
    // o si el driver no puede compilar el fuente.
    //
    // El programa se devuelve por puntero compartido: el gestor puede
    // liberarlo (al invalidar los shaders) sin que quien lo tiene quede con
    // un puntero colgante.
    std::shared_ptr<ShaderProgram> obtener(const std::string& nombre);

    bool tiene(const std::string& nombre) const noexcept;

    // Libera los programas compilados (por ejemplo al recargar shaders). Los
    // que se piden despues se vuelven a compilar.
    void invalidar();

    std::size_t cantidadProgramas() const noexcept;

private:
    Backend::IRenderBackend& backend_;
    // Fragmentos que se pueden incluir con #include.
    PreprocesadorShaders::Fragmentos fragmentos_;
    // Fuentes de los shaders, por nombre: vertex y fragment.
    std::unordered_map<std::string, std::pair<std::string, std::string>> fuentes_;
    // Programas ya compilados, por nombre.
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> programas_;

    void registrarIncorporados();
};

}  // namespace Rendering

#endif  // GESTORSHADERS_H
