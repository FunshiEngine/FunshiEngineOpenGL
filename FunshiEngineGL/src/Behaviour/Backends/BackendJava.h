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
#ifndef BACKENDJAVA_H
#define BACKENDJAVA_H

#include "BackendScript.h"

// Backend Java: compila el .java con javac a un directorio de clases y ejecuta
// el comportamiento sobre un JVM cargado DINAMICAMENTE (dlopen de libjvm.so /
// jvm.dll). Solo se compila cuando FUNSHI_JAVA=ON (requiere jni.h del JDK);
// aun asi el JVM es opcional en runtime: si no hay libjvm disponible el error
// se reporta en la GUI.
class BackendJava : public BackendScript {
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

    void inyectar(ComportamientoCargado& comportamiento,
                  const std::vector<ReflejoScripts::ValorCampo>& valores) override;
    std::vector<ReflejoScripts::ValorCampo> extraer(
        ComportamientoCargado& comportamiento) override;

    // Informacion del toolchain para la barra de estado del editor (rutas
    // resueltas en runtime y si la JVM ya esta arrancada).
    static std::string javacRuta();
    static std::string libjvmRuta();
    static bool jvmArrancada();
    static std::string cacheDir();

    // Apaga el JVM embebido (DestroyJavaVM) al cerrar la aplicacion. Libera
    // todo el estado interno del JVM que, de otro modo, LeakSanitizer reporta
    // como fugas; debe llamarse DESPUES de descargar todos los scripts.
    static void apagarJvm();
};

#endif // BACKENDJAVA_H