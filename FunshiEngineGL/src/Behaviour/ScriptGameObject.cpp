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
#include "ScriptGameObject.h"

#include <cstring>
#include <iostream>

#include "ScriptAudioHandles.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../Objetos/Componentes/Colliders/Collider.h"
#include "../Audio/AudioEngine.h"
#include "../Scenes/SceneRegistry.h"
#include "../Input/InputScripts.h"
#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

namespace MotorScript {

namespace {
const char* nombreDelObjeto(const void* objeto) {
    const GameObject* o = static_cast<const GameObject*>(objeto);
    return o ? o->inputName : "";
}

float posicionEje(const void* objeto, int eje) {
    const GameObject* o = static_cast<const GameObject*>(objeto);
    if (!o) return 0.0f;
    GameObject* obj = const_cast<GameObject*>(o);
    Transform* transform = obj->getComponent<Transform>();
    return transform ? transform->getTranslatef()[eje] : 0.0f;
}

// Acceso seguro al transform del objeto (repetido en varios helpers v2).
Transform* transformDe(void* objeto) {
    GameObject* o = static_cast<GameObject*>(objeto);
    return o ? o->getComponent<Transform>() : nullptr;
}

void fijarPosicion(void* objeto, float x, float y, float z) {
    if (Transform* t = transformDe(objeto)) t->setTranslatef(x, y, z);
    GameObject* owner = static_cast<GameObject*>(objeto);
    if (owner) {
        if (RigidBody* body = owner->getComponent<RigidBody>())
            body->syncGameObjectToPhysics(false);
    }
}

void fijarEscala(void* objeto, float x, float y, float z) {
    if (Transform* t = transformDe(objeto)) t->setScalef(x, y, z);
}

void fijarRotacionEjes(void* objeto, float angulo, float x, float y, float z) {
    if (Transform* t = transformDe(objeto))
        t->setRotatef(angulo * 57.29577951308232f, x, y, z);
    GameObject* owner = static_cast<GameObject*>(objeto);
    if (owner) {
        if (RigidBody* body = owner->getComponent<RigidBody>())
            body->syncGameObjectToPhysics(false);
    }
}

bool fijarVelocidadHorizontal(void* objeto, float x, float z) {
    GameObject* owner = static_cast<GameObject*>(objeto);
    RigidBody* body = owner ? owner->getComponent<RigidBody>() : nullptr;
    return body ? body->fijarVelocidadHorizontal(x, z) : false;
}

bool saltar(void* objeto, float velocidad) {
    GameObject* owner = static_cast<GameObject*>(objeto);
    RigidBody* body = owner ? owner->getComponent<RigidBody>() : nullptr;
    return body ? body->saltar(velocidad) : false;
}

const char* etiquetaObjeto(const void* objeto) {
    const GameObject* owner = static_cast<const GameObject*>(objeto);
    return owner ? owner->getTag().c_str() : "";
}

bool tieneEtiqueta(const void* objeto, const char* etiqueta) {
    if (!etiqueta || !*etiqueta) return false;
    const GameObject* owner = static_cast<const GameObject*>(objeto);
    return owner && owner->getTag() == etiqueta;
}

void* objetoDeCollider(const void* collider) {
    const Collider* c = static_cast<const Collider*>(collider);
    return c ? c->getOwner() : nullptr;
}

// Getters v2: leen el estado canonico del componente. Transform mantiene dos
// copias (objectX [privado, usado por el render/gizmo] y arrX [publico,
// sincronizado en set/load]); getTranslatef()/getScalef()/getRotatef()
// devuelven las arr* ya sincronizadas.
float rotacionEje(void* objeto, int indice) {
    Transform* t = transformDe(objeto);
    if (!t) return 0.0f;
    const float valor = t->getRotatef()[indice];
    return indice == 0 ? valor * 0.017453292519943295f : valor;
}

float escalaEje(void* objeto, int indice) {
    Transform* t = transformDe(objeto);
    return t ? t->getScalef()[indice] : 1.0f;
}

void imprimirConsola(const char* texto) {
    if (texto) std::cout << "[script] " << texto << std::endl;
}
} // namespace

const ApiScriptGameObject* tablaApi() {
    static const ApiScriptGameObject tabla = {
        /* --- v1 --- */
        /* .nombre        = */ nombreDelObjeto,
        /* .posicionX     = */ [](const void* o) { return posicionEje(o, 0); },
        /* .posicionY     = */ [](const void* o) { return posicionEje(o, 1); },
        /* .posicionZ     = */ [](const void* o) { return posicionEje(o, 2); },
        /* .fijarPosicion = */ fijarPosicion,
        /* .fijarEscala   = */ fijarEscala,
        /* .fijarRotacionEjes = */ fijarRotacionEjes,
        /* .imprimirConsola   = */ imprimirConsola,
        /* --- v2 --- */
        /* .rotacionAngulo = */ [](const void* o) { return rotacionEje(const_cast<void*>(o), 0); },
        /* .rotacionEjeX   = */ [](const void* o) { return rotacionEje(const_cast<void*>(o), 1); },
        /* .rotacionEjeY   = */ [](const void* o) { return rotacionEje(const_cast<void*>(o), 2); },
        /* .rotacionEjeZ   = */ [](const void* o) { return rotacionEje(const_cast<void*>(o), 3); },
        /* .escalaX        = */ [](const void* o) { return escalaEje(const_cast<void*>(o), 0); },
        /* .escalaY        = */ [](const void* o) { return escalaEje(const_cast<void*>(o), 1); },
        /* .escalaZ        = */ [](const void* o) { return escalaEje(const_cast<void*>(o), 2); },
        /* .fijarVelocidadHorizontal = */ fijarVelocidadHorizontal,
        /* .saltar         = */ saltar,
        /* .etiqueta       = */ etiquetaObjeto,
        /* .tieneEtiqueta  = */ tieneEtiqueta,
        /* .objetoDeCollider = */ objetoDeCollider,
        /* .version        = */ 4,
    };
    return &tabla;
}

// ============================================================================
// ScriptServices (v3): audio + busqueda (nombre/id/etiqueta) + teclado. Implementado aqui para que
// el modulo de scripts no enlace contra Audio/Scenes: GameScene inyecta los
// punteros crudos (AudioEngine*, SceneRegistry*, InputScripts*) en
// inyectarServiciosScript() y esta TU los envuelve en la tabla.
// ============================================================================

namespace {

AudioEngine* g_audio = nullptr;
SceneRegistry* g_escena = nullptr;
InputScripts* g_input = nullptr;
ScriptAudioHandles g_sonidosScript;

int serviciosReproducirSonido(const char* clip, float volumen, bool bucle) {
    if (!g_audio || !clip) return -1;
    const int handle = g_audio->reproducir(clip, volumen, bucle);
    g_sonidosScript.registrar(handle);
    return handle;
}

void serviciosDetenerSonido(int handle) {
    if (g_audio) g_audio->detener(handle);
    g_sonidosScript.retirar(handle);
}

void* serviciosObjetoPorNombre(const char* nombre) {
    if (!g_escena || !nombre) return nullptr;
    // Recorrido en preorden de la lista lineal del registro (ya ordenada por
    // jerarquia tras cada refreshTransformOrigins). next(last) lanza
    // (no devuelve null), asi que se avanza con el idiom seguro de ListaDE.
    auto* lista = g_escena->getGameObjects();
    if (!lista || lista->isEmpty()) return nullptr;
    for (auto* nodo = lista->first(); nodo;
         nodo = (nodo != lista->last()) ? lista->next(nodo) : nullptr) {
        GameObject* o = nodo->getElement();
        // Comparacion por texto: ambos son const char* y el == compararia
        // punteros (el buffer inputName nunca es el literal buscado).
        if (o && std::strcmp(nombreDelObjeto(o), nombre) == 0) return o;
    }
    return nullptr;
}

void* serviciosObjetoPorId(int id) {
    if (!g_escena) return nullptr;
    auto* lista = g_escena->getGameObjects();
    if (!lista || lista->isEmpty()) return nullptr;
    for (auto* nodo = lista->first(); nodo;
         nodo = (nodo != lista->last()) ? lista->next(nodo) : nullptr) {
        GameObject* o = nodo->getElement();
        if (o && o->getId() == id) return o;
    }
    return nullptr;
}

void* serviciosObjetoPorEtiqueta(const char* etiqueta) {
    if (!g_escena || !etiqueta) return nullptr;
    auto* lista = g_escena->getGameObjects();
    if (!lista || lista->isEmpty()) return nullptr;
    for (auto* nodo = lista->first(); nodo;
         nodo = (nodo != lista->last()) ? lista->next(nodo) : nullptr) {
        GameObject* o = nodo->getElement();
        if (o && o->getTag() == etiqueta) return o;
    }
    return nullptr;
}

bool serviciosTeclaSostiene(const char* tecla) {
    return g_input ? g_input->sostiene(tecla) : false;
}

bool serviciosTeclaPresionada(const char* tecla) {
    return g_input ? g_input->presionada(tecla) : false;
}

bool serviciosTeclaSoltada(const char* tecla) {
    return g_input ? g_input->soltada(tecla) : false;
}

float serviciosDeltaMouseX() {
    return g_input ? g_input->deltaMouseX() : 0.0f;
}

float serviciosDeltaMouseY() {
    return g_input ? g_input->deltaMouseY() : 0.0f;
}

void detenerSonidosScript() {
    g_sonidosScript.detenerTodos(
        [](int handle) { if (g_audio) g_audio->detener(handle); });
}

} // namespace

const ScriptServices* tablaServicios() {
    static const ScriptServices tabla = {
        /* .reproducirSonido  = */ serviciosReproducirSonido,
        /* .detenerSonido     = */ serviciosDetenerSonido,
        /* .objetoPorNombre   = */ serviciosObjetoPorNombre,
        /* .objetoPorId       = */ serviciosObjetoPorId,
        /* .objetoPorEtiqueta = */ serviciosObjetoPorEtiqueta,
        /* .teclaSostiene     = */ serviciosTeclaSostiene,
        /* .teclaPresionada   = */ serviciosTeclaPresionada,
        /* .teclaSoltada      = */ serviciosTeclaSoltada,
        /* .deltaMouseX       = */ serviciosDeltaMouseX,
        /* .deltaMouseY       = */ serviciosDeltaMouseY,
        /* .version           = */ 3,
    };
    return &tabla;
}

// Llamada por GameScene al entrar en play (y al salir, con nullptrs) para
// cablear el contexto real de la escena a la tabla de servicios.
void inyectarServiciosScript(AudioEngine* audio, SceneRegistry* escena,
                             InputScripts* input) {
    if (!audio && !escena && !input) detenerSonidosScript();
    g_audio = audio;
    g_escena = escena;
    g_input = input;
}

} // namespace MotorScript