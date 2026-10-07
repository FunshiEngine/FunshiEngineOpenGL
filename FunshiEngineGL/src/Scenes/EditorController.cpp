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
#include "EditorController.h"

#include "SceneRegistry.h"
#include "../Fisicas/PhysicsEngine.h"
#include "../Objetos/ClonadorObjetos.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/NombreUnico.h"
#include "../Objetos/Modelos3D.h"
#include "../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../Events/EventBus.h"
#include "../Comandos/GestorComandos.h"
#include "../Comandos/CrearObjetoComando.h"
#include "../Comandos/BorrarObjetoComando.h"
#include "../Comandos/ReparentarComando.h"
#include "../Comandos/AgregarComponenteComando.h"
#include "../Comandos/QuitarComponenteComando.h"
#include "../Comandos/TransformComando.h"
#include "../Comandos/LimpiarEscenaComando.h"

EditorController::EditorController(SceneRegistry* value, PhysicsEngine* world,
                                   EventBus* bus, AssetManager* assetsManager)
    : scene(value), physics(world), events(bus), assets(assetsManager) {}

void EditorController::setScene(SceneRegistry* value) noexcept { scene = value; }
void EditorController::setPhysics(PhysicsEngine* value) noexcept { physics = value; }
void EditorController::setEventBus(EventBus* value) noexcept { events = value; }
void EditorController::setAssetManager(AssetManager* value) noexcept {
    assets = value;
}

GameObject* EditorController::createGameObject(std::unique_ptr<GameObject> object,
                                               GameObject* parent) {
    // Propagar el AssetManager a los modelos creados desde la GUI: el objeto
    // recien construido todavia es nuestro (antes del std::move) y es el unico
    // momento en que podemos inyectarle la fuente de mallas compartidas.
    if (Modelos3D* modelo = dynamic_cast<Modelos3D*>(object.get()))
        modelo->setAssetManager(assets);
    GameObject* created = scene ? scene->createObject(std::move(object), parent) : nullptr;
    if (created && events)
        events->publish({SceneEventType::ObjectCreated, created, parent});
    return created;
}

bool EditorController::deleteGameObject(GameObject* object) {
    if (!scene || !scene->contains(object)) return false;
    if (object == scene->getRoot()) {
        clearScene();
        return true;
    }
    if (physics && object) {
        if (RigidBody* body = object->getComponent<RigidBody>())
            physics->removeRigidBody(body);
    }
    if (events)
        events->publish({SceneEventType::ObjectDeleted, object, nullptr});
    // La seleccion no debe apuntar a un objeto que se va a liberar.
    if (selected == object) {
        selected = nullptr;
        if (events)
            events->publish({SceneEventType::ObjectSelected, nullptr, nullptr});
    }
    // Un objeto borrado puede ser el owner de un gizmo en curso: desarmarlo
    // evita punteros colgantes en el gizmo del collider.
    if (gizmoTarget.owner == object) clearGizmoTarget();
    const bool deleted = scene->deleteObject(object);
    return deleted;
}

bool EditorController::deleteObjectByID(int id) {
    if (!scene) return false;
    scene->refreshGameObjectView();
    GameObject* object = nullptr;
    if (scene->getGameObjects() && !scene->getGameObjects()->isEmpty()) {
        auto* position = scene->getGameObjects()->first();
        while (position) {
            if (position->getElement()->getId() == id) {
                object = position->getElement();
                break;
            }
            position = (position != scene->getGameObjects()->last())
                           ? scene->getGameObjects()->next(position)
                           : nullptr;
        }
    }
    if (!object && scene->getRoot() && scene->getRoot()->getId() == id)
        object = scene->getRoot();
    return deleteGameObject(object);
}

bool EditorController::reparentGameObject(GameObject* object, GameObject* parent) {
    const bool changed = scene && scene->reparent(object, parent);
    if (changed) {
        // El cuerpo fisico sigue ligado a su transform viejo: al cambiar de
        // padre hay que reconstruirlo para que herede la pose preservada y no
        // se teletransporte de vuelta en el siguiente paso de simulacion.
        // Queda anotado que reconstruirlo reinicia la velocidad: en edicion es
        // inofensivo, pero reparentar durante Play frena el objeto.
        refreshRigidBody(object);
        if (events)
            events->publish({SceneEventType::ObjectReparented, object, parent});
    }
    return changed;
}

