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
#include "ScriptGameObjectDetalle.h"

namespace MotorScript {

namespace ScriptGameObjectDetalle {

// ============================================================================
// ScriptServices (v3): audio + busqueda (nombre/id/etiqueta) + teclado. Implementado aqui para que
// el modulo de scripts no enlace contra Audio/Scenes: GameScene inyecta los
// punteros crudos (AudioEngine*, SceneRegistry*, InputScripts*) en
// inyectarServiciosScript() y esta TU los envuelve en la tabla.
// ============================================================================

AudioEngine* g_audio = nullptr;
SceneRegistry* g_escena = nullptr;
InputScripts* g_input = nullptr;
PhysicsEngine* g_fisica = nullptr;
EditorController* g_editor = nullptr;
EstadoInterfaces* g_interfaces = nullptr;
ScriptAudioHandles g_sonidosScript;
std::vector<AltaPendiente> altasPendientes_;
std::vector<ComponentePendiente> componentesPendientes_;
std::vector<GameObject*> bajasPendientes_;

}  // namespace ScriptGameObjectDetalle
using namespace ScriptGameObjectDetalle;

}  // namespace MotorScript

namespace MotorScript {

namespace {

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

// Acceso al cuerpo fisico (nulo si el objeto no tiene RigidBody). Patron
// comun de todos los helpers v5 de fisica.
RigidBody* cuerpoDe(const void* objeto) {
    const GameObject* owner = static_cast<const GameObject*>(objeto);
    GameObject* mutable_ = const_cast<GameObject*>(owner);
    return mutable_ ? mutable_->getComponent<RigidBody>() : nullptr;
}

float masaObjeto(const void* objeto) {
    RigidBody* body = cuerpoDe(objeto);
    return body ? body->masa() : 0.0f;
}

bool fijarMasaObjeto(void* objeto, float masa) {
    RigidBody* body = cuerpoDe(objeto);
    return body ? body->fijarMasa(masa) : false;
}

bool usaGravedadObjeto(const void* objeto) {
    RigidBody* body = cuerpoDe(objeto);
    return body ? body->usaGravedad() : false;
}

bool fijarUsoGravedadObjeto(void* objeto, bool usar) {
    RigidBody* body = cuerpoDe(objeto);
    if (!body) return false;
    body->fijarUsoGravedad(usar);
    return true;
}

float escalaGravedadObjeto(const void* objeto) {
    RigidBody* body = cuerpoDe(objeto);
    return body ? body->escalaGravedad() : 0.0f;
}

bool fijarEscalaGravedadObjeto(void* objeto, float escala) {
    RigidBody* body = cuerpoDe(objeto);
    return body ? body->fijarEscalaGravedad(escala) : false;
}

float friccionObjeto(const void* objeto) {
    RigidBody* body = cuerpoDe(objeto);
    return body ? body->friccion() : 0.0f;
}

bool fijarFriccionObjeto(void* objeto, float friccion) {
    RigidBody* body = cuerpoDe(objeto);
    return body ? body->fijarFriccion(friccion) : false;
}

bool freezeEje(const void* objeto, int eje, bool rotacion) {
    RigidBody* body = cuerpoDe(objeto);
    if (!body) return false;
    return rotacion ? body->rotacionCongelada(eje)
                    : body->posicionCongelada(eje);
}

bool fijarFreeze(void* objeto, bool x, bool y, bool z, bool rotacion) {
    RigidBody* body = cuerpoDe(objeto);
    if (!body) return false;
    if (rotacion)
        body->fijarFreezeRotacion(x, y, z);
    else
        body->fijarFreezePosicion(x, y, z);
    return true;
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

}  // namespace

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
        /* .masa           = */ masaObjeto,
        /* .fijarMasa      = */ fijarMasaObjeto,
        /* .usaGravedad    = */ usaGravedadObjeto,
        /* .fijarUsoGravedad = */ fijarUsoGravedadObjeto,
        /* .escalaGravedad = */ escalaGravedadObjeto,
        /* .fijarEscalaGravedad = */ fijarEscalaGravedadObjeto,
        /* .friccion       = */ friccionObjeto,
        /* .fijarFriccion  = */ fijarFriccionObjeto,
        /* .posicionCongelada = */
        [](const void* o, int eje) { return freezeEje(o, eje, false); },
        /* .fijarFreezePosicion = */
        [](void* o, bool x, bool y, bool z) {
            return fijarFreeze(o, x, y, z, false);
        },
        /* .rotacionCongelada = */
        [](const void* o, int eje) { return freezeEje(o, eje, true); },
        /* .fijarFreezeRotacion = */
        [](void* o, bool x, bool y, bool z) {
            return fijarFreeze(o, x, y, z, true);
        },
        /* .version        = */ 5,
    };
    return &tabla;
}

}  // namespace MotorScript
