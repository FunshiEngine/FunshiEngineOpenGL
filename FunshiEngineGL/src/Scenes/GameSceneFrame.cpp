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

GameObject* GameScene::pickObject(float mouseX, float mouseY) {
    if (!gizmoController_) return nullptr;
    return gizmoController_->pickObject(mouseX, mouseY, getActiveCamera(),
                                        getGameObjectsScene());
}

void GameScene::gameScene() {
    CameraComponent* camara = getActiveCamera();
    if (!camara) return;

    ImGuiIO& io = ImGui::GetIO();
    const int fbW = static_cast<int>(io.DisplaySize.x);
    const int fbH = static_cast<int>(io.DisplaySize.y);
    if (fbW <= 0 || fbH <= 0) return;

    // Luces CPU de la pasada (misma semantica que GL_LIGHT0..7): la escena
    // recoge y el SceneRenderer las aplica al backend y a los uniforms.
    LightData luces[LightSystem::kMaxLights];
    // Dueno de cada luz, en el mismo orden que las luces: lo necesita el pase
    // de sombras para leer los ajustes que tiene adjuntos la luz.
    GameObject* duenosLuces[LightSystem::kMaxLights] = {};
    int numLuces = 0;
    lightSystem.collectLights(getGameObjectsScene(), luces, numLuces,
                              duenosLuces);

    SceneRenderer::FrameContext ctx;
    ctx.gameObjects = getGameObjectsScene();
    ctx.apariencia = &apariencia;
    ctx.deltaTime = deltaTime;
    ctx.mostrarVisualesDepuracion = !modoJuego;
    ctx.selectedObject =
        editorController ? editorController->getSelectedObject() : nullptr;
    ctx.lights = luces;
    ctx.lightCount = numLuces;
    ctx.lightOwners = duenosLuces;
    ctx.globalAmbient = lightSystem.getGlobalAmbient();
    ctx.framebufferWidth = fbW;
    ctx.framebufferHeight = fbH;
    // Guia de eje: el estado vive en EditorController y la recta se dibuja con
    // el mismo sistema de referencia que el gizmo. Depuracion conserva el
    // overlay aunque E oculte los paneles; Juego lo omite junto con los demas
    // visuales auxiliares.
    ctx.guiaEje = !modoJuego ? getGuiaEje() : GuiaEje::kSinGuia;
    ctx.guiaCoordenadasGlobales = isGizmoGlobal();
    ctx.assetManager = assetManager.get();
    // Proyecto 2D: la pasada omite el cielo y el difuminado por distancia.
    ctx.modo2D = dimensionProyecto == DimensionProyecto::DosD;
    ctx.sistemaParticulas = &sistemaParticulas;

    // Audio 3D: el listener vive en la camara activa, así su posición se
    // refresca cada frame (tanto en el editor como en el juego).
    if (audioEngine && activeCameraObject) {
        if (Transform* t = activeCameraObject->getGlobalTransform()) {
            const float* p = t->getTranslatef();
            audioEngine->setListenerPosition(p[0], p[1], p[2]);
        }
    }

    // Pasada de la escena (vistas previas + pasada principal + diag) en la
    // capa de Rendering.
    sceneRenderer->render(ctx, activeCameraObject, camara);

    // La seleccion y el gizmo son herramientas de Depuracion, independientes
    // de la visibilidad de los paneles y ausentes en Juego.
    // Pintado de tiles en la escena: si la herramienta usa el clic, la escena
    // no selecciona ni mueve el gizmo con el.
    bool pintandoTiles = false;
    if (!modoJuego && editorController) {
        HerramientaTilemap& herramienta = editorController->getHerramientaTilemap();
        GameObject* seleccionado = editorController->getSelectedObject();
        herramienta.procesar(camara, seleccionado, sceneRegistry.get(),
                             editorController->getGestorComandos());
        pintandoTiles = herramienta.activa() && seleccionado &&
                        seleccionado->getComponent<Tilemap>() != nullptr;
    }
    if (!modoJuego && gizmoController_ && !pintandoTiles)
        gizmoController_->procesarSeleccion(io, camara, getGameObjectsScene());

    /*
     * Navegación libre (sin E y sin objeto seleccionado): el sistema de
     * ventanas y el gizmo no se dibujan, solo la escena 3D.
     */
    if (modoJuego && gizmoController_) {
        gizmoController_->apagar();
    }

    GUI();
    // Archivos del explorador soltados sobre la escena (viewport): crean el
    // objeto que corresponde a su tipo donde apunta el mouse.
    if (!modoJuego && managerGUI && editorController)
        DropEscena::procesar(*editorController, camara, getGameObjectsScene(),
                             gizmoController_.get(),
                             dimensionProyecto == DimensionProyecto::DosD);
    // El overlay HUD de las interfaces de juego solo se dibuja en el juego
    // exportado (sin GUI del editor). En el editor, cada interfaz se previsualiza
    // unicamente en la ventana dockable "Vista de interfaz" (pintarVistaInterfaz)
    // para no tapar la GUI del editor con un overlay a pantalla completa.
    if (!managerGUI || modoJuego)
        pintarInterfacesJuego();
    else
        overlayInterfaces_.descargarHud();
    // El HUD nativo (widgets del motor) se dibuja encima de todo: es la
    // interfaz del juego y no depende de la GUI del editor.
    pintarInterfazNativa();

    if (!CameraFrameSafety::validarCamara(sceneRegistry.get(),
                                          activeCameraObject, camara)) {
        activeCamera = nullptr;
        activeCameraObject = nullptr;
        camara = getActiveCamera();
    }
    if (!camara) {
        if (gizmoController_) gizmoController_->apagar();
        return;
    }

    if (!modoJuego && gizmoController_ && !pintandoTiles)
        gizmoController_->dibujarYRastrear(
            io, camara, getDimensionProyecto() == DimensionProyecto::DosD);
}

