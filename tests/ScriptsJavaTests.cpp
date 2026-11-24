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
#include "ScriptsJavaTestsDetalle.h"

namespace ScriptsJavaTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace ScriptsJavaTestsDetalle
using namespace ScriptsJavaTestsDetalle;

namespace MotorScript {

namespace ScriptsJavaTestsDetalle {

float posicion[3] = {1.0f, 2.0f, 3.0f};
float escala[3] = {1.5f, 2.5f, 3.5f};
float rotacion[4] = {0.25f, 0.0f, 1.0f, 0.0f};
int handleSonidoDetenido = -1;
std::string textoImpreso;
float masaCuerpo = 2.0f;
float escalaGrav = 1.0f;
float friccionCuerpo = 0.5f;
bool freezeP[3] = {false, false, false};
bool freezeR[3] = {false, false, false};
float gravMundo[3] = {0.0f, -1.0f, 0.0f};
float fundidoSalida = -1.0f;
float volumenBus = -1.0f;

}  // namespace ScriptsJavaTestsDetalle
using namespace ScriptsJavaTestsDetalle;

}  // namespace MotorScript

namespace MotorScript {

namespace {

bool usaGrav = true;

void* objetoCreado = reinterpret_cast<void*>(static_cast<intptr_t>(0x4000));

void* objetoClonado = reinterpret_cast<void*>(static_cast<intptr_t>(0x5000));

bool musicaReproducida = false;

}  // namespace

const ApiScriptGameObject* tablaApi() {
    static const ApiScriptGameObject tabla = {
        [](const void* objeto) {
            return objeto ==
                           reinterpret_cast<void*>(
                               static_cast<intptr_t>(0x1234))
                       ? "meta"
                       : "stub";
        },
        [](const void*) { return posicion[0]; },
        [](const void*) { return posicion[1]; },
        [](const void*) { return posicion[2]; },
        [](void*, float x, float y, float z) {
            posicion[0] = x;
            posicion[1] = y;
            posicion[2] = z;
        },
        [](void*, float x, float y, float z) {
            escala[0] = x;
            escala[1] = y;
            escala[2] = z;
        },
        [](void*, float a, float x, float y, float z) {
            rotacion[0] = a;
            rotacion[1] = x;
            rotacion[2] = y;
            rotacion[3] = z;
        },
        [](const char* texto) { textoImpreso = texto ? texto : ""; },
        [](const void*) { return rotacion[0]; },
        [](const void*) { return rotacion[1]; },
        [](const void*) { return rotacion[2]; },
        [](const void*) { return rotacion[3]; },
        [](const void*) { return escala[0]; },
        [](const void*) { return escala[1]; },
        [](const void*) { return escala[2]; },
        [](void*, float, float) { return true; },
        [](void*, float) { return true; },
        [](const void* objeto) {
            return objeto ==
                           reinterpret_cast<void*>(
                               static_cast<intptr_t>(0x1234))
                       ? "suelo"
                       : "jugador";
        },
        [](const void* objeto, const char* tag) {
            return objeto ==
                       reinterpret_cast<void*>(
                           static_cast<intptr_t>(0x1234)) &&
                   tag && std::string(tag) == "suelo";
        },
        [](const void* collider) {
            return collider ==
                            reinterpret_cast<void*>(
                                static_cast<intptr_t>(0xBEEF))
                        ? reinterpret_cast<void*>(
                              static_cast<intptr_t>(0x1234))
                        : nullptr;
        },
        [](const void*) { return masaCuerpo; },
        [](void*, float m) {
            masaCuerpo = m;
            return true;
        },
        [](const void*) { return usaGrav; },
        [](void*, bool u) {
            usaGrav = u;
            return true;
        },
        [](const void*) { return escalaGrav; },
        [](void*, float e) {
            escalaGrav = e;
            return true;
        },
        [](const void*) { return friccionCuerpo; },
        [](void*, float f) {
            friccionCuerpo = f;
            return true;
        },
        [](const void*, int eje) {
            return eje >= 0 && eje < 3 && freezeP[eje];
        },
        [](void*, bool x, bool y, bool z) {
            freezeP[0] = x;
            freezeP[1] = y;
            freezeP[2] = z;
            return true;
        },
        [](const void*, int eje) {
            return eje >= 0 && eje < 3 && freezeR[eje];
        },
        [](void*, bool x, bool y, bool z) {
            freezeR[0] = x;
            freezeR[1] = y;
            freezeR[2] = z;
            return true;
        },
        5,
    };
    return &tabla;
}

const ScriptServices* tablaServicios() {
    static const ScriptServices servicios = {
        // --- v1 ---
        [](const char* ruta, float volumen, bool bucle) {
            return ruta && std::string(ruta) == "disparo.wav" &&
                           volumen == 0.5f && !bucle
                       ? 73
                       : -1;
        },
        [](int handle) { handleSonidoDetenido = handle; },
        // --- v3 ---
        [](const char* nombre) {
            return nombre && std::string(nombre) == "Meta"
                       ? reinterpret_cast<void*>(static_cast<intptr_t>(0x1234))
                       : nullptr;
        },
        [](int id) {
            return id == 7 ? reinterpret_cast<void*>(static_cast<intptr_t>(0x1234))
                           : nullptr;
        },
        [](const char* etiqueta) {
            return etiqueta && std::string(etiqueta) == "suelo"
                       ? reinterpret_cast<void*>(static_cast<intptr_t>(0x1234))
                       : nullptr;
        },
        // teclado
        [](const char* tecla) { return tecla && std::string(tecla) == "W"; },
        [](const char* tecla) { return tecla && std::string(tecla) == "SPACE"; },
        [](const char* tecla) { return tecla && std::string(tecla) == "D0"; },
        // mouse
        []() { return 12.5f; },
        []() { return -4.0f; },
        // --- v4: gravedad global y gestion de objetos ---
        [](float x, float y, float z) {
            gravMundo[0] = x;
            gravMundo[1] = y;
            gravMundo[2] = z;
        },
        []() { return gravMundo[0]; },
        []() { return gravMundo[1]; },
        []() { return gravMundo[2]; },
        [](const char*, void*) { return objetoCreado; },
        [](void* o) { return o == objetoCreado || o == objetoClonado; },
        [](const void*, void*) { return objetoClonado; },
        // v4 continuacion: colliders y rigidbody
        [](void*, float) { return true; },  // agregarColliderEsfera
        [](void*, float) { return true; },  // agregarColliderCubo
        [](void*, float) { return true; },  // agregarRigidBody
        // --- v5: colisiones y raycast ---
        [](const void*) { return objetoCreado; },  // objetoDeCollider
        [](float, float, float, float, float, float, float,
             float*, float*, float*, float*, float*, float*, float*, void**)
            { return false; },  // raycaste
        []() { return 0.0f; },  // puntoImpactoX
        []() { return 0.0f; },  // puntoImpactoY
        []() { return 0.0f; },  // puntoImpactoZ
        []() { return 0.0f; },  // normalImpactoX
        []() { return 0.0f; },  // normalImpactoY
        []() { return 0.0f; },  // normalImpactoZ
        []() { return 0.0f; },  // distanciaImpacto
        // --- v6: prefabs e interfaz web ---
        [](const char*, void*) { return objetoCreado; },  // instanciarPrefab
        []() {
            static const char* prefabs[] = { "Enemigo", "Jugador", nullptr };
            return prefabs;
        },  // listarPrefabs
        [](const char*) { return reinterpret_cast<void*>(static_cast<intptr_t>(0x1234)); },  // interfazDeObjeto
        [](const void*) { return true; },  // interfazActiva
        [](void*, bool) {},  // interfazEstablecerActiva
        [](void*, const char*) { return true; },  // interfazEstablecerEstado
        [](void*, const char*, const char*) { return true; },  // interfazEstablecerValor
        [](void*, const char*) { return "3"; },  // interfazValor
        // --- v8: musica ---
        [](const char* ruta, float volumen, float fundido) {
            musicaReproducida = ruta && std::string(ruta) == "tema.ogg" &&
                                volumen == 0.8f && fundido == 0.0f;
            return musicaReproducida;
        },  // reproducirMusica
        [](float fundido) { fundidoSalida = fundido; },  // detenerMusica
        [](float volumen) { volumenBus = volumen; },  // volumenMusica
        // --- v9: interfaz de juego nativa ---
        [](const char* nombre) {
            return nombre && std::string(nombre) == "HUD"
                       ? reinterpret_cast<void*>(static_cast<intptr_t>(0x5678))
                       : nullptr;
        },  // interfazNativaDeObjeto
        [](const void*) { return true; },  // interfazNativaVisible
        [](void*, bool) {},  // interfazNativaEstablecerVisible
        [](void*, const char*, const char*) { return true; },  // interfazNativaEstablecerTexto
        [](void*, const char*, bool) { return true; },  // interfazNativaEstablecerVisibleWidget
        [](void*, const char*) { return false; },  // interfazNativaBotonPulsado
        10,  // version
    };
    return &servicios;
}

}  // namespace MotorScript

// Sondeo del toolchain Java con la MISMA resolucion que usa el backend
// (BackendJava::javacRuta: JAVAC > raices del JDK como JAVA_HOME > default
// horneado por CMake). Sin shell (H-3 nivel 2): sondear() ejecuta javac
// directamente y el output va al dispositivo nulo de la plataforma (H-14:
// NUL / /dev/null, ahora abierto por el runner en vez de escrito por
// cmd.exe). Si la resolucion da una ruta (lo normal), tampoco hay shell que
// intermedie — lo que se verifica es que javac realmente corre.
bool hayJavac() {
    const std::string ruta = BackendJava::javacRuta();
    return SondeoToolchain::sondear(ruta, "-version");
}
