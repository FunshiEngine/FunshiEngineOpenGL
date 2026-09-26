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
#include "ScriptRuntime.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>

#include "Backends/BackendCpp.h"
#include "Backends/BackendScript.h"

#if defined(FUNSHI_JAVA)
#include "Backends/BackendJava.h"
#endif

namespace {
// Instancias unicas de backend. El registro es estatico para que el motor y la
// GUI compartan el mismo ciclo de compilacion/carga (cache de artefactos).
std::map<std::string, std::unique_ptr<BackendScript>>& backendRegistro() {
    static std::map<std::string, std::unique_ptr<BackendScript>> registro;
    if (registro.empty()) {
        registro.insert({"cpp", std::make_unique<BackendCpp>()});
#if defined(FUNSHI_JAVA)
        registro.insert({"java", std::make_unique<BackendJava>()});
#endif
    }
    return registro;
}

std::string mtimeDe(const std::string& ruta) {
    std::error_code ec;
    auto t = std::filesystem::last_write_time(ruta, ec);
    if (ec) return "";
    return std::to_string(t.time_since_epoch().count());
}
} // namespace

const char* ScriptRuntime::lenguajeDeFuente(const std::string& fuente) {
    if (fuente.size() >= 4 &&
        fuente.compare(fuente.size() - 4, 4, ".cpp") == 0)
        return "cpp";
    if (fuente.size() >= 5 &&
        fuente.compare(fuente.size() - 5, 5, ".java") == 0)
        return "java";
    return "";
}

BackendScript* ScriptRuntime::backendPara(const std::string& lenguaje) {
    auto& registro = backendRegistro();
    auto it = registro.find(lenguaje);
    return it != registro.end() ? it->second.get() : nullptr;
}

bool ScriptRuntime::compilarYCargar(const std::string& fuente,
                                    const std::string& nombreClase,
                                    ComportamientoCargado& salida,
                                    std::string& error) {
    const char* lenguaje = lenguajeDeFuente(fuente);
    if (!*lenguaje) {
        error = "El fuente del script debe ser .cpp o .java:\n" + fuente;
        return false;
    }
    BackendScript* backend = backendPara(lenguaje);
    if (!backend) {
        error = "No hay backend para '" + std::string(lenguaje) + "'.";
        if (std::string(lenguaje) == "java") {
            error += " El motor fue compilado sin soporte Java (falta el "
                     "JDK en el build).";
        } else {
            error += " ¿El motor fue compilado con soporte para ese lenguaje?";
        }
        return false;
    }
    return backend->compilarYCargar(fuente, nombreClase, salida, error);
}

void ScriptRuntime::descargar(ComportamientoCargado& c) {
    if (BackendScript* backend = backendPara(c.lenguaje))
        backend->descargar(c);
    else
        c = ComportamientoCargado{};
}

void ScriptRuntime::descargar(ComportamientoCargado& c, GameObject* owner) {
    if (BackendScript* backend = backendPara(c.lenguaje))
        backend->descargar(c, owner);
    else
        c = ComportamientoCargado{};
}

void ScriptRuntime::llamarInicio(ComportamientoCargado& c, GameObject* owner) {
    if (BackendScript* backend = backendPara(c.lenguaje))
        backend->llamarInicio(c, owner);
}

void ScriptRuntime::llamarActualizar(ComportamientoCargado& c, GameObject* owner,
                                     float deltaTime) {
    if (BackendScript* backend = backendPara(c.lenguaje))
        backend->llamarActualizar(c, owner, deltaTime);
}

void ScriptRuntime::llamarDetener(ComportamientoCargado& c, GameObject* owner) {
    if (BackendScript* backend = backendPara(c.lenguaje))
        backend->llamarDetener(c, owner);
}

void ScriptRuntime::inyectar(
    ComportamientoCargado& c,
    const std::vector<ReflejoScripts::ValorCampo>& valores) {
    if (BackendScript* backend = backendPara(c.lenguaje))
        backend->inyectar(c, valores);
}

std::vector<ReflejoScripts::ValorCampo> ScriptRuntime::extraer(
    ComportamientoCargado& c) {
    if (BackendScript* backend = backendPara(c.lenguaje))
        return backend->extraer(c);
    return {};
}

bool ScriptRuntime::cambioElFuente(const ComportamientoCargado& c) {
    if (!c.cargado || c.fuente.empty()) return false;
    return mtimeDe(c.fuente) != c.mtimeFuente;
}

ScriptRuntime::EstadoHerramientas ScriptRuntime::estadoHerramientas() {
    EstadoHerramientas e;
    auto& registro = backendRegistro();
    if (auto it = registro.find("cpp"); it != registro.end())
        e.compiladorCpp = BackendCpp::compiladorRuta();
#if defined(FUNSHI_JAVA)
    e.soporteJava = true;
    if (auto it = registro.find("java"); it != registro.end()) {
        e.javac = BackendJava::javacRuta();
        e.javacDisponible = !e.javac.empty();
        e.libjvm = BackendJava::libjvmRuta();
        e.jvmArrancada = BackendJava::jvmArrancada();
    }
#endif
    e.cache = BackendCpp::cacheDir();
    return e;
}

void ScriptRuntime::apagarScripts() {
#if defined(FUNSHI_JAVA)
    if (BackendJava::jvmArrancada()) BackendJava::apagarJvm();
#endif
}