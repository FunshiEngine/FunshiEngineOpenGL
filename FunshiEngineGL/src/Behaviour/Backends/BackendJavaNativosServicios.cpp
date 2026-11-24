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

jboolean nativoTieneEtiqueta(JNIEnv* env, jclass, jlong objeto,
                             jstring etiqueta) {
    if (!etiqueta) return JNI_FALSE;
    const std::string texto = leerCadena(env, etiqueta);
    return MotorScript::tablaApi()->tieneEtiqueta(comoObjeto(objeto),
                                                   texto.c_str())
               ? JNI_TRUE
               : JNI_FALSE;
}

void nativoFijarRotacion(JNIEnv*, jclass, jlong o, jfloat a, jfloat x, jfloat y,
                         jfloat z) {
    MotorScript::tablaApi()->fijarRotacionEjes(comoObjeto(o), a, x, y, z);
}

jint nativoReproducirSonido(JNIEnv* env, jclass, jstring ruta, jfloat volumen,
                            jboolean bucle) {
    if (!ruta) return -1;
    const std::string archivo = leerCadena(env, ruta);
    return MotorScript::tablaServicios()->reproducirSonido(
        archivo.c_str(), volumen, bucle == JNI_TRUE);
}

void nativoDetenerSonido(JNIEnv*, jclass, jint handle) {
    MotorScript::tablaServicios()->detenerSonido(handle);
}

jboolean nativoReproducirMusica(JNIEnv* env, jclass, jstring ruta, jfloat volumen,
                                jfloat fundido) {
    if (!ruta) return JNI_FALSE;
    const std::string archivo = leerCadena(env, ruta);
    return MotorScript::tablaServicios()->reproducirMusica(
               archivo.c_str(), volumen, fundido)
               ? JNI_TRUE
               : JNI_FALSE;
}

void nativoDetenerMusica(JNIEnv*, jclass, jfloat fundido) {
    MotorScript::tablaServicios()->detenerMusica(fundido);
}

