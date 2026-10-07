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
#ifndef BACKENDCPP_H
#define BACKENDCPP_H

#include "BackendScript.h"

// Backend C++: compila el .cpp a una biblioteca compartida con el compilador
// del engine (FUNSHI_CXX_COMPILER) y la carga con dlopen/LoadLibrary.
// El template de script exporta: extern "C" IScriptBehaviour*
// FUNSHI_CREAR_COMPORTAMIENTO(const MotorScript::ApiScriptGameObject*).
class BackendCpp : public BackendScript {
public:
    const char* lenguaje() const override;

    bool compilarYCargar(const std::string& fuente,
                         const std::string& nombreClase,
                         ComportamientoCargado& salida,
                         std::string& error) override;

    void descargar(ComportamientoCargado& comportamiento) override;
    void descargar(ComportamientoCargado& comportamiento,
                   GameObject* owner) override;

    void llamarInicio(ComportamientoCargado& comportamiento,
                      GameObject* owner) override;
    void llamarActualizar(ComportamientoCargado& comportamiento,
                          GameObject* owner, float deltaTime) override;
    void llamarDetener(ComportamientoCargado& comportamiento,
                       GameObject* owner) override;
    void llamarContacto(ComportamientoCargado& comportamiento,
                        GameObject* owner, Collider* propio, Collider* otro,
                        TipoContacto tipo) override;

private:
    static std::string artefacto(const std::string& fuente);

public:
    // Informacion del toolchain para la barra de estado del editor.
    static std::string compiladorRuta();
    static std::string cacheDir();
};

#endif // BACKENDCPP_H