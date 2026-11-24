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

jlong nativoObjetoDeCollider(JNIEnv*, jclass, jlong collider) {
    void* owner =
        MotorScript::tablaApi()->objetoDeCollider(comoObjeto(collider));
    return static_cast<jlong>(reinterpret_cast<intptr_t>(owner));
}

void nativoFijarEscala(JNIEnv*, jclass, jlong o, jfloat x, jfloat y, jfloat z) {
    MotorScript::tablaApi()->fijarEscala(comoObjeto(o), x, y, z);
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
