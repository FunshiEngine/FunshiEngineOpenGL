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

namespace BackendJavaDetalle {

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
    "    public static native long instanciarPrefab(String nombre, long padre);\n"
    "    public static native String[] listarPrefabs();\n"
    "    public static native long interfazDeObjeto(String nombreObjeto);\n"
    "    public static native boolean interfazActiva(long interfaz);\n"
    "    public static native void interfazEstablecerActiva(long interfaz, boolean activa);\n"
    "    public static native boolean interfazEstablecerEstado(long interfaz, String jsonEstado);\n"
    "    public static native boolean interfazEstablecerValor(long interfaz, String clave, String valorJson);\n"
    "    public static native String interfazValor(long interfaz, String rutaJson);\n"
    "    public static native long interfazNativaDeObjeto(String nombreObjeto);\n"
    "    public static native boolean interfazNativaVisible(long interfaz);\n"
    "    public static native void interfazNativaEstablecerVisible(long interfaz, boolean visible);\n"
    "    public static native boolean interfazNativaEstablecerTexto(long interfaz, String idWidget, String texto);\n"
    "    public static native boolean interfazNativaEstablecerVisibleWidget(long interfaz, String idWidget, boolean visible);\n"
    "    public static native boolean interfazNativaBotonPulsado(long interfaz, String idWidget);\n"
    "    public static native void imprimir(String texto);\n"
    "    public static native int reproducirSonido(String ruta, float volumen, boolean bucle);\n"
    "    public static native void detenerSonido(int handle);\n"
    "    public static native boolean reproducirMusica(String ruta, float volumen, float fundido);\n"
    "    public static native void detenerMusica(float fundido);\n"
    "    public static native void volumenMusica(float volumen);\n"
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

}  // namespace BackendJavaDetalle
using namespace BackendJavaDetalle;

Jvm& jvm() {
    static Jvm instancia;
    return instancia;
}

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
