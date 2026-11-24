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

// Plugin de prueba del sistema de mods: una biblioteca compartida que exporta
// la funcion de entrada que el motor busca en el binario de un mod. Escribe en
// un archivo (indicado por la variable de entorno FUNSHI_MODS_TEST_LOG) los
// eventos que recibe, para que el test verifique el orden de carga, la
// inicializacion y la descarga sin depender de la GUI.
//
// Hay tres variantes, distinguidas por el token que define el build:
//   A y B  exportan la funcion de entrada (dos mods distintos, para comprobar
//          el orden de carga y de descarga).
//   S      NO exporta la funcion de entrada: sirve para comprobar que el motor
//          rechaza un binario que no trae la API.
//
// El plugin no usa la biblioteca estandar de C++ mas alla de C: asi el
// binario no arrastra el runtime de C++ ni libgcc, que en la maquina donde se
// carga pueden no estar.

#include <cstdio>
#include <cstdlib>

#include "Mods/Api/ApiMod.h"

namespace {

// Token del build: distingue las variantes en el archivo de log.
#if defined(FUNSHI_MODS_TEST_TOKEN_A)
constexpr const char* kToken = "A";
#elif defined(FUNSHI_MODS_TEST_TOKEN_B)
constexpr const char* kToken = "B";
#else
constexpr const char* kToken = "S";
#endif

FILE* archivoLog() {
    static FILE* archivo = nullptr;
    if (!archivo) {
        const char* ruta = std::getenv("FUNSHI_MODS_TEST_LOG");
        if (ruta) archivo = std::fopen(ruta, "ab");
    }
    return archivo;
}

void escribir(const char* evento) {
    FILE* archivo = archivoLog();
    if (!archivo) return;
    std::fprintf(archivo, "%s:%s\n", kToken, evento);
    std::fflush(archivo);
}

void alIniciar(const FunshiMods::ApiModServicios* servicios) {
    escribir("init");
    FILE* archivo = archivoLog();
    if (archivo && servicios) {
        std::fprintf(archivo, "%s:servicios=%d\n", kToken, servicios->version);
        std::fflush(archivo);
    }
}

void alActualizar(float deltaTime) {
    (void)deltaTime;
    escribir("update");
}

void alDibujar(const FunshiMods::ContextoMod* contexto) {
    (void)contexto;
    escribir("dibujar");
}

void alDetener() {
    escribir("shutdown");
}

const char* alNombre() {
    return "Mod de prueba";
}

const char* alVersion() {
    return "1.0.0";
}

FunshiMods::IMod interfaz = {alIniciar, alActualizar, alDibujar, alDetener,
                             alNombre, alVersion};

} // namespace

#if !defined(FUNSHI_MODS_TEST_SIN_SIMBOLO)
extern "C" FUNSHI_MOD_EXPORT int funshi_mod_api(FunshiMods::IMod** salida) {
    escribir("entrada");
    if (!salida) return 0;
    *salida = &interfaz;
    return FunshiMods::versionApiMod;
}
#endif