GameObject* EditorController::duplicarObjeto(GameObject* original,
                                             GameObject* parent) {
    if (!scene || !original || !scene->contains(original)) return nullptr;
    if (original == scene->getRoot()) return nullptr;
    if (parent && !scene->contains(parent)) parent = nullptr;
    auto nodos = ClonadorObjetos::clonar(
        original, parent, NombresUnicos::enUso(scene->getRoot()));
    GameObject* cima = nullptr;
    for (auto& nodo : nodos) {
        GameObject* destino =
            (nodo.second && scene->contains(nodo.second)) ? nodo.second
                                                         : nullptr;
        GameObject* insertado =
            createGameObject(std::move(nodo.first), destino);
        if (!cima) cima = insertado;
        // createGameObject no registra cuerpos (solo addComponent lo hace).
        if (insertado && insertado->getComponent<RigidBody>())
            refreshRigidBody(insertado);
    }
    return cima;
}

namespace {
void cuerposDeSubarbol(GameObject* raiz, std::vector<RigidBody*>& salida) {
    if (!raiz) return;
    if (RigidBody* body = raiz->getComponent<RigidBody>()) salida.push_back(body);
    for (Entity* hijo : raiz->getChildEntities()) {
        if (GameObject* hijoObjeto = dynamic_cast<GameObject*>(hijo))
            cuerposDeSubarbol(hijoObjeto, salida);
    }
}

bool estaDentro(GameObject* nodo, GameObject* ancestro) {
    for (Entity* p = nodo ? nodo->getParentEntity() : nullptr; p;
         p = p->getParentEntity()) {
        if (p == ancestro) return true;
    }
    return false;
}
} // namespace

std::vector<std::unique_ptr<GameObject>> EditorController::extraerSubarbol(
    GameObject* object) {
    if (!scene || !object || !scene->contains(object)) return {};
    if (object == scene->getRoot()) return {};
    if (physics) {
        std::vector<RigidBody*> cuerpos;
        cuerposDeSubarbol(object, cuerpos);
        for (RigidBody* body : cuerpos) physics->removeRigidBody(body);
    }
    // La seleccion no debe apuntar adentro de lo extraido (puntero colgante
    // al restaurar en otro lado); el gizmo tampoco.
    if (selected && (selected == object || estaDentro(selected, object))) {
        selected = nullptr;
        if (events)
            events->publish({SceneEventType::ObjectSelected, nullptr, nullptr});
    }
    if (gizmoTarget.owner &&
        (gizmoTarget.owner == object ||
         estaDentro(gizmoTarget.owner, object)))
        clearGizmoTarget();
    if (events)
        events->publish({SceneEventType::ObjectDeleted, object, nullptr});
    auto nodos = scene->takeSubtree(object);
    return nodos;
}

GameObject* EditorController::restaurarSubarbol(
    std::vector<std::unique_ptr<GameObject>> nodos, GameObject* parent) {
    if (!scene || nodos.empty()) return nullptr;
    if (!parent || !scene->contains(parent)) parent = scene->getRoot();
    GameObject* cima = scene->restoreSubtree(std::move(nodos), parent);
    if (!cima) return nullptr;
    if (physics) {
        std::vector<RigidBody*> cuerpos;
        cuerposDeSubarbol(cima, cuerpos);
        for (RigidBody* body : cuerpos) {
            if (body->getRigidBody()) physics->addRigidBody(body);
        }
    }
    return cima;
}

void EditorController::clearScene() {
    if (!scene) return;
    scene->refreshGameObjectView();
    auto* objects = scene->getGameObjects();
    if (physics && objects && !objects->isEmpty()) {
        auto* position = objects->first();
        while (position) {
            GameObject* object = position->getElement();
            if (object) {
                if (auto* body = object->getComponent<RigidBody>())
                    physics->removeRigidBody(body);
            }
            position = (position != objects->last())
                           ? objects->next(position)
                           : nullptr;
        }
    }
    if (physics && scene->getRoot()) {
        if (auto* body = scene->getRoot()->getComponent<RigidBody>())
            physics->removeRigidBody(body);
    }
    scene->clear();
    selected = nullptr;
    clearGizmoTarget();
    if (events) {
        events->publish({SceneEventType::SceneCleared, nullptr, nullptr});
        events->publish({SceneEventType::ObjectSelected, nullptr, nullptr});
    }
}

void EditorController::registerSceneRigidBodies() {
    if (!scene || !physics) return;
    scene->refreshGameObjectView();
    auto* objects = scene->getGameObjects();
    if (objects && !objects->isEmpty()) {
        auto* position = objects->first();
        while (position) {
            refreshRigidBody(position->getElement());
            position = (position != objects->last())
                           ? objects->next(position)
                           : nullptr;
        }
    }
    refreshRigidBody(scene->getRoot());
}

