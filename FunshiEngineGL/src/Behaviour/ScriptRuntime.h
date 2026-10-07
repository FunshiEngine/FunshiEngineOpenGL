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
#ifndef SCRIPTRUNTIME_H
#define SCRIPTRUNTIME_H

#include <string>

#include "ComportamientoCargado.h"
#include "../Fisicas/ContactoFisico.h"

class BackendScript;

// Registro de backends de lenguaje (por extension del fuente) y despacho del
// ciclo de vida de comportamientos. Es agnostico del lenguaje: el Fuente .cpp
// se resuelve con BackendCpp y el .java con BackendJava (si FUNSHI_JAVA=ON).
class ScriptRuntime {
public:
    static const char* lenguajeDeFuente(const std::string& fuente);
    static BackendScript* backendPara(const std::string& lenguaje);

    // Despachos que usan las demas capas (Script, GameScene, GUI)
    static bool compilarYCargar(const std::string& fuente,
                                const std::string& nombreClase,
                                ComportamientoCargado& salida,
                                std::string& error);
    static void descargar(ComportamientoCargado& c);
    static void descargar(ComportamientoCargado& c, GameObject* owner);
    static void llamarInicio(ComportamientoCargado& c, GameObject* owner);
    static void llamarActualizar(ComportamientoCargado& c, GameObject* owner,
                                 float deltaTime);
    static void llamarDetener(ComportamientoCargado& c, GameObject* owner);
    static void llamarContacto(ComportamientoCargado& c, GameObject* owner,
                               Collider* propio, Collider* otro,
                               TipoContacto tipo);

    // SerializeField: despacha al backend del lenguaje.
    static void inyectar(ComportamientoCargado& c,
                         const std::vector<ReflejoScripts::ValorCampo>& valores);
    static std::vector<ReflejoScripts::ValorCampo> extraer(
        ComportamientoCargado& c);

    // Hot reload: true si el fuente en disco difiere del que fue compilado.
    static bool cambioElFuente(const ComportamientoCargado& c);

    // Cierre de la aplicacion: apaga los backends (p. ej. DestroyJavaVM para
    // el JVM). Debe invocarse despues de descargar todos los comportamientos.
    static void apagarScripts();

    // Resultado de compilar un script (para la barra "Estado" del editor).
    struct ResultadoCarga {
        std::string nombre;
        bool ok = false;
        std::string mensaje;
    };

    // Estado del toolchain externo (compilador/javac/libjvm/cache) para la
    // barra "Estado" del editor. Los campos vacios indican "no detectado".
    struct EstadoHerramientas {
        std::string compiladorCpp;
        std::string javac;
        std::string libjvm;
        bool soporteJava = false; // el motor fue compilado con FUNSHI_JAVA=ON
        bool javacDisponible = false;
        bool jvmArrancada = false;
        std::string cache;
    };
    static EstadoHerramientas estadoHerramientas();
};

#endif // SCRIPTRUNTIME_H