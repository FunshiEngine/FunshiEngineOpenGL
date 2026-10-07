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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ScriptAudioHandles.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/GameObjectFactory.h"
#include "../Objetos/ClonadorObjetos.h"
#include "../Objetos/NombreUnico.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../Objetos/Componentes/Colliders/Collider.h"
#include "../Objetos/Componentes/Colliders/CubeCollider.h"
#include "../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../Audio/AudioEngine.h"
#include "../Scenes/SceneRegistry.h"
#include "../Scenes/EditorController.h"
#include "../Fisicas/PhysicsEngine.h"
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
PhysicsEngine* g_fisica = nullptr;
EditorController* g_editor = nullptr;
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

void serviciosFijarGravedadGlobal(float x, float y, float z) {
    if (g_fisica) g_fisica->fijarGravedad(x, y, z);
}

float serviciosGravedadGlobalEje(int eje) {
    float x = 0.0f;
    float y = -1.0f;
    float z = 0.0f;
    if (g_fisica) g_fisica->gravedad(x, y, z);
    if (eje == 0) return x;
    if (eje == 2) return z;
    return y;
}

// ============================================================================
// Gestion de objetos desde scripts (v4). Toda mutacion estructural es
// DIFERIDA al final del frame (procesarPeticionesObjetos): crear o borrar
// durante el recorrido de actualizacion reconstruiria la vista lineal
// (refreshGameObjectView) e invalidaria el iterador en curso.
// ============================================================================

struct AltaPendiente {
    std::unique_ptr<GameObject> objeto;
    GameObject* padre = nullptr;
};

struct ComponentePendiente {
    GameObject* objeto = nullptr;
    std::unique_ptr<Component> componente;
};

std::vector<AltaPendiente> altasPendientes_;
std::vector<ComponentePendiente> componentesPendientes_;
std::vector<GameObject*> bajasPendientes_;

void descartarPeticionesObjetos() {
    altasPendientes_.clear();
    componentesPendientes_.clear();
    bajasPendientes_.clear();
}

// Nombre libre en la escena: contempla la escena y las altas pendientes (aun
// no insertadas). `tomados` se completa con el elegido.
std::string nombreLibrePara(const std::string& base,
                            std::vector<std::string>& tomados) {
    return ClonadorObjetos::nombreLibre(base, tomados);
}

std::vector<std::string> nombresTomados() {
    std::vector<std::string> tomados;
    if (g_escena) tomados = NombresUnicos::enUso(g_escena->getRoot());
    for (const AltaPendiente& alta : altasPendientes_) {
        if (alta.objeto && alta.objeto->inputName[0] != '\0')
            tomados.emplace_back(alta.objeto->inputName);
    }
    return tomados;
}

void* serviciosCrearObjeto(const char* nombre, void* padre) {
    if (!g_editor || !g_escena) return nullptr;
    GameObject* padreObj = static_cast<GameObject*>(padre);
    if (padreObj && !g_escena->contains(padreObj)) padreObj = nullptr;
    std::unique_ptr<GameObject> objeto = GameObjectFactory::createSimpleObject(
        padreObj ? padreObj : g_escena->getRoot());
    if (nombre && *nombre) {
        std::vector<std::string> tomados = nombresTomados();
        const std::string unico = nombreLibrePara(nombre, tomados);
        std::snprintf(objeto->inputName, sizeof(objeto->inputName), "%s",
                      unico.c_str());
    }
    GameObject* crudo = objeto.get();
    altasPendientes_.push_back({std::move(objeto), padreObj});
    return crudo;
}

bool serviciosDestruirObjeto(void* objeto) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!g_editor || !g_escena || !o || !g_escena->contains(o)) return false;
    for (GameObject* marcado : bajasPendientes_) {
        if (marcado == o) return true;
    }
    bajasPendientes_.push_back(o);
    return true;
}

void* serviciosClonarObjeto(const void* original, void* padre) {
    GameObject* o = const_cast<GameObject*>(
        static_cast<const GameObject*>(original));
    if (!g_editor || !g_escena || !o || !g_escena->contains(o)) return nullptr;
    if (o == g_escena->getRoot()) return nullptr;
    GameObject* padreObj = static_cast<GameObject*>(padre);
    if (padreObj && !g_escena->contains(padreObj)) padreObj = nullptr;
    // Copia profunda (componentes y subarbol) sin insertar: entra a la escena
    // al final del frame con el resto de las altas pendientes.
    auto nodos =
        ClonadorObjetos::clonar(o, padreObj, nombresTomados());
    if (nodos.empty()) return nullptr;
    GameObject* crudo = nodos.front().first.get();
    for (auto& nodo : nodos)
        altasPendientes_.push_back(
            {std::move(nodo.first), nodo.second});
    return crudo;
}

bool encolarComponente(GameObject* o, std::unique_ptr<Component> componente) {
    if (!g_editor || !g_escena || !o || !g_escena->contains(o) ||
        !componente)
        return false;
    componentesPendientes_.push_back({o, std::move(componente)});
    return true;
}