void EditorController::refreshRigidBody(GameObject* object) {
    if (!scene || !physics || !object || !scene->contains(object)) return;
    RigidBody* body = object->getComponent<RigidBody>();
    if (!body) return;
    // La shape de Bullet se cachea con el radio/escala/malla con que se creo:
    // al cambialo desde la GUI (p. ej. el radio de la esfera/cubo) la shape
    // queda desactualizada y la fisica choca con la forma vieja. Invalidarla
    // hace que getCollisionShape() la reconstruya con el valor actual.
    if (Collider* collider = object->getComponent<Collider>())
        collider->invalidateCollisionShape();
    // createRigidBody() construye un btRigidBody NUEVO: si el anterior queda
    // registrado en el mundo tendriamos DOS cuerpos en la misma posicion
    // (autocolision real del objeto contra su gemelo invisible).
    physics->removeRigidBody(body);
    body->createRigidBody();
    if (!body->getRigidBody()) return;
    physics->addRigidBody(body);
}

void EditorController::selectObject(GameObject* object) {
    // Nunca seleccionar un puntero que ya no pertenece a la escena.
    if (object && scene && !scene->contains(object)) return;
    // Un cambio de seleccion desarma el gizmo de componentes: el gizmo vuelve
    // a editar el transform del objeto recien seleccionado. La guia de eje
    // tambien se apaga: es la recta del objeto anterior, y dejarla prendida
    // dibujaria un eje flotando donde ya no hay nada que mover.
    if (selected != object) {
        clearGizmoTarget();
        clearGuiaEje();
    }
    selected = object;
    if (events)
        events->publish({SceneEventType::ObjectSelected, selected, nullptr});
}

void EditorController::clearSelection() { selectObject(nullptr); }

void EditorController::setGuiaEje(int eje) {
    guiaEje = (eje >= 0 && eje <= 2) ? eje : -1;
}

void EditorController::alternarGuiaEje(int eje) {
    if (eje < 0 || eje > 2) return;
    guiaEje = (guiaEje == eje) ? -1 : eje;
}

void EditorController::clearGuiaEje() { guiaEje = -1; }

void EditorController::setGizmoTarget(const GizmoTarget& target) {
    gizmoTarget = target;
}

void EditorController::clearGizmoTarget() { gizmoTarget = GizmoTarget{}; }

bool EditorController::addComponent(GameObject* object,
                                    std::unique_ptr<Component> component) {
    if (!scene || !scene->contains(object) || !component) return false;
    RigidBody* body = dynamic_cast<RigidBody*>(component.get());
    object->addComponent(std::move(component));
    if (physics && body) physics->addRigidBody(body);
    if (events)
        events->publish({SceneEventType::ComponentStructureChanged, object, nullptr});
    return true;
}

bool EditorController::removeComponent(GameObject* object, Component* component) {
    return extraerComponente(object, component) != nullptr;
}

std::unique_ptr<Component> EditorController::extraerComponente(
    GameObject* object, Component* component) {
    if (!scene || !scene->contains(object) || !component) return nullptr;
    // El gizmo puede estar editando el transform local (myTransform) del
    // collider que se va a borrar: desarmarlo ANTES de liberar el componente.
    // Hacer dynamic_cast despues de deleteComponent() es use-after-free (el
    // puntero queda colgante y __dynamic_cast muere al leer el RTTI).
    if (Collider* collider = dynamic_cast<Collider*>(component)) {
        if (collider->getTransform() == gizmoTarget.local)
            clearGizmoTarget();
        // El RigidBody guarda su collider por puntero y lo usa en cada
        // update/sync (syncPhysicsToGameObject): si se libera el collider sin
        // desacoplarlo, queda un puntero colgante (heap-use-after-free).
        // Des-registrar el cuerpo del mundo y dejarlo inerte (collider=null)
        // ANTES de extraerlo.
        if (RigidBody* body = object->getComponent<RigidBody>()) {
            if (physics) physics->removeRigidBody(body);
            body->detachCollider();
        }
    }
    if (physics) {
        if (auto* body = dynamic_cast<RigidBody*>(component))
            physics->removeRigidBody(body);
    }
    std::unique_ptr<Component> extraido = object->extractComponent(component);
    if (extraido && events)
        events->publish({SceneEventType::ComponentStructureChanged, object, nullptr});
    return extraido;
}

std::string EditorController::deshacer() {
    return gestorComandos.deshacer();
}

std::string EditorController::rehacer() {
    return gestorComandos.rehacer();
}

bool EditorController::puedeDeshacer() const noexcept {
    return gestorComandos.puedeDeshacer();
}

bool EditorController::puedeRehacer() const noexcept {
    return gestorComandos.puedeRehacer();
}
