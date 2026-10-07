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

// Windows.h ANTES del header propio y de la stdlib, para que
// _HAS_STD_BYTE=0 surta efecto antes de que la stdlib defina std::byte.
#if defined(_WIN32)
#define _HAS_STD_BYTE 0
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "BackendJava.h"
#include "ResolucionJdk.h"

#include <jni.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "../ScriptGameObject.h"
#include "../../Configuracion/ProjectPaths.h"
#include "../../FileManager/Proceso.h"

#ifndef FUNSHI_LIBJVM_DEFAULT
#define FUNSHI_LIBJVM_DEFAULT ""
#endif
#ifndef FUNSHI_JAVAC_DEFAULT
#define FUNSHI_JAVAC_DEFAULT "javac"
#endif

namespace fs = std::filesystem;

namespace {

// --- SDK Java embebido ------------------------------------------------------
// Se escribe en el cache y se compila junto con los scripts del usuario. Sin
// paquete para simplificar la classpath y el template generado por la GUI.
const char* SRC_COMPORTAMIENTO =
    "public interface Comportamiento {\n"
    "    void iniciar(long objeto);\n"
    "    void actualizar(long objeto, double deltaTime);\n"
    "    default void detener(long objeto) {}\n"
    "    default void colisionInicio(long objeto, long collider, long otroCollider) {}\n"
    "    default void colisionPersistencia(long objeto, long collider, long otroCollider) {}\n"
    "    default void colisionFin(long objeto, long collider, long otroCollider) {}\n"
    "}\n";

const char* SRC_NATIVO =
    "public final class Nativo {\n"
    "    public static native String nombre(long objeto);\n"
    "    public static native float  posicionX(long objeto);\n"
    "    public static native float  posicionY(long objeto);\n"
    "    public static native float  posicionZ(long objeto);\n"
    "    public static native void fijarPosicion(long objeto, float x, float y, float z);\n"
    "    public static native void fijarEscala(long objeto, float x, float y, float z);\n"
    "    public static native void fijarRotacion(long objeto, float angulo, float x, float y, float z);\n"
    "    public static native float rotacionAngulo(long objeto);\n"
    "    public static native float rotacionEjeX(long objeto);\n"
    "    public static native float rotacionEjeY(long objeto);\n"
    "    public static native float rotacionEjeZ(long objeto);\n"
    "    public static native float escalaX(long objeto);\n"
    "    public static native float escalaY(long objeto);\n"
    "    public static native float escalaZ(long objeto);\n"
    "    public static native boolean fijarVelocidadHorizontal(long objeto, float x, float z);\n"
    "    public static native boolean saltar(long objeto, float velocidad);\n"
    "    public static native String etiqueta(long objeto);\n"
    "    public static native boolean tieneEtiqueta(long objeto, String etiqueta);\n"
    "    public static native long objetoDeCollider(long collider);\n"
    "    public static native float masa(long objeto);\n"
    "    public static native boolean fijarMasa(long objeto, float masa);\n"
    "    public static native boolean usaGravedad(long objeto);\n"
    "    public static native boolean fijarUsoGravedad(long objeto, boolean usar);\n"
    "    public static native float escalaGravedad(long objeto);\n"
    "    public static native boolean fijarEscalaGravedad(long objeto, float escala);\n"
    "    public static native float friccion(long objeto);\n"
    "    public static native boolean fijarFriccion(long objeto, float friccion);\n"
    "    public static native boolean posicionCongelada(long objeto, int eje);\n"
    "    public static native boolean fijarFreezePosicion(long objeto, boolean x, boolean y, boolean z);\n"
    "    public static native boolean rotacionCongelada(long objeto, int eje);\n"
    "    public static native boolean fijarFreezeRotacion(long objeto, boolean x, boolean y, boolean z);\n"
    "    public static native void fijarGravedadGlobal(float x, float y, float z);\n"
    "    public static native float gravedadGlobalX();\n"
    "    public static native float gravedadGlobalY();\n"
    "    public static native float gravedadGlobalZ();\n"
    "    public static native long crearObjeto(String nombre, long padre);\n"
    "    public static native boolean destruirObjeto(long objeto);\n"
    "    public static native long clonarObjeto(long original, long padre);\n"
    "    public static native boolean agregarColliderEsfera(long objeto, float radio);\n"
    "    public static native boolean agregarColliderCubo(long objeto, float radio);\n"
    "    public static native boolean agregarRigidBody(long objeto, float masa);\n"
    "    public static native void imprimir(String texto);\n"
    "    public static native int reproducirSonido(String clip, float volumen, boolean bucle);\n"
    "    public static native void detenerSonido(int handle);\n"
    "    public static native long objetoPorNombre(String nombre);\n"
    "    public static native long objetoPorId(int id);\n"
    "    public static native long objetoPorEtiqueta(String etiqueta);\n"
    "    public static native boolean teclaSostiene(String tecla);\n"
    "    public static native boolean teclaPresionada(String tecla);\n"
    "    public static native boolean teclaSoltada(String tecla);\n"
    "    public static native float deltaMouseX();\n"
    "    public static native float deltaMouseY();\n"
    "    private Nativo() {}\n"
    "}\n";

// Classloader hijo (child-first) para HOT RELOAD real de Java: el classloader
// del sistema cachea una clase por nombre y la JVM no la vuelve a leer aunque
// javac reescriba el .class. Cada carga crea una instancia NUEVA de Cargador,
// que define la clase a partir del binario recien compilado (child-first),
// delegando al padre solo el SDK (Nativo/Comportamiento) y las clases del JDK.
const char* SRC_CARGADOR =
    "public final class Cargador {\n"
    "    private final ClassLoader interno;\n"
    "    public Cargador(final String base) {\n"
    "        interno = new ClassLoader() {\n"
    "            @Override\n"
    "            protected Class<?> findClass(String nombre)\n"
    "                    throws ClassNotFoundException {\n"
    "                try {\n"
    "                    java.nio.file.Path ruta = java.nio.file.Paths.get(\n"
    "                        base, nombre.replace('.', '/') + \".class\");\n"
    "                    byte[] bytes = java.nio.file.Files.readAllBytes(ruta);\n"
    "                    return defineClass(nombre, bytes, 0, bytes.length);\n"
    "                } catch (java.io.IOException e) {\n"
    "                    throw new ClassNotFoundException(nombre, e);\n"
    "                }\n"
    "            }\n"
    "            @Override\n"
    "            protected Class<?> loadClass(String nombre, boolean resolve)\n"
    "                    throws ClassNotFoundException {\n"
    "                if (nombre.equals(\"Nativo\") ||\n"
    "                    nombre.equals(\"Comportamiento\") ||\n"
    "                    nombre.startsWith(\"java.\") ||\n"
    "                    nombre.startsWith(\"javax.\") ||\n"
    "                    nombre.startsWith(\"jdk.\") ||\n"
    "                    nombre.startsWith(\"sun.\") ||\n"
    "                    nombre.startsWith(\"com.sun.\")) {\n"
    "                    return super.loadClass(nombre, resolve);\n"
    "                }\n"
    "                synchronized (getClassLoadingLock(nombre)) {\n"
    "                    Class<?> c = findLoadedClass(nombre);\n"
    "                    if (c == null) {\n"
    "                        try {\n"
    "                            c = findClass(nombre);\n"
    "                        } catch (ClassNotFoundException ign) {\n"
    "                            c = super.loadClass(nombre, resolve);\n"
    "                        }\n"
    "                    }\n"
    "                    if (resolve) resolveClass(c);\n"
    "                    return c;\n"
    "                }\n"
    "            }\n"
    "        };\n"
    "    }\n"
    "    public Class<?> cargar(String nombre) throws ClassNotFoundException {\n"
    "        return interno.loadClass(nombre);\n"
    "    }\n"
    "}\n";

// --- Bootstrap dinamico del JVM --------------------------------------------
struct Jvm {
    void* biblioteca = nullptr;
    JavaVM* jvm = nullptr;
    JNIEnv* env = nullptr;
    std::string error;
};
Jvm& jvm() {
    static Jvm instancia;
    return instancia;
}

typedef jint(JNICALL* FnCrearJavaVM)(JavaVM**, void**, void*);

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

// --- Descubrimiento del JDK -------------------------------------------------
// El binario se compila con FUNSHI_JAVA=ON en una maquina (el runner de CI) y
// corre en otra (la del usuario), asi que las rutas del build casi nunca
// existen en destino. Antes de rendirse, el backend busca un JDK instalado en
// la maquina: un JRE embebido junto al exe, JAVA_HOME, el JDK del build y por
// ultimo las carpetas donde los JDK se instalan de verdad. La misma lista
// sirve para encontrar el javac, de modo que la JVM y el compilador de los
// scripts salen siempre del mismo JDK.

#if defined(_WIN32)
constexpr const char* kLibjvmNombre = "jvm.dll";
#elif defined(__APPLE__)
constexpr const char* kLibjvmNombre = "libjvm.dylib";
#else
constexpr const char* kLibjvmNombre = "libjvm.so";
#endif

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

#if defined(_WIN32)
// Raices de JDK registradas en Windows. JavaSoft es el esquema clasico
// (JavaHome por version); Adoptium y los JDK modernos usan "Path".
std::vector<fs::path> raicesRegistroWindows() {
    struct Origen {
        const wchar_t* clave;
        const wchar_t* valor;
    };
    const Origen origenes[] = {
        {L"SOFTWARE\\JavaSoft\\JDK", L"JavaHome"},
        {L"SOFTWARE\\JavaSoft\\Java Development Kit", L"JavaHome"},
        {L"SOFTWARE\\Eclipse Adoptium\\JDK", L"Path"},
    };
    std::vector<fs::path> salida;
    for (const Origen& origen : origenes) {
        HKEY raiz = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, origen.clave, 0,
                          KEY_READ | KEY_WOW64_64KEY, &raiz) != ERROR_SUCCESS) {
            continue;
        }
        for (DWORD i = 0;; ++i) {
            wchar_t nombre[256] = {};
            DWORD len = ARRAYSIZE(nombre);
            if (RegEnumKeyExW(raiz, i, nombre, &len, nullptr, nullptr, nullptr,
                              nullptr) != ERROR_SUCCESS) {
                break;
            }
            HKEY sub = nullptr;
            if (RegOpenKeyExW(raiz, nombre, 0, KEY_READ | KEY_WOW64_64KEY,
                              &sub) != ERROR_SUCCESS) {
                continue;
            }
            wchar_t valor[32768] = {};
            DWORD tam = ARRAYSIZE(valor);
            DWORD tipo = 0;
            if (RegQueryValueExW(sub, origen.valor, nullptr, &tipo,
                                 reinterpret_cast<LPBYTE>(valor),
                                 &tam) == ERROR_SUCCESS &&
                (tipo == REG_SZ || tipo == REG_EXPAND_SZ)) {
                salida.emplace_back(valor);
            }
            RegCloseKey(sub);
        }
        RegCloseKey(raiz);
    }
    return salida;
}

