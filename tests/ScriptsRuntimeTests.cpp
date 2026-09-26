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
// Prueba de punta a punta del runtime de scripts: escribe un fuente C++ en un
// directorio temporal, lo compila con el BackendCpp a .so, lo carga con
// dlopen, inyecta valores SerializeField, ejecuta onInicio/onActualizar/onStop
// y valida el hot reload (recompilacion al cambiar el fuente + mtime).
// Si no hay compilador C++ en el entorno el test sale con SKIP (77) para que
// CI de maquinas minimalistas no lo marque como fallo.

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Behaviour/Reflection/BehaviourReflection.h"
#include "../FunshiEngineGL/src/Behaviour/ScriptRuntime.h"

// Stub de MotorScript::tablaApi(): en el motor real lo implementa
// ScriptGameObject.cpp (necesita GameObject completo); el test solo verifica
// que el backend entrega una tabla no nula a la fabrica. Vive FUERA del guard
// _WIN32 para que BackendCpp.cpp (que lo referencia) enlace tambien en
// Windows: alli el main solo devuelve 77 (SKIP) pero el simbolo debe existir.
namespace MotorScript {
const ApiScriptGameObject* tablaApi() {
    static const ApiScriptGameObject tabla = {
        /* .nombre           = */ [](const void*) { return "stub"; },
        /* .posicionX        = */ [](const void*) { return 0.0f; },
        /* .posicionY        = */ [](const void*) { return 0.0f; },
        /* .posicionZ        = */ [](const void*) { return 0.0f; },
        /* .fijarPosicion    = */
        [](void*, float, float, float) {},
        /* .fijarEscala      = */
        [](void*, float, float, float) {},
        /* .fijarRotacionEjes = */
        [](void*, float, float, float, float) {},
        /* .imprimirConsola  = */
        [](const char*) {},
    };
    return &tabla;
}
} // namespace MotorScript

#ifdef _WIN32
int main() {
    std::cout << "scripts-runtime-tests: SKIP en Windows (requiere cl.exe con "
                 "entorno de Visual Studio)."
              << std::endl;
    return 77;
}
#else

using namespace ReflejoScripts;

int total = 0;
int fallos = 0;

static bool casiIgual(float a, float b) {
    return std::fabs(a - b) < 1e-5f;
}

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

namespace fs = std::filesystem;

static std::string fuenteScript(const std::string& clase) {
    return
        "#include \"Behaviour/IScriptBehaviour.h\"\n"
        "\n"
        "class FUNSHI_NOMBRE_CLASE : public IScriptBehaviour {\n"
        "public:\n"
        "    float velocidad = 2.0f;\n"
        "    int vidas = 3;\n"
        "    int pasos = 0;\n"
        "\n"
        "    REFLECT_INICIO(FUNSHI_NOMBRE_CLASE)\n"
        "        REFLECT_CAMPO(velocidad)\n"
        "        REFLECT_CAMPO(vidas)\n"
        "        REFLECT_CAMPO(pasos)\n"
        "    REFLECT_FIN\n"
        "\n"
        "    void onStart(GameObject* owner) override { (void)owner; pasos = 100; }\n"
        "    void onUpdate(GameObject* owner, float deltaTime) override {\n"
        "        (void)owner; (void)deltaTime;\n"
        "        pasos += 1;\n"
        "        if (api) api->imprimirConsola(\"hola\");\n"
        "    }\n"
        "    void onStop(GameObject* owner) override { (void)owner; vidas = -1; }\n"
        "\n"
        "    std::vector<::ReflejoScripts::DefCampo>\n"
        "    camposReflejados() const override { return reflexion(); }\n"
        "};\n"
        "\n"
        "extern \"C\" IScriptBehaviour* FUNSHI_CREAR_COMPORTAMIENTO(\n"
        "    const MotorScript::ApiScriptGameObject* api) {\n"
        "    (void)api;\n"
        "    return new FUNSHI_NOMBRE_CLASE();\n"
        "}\n";
}

