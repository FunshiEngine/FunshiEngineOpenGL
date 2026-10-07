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
#define FUNSHI_DLOPEN(name) LoadLibraryA((name).c_str())
#define FUNSHI_DLSYM(handle, symbol) GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol)
#define FUNSHI_DLOPENCERRAR(handle) FreeLibrary(reinterpret_cast<HMODULE>(handle))
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "dll"
#elif defined(__APPLE__)
#include <dlfcn.h>
#define FUNSHI_DLOPEN(name) dlopen((name).c_str(), RTLD_NOW | RTLD_LOCAL)
#define FUNSHI_DLSYM(handle, symbol) dlsym(handle, symbol)
#define FUNSHI_DLOPENCERRAR(handle) dlclose(handle)
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "dylib"
#else
#include <dlfcn.h>
#include <unistd.h>
#define FUNSHI_DLOPEN(name) dlopen((name).c_str(), RTLD_NOW | RTLD_LOCAL)
#define FUNSHI_DLSYM(handle, symbol) dlsym(handle, symbol)
#define FUNSHI_DLOPENCERRAR(handle) dlclose(handle)
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "so"
#endif

#include "BackendCpp.h"
#include "ComandoCompilacionCpp.h"
#include "RutaCabecerasScript.h"
#include "../../Configuracion/ProjectPaths.h"
#include "../../FileManager/Proceso.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "../ScriptGameObject.h"
#include "../IScriptBehaviour.h"

#ifndef FUNSHI_CXX_COMPILER
#define FUNSHI_CXX_COMPILER "g++"
#endif
#ifndef FUNSHI_SRC_DIR
#define FUNSHI_SRC_DIR ""
#endif

namespace {
const char* nombreFabrica() { return FUNSHI_SYM_CREAR; }

// Ruta del compilador a invocar, en este orden:
//   1. FUNSHI_CXX del entorno: override explicito del usuario, gana siempre.
//   2. La horneada por CMake, SOLO si existe en disco. Sin esta comprobacion el
//      paquete publicado invoca el compilador del runner que lo compilo
//      (C:\Program Files\Microsoft Visual Studio\...\cl.exe), una ruta que en
//      el equipo del usuario no existe: el fallo era un "no se encuentra" con
//      una ruta de otra maquina, sin pista de cual era el problema.
//   3. "cl" a secas si esta el toolset de MSVC en la maquina. Con el entorno de
//      vcvars en el PATH (ver mas abajo) el nombre sin ruta basta, y asi el
//      motor no queda atado a la version instalada.
//   4. "g++" como nombre standard: lo resuelve el PATH, que es lo unico que se
//      puede afirmar sin conocer la maquina.
// El valor puede quedar con comillas externas en algunos generadores
// (Ninja/MinGW): se normalizan aca y el comando las repone explicitamente,
// porque una ruta sin comillas se parte en el primer espacio (cmd.exe intenta
// ejecutar "C:/Program" y no lo reconoce).
std::string compilador() {
    const char* env = std::getenv("FUNSHI_CXX");
    const std::string overrideEntorno = (env && *env) ? env : std::string();
    if (!overrideEntorno.empty()) {
        std::error_code ec;
        if (!std::filesystem::exists(overrideEntorno, ec))
            std::cerr << "[scripts] FUNSHI_CXX apunta a '" << overrideEntorno
                      << "', que no existe en este equipo; se usara tal cual "
                         "y la compilacion fallara\n";
    }
    return CompilacionCpp::elegirCompilador(
        overrideEntorno, FUNSHI_CXX_COMPILER,
        !CompilacionCpp::vcvars64EnRaices(
             CompilacionCpp::raicesVisualStudio()).empty());
}

// Directorio con las cabeceras del motor para que el script pueda incluir
// ScriptGameObject.h. Prioridad: variable de entorno FUNSHI_SRC_DIR; si no, el
// valor horneado en el build. El valor puede ser absoluto (checkout de
// desarrollo o tests) o un nombre relativo a la carpeta del ejecutable (paquete
// instalado, donde vale "include"). Vacio si no hay cabeceras utilizables, para
// que el backend avise en vez de pasar un -I a una ruta inexistente.
std::string directorioSrcMotor() {
    const char* env = std::getenv("FUNSHI_SRC_DIR");
    const std::string configurado = (env && *env) ? env : FUNSHI_SRC_DIR;
    const auto existe = [](const std::string& ruta) {
        std::error_code ec;
        return std::filesystem::exists(ruta, ec);
    };
    return RutaCabecerasScript::resolver(
        configurado, ProjectPaths::directorioEjecutable(), existe);
}

std::string directorioCache() {
    std::error_code ec;
    std::filesystem::path base;
#if defined(_WIN32)
    const char* tmp = std::getenv("TEMP");
    if (tmp && *tmp) base = tmp;
    else base = ".";
#else
    const char* tmp = std::getenv("TMPDIR");
    if (tmp && *tmp) base = tmp;
    else base = "/tmp";
#endif
    return (base / "funshi_scripts").string();
}

std::string mtimeDe(const std::string& ruta) {
    std::error_code ec;
    auto t = std::filesystem::last_write_time(ruta, ec);
    if (ec) return "";
    return std::to_string(t.time_since_epoch().count());
}

// El artefacto esta vigente si existe y no es mas viejo que su fuente. La
// clave del artefacto es por fuente (con compilador y flags), asi que dos
// componentes de la escena que apuntan al mismo .cpp comparten la salida:
// decidir la recompilacion por la frescura del ARCHIVO, y no por el mtime
// que guarda cada componente (vacio en uno recien cargado), evita volver a
// enlazar una salida que otro componente del mismo proceso ya dejo cargada.
// En Windows una imagen cargada bloquea su archivo y el enlazador no puede
// reescribirlo (permiso denegado); con el artefacto al dia, el segundo
// componente solo lo vuelve a abrir (la biblioteca se referencia, no se
// duplica).
bool artefactoVigente(const std::string& artefactoPath,
                      const std::string& fuente) {
    std::error_code ecArtefacto;
    const auto tArtefacto =
        std::filesystem::last_write_time(artefactoPath, ecArtefacto);
    if (ecArtefacto) return false; // no existe o ilegible: hay que compilar
    std::error_code ecFuente;
    const auto tFuente = std::filesystem::last_write_time(fuente, ecFuente);
    if (ecFuente) return false;
    return tArtefacto >= tFuente;
}
} // namespace