void nativoVolumenMusica(JNIEnv*, jclass, jfloat volumen) {
    MotorScript::tablaServicios()->volumenMusica(volumen);
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

jlong nativoInstanciarPrefab(JNIEnv* env, jclass, jstring nombre, jlong padre) {
    if (!nombre) return 0;
    const std::string texto = leerCadena(env, nombre);
    void* res = MotorScript::tablaServicios()->instanciarPrefab(texto.c_str(),
                                                               comoObjeto(padre));
    return static_cast<jlong>(reinterpret_cast<intptr_t>(res));
}

jobjectArray nativoListarPrefabs(JNIEnv* env, jclass) {
    const char** nombres = MotorScript::tablaServicios()->listarPrefabs();
    if (!nombres) return nullptr;
    int count = 0;
    while (nombres[count]) ++count;
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(count, stringClass, nullptr);
    for (int i = 0; i < count; ++i) {
        env->SetObjectArrayElement(arr, i, env->NewStringUTF(nombres[i]));
    }
    return arr;
}

// --- Interfaces web (HUD): el handle es opaco(la direccion del
// InterfaceComponent del objeto), viaja como long igual que los objetos.
jlong nativoInterfazDeObjeto(JNIEnv* env, jclass, jstring nombreObjeto) {
    if (!nombreObjeto) return 0;
    const std::string texto = leerCadena(env, nombreObjeto);
    void* interfaz =
        MotorScript::tablaServicios()->interfazDeObjeto(texto.c_str());
    return static_cast<jlong>(reinterpret_cast<intptr_t>(interfaz));
}

jboolean nativoInterfazActiva(JNIEnv*, jclass, jlong interfaz) {
    return MotorScript::tablaServicios()->interfazActiva(comoObjeto(interfaz))
               ? JNI_TRUE
               : JNI_FALSE;
}

void nativoInterfazEstablecerActiva(JNIEnv*, jclass, jlong interfaz,
                                    jboolean activa) {
    MotorScript::tablaServicios()->interfazEstablecerActiva(
        comoObjeto(interfaz), activa == JNI_TRUE);
}

jboolean nativoInterfazEstablecerEstado(JNIEnv* env, jclass, jlong interfaz,
                                        jstring jsonEstado) {
    if (!jsonEstado) return JNI_FALSE;
    const std::string texto = leerCadena(env, jsonEstado);
    return MotorScript::tablaServicios()->interfazEstablecerEstado(
               comoObjeto(interfaz), texto.c_str())
               ? JNI_TRUE
               : JNI_FALSE;
}

jboolean nativoInterfazEstablecerValor(JNIEnv* env, jclass, jlong interfaz,
                                       jstring clave, jstring valorJson) {
    if (!clave || !valorJson) return JNI_FALSE;
    const std::string k = leerCadena(env, clave);
    const std::string v = leerCadena(env, valorJson);
    return MotorScript::tablaServicios()->interfazEstablecerValor(
               comoObjeto(interfaz), k.c_str(), v.c_str())
               ? JNI_TRUE
               : JNI_FALSE;
}

jstring nativoInterfazValor(JNIEnv* env, jclass, jlong interfaz,
                            jstring rutaJson) {
    if (!rutaJson) return nullptr;
    const std::string r = leerCadena(env, rutaJson);
    const char* valor = MotorScript::tablaServicios()->interfazValor(
        comoObjeto(interfaz), r.c_str());
    if (!valor) return nullptr;
    return env->NewStringUTF(nuevoNombreUTF8(valor).c_str());
}

// --- Interfaz de juego NATIVA (HUD que dibuja el motor): el handle viaja como
// long igual que el de la interfaz web, pero apunta al UIComponent.
jlong nativoInterfazNativaDeObjeto(JNIEnv* env, jclass, jstring nombreObjeto) {
    if (!nombreObjeto) return 0;
    const std::string texto = leerCadena(env, nombreObjeto);
    void* interfaz =
        MotorScript::tablaServicios()->interfazNativaDeObjeto(texto.c_str());
    return static_cast<jlong>(reinterpret_cast<intptr_t>(interfaz));
}

jboolean nativoInterfazNativaVisible(JNIEnv*, jclass, jlong interfaz) {
    return MotorScript::tablaServicios()->interfazNativaVisible(
               comoObjeto(interfaz))
               ? JNI_TRUE
               : JNI_FALSE;
}

void nativoInterfazNativaEstablecerVisible(JNIEnv*, jclass, jlong interfaz,
                                            jboolean visible) {
    MotorScript::tablaServicios()->interfazNativaEstablecerVisible(
        comoObjeto(interfaz), visible == JNI_TRUE);
}

jboolean nativoInterfazNativaEstablecerTexto(JNIEnv* env, jclass,
                                              jlong interfaz, jstring idWidget,
                                              jstring texto) {
    if (!idWidget || !texto) return JNI_FALSE;
    const std::string id = leerCadena(env, idWidget);
    const std::string contenido = leerCadena(env, texto);
    return MotorScript::tablaServicios()->interfazNativaEstablecerTexto(
               comoObjeto(interfaz), id.c_str(), contenido.c_str())
               ? JNI_TRUE
               : JNI_FALSE;
}

jboolean nativoInterfazNativaEstablecerVisibleWidget(JNIEnv* env, jclass,
                                                      jlong interfaz,
                                                      jstring idWidget,
                                                      jboolean visible) {
    if (!idWidget) return JNI_FALSE;
    const std::string id = leerCadena(env, idWidget);
    return MotorScript::tablaServicios()->interfazNativaEstablecerVisibleWidget(
               comoObjeto(interfaz), id.c_str(), visible == JNI_TRUE)
               ? JNI_TRUE
               : JNI_FALSE;
}

jboolean nativoInterfazNativaBotonPulsado(JNIEnv* env, jclass, jlong interfaz,
                                          jstring idWidget) {
    if (!idWidget) return JNI_FALSE;
    const std::string id = leerCadena(env, idWidget);
    return MotorScript::tablaServicios()->interfazNativaBotonPulsado(
               comoObjeto(interfaz), id.c_str())
               ? JNI_TRUE
               : JNI_FALSE;
}
