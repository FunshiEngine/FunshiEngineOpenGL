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

// Stub de MotorScript::tablaApi(): en el motor real lo implementa
// ScriptGameObject.cpp (necesita GameObject completo); el test solo verifica
// que el backend entrega una tabla no nula a la fabrica. Vive fuera de
// cualquier guard de plataforma: BackendCpp.cpp (que lo referencia) se
// compila en todas, y el main tampoco se salta por SO sino por familia de
// toolchain (ver abajo).
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
} // namespace MotorScript

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

static bool hayCompilador() {
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

// Fija o quita una variable de entorno de forma portable: POSIX usa setenv /
// unsetenv y Windows _putenv_s. El valor nullptr borra la variable.
static void fijarVariable(const char* nombre, const char* valor) {
#if defined(_WIN32)
    _putenv_s(nombre, valor ? valor : "");
#else
    if (valor)
        setenv(nombre, valor, 1);
    else
        unsetenv(nombre);
#endif
}

int main() {
    // El skip es por FAMILIA de toolchain, no por SO (H-14): con MSVC el
    // compilador necesita el entorno de Visual Studio (vcvars: INCLUDE, LIB,
    // link.exe), asi que desde un shell normal no hay forma de correrlo. Con
    // GCC/Clang el test corre en cualquier plataforma, Windows incluido
    // (MinGW): la familia decide los flags, no el sistema operativo.
    if (CompilacionCpp::familiaCompilador() == CompilacionCpp::Familia::Msvc) {
        std::cout << "scripts-runtime-tests: SKIP (toolchain MSVC: requiere "
                     "el entorno de Visual Studio)."
                  << std::endl;
        return 77;
    }
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
        ScriptRuntime::llamarContacto(comportamiento, nullptr, nullptr, nullptr,
                                      TipoContacto::Inicio);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores)
            if (v.nombre == "pasos")
                CHECK(v.como<int>() == 112,
                      "el callback de inicio de contacto llega al script C++");
        ScriptRuntime::llamarContacto(comportamiento, nullptr, nullptr, nullptr,
                                      TipoContacto::Persistencia);
        ScriptRuntime::llamarContacto(comportamiento, nullptr, nullptr, nullptr,
                                      TipoContacto::Fin);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores)
            if (v.nombre == "pasos")
                CHECK(v.como<int>() == 162,
                      "los callbacks de persistencia y fin llegan al script C++");
        ScriptRuntime::llamarDetener(comportamiento, nullptr);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores) {
            if (v.nombre == "vidas")
                CHECK(v.como<int>() == -1, "onStop deja vidas = -1");
        }
    }

    // 5. Segundo componente sobre el MISMO fuente: la cola de compilacion de
    //    la escena lo entrega con un ComportamientoCargado vacio (estado por
    //    componente), mientras el primero sigue cargado en este proceso. El
    //    artefacto ya esta al dia, asi que no hay que volver a enlazarlo:
    //    reescribir una imagen que esta cargada es un error de escritura que
    //    el enlazador reporta como permiso denegado.
    if (comportamiento.valido()) {
        std::error_code ecArtefacto;
        const auto mtimeAntes =
            fs::last_write_time(comportamiento.artefacto, ecArtefacto);
        CHECK(!ecArtefacto, "el artefacto del primer componente existe");

        const std::string claveCacheAnterior =
            fs::weakly_canonical(fuente).string() + "|" +
            BackendCpp::compiladorRuta() + "|" +
            CompilacionCpp::flagsCompilador(std::string());
        const fs::path artefactoCacheAnterior =
            fs::path(BackendCpp::cacheDir()) /
            ("script_" +
             std::to_string(std::hash<std::string>{}(claveCacheAnterior)) +
             fs::path(comportamiento.artefacto).extension().string());
        CHECK(artefactoCacheAnterior.string() != comportamiento.artefacto,
              "la clave actual del cache no coincide con la version sin API");
        if (artefactoCacheAnterior.string() != comportamiento.artefacto) {
            std::error_code ecCopia;
            fs::copy_file(comportamiento.artefacto, artefactoCacheAnterior,
                          fs::copy_options::overwrite_existing, ecCopia);
            CHECK(!ecCopia,
                  "se puede preparar un artefacto de cache sin version API");
            if (!ecCopia)
                fs::last_write_time(
                    artefactoCacheAnterior,
                    fs::file_time_type::clock::now() +
                        std::chrono::hours(1),
                    ecCopia);
        }

        ComportamientoCargado segundo;
        std::string errorSegundo;
        const bool okSegundo = ScriptRuntime::compilarYCargar(
            fuente, clase, segundo, errorSegundo);
        CHECK(okSegundo, "segundo componente sobre el mismo fuente carga bien");
        if (!okSegundo)
            std::cout << "  Error del backend: " << errorSegundo << std::endl;
        CHECK(segundo.valido(), "segundo comportamiento valido");
        CHECK(segundo.artefacto != artefactoCacheAnterior.string(),
              "un binario del cache anterior no se carga aunque este vigente");
        if (!ecArtefacto) {
            std::error_code ecDespues;
            const auto mtimeDespues =
                fs::last_write_time(comportamiento.artefacto, ecDespues);
            CHECK(!ecDespues && mtimeDespues == mtimeAntes,
                  "el artefacto no se volvio a escribir (sigue al dia)");
        }
        ScriptRuntime::descargar(segundo, nullptr);
    }

    // 6. Hot reload: reescribir el fuente agregando un campo nuevo y tocando
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

    // 7. Cabeceras del motor ausentes: el backend avisa con su propio mensaje
    //    en vez de lanzar el compilador contra un -I a una ruta inexistente
    //    (que solo daria el "No such file or directory" del toolchain).
    {
        const std::string fuenteSinCabeceras =
            (dir / "SinCabeceras.cpp").string();
        escribirFuente(fuenteSinCabeceras, fuenteScript("SinCabeceras"));
        const std::string rutaMuerta = (dir / "no_existe").string();
        fijarVariable("FUNSHI_SRC_DIR", rutaMuerta.c_str());
        ComportamientoCargado sinCabeceras;
        std::string errorCabeceras;
        const bool okSin = ScriptRuntime::compilarYCargar(
            fuenteSinCabeceras, "SinCabeceras", sinCabeceras, errorCabeceras);
        fijarVariable("FUNSHI_SRC_DIR", nullptr);
        CHECK(!okSin, "sin cabeceras del motor no se compila el script");
        CHECK(!sinCabeceras.valido(), "no queda comportamiento cargado");
        CHECK(errorCabeceras.find("cabeceras") != std::string::npos,
              "el aviso es del motor (habla de las cabeceras), no del "
              "compilador");
    }

    // 8. Limpieza.
    fs::remove_all(dir, ec);

    std::cout << "ScriptsRuntime: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos == 0 ? 0 : 1;
}