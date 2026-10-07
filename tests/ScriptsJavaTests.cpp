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
#include "../FunshiEngineGL/src/Behaviour/Backends/BackendJava.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/SondeoToolchain.h"
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
namespace {
float posicion[3] = {1.0f, 2.0f, 3.0f};
float escala[3] = {1.5f, 2.5f, 3.5f};
float rotacion[4] = {0.25f, 0.0f, 1.0f, 0.0f};
int handleSonidoDetenido = -1;
std::string textoImpreso;
}

const ApiScriptGameObject* tablaApi() {
    static const ApiScriptGameObject tabla = {
        [](const void* objeto) {
            return objeto ==
                           reinterpret_cast<void*>(
                               static_cast<intptr_t>(0x1234))
                       ? "meta"
                       : "stub";
        },
        [](const void*) { return posicion[0]; },
        [](const void*) { return posicion[1]; },
        [](const void*) { return posicion[2]; },
        [](void*, float x, float y, float z) {
            posicion[0] = x;
            posicion[1] = y;
            posicion[2] = z;
        },
        [](void*, float x, float y, float z) {
            escala[0] = x;
            escala[1] = y;
            escala[2] = z;
        },
        [](void*, float a, float x, float y, float z) {
            rotacion[0] = a;
            rotacion[1] = x;
            rotacion[2] = y;
            rotacion[3] = z;
        },
        [](const char* texto) { textoImpreso = texto ? texto : ""; },
        [](const void*) { return rotacion[0]; },
        [](const void*) { return rotacion[1]; },
        [](const void*) { return rotacion[2]; },
        [](const void*) { return rotacion[3]; },
        [](const void*) { return escala[0]; },
        [](const void*) { return escala[1]; },
        [](const void*) { return escala[2]; },
        2,
    };
    return &tabla;
}

const ScriptServices* tablaServicios() {
    static const ScriptServices servicios = {
        [](const char* clip, float volumen, bool bucle) {
            return clip && std::string(clip) == "disparo.wav" &&
                           volumen == 0.5f && !bucle
                       ? 73
                       : -1;
        },
        [](int handle) { handleSonidoDetenido = handle; },
        [](const char* nombre) {
            return nombre && std::string(nombre) == "Meta"
                       ? reinterpret_cast<void*>(static_cast<intptr_t>(0x1234))
                       : nullptr;
        },
        [](const char* tecla) { return tecla && std::string(tecla) == "W"; },
        [](const char* tecla) { return tecla && std::string(tecla) == "SPACE"; },
        [](const char* tecla) { return tecla && std::string(tecla) == "D0"; },
        []() { return 12.5f; },
        []() { return -4.0f; },
        2,
    };
    return &servicios;
}
} // namespace MotorScript

static const char* FUENTE_JAVA =
    "public class MiPruebaJava implements Comportamiento {\n"
    "    public float velocidad = 2.0f;\n"
    "    public int vidas = 3;\n"
    "    public boolean activo = true;\n"
    "    public String etiqueta = \"hola\";\n"
    "    public String nombreObjeto;\n"
    "    public float x, y, z, giro, ejeX, ejeY, ejeZ, escalaX, escalaY, escalaZ;\n"
    "    public int sonido;\n"
    "    public long meta;\n"
    "    public String nombreMeta;\n"
    "    public boolean sostenida, pulsada, soltada, servicioOnStop;\n"
    "    public float mouseX, mouseY;\n"
    "    @Override public void iniciar(long o) {\n"
    "        vidas = 100;\n"
    "        nombreObjeto = Nativo.nombre(o);\n"
    "        x = Nativo.posicionX(o); y = Nativo.posicionY(o); z = Nativo.posicionZ(o);\n"
    "        Nativo.fijarPosicion(o, x + 1, y + 2, z + 3);\n"
    "        Nativo.fijarEscala(o, 4, 5, 6);\n"
    "        Nativo.fijarRotacion(o, 0.5f, 1, 0, 0);\n"
    "        giro = Nativo.rotacionAngulo(o); ejeX = Nativo.rotacionEjeX(o);\n"
    "        ejeY = Nativo.rotacionEjeY(o); ejeZ = Nativo.rotacionEjeZ(o);\n"
    "        escalaX = Nativo.escalaX(o); escalaY = Nativo.escalaY(o); escalaZ = Nativo.escalaZ(o);\n"
    "        sonido = Nativo.reproducirSonido(\"disparo.wav\", 0.5f, false);\n"
    "        meta = Nativo.objetoPorNombre(\"Meta\");\n"
    "        nombreMeta = Nativo.nombre(meta);\n"
    "        sostenida = Nativo.teclaSostiene(\"W\");\n"
    "        pulsada = Nativo.teclaPresionada(\"SPACE\");\n"
    "        soltada = Nativo.teclaSoltada(\"D0\");\n"
    "        mouseX = Nativo.deltaMouseX(); mouseY = Nativo.deltaMouseY();\n"
    "        Nativo.detenerSonido(sonido);\n"
    "    }\n"
    "    @Override public void actualizar(long o, double dt) {\n"
    "        vidas += 1;\n"
    "        Nativo.imprimir(\"acci\\u00F3n\");\n"
    "    }\n"
    "    @Override public void detener(long o) { vidas = -1; servicioOnStop = Nativo.teclaSostiene(\"W\"); }\n"
    "}\n";

