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
#ifndef BACKENDSCRIPT_H
#define BACKENDSCRIPT_H

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#endif

#include <string>

#include "../ComportamientoCargado.h"

// Backend de un lenguaje de script: compila el fuente a un artefacto, lo
// carga, instancia el comportamiento y ejecuta su ciclo. Los mensajes de error
// de compilacion se devuelven en `error` para mostrarlos en la GUI.
class BackendScript {
public:
    virtual ~BackendScript() = default;

    virtual const char* lenguaje() const = 0;

    // Compila `fuente` y carga/instancia el comportamiento en `salida`.
    // `nombreClase` es la clase a instanciar (convencion del template).
    virtual bool compilarYCargar(const std::string& fuente,
                                 const std::string& nombreClase,
                                 ComportamientoCargado& salida,
                                 std::string& error) = 0;

    // Libera el artefacto y la instancia (invalida `salida`).
    virtual void descargar(ComportamientoCargado& comportamiento) = 0;
    virtual void descargar(ComportamientoCargado& comportamiento,
                           GameObject* owner) = 0;

    virtual void llamarInicio(ComportamientoCargado& comportamiento,
                              GameObject* owner) = 0;
    virtual void llamarActualizar(ComportamientoCargado& comportamiento,
                                  GameObject* owner, float deltaTime) = 0;
    virtual void llamarDetener(ComportamientoCargado& comportamiento,
                               GameObject* owner) = 0;

    // SerializeField: inyecta/extrae el arbol de valores sobre la instancia.
    // El default vale para instancias con memoria C++ contigua (BackendCpp);
    // Java lo redefine para usar reflexion JNI.
    virtual void inyectar(ComportamientoCargado& comportamiento,
                          const std::vector<ReflejoScripts::ValorCampo>& valores) {
        inyectarCampos(comportamiento, valores);
    }
    virtual std::vector<ReflejoScripts::ValorCampo> extraer(
        ComportamientoCargado& comportamiento) {
        return extraerCampos(comportamiento);
    }
};

#endif // BACKENDSCRIPT_H