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

namespace {

void informarExcepcion(ComportamientoCargado& comportamiento, JNIEnv* env,
                       const char* operacion) {
    if (!env || !env->ExceptionCheck()) return;
    const std::string detalle = detalleExcepcion(env);
    if (comportamiento.falloRuntime) return;
    comportamiento.falloRuntime = true;
    comportamiento.callbackFallido = operacion;
    comportamiento.errorRuntime = detalle;
    try {
        EventoDiagnostico evento;
        evento.nivel = NivelDiagnostico::Error;
        evento.modulo = "Scripts";
        evento.tipoExcepcion = "SCRIPT_CALLBACK_FAILED";
        evento.mensaje = "El callback Java '" + comportamiento.callbackFallido +
                         "' fallo en '" + comportamiento.fuente + "': " + detalle;
        RegistroDiagnosticos::global().registrar(std::move(evento));
    } catch (...) {
    }
}

}  // namespace

void BackendJava::descargar(ComportamientoCargado& comportamiento) {
    descargar(comportamiento, nullptr);
}

void BackendJava::descargar(ComportamientoCargado& comportamiento,
                            GameObject* owner) {
    if (!comportamiento.cargado) return;
    JNIEnv* env = entorno();
    if (env) {
        if (owner && comportamiento.instancia && !comportamiento.falloRuntime) {
            auto* datos = static_cast<DatosJava*>(comportamiento.datos);
            if (datos && datos->detener)
                env->CallVoidMethod(
                    static_cast<jobject>(comportamiento.instancia),
                    datos->detener, static_cast<jlong>(
                                        reinterpret_cast<intptr_t>(owner)));
            informarExcepcion(comportamiento, env, "detener");
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
    if (!comportamiento.valido() || comportamiento.falloRuntime) return;
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    if (!datos || !datos->iniciar) return;
    entorno()->CallVoidMethod(
        static_cast<jobject>(comportamiento.instancia), datos->iniciar,
        static_cast<jlong>(reinterpret_cast<intptr_t>(owner)));
    informarExcepcion(comportamiento, entorno(), "iniciar");
}

void BackendJava::llamarActualizar(ComportamientoCargado& comportamiento,
                                   GameObject* owner, float deltaTime) {
    if (!comportamiento.valido() || comportamiento.falloRuntime) return;
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    if (!datos || !datos->actualizar) return;
    entorno()->CallVoidMethod(
        static_cast<jobject>(comportamiento.instancia), datos->actualizar,
        static_cast<jlong>(reinterpret_cast<intptr_t>(owner)),
        static_cast<jdouble>(deltaTime));
    informarExcepcion(comportamiento, entorno(), "actualizar");
}

void BackendJava::llamarDetener(ComportamientoCargado& comportamiento,
                                GameObject* owner) {
    if (!comportamiento.valido() || comportamiento.falloRuntime) return;
    auto* datos = static_cast<DatosJava*>(comportamiento.datos);
    if (!datos || !datos->detener) return;
    entorno()->CallVoidMethod(
        static_cast<jobject>(comportamiento.instancia), datos->detener,
        static_cast<jlong>(reinterpret_cast<intptr_t>(owner)));
    informarExcepcion(comportamiento, entorno(), "detener");
}

void BackendJava::llamarContacto(ComportamientoCargado& comportamiento,
                                 GameObject* owner, Collider* propio,
                                 Collider* otro, TipoContacto tipo) {
    if (!comportamiento.valido() || comportamiento.falloRuntime) return;
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
    informarExcepcion(comportamiento, entorno(), nombre);
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
