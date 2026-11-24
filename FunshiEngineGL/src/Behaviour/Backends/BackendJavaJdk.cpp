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
#include "BackendJavaDetalle.h"

std::string directorioCache() {
    std::error_code ec;
    fs::path base;
#if defined(_WIN32)
    const char* tmp = std::getenv("TEMP");
    if (tmp && *tmp) base = tmp;
    else base = ".";
#else
    const char* tmp = std::getenv("TMPDIR");
    if (tmp && *tmp) base = tmp;
    else base = "/tmp";
#endif
    return (base / "funshi_scripts" / "java").string();
}

// Raiz de JDK -> biblioteca de la JVM, si esta ahi.
// FindJNI en Windows entrega un .lib de importacion (LoadLibrary no lo puede
// cargar): el .dll real vive en <jdk>/bin/server/jvm.dll, al lado de lib/.
fs::path libjvmEnRaiz(const fs::path& raiz) {
    std::error_code ec;
    if (raiz.empty() || !fs::exists(raiz, ec)) return {};
#if defined(_WIN32)
    if (raiz.extension() == ".lib") {
        const fs::path dll = raiz.parent_path().parent_path() / "bin" /
                             "server" / kLibjvmNombre;
        return fs::exists(dll, ec) ? dll : fs::path{};
    }
    const fs::path p = raiz / "bin" / "server" / kLibjvmNombre;
#else
    const fs::path p = raiz / "lib" / "server" / kLibjvmNombre;
#endif
    return fs::exists(p, ec) ? p : fs::path{};
}

// Raiz de JDK -> javac, si esta ahi. Se necesita un JDK y no un JRE: el motor
// compila el .java del usuario a bytecode antes de cargarlo en la JVM.
fs::path javacEnRaiz(const fs::path& raiz) {
    std::error_code ec;
    if (raiz.empty() || !fs::is_directory(raiz, ec)) return {};
#if defined(_WIN32)
    const fs::path p = raiz / "bin" / "javac.exe";
#else
    const fs::path p = raiz / "bin" / "javac";
#endif
    return fs::exists(p, ec) ? p : fs::path{};
}

// JRE embebido junto al ejecutable (<exeDir>/jre). Va primero porque es el unico
// que el usuario no puede desinstalar por accidente.
fs::path raizJreEmbebido() {
    const std::string exeDir = ProjectPaths::directorioEjecutable();
    if (exeDir.empty()) return {};
    return fs::path(exeDir) / "jre";
}

// Raices candidatas de JDK, en orden de prioridad.
std::vector<fs::path> raicesJdk() {
    std::vector<fs::path> salida;
    const auto agregar = [&salida](const fs::path& p) {
        if (p.empty()) return;
        for (const fs::path& existente : salida) {
            if (existente == p) return;
        }
        salida.push_back(p);
    };

    agregar(raizJreEmbebido());
    if (const char* home = std::getenv("JAVA_HOME")) agregar(home);
#if defined(_WIN32)
    for (const fs::path& raiz : raicesRegistroWindows()) agregar(raiz);
    for (const fs::path& raiz : raicesComunesWindows()) agregar(raiz);
#else
    for (const fs::path& raiz : raicesComunesLinux()) agregar(raiz);
#endif
    return salida;
}

// Recorrer el registro y las carpetas de Program Files en cada llamada es caro y
// el conjunto no cambia mientras corre el motor (el JDK se instala antes de
// lanzar el juego), asi que la lista se arma una sola vez.
const std::vector<fs::path>& raicesJdkCache() {
    static const std::vector<fs::path> cache = raicesJdk();
    return cache;
}

