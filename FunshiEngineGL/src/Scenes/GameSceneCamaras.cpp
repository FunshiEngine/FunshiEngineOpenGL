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
#include "GameSceneDetalle.h"

CameraComponent* GameScene::getActiveCamera() {
    auto* gameObjects = getGameObjectsScene();

    // Camara elegida con "Usar": se valida que siga viva en la escena y que
    // conserve su CameraComponent (si fue eliminada se vuelve al scan normal).
    if (requestedActiveCamera) {
        if (sceneRegistry->contains(requestedActiveCamera) &&
            requestedActiveCamera->getComponent<CameraComponent>()) {
            CameraComponent* camara =
                requestedActiveCamera->getComponent<CameraComponent>();
            camara->setUp(requestedActiveCamera);
            activeCamera = camara;
            activeCameraObject = requestedActiveCamera;
            return camara;
        }
        requestedActiveCamera = nullptr;
    }

    if (gameObjects && !gameObjects->isEmpty()) {
        Position<GameObject*>* pos = gameObjects->first();
        while (pos && pos->getElement()) {
            GameObject* objeto = pos->getElement();
            if (CameraComponent* camara =
                    objeto->getComponent<CameraComponent>()) {
                camara->setUp(objeto);
                activeCamera = camara;
                activeCameraObject = objeto;
                return camara;
            }
            pos = (pos != gameObjects->last()) ? gameObjects->next(pos)
                                               : nullptr;
        }
    }

    // Si la escena no tiene camara, se siembra "CamaraPrincipal": un objeto
    // vacio con Transform + CameraComponent, encarando la grilla. La camara
    // nueva nace sin rotacion y mira en -Z (convencion del CameraComponent),
    // asi que se coloca en +Z mirando hacia el origen: la posicion vieja
    // (1, 1, -50) quedaba DE ESPALDAS a la grilla (que vive en el origen y
    // con tam 70 se extiende varios metros a la redonda) y al entrar al
    // editor parecia que la grilla "no se imprimia". Como en modo editor el
    // mouse no rota la camara (main.onMouse solo mira con !isEditorActivo),
    // esa orientacion inicial es la que ve el usuario hasta salir al menu.
    if (editorController) {
        GameObject* creada = editorController->createGameObject(
            std::make_unique<SimpleObject>(), nullptr);
        if (creada) {
            std::snprintf(creada->inputName, sizeof(creada->inputName),
                          "CamaraPrincipal");
            creada->addComponent(std::make_unique<Transform>());
            Transform* transform = creada->getComponent<Transform>();
            if (transform) {
                const SiembraEscena::PosicionInicial pos =
                    SiembraEscena::posicionCamaraInicial(dimensionProyecto);
                transform->setTranslatef(pos.x, pos.y, pos.z);
            }
            creada->addComponent(std::make_unique<CameraComponent>());
            // En un proyecto 2D la camara sembrada mira el plano XY de frente:
            // nace ortografica (vista de plano), no en perspectiva.
            if (dimensionProyecto == DimensionProyecto::DosD) {
                if (CameraComponent* camaraNueva =
                        creada->getComponent<CameraComponent>()) {
                    camaraNueva->setProjection(
                        CameraComponent::Projection::Orthographic);
                }
            }
            return getActiveCamera();
        }
    }

    activeCamera = nullptr;
    activeCameraObject = nullptr;
    return nullptr;
}

void GameScene::setDimension(DimensionProyecto dimension) noexcept {
    dimensionProyecto = dimension;
}

DimensionProyecto GameScene::getDimensionProyecto() const noexcept {
    return dimensionProyecto;
}

void GameScene::setActiveCamera(GameObject* object) {
    if (!object || !sceneRegistry || !sceneRegistry->contains(object) ||
        !object->getComponent<CameraComponent>()) {
        requestedActiveCamera = nullptr;
        return;
    }
    requestedActiveCamera = object;
}

int GameScene::getActiveCameraId() const noexcept {
    // El pedido puede quedar colgando si la escena se limpio por otro camino
    // (p. ej. cambio de proyecto): contains() solo compara direcciones, sin
    // desreferenciar, igual que hace getActiveCamera().
    if (!requestedActiveCamera || !sceneRegistry ||
        !sceneRegistry->contains(requestedActiveCamera))
        return -1;
    return requestedActiveCamera->getId();
}

void GameScene::setActiveCameraById(int id) {
    // Id automatico (-1) o sin escena: modo automatico, sin camara pedida (si
    // quedaba una de otro proyecto, se olvida aqui en vez de colgar).
    if (id < 0 || !sceneRegistry) {
        requestedActiveCamera = nullptr;
        return;
    }

    auto* gameObjects = getGameObjectsScene();
    if (!gameObjects || gameObjects->isEmpty()) return;

    Position<GameObject*>* pos = gameObjects->first();
    while (pos && pos->getElement()) {
        GameObject* objeto = pos->getElement();
        if (objeto->getId() == id && objeto->getComponent<CameraComponent>()) {
            setActiveCamera(objeto);
            return;
        }
        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
    // No se encontro la camara persistida: se deja el modo automatico,
    // olvidando cualquier pedido anterior (puede ser de otro proyecto).
    requestedActiveCamera = nullptr;
}

GameObject* GameScene::agregarCamaraEnVistaActiva() {
    if (!editorController) return nullptr;

    getActiveCamera();
    Transform* transformOrigen =
        activeCameraObject ? activeCameraObject->getGlobalTransform()
                           : nullptr;

    GameObject* creada = editorController->createGameObject(
        std::make_unique<SimpleObject>(), nullptr);
    if (!creada) return nullptr;

    creada->addComponent(std::make_unique<Transform>());
    Transform* transform = creada->getComponent<Transform>();
    if (transform) {
        if (transformOrigen) {
            transform->setTranslatef(transformOrigen->getTranslatef()[0],
                                     transformOrigen->getTranslatef()[1],
                                     transformOrigen->getTranslatef()[2]);
            const float* rot = transformOrigen->getRotatef();
            transform->setRotatef(rot[0], rot[1], rot[2], rot[3]);
        } else {
            transform->setTranslatef(0.f, 0.f, 0.f);
        }
    }

    // Nombre unico tipo "Camara N" para que las ventanas de vista previa no
    // colisionen (ImGui identifica ventanas por titulo).
    int sufijo = ++contadorCamaras;
    std::string nombre;
    for (;;) {
        nombre = "Camara " + std::to_string(sufijo);
        bool usado = false;
        auto* lista = getGameObjectsScene();
        if (lista && !lista->isEmpty()) {
            Position<GameObject*>* pos = lista->first();
            while (pos && pos->getElement()) {
                if (std::string(pos->getElement()->inputName) == nombre) {
                    usado = true;
                    break;
                }
                pos = (pos != lista->last()) ? lista->next(pos) : nullptr;
            }
        }
        if (!usado) break;
        ++sufijo;
    }
    contadorCamaras = sufijo;
    std::snprintf(creada->inputName, sizeof(creada->inputName), "%s",
                  nombre.c_str());

    auto componente = std::make_unique<CameraComponent>();
    componente->setPintar(true); // preview automatica de la cámara nueva
    creada->addComponent(std::move(componente));

    // Queda activa y seleccionada para ubicarla con el gizmo.
    setActiveCamera(creada);
    editorController->selectObject(creada);
    return creada;
}