const char* BackendCpp::lenguaje() const { return "cpp"; }

std::string BackendCpp::compiladorRuta() {
    return compilador();
}

std::string BackendCpp::cacheDir() { return directorioCache(); }

std::string BackendCpp::artefacto(const std::string& fuente) {
    // La clave del artefacto incluye el CONTRATO de compilacion (compilador +
    // flags), no solo la ruta del fuente: la decision de recompilar compara
    // tiempos de archivo y no mira los flags, asi que un cambio de flags
    // reusaria el artefacto viejo para siempre (un .dll compilado con el CRT
    // estatico seguiria cargandose y el desajuste de heap no se corregiria
    // nunca). Cambiar la clave equivale a invalidar el cache: el artefacto
    // anterior simplemente no se encuentra y se recompila en el proximo uso,
    // sin que el usuario tenga que borrar %TEMP%/funshi_scripts a mano.
    const std::string clave =
        std::filesystem::weakly_canonical(fuente).string() + "|" + compilador() +
        "|" + CompilacionCpp::flagsCompilador(std::string()) + "|runtime=" +
        std::to_string(MotorScript::versionRuntimeScript);
    std::size_t hash = std::hash<std::string>{}(clave);
    return (std::filesystem::path(directorioCache()) /
            ("script_" + std::to_string(hash) + "." + FUNSHI_ARTEFACTO_EXT))
        .string();
}

