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

namespace {

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

}  // namespace

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