// Herramientas del toolchain Java, resueltas UNA vez y en pareja: la biblioteca
// de la JVM y el compilador salen de la misma raiz de JDK. Recorrer la lista dos
// veces por separado dejaba que cada una cogiera el primer acierto, y esa mezcla
// (javac de una version, JVM de otra) se manifiesta como "no se encontro la
// clase", que es el mismo mensaje que da un .class que no existe.
const ResolucionJdk::Herramientas& herramientasJdk() {
    static const ResolucionJdk::Herramientas h = [] {
        ResolucionJdk::Herramientas resultado;

        // Biblioteca dada a mano: manda sobre la lista de raices, y su raiz se
        // deduce del propio archivo para buscarle el javac al lado.
        const char* explicito = std::getenv("FUNSHI_LIBJVM");
        if (explicito && *explicito) {
            resultado.libjvm = explicito;
            resultado.raiz = ResolucionJdk::raizDesdeLibjvm(explicito);
        } else {
            std::vector<std::string> raices;
            raices.reserve(raicesJdkCache().size());
            for (const fs::path& raiz : raicesJdkCache())
                raices.push_back(raiz.string());
            resultado = ResolucionJdk::desdeRaices(
                raices, [](const std::string& raiz) {
                    return libjvmEnRaiz(raiz).string();
                },
                [](const std::string& raiz) {
                    return javacEnRaiz(raiz).string();
                });
        }

        // La raiz elegida sin javac es un JRE: los .class ya compilados siguen
        // cargando y el aviso se da al compilar, no aqui.
        if (resultado.javac.empty() && !resultado.raiz.empty())
            resultado.javac = javacEnRaiz(resultado.raiz).string();

        // El valor horneado es la ruta del ARCHIVO de la biblioteca, no una raiz:
        // como raiz se descartaba siempre. Se resuelve aparte y es el ultimo
        // recurso, para cuando ninguna raiz del sistema trae la JVM.
        {
            const std::string horneada = FUNSHI_LIBJVM_DEFAULT;
            std::error_code ec;
            resultado = ResolucionJdk::conBibliotecaHorneada(
                resultado, horneada,
                !horneada.empty() && fs::exists(horneada, ec),
                [](const std::string& raiz) { return javacEnRaiz(raiz).string(); });
        }

        // JAVAC manda sobre lo deducido (permite cruzar a proposito un
        // compilador de otra raiz, que es justo lo que el emparejamiento
        // anterior hacia sin querer).
        if (const char* javac = std::getenv("JAVAC"))
            if (*javac) resultado.javac = javac;

        return resultado;
    }();
    return h;
}

bool arrancarJvm(const std::string& classesDir, std::string& error) {
    if (jvm().jvm) {
        return true; // ya arrancado: la classpath es compartida
    }

    const std::string ruta = rutaLibjvm();
    if (ruta.empty()) {
        error = "No se encontro libjvm (defini JAVA_HOME o FUNSHI_LIBJVM) para "
                "ejecutar scripts Java.";
        return false;
    }

    void* lib = nullptr;
#if defined(_WIN32)
    lib = LoadLibraryA(ruta.c_str());
#else
    lib = dlopen(ruta.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (!lib) {
        error = "No se pudo cargar el JVM: " + ruta;
        return false;
    }

    FnCrearJavaVM crear = reinterpret_cast<FnCrearJavaVM>(
#if defined(_WIN32)
        GetProcAddress(static_cast<HMODULE>(lib), "JNI_CreateJavaVM")
#else
        dlsym(lib, "JNI_CreateJavaVM")
#endif
    );
    if (!crear) {
        error = "libjvm no exporta JNI_CreateJavaVM: " + ruta;
        return false;
    }

    const std::string classpath =
        std::string("-Djava.class.path=") + classesDir;
    JavaVMOption opciones[1];
    opciones[0].optionString = const_cast<char*>(classpath.c_str());

    JavaVMInitArgs args;
    args.version = JNI_VERSION_1_8;
    args.nOptions = 1;
    args.options = opciones;
    args.ignoreUnrecognized = JNI_TRUE;

    JNIEnv* env = nullptr;
    JavaVM* maquina = nullptr;
    jint rc = crear(&maquina, reinterpret_cast<void**>(&env), &args);
    if (rc != JNI_OK || !maquina) {
        error = "JNI_CreateJavaVM fallo (codigo " + std::to_string(rc) + ")";
        return false;
    }

    jvm().biblioteca = lib;
    jvm().jvm = maquina;
    jvm().env = env;
    jvm().error.clear();
    return true;
}

std::string javacExe() {
    const ResolucionJdk::Herramientas& h = herramientasJdk();
    // Sin javac en la raiz de la JVM se devuelve el valor horneado (el del build,
    // que no existe en destino) y no una cadena vacia: quien compila decide si
    // eso es un fallo o un .class que ya estaba al dia.
    return h.javac.empty() ? std::string(FUNSHI_JAVAC_DEFAULT) : h.javac;
}

// Contexto del toolchain en los errores de carga: sin el, el mensaje dice que
// falta la clase pero no donde se busco ni con que compilador, que es lo que
// hace falta para arreglarlo.
std::string contextoHerramientas(const std::string& classesDir) {
    const ResolucionJdk::Herramientas& h = herramientasJdk();
    return "\n  cache:   " + directorioCache() + "\n  clases:  " + classesDir +
           "\n  javac:   " + (h.javac.empty() ? std::string("(ninguno)") : h.javac) +
           "\n  libjvm:  " + (h.libjvm.empty() ? std::string("(ninguna)") : h.libjvm) +
           "\n  jdk:     " + (h.raiz.empty() ? std::string("(ninguna)") : h.raiz);
}