bool BackendCpp::compilarYCargar(const std::string& fuente,
                                 const std::string& nombreClase,
                                 ComportamientoCargado& salida,
                                 std::string& error) {
    std::error_code ec;
    if (!std::filesystem::exists(fuente, ec)) {
        error = "El fuente del script no existe:\n" + fuente;
        return false;
    }

    try {
        std::filesystem::create_directories(directorioCache(), ec);
    } catch (...) {
    }

    const std::string artefactoPath = artefacto(fuente);
    const std::string mtime = mtimeDe(fuente);

    // Compilar solo si el artefacto quedo por detras del fuente (hot reload
    // y primera carga); si ya esta al dia se usa tal cual, aunque el
    // componente que lo trae no lo haya cargado nunca.
    const bool hayQueRecompilar = !artefactoVigente(artefactoPath, fuente);
    if (hayQueRecompilar) {
        // Los ARGV del proceso hijo se arman en ComandoCompilacionCpp.h: la
        // familia del compilador (MSVC o GCC/Clang) decide flags, include y
        // salida, y el contrato de CRT con el engine lo verifica el test de
        // esa suite. Sin shell (H-3 nivel 2): cada token va como argumento
        // propio y la salida la redirige Proceso por handles/fd.
        const std::string logPath =
            (std::filesystem::path(directorioCache()) / "compilar.log")
                .string();
        CompilacionCpp::DatosComando datos;
        datos.compilador = compilador();
        datos.nombreClase = nombreClase;
        datos.fuente = fuente;
        // Sin cabeceras del motor el script no puede incluir IScriptBehaviour.h.
        // Se avisa antes de invocar al compilador: un -I a una ruta inexistente
        // solo produce el "No such file or directory" del toolchain, que no
        // explica al usuario que falta el paquete de cabeceras.
        datos.dirSrc = directorioSrcMotor();
        if (datos.dirSrc.empty()) {
            error =
                "No se encontraron las cabeceras del motor para compilar el "
                "script C++ (se esperaba la carpeta 'include' junto al "
                "ejecutable, o la ruta de la variable FUNSHI_SRC_DIR).";
            return false;
        }
        datos.dirObjetos = directorioCache();
        datos.artefacto = artefactoPath;
        const std::vector<std::string> argv =
            CompilacionCpp::argumentosCompilacion(datos);

        int rc = -1;
        // Entorno de MSVC: primero el que se deduce de la ruta del compilador
        // ya resuelto (build de desarrollo, donde la ruta es de esta maquina) y,
        // si no hay ninguno, el que tenga instalado el usuario. Sin este segundo
        // paso el paquete publicado no podia compilar un script C++ ni en un
        // equipo con Visual Studio al lado, porque la ruta horneada no existe
        // ahi. Se reutiliza datos.compilador y no se vuelve a resolver: la
        // eleccion se hace una vez para que flags y binario no puedan
        // discrepar.
        std::string vcvars = CompilacionCpp::vcvars64Ruta(datos.compilador);
        if (vcvars.empty())
            vcvars = CompilacionCpp::vcvars64EnRaices(
                CompilacionCpp::raicesVisualStudio());
#if defined(_WIN32)
        if (!vcvars.empty()) {
            // MSVC: cl.exe necesita el entorno del toolset (INCLUDE/LIB/
            // link.exe). Lo unico que todavia pasa por cmd.exe es la receta
            // fija del harvest (sin datos de usuario, ver
            // comandoEntornoVcvars); el compilador corre solo, con ese bloque
            // UTF-16 como entorno. Si el harvest falla se hereda el entorno
            // del motor y el error exacto de cl queda en el log.
            const std::wstring& bloque =
                CompilacionCpp::entornoVcvars(vcvars);
            if (!bloque.empty())
                rc = Proceso::ejecutarConBloque(argv, bloque, logPath);
            else {
                std::cerr << "[scripts] entorno de vcvars no disponible; se "
                             "compila con el entorno heredado\n";
                rc = Proceso::ejecutar(argv, logPath);
            }
        } else
#endif
        {
            (void)vcvars; // fuera de MSVC siempre viene vacio
            rc = Proceso::ejecutar(argv, logPath);
        }
        if (rc != 0) {
            std::ifstream log(logPath);
            std::string contenido((std::istreambuf_iterator<char>(log)),
                                  std::istreambuf_iterator<char>());
            error = "Error al compilar el script C++:\n" + contenido;
            return false;
        }
    }

    void* manejador = FUNSHI_DLOPEN(artefactoPath);
    if (!manejador) {
        std::string detalle;
#if !defined(_WIN32)
        const char* d = dlerror();
        if (d) detalle = d;
#endif
        error = "No se pudo cargar el artefacto del script:\n" + artefactoPath;
        if (!detalle.empty()) error += "\n" + detalle;
        return false;
    }

    using Fabrica =
        IScriptBehaviour* (*)(const MotorScript::ApiScriptGameObject*);
    Fabrica fabrica =
        reinterpret_cast<Fabrica>(FUNSHI_DLSYM(manejador, nombreFabrica()));
    if (!fabrica) {
        // En Windows/MSVC este error suele significar que el fuente usa el
        // template viejo, sin FUNSHI_COMPORTAMIENTO_EXPORT en la fabrica: la
        // .dll compila, pero el simbolo no se exporta y GetProcAddress no lo
        // encuentra (H-15). El motor ya pide el export en el link para esos
        // fuentes, pero los scripts compilados ANTES de ese cambio siguen sin
        // exportarlo: basta con borrar el artefacto viejo o tocar el fuente
        // para que recompile.
        error = "El .so no exporta 'FUNSHI_CREAR_COMPORTAMIENTO'. ¿El fuente "
                "deriva de IScriptBehaviour y usa el template del motor?";
        FUNSHI_DLOPENCERRAR(manejador);
        return false;
    }

    IScriptBehaviour* instancia = fabrica(MotorScript::tablaApi());
    if (!instancia) {
        error = "La fabrica devolvio un comportamiento nulo.";
        FUNSHI_DLOPENCERRAR(manejador);
        return false;
    }

    // La fábrica recibe la tabla pero el template no la guarda: el motor la
    // inyecta acá para que `this->api` quede siempre disponible en el script.
    instancia->conectarApi(MotorScript::tablaApi());

    salida.fuente = fuente;
    salida.lenguaje = "cpp";
    salida.artefacto = artefactoPath;
    salida.manejador = manejador;
    salida.instancia = instancia;
    salida.campos = instancia->camposReflejados();
    salida.mtimeFuente = mtime;
    salida.cargado = true;
    error.clear();
    return true;
}

