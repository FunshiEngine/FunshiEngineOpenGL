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
// Preambulo compartido por las unidades de BackendJava.cpp (generado al dividirlo).
#ifndef BACKENDJAVADETALLE_H
#define BACKENDJAVADETALLE_H

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
#include "../../Diagnosticos/RegistroDiagnosticos.h"
#include "../../FileManager/Procesos/Proceso.h"
#ifndef FUNSHI_LIBJVM_DEFAULT
#define FUNSHI_LIBJVM_DEFAULT ""
#endif
#ifndef FUNSHI_JAVAC_DEFAULT
#define FUNSHI_JAVAC_DEFAULT "javac"
#endif

namespace fs = std::filesystem;

namespace BackendJavaDetalle {

// --- Bootstrap dinamico del JVM --------------------------------------------
struct Jvm {
    void* biblioteca = nullptr;
    JavaVM* jvm = nullptr;
    JNIEnv* env = nullptr;
    std::string error;
};

}  // namespace BackendJavaDetalle
using namespace BackendJavaDetalle;

Jvm& jvm();

namespace BackendJavaDetalle {

typedef jint(JNICALL* FnCrearJavaVM)(JavaVM**, void**, void*);

}  // namespace BackendJavaDetalle
using namespace BackendJavaDetalle;

std::string directorioCache();

namespace {

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

}  // namespace

fs::path libjvmEnRaiz(const fs::path& raiz);
fs::path javacEnRaiz(const fs::path& raiz);
fs::path raizJreEmbebido();

namespace {

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

}  // namespace

std::vector<fs::path> raicesJdk();
const std::vector<fs::path>& raicesJdkCache();
const ResolucionJdk::Herramientas& herramientasJdk();

namespace {

std::string rutaLibjvm() { return herramientasJdk().libjvm; }

}  // namespace

bool arrancarJvm(const std::string& classesDir, std::string& error);

namespace {

JNIEnv* entorno() { return jvm().env; }

}  // namespace

std::string javacExe();

namespace {

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

}  // namespace

std::string detalleExcepcion(JNIEnv* env);

namespace BackendJavaDetalle {

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

}  // namespace BackendJavaDetalle
using namespace BackendJavaDetalle;

namespace {

std::string leerCadena(JNIEnv* env, jstring valor) {
    if (!env || !valor) return {};
    const char* utf = env->GetStringUTFChars(valor, nullptr);
    if (!utf) return {};
    const std::string utf8(utf);
    env->ReleaseStringUTFChars(valor, utf);
    return utf8;
}

}  // namespace

std::string contextoHerramientas(const std::string& classesDir);

namespace BackendJavaDetalle {

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

}  // namespace BackendJavaDetalle
using namespace BackendJavaDetalle;

namespace {

// --- Nativos expuestos a Java (Nativo.xxx) ----------------------------------
void* comoObjeto(jlong objeto) {
    return reinterpret_cast<void*>(static_cast<intptr_t>(objeto));
}

}  // namespace