// El componente encolado aun no esta en el objeto: sin mirar la cola, dos
// pedidos del mismo frame entrarian igual y el segundo se descartaria mudo.
bool componentePendiente(GameObject* o, bool collider) {
    for (const ComponentePendiente& item : componentesPendientes_) {
        if (item.objeto != o || !item.componente) continue;
        if (collider && dynamic_cast<Collider*>(item.componente.get()))
            return true;
        if (!collider && dynamic_cast<RigidBody*>(item.componente.get()))
            return true;
    }
    return false;
}

bool serviciosAgregarColliderEsfera(void* objeto, float radio) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!o || !std::isfinite(radio) || radio <= 0.0f) return false;
    if (!g_escena || !g_escena->contains(o)) return false;
    if (o->getComponent<Collider>() || componentePendiente(o, true))
        return false;
    Transform* t = o->getComponent<Transform>();
    if (!t) return false;
    return encolarComponente(
        o, std::make_unique<EsfereCollider>(radio, t, o));
}

bool serviciosAgregarColliderCubo(void* objeto, float radio) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!o || !std::isfinite(radio) || radio <= 0.0f) return false;
    if (!g_escena || !g_escena->contains(o)) return false;
    if (o->getComponent<Collider>() || componentePendiente(o, true))
        return false;
    Transform* t = o->getComponent<Transform>();
    if (!t) return false;
    return encolarComponente(o,
                             std::make_unique<CubeCollider>(radio, t, o));
}

bool serviciosAgregarRigidBody(void* objeto, float masa) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!o || !std::isfinite(masa)) return false;
    if (!g_escena || !g_escena->contains(o)) return false;
    Collider* collider = o->getComponent<Collider>();
    if (!collider || o->getComponent<RigidBody>() ||
        componentePendiente(o, false))
        return false;
    if (masa < 0.0f) masa = 0.0f;
    return encolarComponente(o,
                             std::make_unique<RigidBody>(collider, masa));
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
        /* .fijarGravedadGlobal = */ serviciosFijarGravedadGlobal,
        /* .gravedadGlobalX   = */
        []() { return serviciosGravedadGlobalEje(0); },
        /* .gravedadGlobalY   = */
        []() { return serviciosGravedadGlobalEje(1); },
        /* .gravedadGlobalZ   = */
        []() { return serviciosGravedadGlobalEje(2); },
        /* .crearObjeto       = */ serviciosCrearObjeto,
        /* .destruirObjeto    = */ serviciosDestruirObjeto,
        /* .clonarObjeto      = */ serviciosClonarObjeto,
        /* .agregarColliderEsfera = */ serviciosAgregarColliderEsfera,
        /* .agregarColliderCubo = */ serviciosAgregarColliderCubo,
        /* .agregarRigidBody  = */ serviciosAgregarRigidBody,
        /* .version           = */ 4,
    };
    return &tabla;
}

// Llamada por GameScene al entrar en play (y al salir, con nullptrs) para
// cablear el contexto real de la escena a la tabla de servicios.
void inyectarServiciosScript(AudioEngine* audio, SceneRegistry* escena,
                             InputScripts* input, PhysicsEngine* fisica,
                             EditorController* editor) {
    if (!audio && !escena && !input) detenerSonidosScript();
    if (!escena) descartarPeticionesObjetos();
    g_audio = audio;
    g_escena = escena;
    g_input = input;
    g_fisica = fisica;
    g_editor = editor;
}

void procesarPeticionesObjetos() {
    if (!g_editor || !g_escena) {
        descartarPeticionesObjetos();
        return;
    }
    for (AltaPendiente& alta : altasPendientes_) {
        if (!alta.objeto) continue;
        GameObject* padre = (alta.padre && g_escena->contains(alta.padre))
                                ? alta.padre
                                : nullptr;
        GameObject* insertado =
            g_editor->createGameObject(std::move(alta.objeto), padre);
        // createGameObject no registra cuerpos (solo addComponent lo hace):
        // el clon o el objeto nuevo con RigidBody entraria al mundo jamas.
        if (insertado && insertado->getComponent<RigidBody>())
            g_editor->refreshRigidBody(insertado);
    }
    altasPendientes_.clear();
    for (ComponentePendiente& item : componentesPendientes_) {
        if (!item.objeto || !item.componente ||
            !g_escena->contains(item.objeto))
            continue;
        if (dynamic_cast<Collider*>(item.componente.get())) {
            if (item.objeto->getComponent<Collider>()) continue;
        }
        if (dynamic_cast<RigidBody*>(item.componente.get())) {
            if (!item.objeto->getComponent<Collider>() ||
                item.objeto->getComponent<RigidBody>())
                continue;
        }
        g_editor->addComponent(item.objeto, std::move(item.componente));
    }
    componentesPendientes_.clear();
    for (GameObject* o : bajasPendientes_) {
        if (o && g_escena->contains(o)) g_editor->deleteGameObject(o);
    }
    bajasPendientes_.clear();
}

} // namespace MotorScript