void BackendCpp::descargar(ComportamientoCargado& comportamiento) {
    descargar(comportamiento, nullptr);
}

void BackendCpp::descargar(ComportamientoCargado& comportamiento,
                           GameObject* owner) {
    if (!comportamiento.cargado) return;
    if (owner && comportamiento.instancia) {
        reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)
            ->onStop(owner);
    }
    if (comportamiento.instancia) {
        delete reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia);
    }
    void* manejador = comportamiento.manejador;
    // IMPORTANTE: destruir el arbol de campos (std::function con target dentro
    // del .so) ANTES de dlclose; si no, los destructores saltan a codigo
    // descargado y segfaultean.
    comportamiento = ComportamientoCargado{};
    FUNSHI_DLOPENCERRAR(manejador);
}

void BackendCpp::llamarInicio(ComportamientoCargado& comportamiento,
                              GameObject* owner) {
    if (!comportamiento.valido()) return;
    reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)->onStart(owner);
}

void BackendCpp::llamarActualizar(ComportamientoCargado& comportamiento,
                                  GameObject* owner, float deltaTime) {
    if (!comportamiento.valido()) return;
    reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)
        ->onUpdate(owner, deltaTime);
}

void BackendCpp::llamarDetener(ComportamientoCargado& comportamiento,
                               GameObject* owner) {
    if (!comportamiento.valido()) return;
    reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia)->onStop(owner);
}

void BackendCpp::llamarContacto(ComportamientoCargado& comportamiento,
                                GameObject* owner, Collider* propio,
                                Collider* otro, TipoContacto tipo) {
    if (!comportamiento.valido()) return;
    IScriptBehaviour* script =
        reinterpret_cast<IScriptBehaviour*>(comportamiento.instancia);
    switch (tipo) {
        case TipoContacto::Inicio:
            script->onCollisionEnter(owner, propio, otro);
            break;
        case TipoContacto::Persistencia:
            script->onCollisionStay(owner, propio, otro);
            break;
        case TipoContacto::Fin:
            script->onCollisionExit(owner, propio, otro);
            break;
    }
}