jstring nativoNombre(JNIEnv* env, jclass, jlong objeto);
jfloat nativoPosicionX(JNIEnv*, jclass, jlong o);
jfloat nativoPosicionY(JNIEnv*, jclass, jlong o);
jfloat nativoPosicionZ(JNIEnv*, jclass, jlong o);
void nativoFijarPosicion(JNIEnv*, jclass, jlong o, jfloat x, jfloat y, jfloat z);
jboolean nativoFijarVelocidadHorizontal(JNIEnv*, jclass, jlong o, jfloat x, jfloat z);
jboolean nativoSaltar(JNIEnv*, jclass, jlong o, jfloat velocidad);
jstring nativoEtiqueta(JNIEnv* env, jclass, jlong objeto);
jboolean nativoTieneEtiqueta(JNIEnv* env, jclass, jlong objeto, jstring etiqueta);
jlong nativoObjetoDeCollider(JNIEnv*, jclass, jlong collider);
void nativoFijarEscala(JNIEnv*, jclass, jlong o, jfloat x, jfloat y, jfloat z);
void nativoFijarRotacion(JNIEnv*, jclass, jlong o, jfloat a, jfloat x, jfloat y, jfloat z);
jfloat nativoRotacion(JNIEnv*, jclass, jlong objeto, int eje);
jfloat nativoEscala(JNIEnv*, jclass, jlong objeto, int eje);
jfloat nativoRotacionAngulo(JNIEnv* e, jclass c, jlong o);
jfloat nativoRotacionEjeX(JNIEnv* e, jclass c, jlong o);
jfloat nativoRotacionEjeY(JNIEnv* e, jclass c, jlong o);
jfloat nativoRotacionEjeZ(JNIEnv* e, jclass c, jlong o);
jfloat nativoEscalaX(JNIEnv* e, jclass c, jlong o);
jfloat nativoEscalaY(JNIEnv* e, jclass c, jlong o);
jfloat nativoEscalaZ(JNIEnv* e, jclass c, jlong o);
void nativoImprimir(JNIEnv* env, jclass, jstring texto);
jint nativoReproducirSonido(JNIEnv* env, jclass, jstring ruta, jfloat volumen, jboolean bucle);
void nativoDetenerSonido(JNIEnv*, jclass, jint handle);
jboolean nativoReproducirMusica(JNIEnv* env, jclass, jstring ruta, jfloat volumen, jfloat fundido);
void nativoDetenerMusica(JNIEnv*, jclass, jfloat fundido);
void nativoVolumenMusica(JNIEnv*, jclass, jfloat volumen);
jlong nativoObjetoPorNombre(JNIEnv* env, jclass, jstring nombre);
jlong nativoObjetoPorId(JNIEnv*, jclass, jint id);
jlong nativoObjetoPorEtiqueta(JNIEnv* env, jclass, jstring etiqueta);
jboolean nativoTecla(JNIEnv* env, jstring tecla, int consulta);
jboolean nativoTeclaSostiene(JNIEnv* env, jclass, jstring tecla);
jboolean nativoTeclaPresionada(JNIEnv* env, jclass, jstring tecla);
jboolean nativoTeclaSoltada(JNIEnv* env, jclass, jstring tecla);
jfloat nativoDeltaMouseX(JNIEnv*, jclass);
jfloat nativoDeltaMouseY(JNIEnv*, jclass);
jfloat nativoMasa(JNIEnv*, jclass, jlong o);
jboolean nativoFijarMasa(JNIEnv*, jclass, jlong o, jfloat masa);
jboolean nativoUsaGravedad(JNIEnv*, jclass, jlong o);
jboolean nativoFijarUsoGravedad(JNIEnv*, jclass, jlong o, jboolean usar);
jfloat nativoEscalaGravedad(JNIEnv*, jclass, jlong o);
jboolean nativoFijarEscalaGravedad(JNIEnv*, jclass, jlong o, jfloat escala);
jfloat nativoFriccion(JNIEnv*, jclass, jlong o);
jboolean nativoFijarFriccion(JNIEnv*, jclass, jlong o, jfloat friccion);
jboolean nativoFreezeEje(JNIEnv*, jclass, jlong o, jint eje, bool rotacion);
jboolean nativoPosicionCongelada(JNIEnv* e, jclass c, jlong o, jint eje);
jboolean nativoRotacionCongelada(JNIEnv* e, jclass c, jlong o, jint eje);
jboolean nativoFijarFreeze(JNIEnv*, jclass, jlong o, jboolean x, jboolean y, jboolean z, bool rotacion);
jboolean nativoFijarFreezePosicion(JNIEnv* e, jclass c, jlong o, jboolean x, jboolean y, jboolean z);
jboolean nativoFijarFreezeRotacion(JNIEnv* e, jclass c, jlong o, jboolean x, jboolean y, jboolean z);
void nativoFijarGravedadGlobal(JNIEnv*, jclass, jfloat x, jfloat y, jfloat z);
jfloat nativoGravedadGlobalEje(JNIEnv*, jclass, int eje);
jfloat nativoGravedadGlobalX(JNIEnv* e, jclass c);
jfloat nativoGravedadGlobalY(JNIEnv* e, jclass c);
jfloat nativoGravedadGlobalZ(JNIEnv* e, jclass c);
jlong nativoCrearObjeto(JNIEnv* env, jclass, jstring nombre, jlong padre);
jboolean nativoDestruirObjeto(JNIEnv*, jclass, jlong o);
jlong nativoClonarObjeto(JNIEnv*, jclass, jlong original, jlong padre);
jboolean nativoAgregarColliderEsfera(JNIEnv*, jclass, jlong o, jfloat radio);
jboolean nativoAgregarColliderCubo(JNIEnv*, jclass, jlong o, jfloat radio);
jboolean nativoAgregarRigidBody(JNIEnv*, jclass, jlong o, jfloat masa);
jlong nativoInstanciarPrefab(JNIEnv* env, jclass, jstring nombre, jlong padre);
jobjectArray nativoListarPrefabs(JNIEnv* env, jclass);
jlong nativoInterfazDeObjeto(JNIEnv* env, jclass, jstring nombreObjeto);
jboolean nativoInterfazActiva(JNIEnv*, jclass, jlong interfaz);
void nativoInterfazEstablecerActiva(JNIEnv*, jclass, jlong interfaz, jboolean activa);
jboolean nativoInterfazEstablecerEstado(JNIEnv* env, jclass, jlong interfaz, jstring jsonEstado);
jboolean nativoInterfazEstablecerValor(JNIEnv* env, jclass, jlong interfaz, jstring clave, jstring valorJson);
jstring nativoInterfazValor(JNIEnv* env, jclass, jlong interfaz, jstring rutaJson);
jlong nativoInterfazNativaDeObjeto(JNIEnv* env, jclass, jstring nombreObjeto);
jboolean nativoInterfazNativaVisible(JNIEnv*, jclass, jlong interfaz);
void nativoInterfazNativaEstablecerVisible(JNIEnv*, jclass, jlong interfaz, jboolean visible);
jboolean nativoInterfazNativaEstablecerTexto(JNIEnv* env, jclass, jlong interfaz, jstring idWidget, jstring texto);
jboolean nativoInterfazNativaEstablecerVisibleWidget(JNIEnv* env, jclass, jlong interfaz, jstring idWidget, jboolean visible);
jboolean nativoInterfazNativaBotonPulsado(JNIEnv* env, jclass, jlong interfaz, jstring idWidget);
bool registrarNativos(const std::string& clasesDir, std::string& error);

// Estado de la unidad original: una sola instancia (se define en BackendJava.cpp).

namespace BackendJavaDetalle {

extern const char* SRC_COMPORTAMIENTO;
extern const char* SRC_NATIVO;
extern const char* SRC_CARGADOR;

}  // namespace BackendJavaDetalle
using namespace BackendJavaDetalle;

#endif
