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
#ifndef ISCRIPTBEHAVIOUR_H
#define ISCRIPTBEHAVIOUR_H
#include "Reflection/BehaviourReflection.h"
#include "ScriptGameObject.h"

// Marca de exportacion de la fabrica FUNSHI_CREAR_COMPORTAMIENTO (H-15).
// En Windows/MSVC una funcion de una .dll NO se exporta sola: sin
// __declspec(dllexport) la .dll compila pero su tabla de exportaciones no
// contiene la fabrica, GetProcAddress devuelve NULL y el motor reporta "El
// .so no exporta 'FUNSHI_CREAR_COMPORTAMIENTO'". En MinGW/Linux/macOS los
// simbolos se exportan por defecto, por eso el bug no se veia ahi.
// Uso en el fuente del script:
//
//   extern "C" FUNSHI_COMPORTAMIENTO_EXPORT IScriptBehaviour*
//   FUNSHI_CREAR_COMPORTAMIENTO(const MotorScript::ApiScriptGameObject* api) {
//       ...
//   }
#if defined(_WIN32) && defined(_MSC_VER)
#define FUNSHI_COMPORTAMIENTO_EXPORT __declspec(dllexport)
#else
#define FUNSHI_COMPORTAMIENTO_EXPORT
#endif

class IScriptBehaviour {
public:
    // Tabla de acceso a GameObject que inyecta el motor al cargar el script.
    // Los scripts la usan como `if (api) api->posicionX(owner)`.
    const MotorScript::ApiScriptGameObject* api = nullptr;

    // Tabla de servicios de escena (audio, busqueda, teclado), inyectada por
    // el motor al cargar. Los scripts la usan como
    // `if (servicios) servicios->reproducirSonido(...)`; comprobar version con
    // `servicios->version >= 1` si se quiere blindar contra builds viejas.
    const MotorScript::ScriptServices* servicios = nullptr;

    virtual ~IScriptBehaviour() {}
    virtual void onStart(GameObject* owner) = 0;
    virtual void onUpdate(GameObject* owner, float deltaTime) = 0;

    // Opcional: recibe la tabla de punteros que implementa el motor en la TU
    // del ejecutable (así el .so del script no enlaza símbolos del motor).
    virtual void conectarApi(const MotorScript::ApiScriptGameObject* tabla) {
        api = tabla;
    }

    // Ciclo opcional invocado al salir de play mode (limpieza del script).
    virtual void onStop(GameObject* owner) { (void)owner; }

    // Campos editables (SerializeField). Los scripts que usan las macros
    // REFLECT_* la implementan automaticamente; sin reflexion devuelve vacio.
    virtual std::vector<ReflejoScripts::DefCampo> camposReflejados() const {
        return {};
    }
};
#endif