static bool hayCompilador() {
    const char* cxx = std::getenv("FUNSHI_CXX");
    const std::string cmd =
        std::string(cxx && *cxx ? cxx : FUNSHI_CXX_COMPILER) +
        " --version > /dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

static void escribirFuente(const std::string& ruta,
                           const std::string& contenido) {
    std::ofstream f(ruta);
    f << contenido;
}

static void tocarFuente(const std::string& ruta) {
    std::error_code ec;
    auto t = fs::last_write_time(ruta, ec);
    if (!ec) fs::last_write_time(ruta, t + std::chrono::seconds(5), ec);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main() {
    if (!hayCompilador()) {
        std::cout << "scripts-runtime-tests: SKIP (no hay compilador C++)."
                  << std::endl;
        return 77;
    }

    // 1. Escribir el fuente en un directorio temporal (unico por proceso,
    // limpieza automatica al salir); `ec` lo usan los remove_all posteriores.
    TempPruebas::CarpetaPrueba carpetaDir("funshi_scripts_runtime_test");
    const fs::path dir = carpetaDir.ruta();
    std::error_code ec;
    const std::string fuente = (dir / "MiPrueba.cpp").string();
    std::string error;

    ComportamientoCargado comportamiento;
    const std::string clase = "MiPrueba";

    // 2. Compilar y cargar por primera vez.
    escribirFuente(fuente, fuenteScript(clase));
    bool ok = ScriptRuntime::compilarYCargar(fuente, clase, comportamiento,
                                             error);
    CHECK(ok, "compilarYCargar exitoso");
    if (ok) {
        CHECK(comportamiento.valido(), "comportamiento valido (instancia creada)");
        CHECK(comportamiento.lenguaje == "cpp", "lenguaje = cpp");
        CHECK(comportamiento.campos.size() == 3,
              "exactamente 3 campos reflejados (velocidad/vidas/pasos)");
        CHECK(!error.empty() == false, "sin error al compilar");
    } else {
        std::cout << "  Error del backend: " << error << std::endl;
        CHECK(false, "no hubo error de compilacion (ver stdout)");
    }

    if (comportamiento.valido()) {
        // 3. Inyectar valores SerializeField y verificarlos por lectura.
        std::vector<ValorCampo> valores = extraerCampos(comportamiento);
        for (auto& v : valores) {
            if (v.nombre == "velocidad") v.contenido = 7.0f;
            if (v.nombre == "vidas") v.contenido = 9;
        }
        inyectarCampos(comportamiento, valores);

        valores = extraerCampos(comportamiento);
        for (const auto& v : valores) {
            if (v.nombre == "velocidad")
                CHECK(casiIgual(v.como<float>(), 7.0f),
                      "velocidad inyectada = 7");
            if (v.nombre == "vidas")
                CHECK(v.como<int>() == 9, "vidas inyectadas = 9");
        }

        // 4. Ciclo de vida: onStart + onUpdate + onStop.
        ScriptRuntime::llamarInicio(comportamiento, nullptr);
        ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
        ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores) {
            if (v.nombre == "pasos")
                CHECK(v.como<int>() == 102, "onStart(100)+2x onUpdate(+1) = pasos 102");
        }
        ScriptRuntime::llamarDetener(comportamiento, nullptr);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores) {
            if (v.nombre == "vidas")
                CHECK(v.como<int>() == -1, "onStop deja vidas = -1");
        }
    }

    // 5. Hot reload: reescribir el fuente agregando un campo nuevo y tocando
    // el mtime; descargar y recargar. Los valores conocidos se conservan.
    if (comportamiento.valido()) {
        std::string fuente2 =
            "#include \"Behaviour/IScriptBehaviour.h\"\n"
            "class FUNSHI_NOMBRE_CLASE : public IScriptBehaviour {\n"
            "public:\n"
            "    float velocidad = 2.0f;\n"
            "    int vidas = 3;\n"
            "    int pasos = 0;\n"
            "    int nuevos = 0;\n"
            "    REFLECT_INICIO(FUNSHI_NOMBRE_CLASE)\n"
            "        REFLECT_CAMPO(velocidad)\n"
            "        REFLECT_CAMPO(vidas)\n"
            "        REFLECT_CAMPO(pasos)\n"
            "        REFLECT_CAMPO(nuevos)\n"
            "    REFLECT_FIN\n"
            "    void onStart(GameObject* o) override { (void)o; }\n"
            "    void onUpdate(GameObject* o, float d) override { (void)o; (void)d; }\n"
            "    void onStop(GameObject* o) override { (void)o; }\n"
            "    std::vector<::ReflejoScripts::DefCampo>\n"
            "    camposReflejados() const override { return reflexion(); }\n"
            "};\n"
            "extern \"C\" IScriptBehaviour* FUNSHI_CREAR_COMPORTAMIENTO(\n"
            "    const MotorScript::ApiScriptGameObject* api) {\n"
            "    (void)api;\n"
            "    return new FUNSHI_NOMBRE_CLASE();\n"
            "}\n";
        escribirFuente(fuente, fuente2);
        tocarFuente(fuente);

        CHECK(ScriptRuntime::cambioElFuente(comportamiento),
              "cambioElFuente detecta el mtime nuevo");

        std::vector<ValorCampo> antes = extraerCampos(comportamiento);
        ScriptRuntime::descargar(comportamiento, nullptr);
        CHECK(!comportamiento.valido(), "descargar invalida el comportamiento");

        ComportamientoCargado reload;
        ok = ScriptRuntime::compilarYCargar(fuente, clase, reload, error);
        CHECK(ok, "recarga tras hot reload");
        if (ok) {
            CHECK(reload.campos.size() == 4, "4 campos tras agregar 'nuevos'");
            // Los valores antiguos se conservan (emparejado por nombre).
            inyectarCampos(reload, antes);
            std::vector<ValorCampo> despues = extraerCampos(reload);
            for (const auto& v : despues) {
                if (v.nombre == "velocidad")
                    CHECK(casiIgual(v.como<float>(), 7.0f),
                          "velocidad conservada tras hot reload");
                if (v.nombre == "nuevos")
                    CHECK(v.como<int>() == 0, "'nuevos' default = 0");
            }
            ScriptRuntime::descargar(reload, nullptr);
        }
    }

    // 6. Limpieza.
    fs::remove_all(dir, ec);

    std::cout << "ScriptsRuntime: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos == 0 ? 0 : 1;
}
#endif