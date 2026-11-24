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
#include "ScriptsRuntimeTestsDetalle.h"

namespace ScriptsRuntimeTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace ScriptsRuntimeTestsDetalle
using namespace ScriptsRuntimeTestsDetalle;

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
        /* .rotacionAngulo = */ [](const void*) { return 0.0f; },
        /* .rotacionEjeX = */ [](const void*) { return 0.0f; },
        /* .rotacionEjeY = */ [](const void*) { return 1.0f; },
        /* .rotacionEjeZ = */ [](const void*) { return 0.0f; },
        /* .escalaX = */ [](const void*) { return 1.0f; },
        /* .escalaY = */ [](const void*) { return 1.0f; },
        /* .escalaZ = */ [](const void*) { return 1.0f; },
        /* .fijarVelocidadHorizontal = */ [](void*, float, float) {
            return false;
        },
        /* .saltar = */ [](void*, float) { return false; },
        /* .etiqueta = */ [](const void*) { return ""; },
        /* .tieneEtiqueta = */ [](const void*, const char*) { return false; },
        /* .objetoDeCollider = */ [](const void*) -> void* { return nullptr; },
        /* .masa = */ [](const void*) { return 0.0f; },
        /* .fijarMasa = */ [](void*, float) { return false; },
        /* .usaGravedad = */ [](const void*) { return false; },
        /* .fijarUsoGravedad = */ [](void*, bool) { return false; },
        /* .escalaGravedad = */ [](const void*) { return 0.0f; },
        /* .fijarEscalaGravedad = */ [](void*, float) { return false; },
        /* .friccion = */ [](const void*) { return 0.0f; },
        /* .fijarFriccion = */ [](void*, float) { return false; },
        /* .posicionCongelada = */ [](const void*, int) { return false; },
        /* .fijarFreezePosicion = */ [](void*, bool, bool, bool) {
            return false;
        },
        /* .rotacionCongelada = */ [](const void*, int) { return false; },
        /* .fijarFreezeRotacion = */ [](void*, bool, bool, bool) {
            return false;
        },
        /* .version = */ 5,
    };
    return &tabla;
}

}  // namespace MotorScript

bool casiIgual(float a, float b) {
    return std::fabs(a - b) < 1e-5f;
}

std::string fuenteScript(const std::string& clase) {
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
        "    void onCollisionEnter(GameObject*, Collider*, Collider*) override {\n"
        "        pasos += 10;\n"
        "    }\n"
        "    void onCollisionStay(GameObject*, Collider*, Collider*) override {\n"
        "        pasos += 20;\n"
        "    }\n"
        "    void onCollisionExit(GameObject*, Collider*, Collider*) override {\n"
        "        pasos += 30;\n"
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

bool hayCompilador() {
    const char* cxx = std::getenv("FUNSHI_CXX");
    const std::string ruta = cxx && *cxx ? cxx : FUNSHI_CXX_COMPILER;
    // Sondeo SIN shell (H-3 nivel 2): Proceso::ejecutar tira el output al
    // dispositivo nulo de la plataforma (H-14: NUL / /dev/null — ahora
    // abierto por el runner en vez de escrito por cmd.exe). El flag depende
    // de la familia: g++/clang++ entienden --version; cl.exe usa /?, porque
    // `cl --version` no existe y dejaba el test en skip. Con MSVC el test
    // puede seguir: el entorno del toolset (INCLUDE/LIB/PATH) lo harvesta
    // BackendCpp el mismo.
    const std::string flag = CompilacionCpp::familiaCompilador() ==
                                     CompilacionCpp::Familia::Msvc
                                 ? "/?"
                                 : "--version";
    return SondeoToolchain::sondear(ruta, flag);
}

void escribirFuente(const std::string& ruta,
                           const std::string& contenido) {
    std::ofstream f(ruta);
    f << contenido;
}

void tocarFuente(const std::string& ruta) {
    std::error_code ec;
    auto t = fs::last_write_time(ruta, ec);
    if (!ec) fs::last_write_time(ruta, t + std::chrono::seconds(5), ec);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Fija o quita una variable de entorno de forma portable: POSIX usa setenv /
// unsetenv y Windows _putenv_s. El valor nullptr borra la variable.
void fijarVariable(const char* nombre, const char* valor) {
#if defined(_WIN32)
    _putenv_s(nombre, valor ? valor : "");
#else
    if (valor)
        setenv(nombre, valor, 1);
    else
        unsetenv(nombre);
#endif
}
