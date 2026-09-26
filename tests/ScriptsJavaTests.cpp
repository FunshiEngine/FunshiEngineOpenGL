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
// Prueba de punta a punta del backend Java: escribe un fuente .java, lo
// compila con javac, arranca el JVM dinamicamente (libjvm via dlopen), crea el
// objeto, refleja e inyecta campos publicos (SerializeField) y ejecuta el
// ciclo iniciar/actualizar/detener. Si no hay JDK/javac sale con SKIP (77).

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Behaviour/Reflection/BehaviourReflection.h"
#include "../FunshiEngineGL/src/Behaviour/ScriptRuntime.h"

using namespace ReflejoScripts;
namespace fs = std::filesystem;

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Stub de la tabla de acceso: en el motor la implementa ScriptGameObject.cpp.
namespace MotorScript {
const ApiScriptGameObject* tablaApi() {
    static const ApiScriptGameObject tabla = {
        [](const void*) { return "stub"; },
        [](const void*) { return 0.0f; },
        [](const void*) { return 0.0f; },
        [](const void*) { return 0.0f; },
        [](void*, float, float, float) {},
        [](void*, float, float, float) {},
        [](void*, float, float, float, float) {},
        [](const char*) {},
    };
    return &tabla;
}
} // namespace MotorScript

static const char* FUENTE_JAVA =
    "public class MiPruebaJava implements Comportamiento {\n"
    "    public float velocidad = 2.0f;\n"
    "    public int vidas = 3;\n"
    "    public boolean activo = true;\n"
    "    public String etiqueta = \"hola\";\n"
    "    @Override public void iniciar(long o) { vidas = 100; }\n"
    "    @Override public void actualizar(long o, double dt) {\n"
    "        vidas += 1;\n"
    "        Nativo.imprimir(\"tick\");\n"
    "    }\n"
    "    @Override public void detener(long o) { vidas = -1; }\n"
    "}\n";

static const ValorCampo* buscar(const std::vector<ValorCampo>& v,
                                const std::string& n) {
    for (const auto& x : v)
        if (x.nombre == n) return &x;
    return nullptr;
}

int main() {
    if (std::system("javac -version > /dev/null 2>&1") != 0) {
        std::cout << "scripts-java-tests: SKIP (no hay javac en PATH)."
                  << std::endl;
        return 77;
    }

    // Carpeta temporal unica por proceso (RAII); `ec` se conserva porque la
    // limpieza explicita de mas abajo lo usa.
    TempPruebas::CarpetaPrueba carpetaDir("funshi_java_test");
    const fs::path dir = carpetaDir.ruta();
    std::error_code ec;
    const std::string fuente = (dir / "MiPruebaJava.java").string();
    {
        std::ofstream f(fuente);
        f << FUENTE_JAVA;
    }

    std::string error;
    ComportamientoCargado comportamiento;
    bool ok = ScriptRuntime::compilarYCargar(fuente, "MiPruebaJava",
                                             comportamiento, error);
    CHECK(ok, "compilarYCargar Java exitoso");
    if (!ok) {
        std::cout << "  Error: " << error << std::endl;
        CHECK(false, "sin error de compilacion Java (ver stdout)");
        fs::remove_all(dir, ec);
        std::cout << "ScriptsJava: " << total << " verificaciones, " << fallos
                  << " fallos" << std::endl;
        return 1;
    }

    CHECK(comportamiento.valido(), "objeto Java creado");
    CHECK(comportamiento.lenguaje == "java", "lenguaje = java");
    CHECK(comportamiento.campos.size() == 4,
          "4 campos publicos reflejados (velocidad/vidas/activo/etiqueta)");

    // Inyectar SerializeField y verificar por lectura.
    std::vector<ValorCampo> valores = ScriptRuntime::extraer(comportamiento);
    for (auto& v : valores) {
        if (v.nombre == "velocidad") v.contenido = 7.0f;
        if (v.nombre == "vidas") v.contenido = 9;
        if (v.nombre == "activo") v.contenido = false;
        if (v.nombre == "etiqueta") v.contenido = std::string("mundo");
    }
    ScriptRuntime::inyectar(comportamiento, valores);

    valores = ScriptRuntime::extraer(comportamiento);
    const ValorCampo* vel = buscar(valores, "velocidad");
    const ValorCampo* vid = buscar(valores, "vidas");
    const ValorCampo* act = buscar(valores, "activo");
    const ValorCampo* eti = buscar(valores, "etiqueta");
    CHECK(vel && vel->como<float>() == 7.0f, "velocidad inyectada = 7");
    CHECK(vid && vid->como<int>() == 9, "vidas inyectadas = 9");
    CHECK(act && act->como<bool>() == false, "activo inyectado = false");
    CHECK(eti && eti->como<std::string>() == "mundo", "etiqueta = mundo");

    // Ciclo: iniciar fija vidas=100, dos actualizar suman 2.
    ScriptRuntime::llamarInicio(comportamiento, nullptr);
    ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
    ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
    valores = ScriptRuntime::extraer(comportamiento);
    vid = buscar(valores, "vidas");
    CHECK(vid && vid->como<int>() == 102, "iniciar(100)+2x actualizar = 102");

    ScriptRuntime::llamarDetener(comportamiento, nullptr);
    valores = ScriptRuntime::extraer(comportamiento);
    vid = buscar(valores, "vidas");
    CHECK(vid && vid->como<int>() == -1, "detener deja vidas = -1");

    ScriptRuntime::descargar(comportamiento);
    CHECK(!comportamiento.valido(), "descargar invalida el comportamiento Java");

    // Hot reload: editar el fuente (misma clase) y volver a cargar en la MISMA
    // JVM debe aplicar la nueva version. El classloader del sistema cachea por
    // nombre (FindClass devolveria la clase vieja); el backend carga con un
    // classloader hijo fresco -> la nueva version manda.
    {
        std::ofstream f(fuente);
        f << "public class MiPruebaJava implements Comportamiento {\n"
             "    public int vidas = 3;\n"
             "    @Override public void iniciar(long o) { vidas = 200; }\n"
             "    @Override public void actualizar(long o, double dt) { vidas += 1; }\n"
             "}\n";
    }
    // Asegurar un mtime estrictamente posterior (gate del hot reload).
    fs::last_write_time(
        fuente, fs::last_write_time(fuente) + std::chrono::seconds(1));

    ComportamientoCargado recargado;
    bool okRecarga = ScriptRuntime::compilarYCargar(
        fuente, "MiPruebaJava", recargado, error);
    CHECK(okRecarga, "recompilar Java tras editar el fuente (misma JVM)");
    if (okRecarga) {
        ScriptRuntime::llamarInicio(recargado, nullptr);
        valores = ScriptRuntime::extraer(recargado);
        const ValorCampo* vid2 = buscar(valores, "vidas");
        CHECK(vid2 && vid2->como<int>() == 200,
              "el reload aplica la NUEVA version (iniciar deja vidas=200)");
        ScriptRuntime::llamarActualizar(recargado, nullptr, 0.016f);
        valores = ScriptRuntime::extraer(recargado);
        vid2 = buscar(valores, "vidas");
        CHECK(vid2 && vid2->como<int>() == 201,
              "actualizar de la clase nueva corre (vidas=201)");
        ScriptRuntime::descargar(recargado);
    }

    fs::remove_all(dir, ec);
    std::cout << "ScriptsJava: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos == 0 ? 0 : 1;
}