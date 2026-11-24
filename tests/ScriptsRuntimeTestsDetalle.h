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
// Preambulo compartido por las unidades de ScriptsRuntimeTests.cpp (generado al dividirlo).
#ifndef SCRIPTSRUNTIMETESTSDETALLE_H
#define SCRIPTSRUNTIMETESTSDETALLE_H

// Prueba de punta a punta del runtime de scripts: escribe un fuente C++ en un
// directorio temporal, lo compila con el BackendCpp a .so, lo carga con
// dlopen, inyecta valores SerializeField, ejecuta onInicio/onActualizar/onStop
// y valida el hot reload (recompilacion al cambiar el fuente + mtime) y la
// carga de un segundo componente sobre el MISMO fuente (reutiliza el
// artefacto al dia sin volver a enlazar).
// Si no hay compilador C++ en el entorno el test sale con SKIP (77) para que
// CI de maquinas minimalistas no lo marque como fallo.

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/BackendCpp.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/ComandoCompilacionCpp.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/SondeoToolchain.h"
#include "../FunshiEngineGL/src/Behaviour/Reflection/BehaviourReflection.h"
#include "../FunshiEngineGL/src/Behaviour/ScriptRuntime.h"
#include "../FunshiEngineGL/src/Diagnosticos/RegistroDiagnosticos.h"

namespace MotorScript {

// Stub de MotorScript::tablaApi(): en el motor real lo implementa
// ScriptGameObject.cpp (necesita GameObject completo); el test solo verifica
// que el backend entrega una tabla no nula a la fabrica. Vive fuera de
// cualquier guard de plataforma: BackendCpp.cpp (que lo referencia) se
// compila en todas, y el main tampoco se salta por SO sino por familia de
// toolchain (ver abajo).

}  // namespace MotorScript

using namespace ReflejoScripts;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

namespace fs = std::filesystem;

// Estado de la unidad original: una sola instancia (se define en ScriptsRuntimeTests.cpp).

namespace ScriptsRuntimeTestsDetalle {

extern int total;
extern int fallos;

}  // namespace ScriptsRuntimeTestsDetalle
using namespace ScriptsRuntimeTestsDetalle;

// Funciones repartidas entre las unidades.

namespace MotorScript {

const ApiScriptGameObject* tablaApi();

}  // namespace MotorScript

bool casiIgual(float a, float b);
std::string fuenteScript(const std::string& clase);
bool hayCompilador();
void escribirFuente(const std::string& ruta, const std::string& contenido);
void tocarFuente(const std::string& ruta);
void fijarVariable(const char* nombre, const char* valor);

#endif