// Carpetas donde los JDK se instalan de verdad en Windows.
std::vector<fs::path> raicesComunesWindows() {
    std::vector<fs::path> salida;
    std::error_code ec;
    const char* bases[] = {"C:\\Program Files\\Java",
                           "C:\\Program Files\\Eclipse Adoptium",
                           "C:\\Program Files\\Microsoft",
                           "C:\\Program Files\\Amazon Corretto",
                           "C:\\Program Files\\Zulu",
                           "C:\\Program Files (x86)\\Java"};
    for (const char* base : bases) {
        for (const fs::directory_entry& e : fs::directory_iterator(base, ec)) {
            if (e.is_directory(ec)) salida.push_back(e.path());
        }
    }
    if (const char* local = std::getenv("LOCALAPPDATA")) {
        const fs::path base = fs::path(local) / "Programs" / "Eclipse Adoptium";
        for (const fs::directory_entry& e : fs::directory_iterator(base, ec)) {
            if (e.is_directory(ec)) salida.push_back(e.path());
        }
    }
    return salida;
}
#else
// Instalaciones tipicas de Linux. Se listan primero las conocidas y despues se
// recorre /usr/lib/jvm, que es donde las distros las ponen.
std::vector<fs::path> raicesComunesLinux() {
    std::vector<fs::path> salida;
    for (const char* base : {"/usr/lib/jvm/default-java",
                             "/usr/lib/jvm/java-1.21.0-openjdk-amd64",
                             "/usr/lib/jvm/java-17-openjdk-amd64",
                             "/usr/lib/jvm/java-11-openjdk-amd64"}) {
        salida.emplace_back(base);
    }
    std::error_code ec;
    for (const fs::directory_entry& e : fs::directory_iterator("/usr/lib/jvm", ec)) {
        if (e.is_directory(ec)) salida.push_back(e.path());
    }
    return salida;
}
#endif

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

std::string rutaLibjvm() { return herramientasJdk().libjvm; }

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

JNIEnv* entorno() { return jvm().env; }

std::string javacExe() {
    const ResolucionJdk::Herramientas& h = herramientasJdk();
    // Sin javac en la raiz de la JVM se devuelve el valor horneado (el del build,
    // que no existe en destino) y no una cadena vacia: quien compila decide si
    // eso es un fallo o un .class que ya estaba al dia.
    return h.javac.empty() ? std::string(FUNSHI_JAVAC_DEFAULT) : h.javac;
}

// Texto de una cadena estrecha del motor (`getenv("TEMP")`, nombres de objeto,
// valores de campo) al formato que exige la frontera JNI. NewStringUTF NO
// acepta la codificacion ANSI de Windows: un byte suelto de un "%TEMP%" acentuado
// no es UTF-8 valido y HotSpot lo sustituye por U+FFFD, de modo que la clase
// recibe una ruta que no corresponde a ningun directorio. En POSIX no hay nada
// que hacer porque la codificacion nativa ya es UTF-8.
//
// La conversion va SOLO aqui. `optionString` y `Proceso::ejecutar` siguen
// recibiendo la forma estrecha: es la que entiende el sistema de ficheros de
// Windows, y OptionString no la lleva por UTF-8 sino por la codificacion
// nativa.
inline std::string nuevoNombreUTF8(const std::string& estrecho) {
    return fs::path(estrecho).u8string();
}

// Texto de la excepcion JNI pendiente, vacio si la respuesta es util. Sin esto
// ExceptionClear() se lleva por delante la causa: "no se encontro la clase"
// no distingue entre "javac no produjo nada", "la clase es de otra version de
// Java" (UnsupportedClassVersionError, el fallo tipico de emparejar un javac
// con otra JVM) y "la classpath no apunta a nada".
std::string detalleExcepcion(JNIEnv* env) {
    jthrowable pendiente = env->ExceptionOccurred();
    if (!pendiente) return {};

    std::string detalle;
    if (jclass clase = env->GetObjectClass(pendiente)) {
        if (jmethodID mensaje =
                env->GetMethodID(clase, "getMessage", "()Ljava/lang/String;")) {
            if (jstring texto =
                    static_cast<jstring>(env->CallObjectMethod(pendiente, mensaje))) {
                const char* utf = env->GetStringUTFChars(texto, nullptr);
                if (utf) {
                    detalle = utf;
                    env->ReleaseStringUTFChars(texto, utf);
                }

                env->DeleteLocalRef(texto);
            }
        }
        env->DeleteLocalRef(clase);
    }
    env->ExceptionClear();

    // Sin mensaje, al menos el tipo de la excepcion, que ya dice bastante
    // (ClassNotFoundException, NoClassDefFoundError, UnsupportedClassVersionError).
    if (detalle.empty())
        if (jclass clase = env->FindClass("java/lang/Throwable")) {
            if (jmethodID nombre =
                    env->GetMethodID(clase, "getClass", "()Ljava/lang/Class;")) {
                if (jobject claseDelError =
                        env->CallObjectMethod(pendiente, nombre)) {
                    if (jmethodID simple =
                            env->GetMethodID(static_cast<jclass>(claseDelError), "getSimpleName",
                                             "()Ljava/lang/String;")) {
                        if (jstring texto = static_cast<jstring>(
                                env->CallObjectMethod(claseDelError, simple))) {
                            const char* utf = env->GetStringUTFChars(texto, nullptr);
                            if (utf) {
                                if (!detalle.empty()) detalle += " ";
                                detalle += utf;
                                env->ReleaseStringUTFChars(texto, utf);
                            }
                            env->DeleteLocalRef(texto);
                        }
                    }
                    env->DeleteLocalRef(claseDelError);
                }
            }
            env->DeleteLocalRef(clase);
        }
    if (env->ExceptionCheck()) env->ExceptionClear();
    return detalle;
}