static const ValorCampo* buscar(const std::vector<ValorCampo>& v,
                                const std::string& n) {
    for (const auto& x : v)
        if (x.nombre == n) return &x;
    return nullptr;
}

// Sondeo del toolchain Java con la MISMA resolucion que usa el backend
// (BackendJava::javacRuta: JAVAC > raices del JDK como JAVA_HOME > default
// horneado por CMake). Sin shell (H-3 nivel 2): sondear() ejecuta javac
// directamente y el output va al dispositivo nulo de la plataforma (H-14:
// NUL / /dev/null, ahora abierto por el runner en vez de escrito por
// cmd.exe). Si la resolucion da una ruta (lo normal), tampoco hay shell que
// intermedie — lo que se verifica es que javac realmente corre.
static bool hayJavac() {
    const std::string ruta = BackendJava::javacRuta();
    return SondeoToolchain::sondear(ruta, "-version");
}

int main() {
    if (!hayJavac()) {
        std::cout << "scripts-java-tests: SKIP (no hay javac; resuelto por "
                     "JAVAC/JAVA_HOME/PATH y no encontrado)."
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
    CHECK(comportamiento.campos.size() == 23,
          "campos publicos Java soportados reflejados");

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
    valores = ScriptRuntime::extraer(comportamiento);
    const ValorCampo* nombreObjeto = buscar(valores, "nombreObjeto");
    const ValorCampo* posX = buscar(valores, "x");
    const ValorCampo* posY = buscar(valores, "y");
    const ValorCampo* posZ = buscar(valores, "z");
    const ValorCampo* giro = buscar(valores, "giro");
    const ValorCampo* ejeX = buscar(valores, "ejeX");
    const ValorCampo* ejeY = buscar(valores, "ejeY");
    const ValorCampo* ejeZ = buscar(valores, "ejeZ");
    const ValorCampo* escalaZ = buscar(valores, "escalaZ");
    const ValorCampo* sonido = buscar(valores, "sonido");
    const ValorCampo* nombreMeta = buscar(valores, "nombreMeta");
    const ValorCampo* sostenida = buscar(valores, "sostenida");
    const ValorCampo* pulsada = buscar(valores, "pulsada");
    const ValorCampo* soltada = buscar(valores, "soltada");
    const ValorCampo* mouseX = buscar(valores, "mouseX");
    const ValorCampo* mouseY = buscar(valores, "mouseY");
    CHECK(nombreObjeto && nombreObjeto->como<std::string>() == "stub",
          "Java accede al nombre mediante la API del objeto");
    CHECK(posX && posY && posZ && posX->como<float>() == 1.0f &&
              posY->como<float>() == 2.0f && posZ->como<float>() == 3.0f,
          "Java consulta la posicion local por eje");
    CHECK(MotorScript::posicion[0] == 2.0f &&
              MotorScript::posicion[1] == 4.0f &&
              MotorScript::posicion[2] == 6.0f,
          "Java fija la posicion local mediante la API");
    CHECK(MotorScript::escala[0] == 4.0f &&
              MotorScript::escala[1] == 5.0f &&
              MotorScript::escala[2] == 6.0f,
          "Java fija y consulta escala por eje");
    CHECK(MotorScript::rotacion[0] == 0.5f &&
              MotorScript::rotacion[1] == 1.0f &&
              MotorScript::rotacion[2] == 0.0f &&
              MotorScript::rotacion[3] == 0.0f,
          "Java fija y consulta angulo y eje de rotacion");
    CHECK(giro && ejeX && ejeY && ejeZ && escalaZ &&
              giro->como<float>() == 0.5f && ejeX->como<float>() == 1.0f &&
              ejeY->como<float>() == 0.0f && ejeZ->como<float>() == 0.0f &&
              escalaZ->como<float>() == 6.0f,
          "Java lee todos los getters de rotacion y escala");
    CHECK(sonido && sonido->como<int>() == 73 &&
              MotorScript::handleSonidoDetenido == 73,
          "Java reproduce y detiene sonido por servicio");
    CHECK(nombreMeta && nombreMeta->como<std::string>() == "meta",
          "Java usa el handle opaco del objeto encontrado en la API");
    CHECK(sostenida && pulsada && soltada && sostenida->como<bool>() &&
              pulsada->como<bool>() && soltada->como<bool>(),
          "Java consulta tecla sostenida, pulsada y liberada");
    CHECK(mouseX && mouseY && mouseX->como<float>() == 12.5f &&
              mouseY->como<float>() == -4.0f,
          "Java consulta el delta del mouse por servicios");

    ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
    ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
    valores = ScriptRuntime::extraer(comportamiento);
    vid = buscar(valores, "vidas");
    CHECK(vid && vid->como<int>() == 102, "iniciar(100)+2x actualizar = 102");

    ScriptRuntime::llamarDetener(comportamiento, nullptr);
    valores = ScriptRuntime::extraer(comportamiento);
    vid = buscar(valores, "vidas");
    CHECK(vid && vid->como<int>() == -1, "detener deja vidas = -1");
    CHECK(buscar(valores, "servicioOnStop") &&
              buscar(valores, "servicioOnStop")->como<bool>(),
          "los servicios de escena siguen disponibles durante detener");
    CHECK(MotorScript::textoImpreso == std::string("acci") + "\xC3\xB3" + "n",
          "Java conserva texto UTF-8 al imprimir por la API del motor");

    // Una version anterior del runtime invalida incluso clases que tengan un
    // mtime posterior al fuente; no se reutiliza bytecode de otra API.
    {
        const fs::path raizCache = BackendJava::cacheDir();
        const fs::path raizVersion =
            raizCache / ("runtime_" +
                         std::to_string(MotorScript::versionRuntimeScript));
        const fs::path claseCompilada =
            raizVersion / "clases" / "MiPruebaJava.class";
        const fs::path versionRuntime =
            raizVersion / "sdk" / "runtime.version";
        const auto mtimePrevio = fs::last_write_time(claseCompilada);
        fs::last_write_time(
            claseCompilada, fs::file_time_type::clock::now() +
                                std::chrono::hours(1));
        {
            std::ofstream version(versionRuntime);
            version << "obsoleto\n";
        }
        ComportamientoCargado runtimeActualizado;
        const bool okRuntime = ScriptRuntime::compilarYCargar(
            fuente, "MiPruebaJava", runtimeActualizado, error);
        CHECK(okRuntime, "un cache Java de runtime anterior se recompila");
        if (okRuntime) {
            const auto mtimeActual =
                fs::last_write_time(claseCompilada, ec);
            CHECK(!ec && mtimeActual != mtimePrevio &&
                      mtimeActual < fs::file_time_type::clock::now() +
                                        std::chrono::minutes(1),
                  "se reemplaza la clase obsoleta aunque su mtime fuera reciente");
            std::ifstream version(versionRuntime);
            std::string valorVersion;
            std::getline(version, valorVersion);
            CHECK(valorVersion ==
                      std::to_string(MotorScript::versionRuntimeScript),
                  "el cache registra la version actual del runtime");
        }
        ScriptRuntime::descargar(runtimeActualizado);
    }

    // Segundo componente sobre el MISMO fuente: la cola de la escena lo
    // entrega con un ComportamientoCargado vacio (estado por componente).
    // La clase ya esta compilada, asi que no hay que volver a pasar por javac.
    {
        const fs::path claseCompilada =
            fs::path(BackendJava::cacheDir()) /
            ("runtime_" +
             std::to_string(MotorScript::versionRuntimeScript)) /
            "clases" / "MiPruebaJava.class";
        std::error_code ecClase;
        const auto mtimeAntes = fs::last_write_time(claseCompilada, ecClase);
        CHECK(!ecClase, "la clase del primer componente existe");

        ComportamientoCargado segundo;
        const bool okSegundo = ScriptRuntime::compilarYCargar(
            fuente, "MiPruebaJava", segundo, error);
        CHECK(okSegundo, "segundo componente Java sobre el mismo fuente carga bien");
        if (!okSegundo) std::cout << "  Error: " << error << std::endl;
        if (!ecClase) {
            std::error_code ecDespues;
            const auto mtimeDespues =
                fs::last_write_time(claseCompilada, ecDespues);
            CHECK(!ecDespues && mtimeDespues == mtimeAntes,
                  "la clase no se volvio a compilar (sigue al dia)");
        }
        ScriptRuntime::descargar(segundo);
    }

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

    // Diagnostico del fallo de carga: pedir una clase que no esta en el .class
    // obliga a la JVM a lanzar, y el mensaje tiene que traer el motivo real
    // (ClassNotFoundException) y donde se busco. Con ExceptionClear() a secas el
    // texto era "no se encontro la clase", indistinguible del caso en que el
    // .class es de otra version de Java (UnsupportedClassVersionError), que es
    // justo lo que pasa cuando javac y la JVM salen de raices distintas.
    {
        ComportamientoCargado ausente;
        std::string errorAusente;
        const bool okAusente = ScriptRuntime::compilarYCargar(
            fuente, "ClaseQueNoExiste", ausente, errorAusente);
        CHECK(!okAusente, "una clase inexistente no carga");
        CHECK(!ausente.valido(), "no queda un comportamiento a medias");
        CHECK(errorAusente.find("motivo:") != std::string::npos,
              "el error de carga incluye el motivo de la excepcion");
        CHECK(errorAusente.find("ClaseQueNoExiste") != std::string::npos,
              "el error de carga nombra la clase que no se encontro");
        CHECK(errorAusente.find("libjvm:") != std::string::npos &&
                  errorAusente.find("javac:") != std::string::npos,
              "el error de carga dice con que compilador y con que JVM se busco");
        std::cout << "  Error informado: " << errorAusente << std::endl;
    }

    fs::remove_all(dir, ec);
    std::cout << "ScriptsJava: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos == 0 ? 0 : 1;
}