void GameScene::mostrarMensaje(const std::string& mensaje) {
    if (!managerGUI) return;
    if (StatusBarInterface* barra = managerGUI->getStatusBarGUI())
        barra->mostrarMensaje(mensaje);
}

void GameScene::setGizmoOperation(OperacionGizmo operation) {
    if (gizmoController_) gizmoController_->setGizmoOperation(operation);
}

OperacionGizmo GameScene::getGizmoOperation() const {
    if (!gizmoController_) return OperacionGizmo::Trasladar;
    return gizmoController_->getGizmoOperation();
}

bool GameScene::isGizmoGlobal() const noexcept {
    return gizmoController_ && gizmoController_->isGizmoGlobal();
}

void GameScene::setGizmoGlobal(bool global) noexcept {
    if (gizmoController_) gizmoController_->setGizmoGlobal(global);
}

bool GameScene::isGizmoCapturingInput() const {
    return gizmoController_ && gizmoController_->isCapturingInput();
}

bool GameScene::gizmoInUse() const {
    return gizmoController_ && gizmoController_->gizmoInUse();
}

void GameScene::alternarGuiaEje(int eje) {
    if (!editorController) return;
    // Sin objeto seleccionado no hay guia que dibujar: la tecla se ignora en
    // lugar de dejar prendido un estado invisible que el usuario no puede ver
    // ni apagar.
    if (!editorController->getSelectedObject()) {
        editorController->clearGuiaEje();
        return;
    }
    editorController->alternarGuiaEje(eje);
}

int GameScene::getGuiaEje() const noexcept {
    return editorController ? editorController->getGuiaEje() : -1;
}

bool GameScene::hayGuiaEje() const noexcept {
    return editorController && editorController->hayGuiaEje();
}

void GameScene::clearGuiaEje() {
    if (editorController) editorController->clearGuiaEje();
}

void GameScene::toggleEditorInterfaces() {
    menuActivo = !menuActivo;
}

float GameScene::getSensibilidadCamara() const noexcept {
    return sensibilidadCamara;
}

void GameScene::setSensibilidadCamara(float sensibilidad) noexcept {
    if (sensibilidad > 0.0f) sensibilidadCamara = sensibilidad;
}

float GameScene::getSensibilidadMovimientoCamara() const noexcept {
    return sensibilidadMovimientoCamara;
}

void GameScene::setSensibilidadMovimientoCamara(float sensibilidad) noexcept {
    if (sensibilidad > 0.0f) sensibilidadMovimientoCamara = sensibilidad;
}

const Apariencia& GameScene::getApariencia() const noexcept {
    return apariencia;
}

void GameScene::setApariencia(const Apariencia& valor) noexcept {
    apariencia = valor;
}

bool GameScene::getVentanaCamarasAbierta() const noexcept {
    return ventanaCamarasAbierta;
}

void GameScene::setVentanaCamarasAbierta(bool abierta) noexcept {
    ventanaCamarasAbierta = abierta;
}

void GameScene::setMenuActivo(bool activo) noexcept { menuActivo = activo; }

bool GameScene::isEditorActivo() const {
    return menuActivo;
}

void GameScene::clearSelection() {
    if (selecteableGUI) selecteableGUI->setReturnableEntity(nullptr);
}
