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
#ifndef EDITOR_CONTROLLER_H
#define EDITOR_CONTROLLER_H

#include <memory>

#include "Comandos/GestorComandos.h"

class Component;
class GameObject;
class PhysicsEngine;
class SceneRegistry;
class EventBus;
class Transform;
class AssetManager;

// Objetivo generico del gizmo: el Transform LOCAL a editar mas el Transform
// GLOBAL del contexto padre (para recomponer/mover en espacio local). Con
// owner = objeto con RigidBody a sincronizar tras la edicion.
//
// Es la misma maquinaria para editar el transform de un GameObject o el
// transform local (offset) de un componente como un collider: cambia el par
// {local, parentGlobal} y el gizmo hace lo mismo.
struct GizmoTarget {
    Transform* local = nullptr;
    Transform* parentGlobal = nullptr;
    GameObject* owner = nullptr;
};

/**
 * Application-facing editor operations. It does not own the registry or
 * physics world; both are injected and outlive this controller.
 */
class EditorController {
private:
    SceneRegistry* scene = nullptr;
    PhysicsEngine* physics = nullptr;
    EventBus* events = nullptr;
    // Proveedor de assets de malla inyectado: se propaga a los Modelos3D que
    // crea el controlador para que compartan meshes (Flyweight).
    AssetManager* assets = nullptr;

    // Unica fuente de verdad de la seleccion del editor. GUI, gizmo e
    // inspectores leen de aqui y publican cambios con selectObject() para que
    // el resto de sistemas se sincronize via EventBus.
    GameObject* selected = nullptr;

    // Objetivo del gizmo cuando NO es el transform del objeto seleccionado
    // (p. ej. el offset local de un collider). Se limpia al cambiar de
    // seleccion o borrar objetos.
    GizmoTarget gizmoTarget;

    // Guia de eje activa: 0 = X, 1 = Y, 2 = Z, -1 = ninguna. Es la fuente de
    // verdad compartida por los tres consumidores de la funcionalidad:
    // el input (X/Y/Z la alternan), el gizmo (mientras haya guia NO se dibuja,
    // porque la guia ya limita el movimiento del objeto a un unico eje) y el
    // renderer (dibuja la recta). Se limpia sola al cambiar de seleccion.
    int guiaEje = -1;

    // Gestor de comandos para undo/redo
    GestorComandos gestorComandos;

public:
    EditorController(SceneRegistry* scene, PhysicsEngine* physics = nullptr,
                     EventBus* events = nullptr,
                     AssetManager* assetsManager = nullptr);

    void setScene(SceneRegistry* scene) noexcept;
    void setPhysics(PhysicsEngine* physics) noexcept;
    void setEventBus(EventBus* events) noexcept;
    void setAssetManager(AssetManager* assetsManager) noexcept;

    // Raw pointers are non-owning scene views.
    GameObject* createGameObject(std::unique_ptr<GameObject> object,
                                 GameObject* parent = nullptr);
    bool deleteGameObject(GameObject* object);
    bool deleteObjectByID(int id);
    bool reparentGameObject(GameObject* object, GameObject* parent);
    void clearScene();
    bool addComponent(GameObject* object, std::unique_ptr<Component> component);
    bool removeComponent(GameObject* object, Component* component);

    // Registra los RigidBody de TODA la escena en el mundo de fisica. Al
    // deserializar una escena, los componentes se crean con GameObject::
    // addComponent (no via EditorController), por lo que los cuerpos nunca
    // entran al mundo y los colliders quedan con la shape de respaldo (la malla
    // aun no estaba cargada). Este metodo invalida la shape (se reconstruye con
    // la malla ya disponible) y recrea+registra cada cuerpo.
    void registerSceneRigidBodies();

    // Recrea y re-registra el RigidBody de UN objeto: invalida la shape del
    // collider (se reconstruye lazy con el radio/escala/malla actual), saca el
    // cuerpo viejo del mundo, lo crea de nuevo con la shape fresca y lo
    // vuelve a agregar. Se usa cuando cambia el radio del collider desde la
    // GUI: de lo contrario la shape de Bullet queda cacheada con el radio
    // inicial (esfera/caja invisible mas grande/smaller) y la fisica choca
    // con esa forma stale.
    void refreshRigidBody(GameObject* object);

    // Seleccion del editor: unica fuente de verdad compartida por GUI y gizmo.
    GameObject* getSelectedObject() const noexcept { return selected; }
    // Valida que el objeto siga en la escena y propaga ObjectSelected.
    void selectObject(GameObject* object);
    void clearSelection();

    // Objetivo del gizmo: si se setea un GizmoTarget, el gizmo de GameScene
    // edita ESE transform (local) en vez del del objeto seleccionado. Se usa
    // para editar el offset local del collider desde SettingsCollider*.
    void setGizmoTarget(const GizmoTarget& target);
    void clearGizmoTarget();
    bool hasGizmoTarget() const noexcept { return gizmoTarget.local != nullptr; }
    const GizmoTarget& getGizmoTarget() const noexcept { return gizmoTarget; }

    // Guia de eje (X/Y/Z): la recta que marca sobre que eje se mueve el objeto
    // seleccionado. "eje" es 0 = X, 1 = Y, 2 = Z. AlternarGuiaEje la enciende
    // como un interruptor (pulsar dos veces la misma tecla la apaga) y
    // setGuiaEje la fija outright (usada por los atajos de operacion al
    // cambiar de operacion). Con la guia activa el gizmo se apaga; al
    // deseleccionar el objeto la guia se limpia sola.
    int getGuiaEje() const noexcept { return guiaEje; }
    bool hayGuiaEje() const noexcept { return guiaEje >= 0; }
    void setGuiaEje(int eje);
    void alternarGuiaEje(int eje);
    void clearGuiaEje();

    // Sistema de comandos (undo/redo). deshacer/rehacer devuelven la descripcion
    // del comando aplicado (vacia si no habia nada que deshacer/rehacer) para que
    // la vista pueda avisarlo en la barra de estado.
    GestorComandos* getGestorComandos() noexcept { return &gestorComandos; }
    std::string deshacer();
    std::string rehacer();
    bool puedeDeshacer() const noexcept;
    bool puedeRehacer() const noexcept;
};

#endif