class MarcoLocalJNI {
public:
    explicit MarcoLocalJNI(JNIEnv* entorno) : env_(entorno) {}
    ~MarcoLocalJNI() {
        if (env_ && abierto_) env_->PopLocalFrame(nullptr);
    }
    bool abrir(jint capacidad) {
        abierto_ = env_ && env_->PushLocalFrame(capacidad) == JNI_OK;
        return abierto_;
    }

private:
    JNIEnv* env_ = nullptr;
    bool abierto_ = false;
};

std::string leerCadena(JNIEnv* env, jstring valor) {
    if (!env || !valor) return {};
    const char* utf = env->GetStringUTFChars(valor, nullptr);
    if (!utf) return {};
    const std::string utf8(utf);
    env->ReleaseStringUTFChars(valor, utf);
    return utf8;
}

void informarExcepcion(JNIEnv* env, const char* operacion) {
    if (!env || !env->ExceptionCheck()) return;
    std::cerr << "[scripts-java] " << operacion << ": "
              << detalleExcepcion(env) << std::endl;
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

std::string firmarCaracter(const std::string& tipo) {
    if (tipo == "int") return "I";
    if (tipo == "long") return "J";
    if (tipo == "float") return "F";
    if (tipo == "double") return "D";
    if (tipo == "boolean") return "Z";
    if (tipo == "java.lang.String") return "Ljava/lang/String;";
    return "";
}

ReflejoScripts::TagTipo etiquetaDeTipo(const std::string& tipo) {
    using ReflejoScripts::TagTipo;
    if (tipo == "int") return TagTipo::Entero;
    // Un `long` publico es una referencia a GameObject (handle opaco, como el
    // `long objeto` de iniciar/actualizar): en el inspector se edita por
    // drag & drop y en disco se guarda por nombre, igual que `GameObject*`
    // en C++. Otros tipos de referencia no se soportan como SerializeField.
    if (tipo == "long") return TagTipo::Objeto;
    if (tipo == "float") return TagTipo::Flotante;
    if (tipo == "double") return TagTipo::Doble;
    if (tipo == "boolean") return TagTipo::Booleano;
    return TagTipo::Texto; // java.lang.String
}

struct DatosJava {
    jclass clase = nullptr;
    jobject cargador = nullptr; // classloader hijo (hot reload de la clase)
    jmethodID iniciar = nullptr;
    jmethodID actualizar = nullptr;
    jmethodID detener = nullptr;
    jmethodID colisionInicio = nullptr;
    jmethodID colisionPersistencia = nullptr;
    jmethodID colisionFin = nullptr;
    std::map<std::string, jfieldID> campos; // nombre -> jfieldID
};

std::string leerArchivo(const std::string& ruta) {
    std::ifstream f(ruta);
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

void escribirSiCambia(const std::string& ruta, const std::string& contenido) {
    std::error_code ec;
    if (fs::exists(ruta, ec) && leerArchivo(ruta) == contenido) return;
    fs::create_directories(fs::path(ruta).parent_path(), ec);
    std::ofstream f(ruta);
    f << contenido;
}

} // namespace

// --- Nativos expuestos a Java (Nativo.xxx) ----------------------------------
namespace {
void* comoObjeto(jlong objeto) {
    return reinterpret_cast<void*>(static_cast<intptr_t>(objeto));
}

jstring nativoNombre(JNIEnv* env, jclass, jlong objeto) {
    const char* n = MotorScript::tablaApi()->nombre(comoObjeto(objeto));
    return env->NewStringUTF(nuevoNombreUTF8(n ? n : "").c_str());
}
jfloat nativoPosicionX(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaApi()->posicionX(comoObjeto(o));
}
jfloat nativoPosicionY(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaApi()->posicionY(comoObjeto(o));
}
jfloat nativoPosicionZ(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaApi()->posicionZ(comoObjeto(o));
}
void nativoFijarPosicion(JNIEnv*, jclass, jlong o, jfloat x, jfloat y, jfloat z) {
    MotorScript::tablaApi()->fijarPosicion(comoObjeto(o), x, y, z);
}
 jboolean nativoFijarVelocidadHorizontal(JNIEnv*, jclass, jlong o, jfloat x,
                                         jfloat z) {
    return MotorScript::tablaApi()->fijarVelocidadHorizontal(comoObjeto(o), x, z);
}
jboolean nativoSaltar(JNIEnv*, jclass, jlong o, jfloat velocidad) {
    return MotorScript::tablaApi()->saltar(comoObjeto(o), velocidad);
}
jstring nativoEtiqueta(JNIEnv* env, jclass, jlong objeto) {
    const char* tag = MotorScript::tablaApi()->etiqueta(comoObjeto(objeto));
    return env->NewStringUTF(nuevoNombreUTF8(tag ? tag : "").c_str());
}
jboolean nativoTieneEtiqueta(JNIEnv* env, jclass, jlong objeto,
                             jstring etiqueta) {
    if (!etiqueta) return JNI_FALSE;
    const std::string texto = leerCadena(env, etiqueta);
    return MotorScript::tablaApi()->tieneEtiqueta(comoObjeto(objeto),
                                                   texto.c_str())
               ? JNI_TRUE
               : JNI_FALSE;
}
jlong nativoObjetoDeCollider(JNIEnv*, jclass, jlong collider) {
    void* owner =
        MotorScript::tablaApi()->objetoDeCollider(comoObjeto(collider));
    return static_cast<jlong>(reinterpret_cast<intptr_t>(owner));
}
void nativoFijarEscala(JNIEnv*, jclass, jlong o, jfloat x, jfloat y, jfloat z) {
    MotorScript::tablaApi()->fijarEscala(comoObjeto(o), x, y, z);
}
void nativoFijarRotacion(JNIEnv*, jclass, jlong o, jfloat a, jfloat x, jfloat y,
                         jfloat z) {
    MotorScript::tablaApi()->fijarRotacionEjes(comoObjeto(o), a, x, y, z);
}
jfloat nativoRotacion(JNIEnv*, jclass, jlong objeto, int eje) {
    const auto* api = MotorScript::tablaApi();
    const void* handle = comoObjeto(objeto);
    switch (eje) {
        case 0: return api->rotacionAngulo(handle);
        case 1: return api->rotacionEjeX(handle);
        case 2: return api->rotacionEjeY(handle);
        default: return api->rotacionEjeZ(handle);
    }
}
jfloat nativoEscala(JNIEnv*, jclass, jlong objeto, int eje) {
    const auto* api = MotorScript::tablaApi();
    const void* handle = comoObjeto(objeto);
    switch (eje) {
        case 0: return api->escalaX(handle);
        case 1: return api->escalaY(handle);
        default: return api->escalaZ(handle);
    }
}
jfloat nativoRotacionAngulo(JNIEnv* e, jclass c, jlong o) {
    return nativoRotacion(e, c, o, 0);
}
jfloat nativoRotacionEjeX(JNIEnv* e, jclass c, jlong o) {
    return nativoRotacion(e, c, o, 1);
}
jfloat nativoRotacionEjeY(JNIEnv* e, jclass c, jlong o) {
    return nativoRotacion(e, c, o, 2);
}
jfloat nativoRotacionEjeZ(JNIEnv* e, jclass c, jlong o) {
    return nativoRotacion(e, c, o, 3);
}
jfloat nativoEscalaX(JNIEnv* e, jclass c, jlong o) {
    return nativoEscala(e, c, o, 0);
}
jfloat nativoEscalaY(JNIEnv* e, jclass c, jlong o) {
    return nativoEscala(e, c, o, 1);
}
jfloat nativoEscalaZ(JNIEnv* e, jclass c, jlong o) {
    return nativoEscala(e, c, o, 2);
}
void nativoImprimir(JNIEnv* env, jclass, jstring texto) {
    if (!texto) return;
    const std::string mensaje = leerCadena(env, texto);
    MotorScript::tablaApi()->imprimirConsola(mensaje.c_str());
}
jint nativoReproducirSonido(JNIEnv* env, jclass, jstring clip, jfloat volumen,
                            jboolean bucle) {
    if (!clip) return -1;
    const std::string nombre = leerCadena(env, clip);
    return MotorScript::tablaServicios()->reproducirSonido(
        nombre.c_str(), volumen, bucle == JNI_TRUE);
}
void nativoDetenerSonido(JNIEnv*, jclass, jint handle) {
    MotorScript::tablaServicios()->detenerSonido(handle);
}
jlong nativoObjetoPorNombre(JNIEnv* env, jclass, jstring nombre) {
    if (!nombre) return 0;
    const std::string texto = leerCadena(env, nombre);
    return static_cast<jlong>(reinterpret_cast<intptr_t>(
        MotorScript::tablaServicios()->objetoPorNombre(texto.c_str())));
}
jlong nativoObjetoPorId(JNIEnv*, jclass, jint id) {
    return static_cast<jlong>(reinterpret_cast<intptr_t>(
        MotorScript::tablaServicios()->objetoPorId(static_cast<int>(id))));
}
jlong nativoObjetoPorEtiqueta(JNIEnv* env, jclass, jstring etiqueta) {
    if (!etiqueta) return 0;
    const std::string texto = leerCadena(env, etiqueta);
    return static_cast<jlong>(reinterpret_cast<intptr_t>(
        MotorScript::tablaServicios()->objetoPorEtiqueta(texto.c_str())));
}
jboolean nativoTecla(JNIEnv* env, jstring tecla, int consulta) {
    if (!tecla) return JNI_FALSE;
    const std::string nombre = leerCadena(env, tecla);
    const auto* servicios = MotorScript::tablaServicios();
    bool resultado = false;
    if (consulta == 0) resultado = servicios->teclaSostiene(nombre.c_str());
    else if (consulta == 1)
        resultado = servicios->teclaPresionada(nombre.c_str());
    else
        resultado = servicios->teclaSoltada(nombre.c_str());
    return resultado ? JNI_TRUE : JNI_FALSE;
}
jboolean nativoTeclaSostiene(JNIEnv* env, jclass, jstring tecla) {
    return nativoTecla(env, tecla, 0);
}
jboolean nativoTeclaPresionada(JNIEnv* env, jclass, jstring tecla) {
    return nativoTecla(env, tecla, 1);
}
jboolean nativoTeclaSoltada(JNIEnv* env, jclass, jstring tecla) {
    return nativoTecla(env, tecla, 2);
}
jfloat nativoDeltaMouseX(JNIEnv*, jclass) {
    return MotorScript::tablaServicios()->deltaMouseX();
}
jfloat nativoDeltaMouseY(JNIEnv*, jclass) {
    return MotorScript::tablaServicios()->deltaMouseY();
}
jfloat nativoMasa(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaApi()->masa(comoObjeto(o));
}
jboolean nativoFijarMasa(JNIEnv*, jclass, jlong o, jfloat masa) {
    return MotorScript::tablaApi()->fijarMasa(comoObjeto(o), masa) ? JNI_TRUE
                                                                   : JNI_FALSE;
}
jboolean nativoUsaGravedad(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaApi()->usaGravedad(comoObjeto(o)) ? JNI_TRUE
                                                                : JNI_FALSE;
}
jboolean nativoFijarUsoGravedad(JNIEnv*, jclass, jlong o, jboolean usar) {
    return MotorScript::tablaApi()->fijarUsoGravedad(comoObjeto(o),
                                                     usar == JNI_TRUE)
                ? JNI_TRUE
                : JNI_FALSE;
}
jfloat nativoEscalaGravedad(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaApi()->escalaGravedad(comoObjeto(o));
}
jboolean nativoFijarEscalaGravedad(JNIEnv*, jclass, jlong o, jfloat escala) {
    return MotorScript::tablaApi()->fijarEscalaGravedad(comoObjeto(o), escala)
                ? JNI_TRUE
                : JNI_FALSE;
}
jfloat nativoFriccion(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaApi()->friccion(comoObjeto(o));
}
jboolean nativoFijarFriccion(JNIEnv*, jclass, jlong o, jfloat friccion) {
    return MotorScript::tablaApi()->fijarFriccion(comoObjeto(o), friccion)
                ? JNI_TRUE
                : JNI_FALSE;
}
jboolean nativoFreezeEje(JNIEnv*, jclass, jlong o, jint eje, bool rotacion) {
    const auto* api = MotorScript::tablaApi();
    const void* handle = comoObjeto(o);
    const bool congelado = rotacion ? api->rotacionCongelada(handle, eje)
                                    : api->posicionCongelada(handle, eje);
    return congelado ? JNI_TRUE : JNI_FALSE;
}
jboolean nativoPosicionCongelada(JNIEnv* e, jclass c, jlong o, jint eje) {
    return nativoFreezeEje(e, c, o, eje, false);
}
jboolean nativoRotacionCongelada(JNIEnv* e, jclass c, jlong o, jint eje) {
    return nativoFreezeEje(e, c, o, eje, true);
}
jboolean nativoFijarFreeze(JNIEnv*, jclass, jlong o, jboolean x, jboolean y,
                           jboolean z, bool rotacion) {
    const auto* api = MotorScript::tablaApi();
    void* handle = comoObjeto(o);
    const bool ok = rotacion
                        ? api->fijarFreezeRotacion(handle, x == JNI_TRUE,
                                                  y == JNI_TRUE, z == JNI_TRUE)
                        : api->fijarFreezePosicion(handle, x == JNI_TRUE,
                                                  y == JNI_TRUE, z == JNI_TRUE);
    return ok ? JNI_TRUE : JNI_FALSE;
}
jboolean nativoFijarFreezePosicion(JNIEnv* e, jclass c, jlong o, jboolean x,
                                   jboolean y, jboolean z) {
    return nativoFijarFreeze(e, c, o, x, y, z, false);
}
jboolean nativoFijarFreezeRotacion(JNIEnv* e, jclass c, jlong o, jboolean x,
                                   jboolean y, jboolean z) {
    return nativoFijarFreeze(e, c, o, x, y, z, true);
}
void nativoFijarGravedadGlobal(JNIEnv*, jclass, jfloat x, jfloat y, jfloat z) {
    MotorScript::tablaServicios()->fijarGravedadGlobal(x, y, z);
}
jfloat nativoGravedadGlobalEje(JNIEnv*, jclass, int eje) {
    const auto* servicios = MotorScript::tablaServicios();
    if (eje == 0) return servicios->gravedadGlobalX();
    if (eje == 2) return servicios->gravedadGlobalZ();
    return servicios->gravedadGlobalY();
}
jfloat nativoGravedadGlobalX(JNIEnv* e, jclass c) {
    return nativoGravedadGlobalEje(e, c, 0);
}
jfloat nativoGravedadGlobalY(JNIEnv* e, jclass c) {
    return nativoGravedadGlobalEje(e, c, 1);
}
jfloat nativoGravedadGlobalZ(JNIEnv* e, jclass c) {
    return nativoGravedadGlobalEje(e, c, 2);
}
jlong nativoCrearObjeto(JNIEnv* env, jclass, jstring nombre, jlong padre) {
    const auto* servicios = MotorScript::tablaServicios();
    if (!nombre)
        return static_cast<jlong>(reinterpret_cast<intptr_t>(
            servicios->crearObjeto(nullptr, comoObjeto(padre))));
    const std::string texto = leerCadena(env, nombre);
    return static_cast<jlong>(reinterpret_cast<intptr_t>(
        servicios->crearObjeto(texto.c_str(), comoObjeto(padre))));
}
jboolean nativoDestruirObjeto(JNIEnv*, jclass, jlong o) {
    return MotorScript::tablaServicios()->destruirObjeto(comoObjeto(o))
                ? JNI_TRUE
                : JNI_FALSE;
}
jlong nativoClonarObjeto(JNIEnv*, jclass, jlong original, jlong padre) {
    return static_cast<jlong>(reinterpret_cast<intptr_t>(
        MotorScript::tablaServicios()->clonarObjeto(comoObjeto(original),
                                                   comoObjeto(padre))));
}
jboolean nativoAgregarColliderEsfera(JNIEnv*, jclass, jlong o, jfloat radio) {
    return MotorScript::tablaServicios()->agregarColliderEsfera(comoObjeto(o),
                                                               radio)
                ? JNI_TRUE
                : JNI_FALSE;
}
jboolean nativoAgregarColliderCubo(JNIEnv*, jclass, jlong o, jfloat radio) {
    return MotorScript::tablaServicios()->agregarColliderCubo(comoObjeto(o),
                                                             radio)
                ? JNI_TRUE
                : JNI_FALSE;
}
jboolean nativoAgregarRigidBody(JNIEnv*, jclass, jlong o, jfloat masa) {
    return MotorScript::tablaServicios()->agregarRigidBody(comoObjeto(o), masa)
                ? JNI_TRUE
                : JNI_FALSE;
}

bool registrarNativos(const std::string& clasesDir, std::string& error) {
    JNIEnv* env = entorno();
    jclass nativo = env->FindClass("Nativo");
    if (!nativo) {
        error = "No se encontro la clase Nativo en la classpath." +
                contextoHerramientas(clasesDir) +
                "\n  motivo:  " + detalleExcepcion(env);
        return false;
    }
    JNINativeMethod metodos[] = {
        {const_cast<char*>("nombre"), const_cast<char*>("(J)Ljava/lang/String;"),
         reinterpret_cast<void*>(&nativoNombre)},
        {const_cast<char*>("posicionX"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoPosicionX)},
        {const_cast<char*>("posicionY"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoPosicionY)},
        {const_cast<char*>("posicionZ"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoPosicionZ)},
        {const_cast<char*>("fijarPosicion"), const_cast<char*>("(JFFF)V"),
         reinterpret_cast<void*>(&nativoFijarPosicion)},
        {const_cast<char*>("fijarEscala"), const_cast<char*>("(JFFF)V"),
         reinterpret_cast<void*>(&nativoFijarEscala)},
        {const_cast<char*>("fijarRotacion"), const_cast<char*>("(JFFFF)V"),
         reinterpret_cast<void*>(&nativoFijarRotacion)},
        {const_cast<char*>("rotacionAngulo"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoRotacionAngulo)},
        {const_cast<char*>("rotacionEjeX"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoRotacionEjeX)},
        {const_cast<char*>("rotacionEjeY"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoRotacionEjeY)},
        {const_cast<char*>("rotacionEjeZ"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoRotacionEjeZ)},
        {const_cast<char*>("escalaX"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoEscalaX)},
        {const_cast<char*>("escalaY"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoEscalaY)},
        {const_cast<char*>("escalaZ"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoEscalaZ)},
        {const_cast<char*>("fijarVelocidadHorizontal"),
         const_cast<char*>("(JFF)Z"),
         reinterpret_cast<void*>(&nativoFijarVelocidadHorizontal)},
        {const_cast<char*>("saltar"), const_cast<char*>("(JF)Z"),
         reinterpret_cast<void*>(&nativoSaltar)},
        {const_cast<char*>("etiqueta"),
         const_cast<char*>("(J)Ljava/lang/String;"),
         reinterpret_cast<void*>(&nativoEtiqueta)},
        {const_cast<char*>("tieneEtiqueta"),
         const_cast<char*>("(JLjava/lang/String;)Z"),
         reinterpret_cast<void*>(&nativoTieneEtiqueta)},
        {const_cast<char*>("objetoDeCollider"), const_cast<char*>("(J)J"),
         reinterpret_cast<void*>(&nativoObjetoDeCollider)},
        {const_cast<char*>("imprimir"), const_cast<char*>("(Ljava/lang/String;)V"),
         reinterpret_cast<void*>(&nativoImprimir)},
        {const_cast<char*>("reproducirSonido"),
         const_cast<char*>("(Ljava/lang/String;FZ)I"),
         reinterpret_cast<void*>(&nativoReproducirSonido)},
        {const_cast<char*>("detenerSonido"), const_cast<char*>("(I)V"),
         reinterpret_cast<void*>(&nativoDetenerSonido)},
        {const_cast<char*>("objetoPorNombre"),
         const_cast<char*>("(Ljava/lang/String;)J"),
         reinterpret_cast<void*>(&nativoObjetoPorNombre)},
        {const_cast<char*>("objetoPorId"), const_cast<char*>("(I)J"),
         reinterpret_cast<void*>(&nativoObjetoPorId)},
        {const_cast<char*>("objetoPorEtiqueta"),
         const_cast<char*>("(Ljava/lang/String;)J"),
         reinterpret_cast<void*>(&nativoObjetoPorEtiqueta)},
        {const_cast<char*>("teclaSostiene"),
         const_cast<char*>("(Ljava/lang/String;)Z"),
         reinterpret_cast<void*>(&nativoTeclaSostiene)},
        {const_cast<char*>("teclaPresionada"),
         const_cast<char*>("(Ljava/lang/String;)Z"),
         reinterpret_cast<void*>(&nativoTeclaPresionada)},
        {const_cast<char*>("teclaSoltada"),
         const_cast<char*>("(Ljava/lang/String;)Z"),
         reinterpret_cast<void*>(&nativoTeclaSoltada)},
        {const_cast<char*>("deltaMouseX"), const_cast<char*>("()F"),
         reinterpret_cast<void*>(&nativoDeltaMouseX)},
        {const_cast<char*>("deltaMouseY"), const_cast<char*>("()F"),
         reinterpret_cast<void*>(&nativoDeltaMouseY)},
        {const_cast<char*>("masa"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoMasa)},
        {const_cast<char*>("fijarMasa"), const_cast<char*>("(JF)Z"),
         reinterpret_cast<void*>(&nativoFijarMasa)},
        {const_cast<char*>("usaGravedad"), const_cast<char*>("(J)Z"),
         reinterpret_cast<void*>(&nativoUsaGravedad)},
        {const_cast<char*>("fijarUsoGravedad"), const_cast<char*>("(JZ)Z"),
         reinterpret_cast<void*>(&nativoFijarUsoGravedad)},
        {const_cast<char*>("escalaGravedad"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoEscalaGravedad)},
        {const_cast<char*>("fijarEscalaGravedad"), const_cast<char*>("(JF)Z"),
         reinterpret_cast<void*>(&nativoFijarEscalaGravedad)},
        {const_cast<char*>("friccion"), const_cast<char*>("(J)F"),
         reinterpret_cast<void*>(&nativoFriccion)},
        {const_cast<char*>("fijarFriccion"), const_cast<char*>("(JF)Z"),
         reinterpret_cast<void*>(&nativoFijarFriccion)},
        {const_cast<char*>("posicionCongelada"), const_cast<char*>("(JI)Z"),
         reinterpret_cast<void*>(&nativoPosicionCongelada)},
        {const_cast<char*>("fijarFreezePosicion"),
         const_cast<char*>("(JZZZ)Z"),
         reinterpret_cast<void*>(&nativoFijarFreezePosicion)},
        {const_cast<char*>("rotacionCongelada"), const_cast<char*>("(JI)Z"),
         reinterpret_cast<void*>(&nativoRotacionCongelada)},
        {const_cast<char*>("fijarFreezeRotacion"),
         const_cast<char*>("(JZZZ)Z"),
         reinterpret_cast<void*>(&nativoFijarFreezeRotacion)},
        {const_cast<char*>("fijarGravedadGlobal"),
         const_cast<char*>("(FFF)V"),
         reinterpret_cast<void*>(&nativoFijarGravedadGlobal)},
        {const_cast<char*>("gravedadGlobalX"), const_cast<char*>("()F"),
         reinterpret_cast<void*>(&nativoGravedadGlobalX)},
        {const_cast<char*>("gravedadGlobalY"), const_cast<char*>("()F"),
         reinterpret_cast<void*>(&nativoGravedadGlobalY)},
        {const_cast<char*>("gravedadGlobalZ"), const_cast<char*>("()F"),
         reinterpret_cast<void*>(&nativoGravedadGlobalZ)},
        {const_cast<char*>("crearObjeto"),
         const_cast<char*>("(Ljava/lang/String;J)J"),
         reinterpret_cast<void*>(&nativoCrearObjeto)},
        {const_cast<char*>("destruirObjeto"), const_cast<char*>("(J)Z"),
         reinterpret_cast<void*>(&nativoDestruirObjeto)},
        {const_cast<char*>("clonarObjeto"), const_cast<char*>("(JJ)J"),
         reinterpret_cast<void*>(&nativoClonarObjeto)},
        {const_cast<char*>("agregarColliderEsfera"),
         const_cast<char*>("(JF)Z"),
         reinterpret_cast<void*>(&nativoAgregarColliderEsfera)},
        {const_cast<char*>("agregarColliderCubo"), const_cast<char*>("(JF)Z"),
         reinterpret_cast<void*>(&nativoAgregarColliderCubo)},
        {const_cast<char*>("agregarRigidBody"), const_cast<char*>("(JF)Z"),
         reinterpret_cast<void*>(&nativoAgregarRigidBody)},
    };
    if (env->RegisterNatives(nativo, metodos,
                             sizeof(metodos) / sizeof(metodos[0])) != JNI_OK) {
        error = "RegisterNatives de Nativo fallo: " +
                detalleExcepcion(env);
        env->DeleteLocalRef(nativo);
        return false;
    }
    env->DeleteLocalRef(nativo);
    return true;
}
} // namespace

const char* BackendJava::lenguaje() const { return "java"; }

std::string BackendJava::javacRuta() { return javacExe(); }
std::string BackendJava::libjvmRuta() { return rutaLibjvm(); }
bool BackendJava::jvmArrancada() { return jvm().jvm != nullptr; }
std::string BackendJava::cacheDir() { return directorioCache(); }

void BackendJava::apagarJvm() {
    Jvm& j = jvm();
    if (!j.jvm) return;

    // DestroyJavaVM libera todo el estado interno de la JVM (nmethods,
    // oopmaps, metaspace, tabla de referencias globales). Sin el, al salir
    // LeakSanitizer lista cientos de bloques internos del JVM como fugas.
    // Segun la spec JNI debe invocarse desde el hilo que creo la VM (el hilo
    // principal); los hilos internos (compilador, GC) son "daemon" y no
    // bloquean el apagado.
    const jint rc = j.jvm->DestroyJavaVM();
    (void)rc;
    j.jvm = nullptr;
    j.env = nullptr;
    // libjvm (y la biblioteca) quedan cargadas hasta que muera el proceso:
    // dlclose ahora liberaria codigo/metadata que aun referencian las pilas
    // de C++, sin aportar nada en el cierre.
}

bool BackendJava::compilarYCargar(const std::string& fuente,
                                  const std::string& nombreClase,
                                  ComportamientoCargado& salida,
                                  std::string& error) {
    std::error_code ec;
    if (!fs::exists(fuente, ec)) {
        error = "El fuente Java no existe:\n" + fuente;
        return false;
    }

    const std::string versionEsperada =
        std::to_string(MotorScript::versionRuntimeScript);
    const std::string cache =
        (fs::path(directorioCache()) / ("runtime_" + versionEsperada)).string();
    const std::string clasesDir = (fs::path(cache) / "clases").string();
    const std::string sdkDir = (fs::path(cache) / "sdk").string();
    const std::string logPath = (fs::path(cache) / "javac.log").string();
    const std::string versionSdk =
        (fs::path(sdkDir) / "runtime.version").string();
    fs::create_directories(sdkDir, ec);
    if (leerArchivo(versionSdk) != versionEsperada + "\n") {
        fs::remove_all(clasesDir, ec);
        if (ec) {
            error = "No se pudo invalidar el cache Java del runtime: " +
                    ec.message();
            return false;
        }
        ec.clear();
        escribirSiCambia(versionSdk, versionEsperada + "\n");
        if (leerArchivo(versionSdk) != versionEsperada + "\n") {
            error = "No se pudo actualizar la version del cache Java.";
            return false;
        }
    }
    fs::create_directories(clasesDir, ec);
    if (ec) {
        error = "No se pudo crear el cache Java: " + ec.message();
        return false;
    }
    escribirSiCambia((fs::path(sdkDir) / "Comportamiento.java").string(),
                     SRC_COMPORTAMIENTO);
    escribirSiCambia((fs::path(sdkDir) / "Nativo.java").string(), SRC_NATIVO);
    escribirSiCambia((fs::path(sdkDir) / "Cargador.java").string(),
                     SRC_CARGADOR);

    auto mtime = [](const std::string& r) {
        std::error_code e;
        auto t = fs::last_write_time(r, e);
        if (e) return std::string();
        return std::to_string(t.time_since_epoch().count());
    };

    const std::string claseCompilada =
        (fs::path(clasesDir) / (nombreClase + ".class")).string();
    const std::string mtimeFuente = mtime(fuente);
    // La clase esta vigente si existe y no es mas vieja que el fuente: la
    // salida la comparten los componentes que apuntan al mismo .java, asi
    // que manda la frescura del archivo, no el mtime que guarda cada
    // componente (vacio en uno recien cargado), igual que en BackendCpp.
    auto claseVigente = [&]() {
        std::error_code ecClase;
        const auto tClase = fs::last_write_time(claseCompilada, ecClase);
        if (ecClase) return false; // no existe o ilegible: hay que compilar
        std::error_code ecFuente;
        const auto tFuente = fs::last_write_time(fuente, ecFuente);
        if (ecFuente) return false;
        return tClase >= tFuente;
    };
    const bool recompilar = !claseVigente();
    if (recompilar) {
        const std::string javac = javacExe();
        // Un nombre suelto ("javac") lo resuelve el PATH de la maquina; una ruta
        // absoluta que no existe no hay forma de ejecutarla, y el error de javac
        // seria un "no se pudo ejecutar" sin decir que falta un JDK.
        if (!ResolucionJdk::compiladorEjecutable(javac)) {
            error = "No se encontro javac para compilar el script Java: " + javac +
                    "\n  hace falta un JDK (no un JRE) con bin/javac, o define "
                    "JAVAC con la ruta del compilador." +
                    contextoHerramientas(clasesDir);
            return false;
        }
        const std::string sdkComportamiento =
            (fs::path(sdkDir) / "Comportamiento.java").string();
        const std::string sdkNativo = (fs::path(sdkDir) / "Nativo.java").string();
        const std::string sdkCargador = (fs::path(sdkDir) / "Cargador.java").string();
        // Sin shell (H-3 nivel 2): javac recibe cada ruta como argumento
        // propio y el log lo redirige Proceso por handles/fd. Se borra el
        // envoltorio de comillas que cmd.exe se comia (y con el, el
        // std::system entero).
        const std::vector<std::string> argv = {
            javac,          "-d",          clasesDir,     "-cp",
            clasesDir,      sdkComportamiento, sdkNativo, sdkCargador,
            fuente};
        int rc = Proceso::ejecutar(argv, logPath);
        if (rc != 0) {
            error = "Error al compilar el script Java:\n" + leerArchivo(logPath);
            return false;
        }
    }

    if (!arrancarJvm(clasesDir, error)) return false;
    JNIEnv* env = entorno();
    MarcoLocalJNI marcoLocal(env);
    if (!marcoLocal.abrir(64)) {
        error = "No se pudo reservar el marco local JNI." +
                contextoHerramientas(clasesDir) +
                "\n  motivo:  " + detalleExcepcion(env);
        return false;
    }
    if (!registrarNativos(clasesDir, error)) return false;

    // El classloader del sistema cachea cada clase por nombre y la JVM no la
    // redefine aunque javac reescriba el .class; por eso FindClass no bastaria
    // para el hot reload. Se carga cada script con una instancia FRESCA de un
    // classloader hijo (child-first) que define la clase desde el binario
    // recien compilado -> editar un .java aplica sin reiniciar el motor.
    jclass claseCargador = env->FindClass("Cargador");
    if (!claseCargador) {
        error = "No se encontro el SDK 'Cargador' (classloader de hot reload)." +
                contextoHerramientas(clasesDir) +
                "\n  motivo:  " + detalleExcepcion(env);
        return false;
    }
    jmethodID ctorCargador =
        env->GetMethodID(claseCargador, "<init>", "(Ljava/lang/String;)V");
    if (!ctorCargador) {
        env->ExceptionClear();
        error = "El SDK 'Cargador' no tiene constructor (String).";
        return false;
    }
    jmethodID cargarDeCargador =
        env->GetMethodID(claseCargador, "cargar",
                         "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!cargarDeCargador) {
        env->ExceptionClear();
        error = "El SDK 'Cargador' no expone cargar(String).";
        return false;
    }

    jstring jBase = env->NewStringUTF(nuevoNombreUTF8(clasesDir).c_str());
    jobject cargadorObj = env->NewObject(claseCargador, ctorCargador, jBase);
    env->DeleteLocalRef(jBase);
    if (!cargadorObj) {
        error = "No se pudo crear el classloader hijo para '" + nombreClase +
                "'." + contextoHerramientas(clasesDir) +
                "\n  motivo:  " + detalleExcepcion(env);
        return false;
    }

    jstring jNombre = env->NewStringUTF(nombreClase.c_str());
    jclass clase = static_cast<jclass>(
        env->CallObjectMethod(cargadorObj, cargarDeCargador, jNombre));
    env->DeleteLocalRef(jNombre);
    if (!clase) {
        // Aqui es donde aparece el UnsupportedClassVersionError cuando el .class
        // lo produjo un javac de otra version que la JVM que lo ejecuta: sin el
        // motivo, el mensaje es indistinguible del de un .class que no existe.
        error = "No se encontro la clase Java '" + nombreClase +
                "' (¿el nombre de la clase coincide con el archivo?)." +
                contextoHerramientas(clasesDir) +
                "\n  motivo:  " + detalleExcepcion(env);
        return false;
    }
    jmethodID constructor = env->GetMethodID(clase, "<init>", "()V");
    if (!constructor) {
        env->ExceptionClear();
        error = "La clase Java '" + nombreClase + "' no tiene constructor vacio.";
        return false;
    }
    jobject objeto = env->NewObject(clase, constructor);
    if (!objeto) {
        env->ExceptionClear();
        error = "No se pudo instanciar la clase Java '" + nombreClase + "'.";
        return false;
    }

    auto* datos = new DatosJava();
    datos->iniciar = env->GetMethodID(clase, "iniciar", "(J)V");
    datos->actualizar = env->GetMethodID(clase, "actualizar", "(JD)V");
    datos->detener = env->GetMethodID(clase, "detener", "(J)V");
    datos->colisionInicio =
        env->GetMethodID(clase, "colisionInicio", "(JJJ)V");
    datos->colisionPersistencia =
        env->GetMethodID(clase, "colisionPersistencia", "(JJJ)V");
    datos->colisionFin = env->GetMethodID(clase, "colisionFin", "(JJJ)V");
    if (!datos->iniciar || !datos->actualizar || !datos->detener ||
        !datos->colisionInicio || !datos->colisionPersistencia ||
        !datos->colisionFin ||
        env->ExceptionCheck()) {
        const std::string detalle = detalleExcepcion(env);
        delete datos;
        error = "La clase Java '" + nombreClase +
                "' debe exponer iniciar(long), actualizar(long,double) y "
                "detener(long)." +
                (detalle.empty() ? std::string() : "\n  motivo:  " + detalle);
        return false;
    }

    // Reflexion de los campos publicos (no estaticos) -> SerializeField.
    std::vector<ReflejoScripts::DefCampo> campos;
    jclass claseClass = env->FindClass("java/lang/Class");
    jmethodID obtenerCampos =
        claseClass
            ? env->GetMethodID(claseClass, "getFields",
                               "()[Ljava/lang/reflect/Field;")
            : nullptr;
    jmethodID nombreClaseJava =
        claseClass
            ? env->GetMethodID(claseClass, "getName",
                               "()Ljava/lang/String;")
            : nullptr;
    if (claseClass) env->DeleteLocalRef(claseClass);
    jclass claseField = env->FindClass("java/lang/reflect/Field");
    jclass claseModifier = env->FindClass("java/lang/reflect/Modifier");
    jmethodID getName = claseField
                            ? env->GetMethodID(claseField, "getName",
                                               "()Ljava/lang/String;")
                            : nullptr;
    jmethodID getType = claseField
                            ? env->GetMethodID(claseField, "getType",
                                               "()Ljava/lang/Class;")
                            : nullptr;
    jmethodID getMods = claseField
                            ? env->GetMethodID(claseField, "getModifiers", "()I")
                            : nullptr;
    jmethodID esEstatico =
        claseModifier
            ? env->GetStaticMethodID(claseModifier, "isStatic", "(I)Z")
            : nullptr;
    if (!obtenerCampos || !nombreClaseJava || !claseField || !claseModifier ||
        !getName || !getType || !getMods || !esEstatico ||
        env->ExceptionCheck()) {
        const std::string detalle = detalleExcepcion(env);
        if (claseField) env->DeleteLocalRef(claseField);
        if (claseModifier) env->DeleteLocalRef(claseModifier);
        delete datos;
        error = "No se pudo inspeccionar la clase Java '" + nombreClase + "'." +
                (detalle.empty() ? std::string() : "\n  motivo:  " + detalle);
        return false;
    }
    jobjectArray arreglo = static_cast<jobjectArray>(
        env->CallObjectMethod(clase, obtenerCampos));
    if (env->ExceptionCheck()) {
        const std::string detalle = detalleExcepcion(env);
        env->DeleteLocalRef(claseField);
        env->DeleteLocalRef(claseModifier);
        delete datos;
        error = "No se pudieron enumerar los campos de '" + nombreClase + "'." +
                (detalle.empty() ? std::string() : "\n  motivo:  " + detalle);
        return false;
    }

    const jsize total = arreglo ? env->GetArrayLength(arreglo) : 0;
    for (jsize i = 0; i < total; ++i) {
        jobject campo =
            env->GetObjectArrayElement(static_cast<jobjectArray>(arreglo), i);
        if (!campo) continue;
        jint mods = env->CallIntMethod(campo, getMods);
        if (env->CallBooleanMethod(claseModifier, esEstatico, mods)) {
            env->DeleteLocalRef(campo);
            continue;
        }

        jstring jnombre =
            static_cast<jstring>(env->CallObjectMethod(campo, getName));
        jclass tipo = static_cast<jclass>(env->CallObjectMethod(campo, getType));
        jstring jtipo =
            static_cast<jstring>(env->CallObjectMethod(tipo, nombreClaseJava));
        const std::string nombre = leerCadena(env, jnombre);
        const std::string tipoNombre = leerCadena(env, jtipo);

        const std::string firma = firmarCaracter(tipoNombre);
        if (!firma.empty()) {
            jfieldID id = env->GetFieldID(clase, nombre.c_str(), firma.c_str());
            if (!id) {
                env->ExceptionClear();
            } else {
                ReflejoScripts::DefCampo def;
                def.nombre = nombre;
                def.tag = etiquetaDeTipo(tipoNombre);
                campos.push_back(std::move(def));
                datos->campos[nombre] = id;
            }
        }
        if (jnombre) env->DeleteLocalRef(jnombre);
        if (jtipo) env->DeleteLocalRef(jtipo);
        if (tipo) env->DeleteLocalRef(tipo);
        env->DeleteLocalRef(campo);
    }
    if (arreglo) env->DeleteLocalRef(arreglo);
    env->DeleteLocalRef(claseField);
    env->DeleteLocalRef(claseModifier);

    datos->clase = static_cast<jclass>(env->NewGlobalRef(clase));
    datos->cargador = env->NewGlobalRef(cargadorObj);
    if (!datos->clase || !datos->cargador) {
        const std::string detalle = detalleExcepcion(env);
        if (datos->clase) env->DeleteGlobalRef(datos->clase);
        if (datos->cargador) env->DeleteGlobalRef(datos->cargador);
        delete datos;
        error = "No se pudieron conservar las referencias JNI de '" +
                nombreClase + "'." +
                (detalle.empty() ? std::string() : "\n  motivo:  " + detalle);
        return false;
    }

    salida.fuente = fuente;
    salida.lenguaje = "java";
    salida.artefacto = clasesDir;
    salida.manejador = jvm().jvm;
    salida.instancia = env->NewGlobalRef(objeto);
    if (!salida.instancia) {
        const std::string detalle = detalleExcepcion(env);
        env->DeleteGlobalRef(datos->cargador);
        env->DeleteGlobalRef(datos->clase);
        delete datos;
        salida = ComportamientoCargado{};
        error = "No se pudo conservar la instancia Java '" + nombreClase +
                "'." +
                (detalle.empty() ? std::string() : "\n  motivo:  " + detalle);
        return false;
    }
    salida.datos = datos;
    salida.campos = std::move(campos);
    salida.mtimeFuente = mtimeFuente;
    salida.cargado = true;
    error.clear();
    return true;
}

void BackendJava::descargar(ComportamientoCargado& comportamiento) {
    descargar(comportamiento, nullptr);
}

void BackendJava::descargar(ComportamientoCargado& comportamiento,
                            GameObject* owner) {
    if (!comportamiento.cargado) return;
    JNIEnv* env = entorno();
    if (env) {
        if (owner && comportamiento.instancia) {
            auto* datos = static_cast<DatosJava*>(comportamiento.datos);
            if (datos && datos->detener)
                env->CallVoidMethod(
                    static_cast<jobject>(comportamiento.instancia),
                    datos->detener, static_cast<jlong>(
                                        reinterpret_cast<intptr_t>(owner)));
            informarExcepcion(env, "detener");
        }
        if (comportamiento.instancia)
            env->DeleteGlobalRef(
                static_cast<jobject>(comportamiento.instancia));
        auto* datos = static_cast<DatosJava*>(comportamiento.datos);
        if (datos) {
            if (datos->cargador) env->DeleteGlobalRef(datos->cargador);
            if (datos->clase) env->DeleteGlobalRef(datos->clase);
            delete datos;
        }
    }
    comportamiento = ComportamientoCargado{};
}

void BackendJava::llamarInicio(ComportamientoCargado& comportamiento,
                               GameObject* owner) {
    if (!comportamiento.valido()) return;
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    if (!datos || !datos->iniciar) return;
    entorno()->CallVoidMethod(
        static_cast<jobject>(comportamiento.instancia), datos->iniciar,
        static_cast<jlong>(reinterpret_cast<intptr_t>(owner)));
    informarExcepcion(entorno(), "iniciar");
}

void BackendJava::llamarActualizar(ComportamientoCargado& comportamiento,
                                   GameObject* owner, float deltaTime) {
    if (!comportamiento.valido()) return;
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    if (!datos || !datos->actualizar) return;
    entorno()->CallVoidMethod(
        static_cast<jobject>(comportamiento.instancia), datos->actualizar,
        static_cast<jlong>(reinterpret_cast<intptr_t>(owner)),
        static_cast<jdouble>(deltaTime));
    informarExcepcion(entorno(), "actualizar");
}

void BackendJava::llamarDetener(ComportamientoCargado& comportamiento,
                                GameObject* owner) {
    if (!comportamiento.valido()) return;
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    if (!datos || !datos->detener) return;
    entorno()->CallVoidMethod(
        static_cast<jobject>(comportamiento.instancia), datos->detener,
        static_cast<jlong>(reinterpret_cast<intptr_t>(owner)));
    informarExcepcion(entorno(), "detener");
}

void BackendJava::llamarContacto(ComportamientoCargado& comportamiento,
                                 GameObject* owner, Collider* propio,
                                 Collider* otro, TipoContacto tipo) {
    if (!comportamiento.valido()) return;
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    if (!datos) return;
    jmethodID metodo = datos->colisionInicio;
    const char* nombre = "colisionInicio";
    if (tipo == TipoContacto::Persistencia) {
        metodo = datos->colisionPersistencia;
        nombre = "colisionPersistencia";
    } else if (tipo == TipoContacto::Fin) {
        metodo = datos->colisionFin;
        nombre = "colisionFin";
    }
    entorno()->CallVoidMethod(
        static_cast<jobject>(comportamiento.instancia), metodo,
        static_cast<jlong>(reinterpret_cast<intptr_t>(owner)),
        static_cast<jlong>(reinterpret_cast<intptr_t>(propio)),
        static_cast<jlong>(reinterpret_cast<intptr_t>(otro)));
    informarExcepcion(entorno(), nombre);
}

void BackendJava::inyectar(
    ComportamientoCargado& comportamiento,
    const std::vector<ReflejoScripts::ValorCampo>& valores) {
    if (!comportamiento.valido()) return;
    JNIEnv* env = entorno();
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    jobject objeto = static_cast<jobject>(comportamiento.instancia);
    for (const auto& def : comportamiento.campos) {
        const ReflejoScripts::ValorCampo* valor = nullptr;
        for (const auto& v : valores)
            if (v.nombre == def.nombre) {
                valor = &v;
                break;
            }
        if (!valor) continue;
        auto it = datos->campos.find(def.nombre);
        if (it == datos->campos.end()) continue;
        jfieldID id = it->second;
        using ReflejoScripts::TagTipo;
        switch (def.tag) {
        case TagTipo::Entero:
            env->SetIntField(objeto, id, valor->como<int>());
            break;
        case TagTipo::Flotante:
            env->SetFloatField(objeto, id, valor->como<float>());
            break;
        case TagTipo::Doble:
            env->SetDoubleField(objeto, id, valor->como<double>());
            break;
        case TagTipo::Booleano:
            env->SetBooleanField(objeto, id, valor->como<bool>());
            break;
        case TagTipo::Texto: {
            jstring s =
                env->NewStringUTF(nuevoNombreUTF8(valor->como<std::string>()).c_str());
            env->SetObjectField(objeto, id, s);
            break;
        }
        case TagTipo::Objeto: {
            void* destino = MotorScript::tablaServicios()->objetoPorNombre(
                valor->como<std::string>().c_str());
            env->SetLongField(objeto, id, static_cast<jlong>(
                                               reinterpret_cast<intptr_t>(destino)));
            break;
        }
        default:
            break;
        }
    }
}

std::vector<ReflejoScripts::ValorCampo> BackendJava::extraer(
    ComportamientoCargado& comportamiento) {
    std::vector<ReflejoScripts::ValorCampo> valores;
    if (!comportamiento.valido()) return valores;
    JNIEnv* env = entorno();
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    jobject objeto = static_cast<jobject>(comportamiento.instancia);
    for (const auto& def : comportamiento.campos) {
        auto it = datos->campos.find(def.nombre);
        if (it == datos->campos.end()) continue;
        jfieldID id = it->second;
        ReflejoScripts::ValorCampo valor;
        valor.nombre = def.nombre;
        valor.tag = def.tag;
        using ReflejoScripts::TagTipo;
        switch (def.tag) {
        case TagTipo::Entero:
            valor.contenido = env->GetIntField(objeto, id);
            break;
        case TagTipo::Flotante:
            valor.contenido = env->GetFloatField(objeto, id);
            break;
        case TagTipo::Doble:
            valor.contenido = env->GetDoubleField(objeto, id);
            break;
        case TagTipo::Booleano:
            valor.contenido = static_cast<bool>(env->GetBooleanField(objeto, id));
            break;
        case TagTipo::Texto: {
            jstring s = static_cast<jstring>(env->GetObjectField(objeto, id));
            if (s) {
                const char* utf = env->GetStringUTFChars(s, nullptr);
                valor.contenido = std::string(utf ? utf : "");
                env->ReleaseStringUTFChars(s, utf);
            } else {
                valor.contenido = std::string();
            }
            break;
        }
        case TagTipo::Objeto: {
            const void* destino = reinterpret_cast<const void*>(
                static_cast<intptr_t>(env->GetLongField(objeto, id)));
            const char* nombre = destino
                                     ? MotorScript::tablaApi()->nombre(destino)
                                     : "";
            valor.contenido = std::string(nombre ? nombre : "");
            break;
        }
        default:
            break;
        }
        valores.push_back(std::move(valor));
    }
    return valores;
}