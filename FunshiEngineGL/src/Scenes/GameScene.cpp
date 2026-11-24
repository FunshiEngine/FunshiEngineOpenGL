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

GameScene::GameScene(GUIManager* manager, Rendering::Backend::IRenderBackend& backend)
    : managerGUI(manager),
      backend_(backend),
      sceneRegistry(std::make_unique<SceneRegistry>()),
      phisics(std::make_unique<PhysicsEngine>()),
      assetManager(
          std::make_unique<AssetManager>(std::make_unique<AssimpMeshLoader>())),
      // El gestor de shaders es el dueno de los programas: el renderer de la
      // escena y los demas renderers le piden el programa por nombre.
      gestorShaders(std::make_unique<Rendering::GestorShaders>(backend)),
      sceneRenderer(std::make_unique<SceneRenderer>(backend)),
      textureManager(std::make_unique<TextureManager>(
          std::make_unique<StbImageLoader>())),
      resolvedorMateriales_(std::make_unique<ResolvedorMateriales>()),
      editorController(
          std::make_unique<EditorController>(sceneRegistry.get(), phisics.get(),
                                              &events, assetManager.get())),
      sceneSerializer(
          std::make_unique<SceneSerializer>(sceneRegistry.get(),
                                             editorController.get(),
                                             assetManager.get())),
      // El backend real es miniaudio; si no hay device de audio, iniciar()
      // devuelve false y el motor queda en modo mudo (todos los reproducir
      // devuelven -1), sin romper nada: los clips existen pero no suenan.
      audioEngine(
          std::make_unique<AudioEngine>(std::make_unique<MiniAudioBackend>())) {
    // En modo runtime (exportación de juego) managerGUI es nullptr: no hay
    // selecteableGUI, menuBarGUI ni gizmo. Solo se inicializa lo necesario
    // para el juego: física, render, scripts, audio.
    if (managerGUI) {
        selecteableGUI = managerGUI->getSelecteableGUI();
        gizmoController_ = std::make_unique<GizmoController>(editorController.get(),
                                                             sceneRegistry.get(),
                                                             selecteableGUI);
        managerGUI->bindScene(sceneRegistry.get(), editorController.get(), &events,
                              [this]() { return getDimensionProyecto(); });
        managerGUI->setAudioEngine(audioEngine.get());
    }
    // El controlador restringe los cuerpos al plano cuando el proyecto es 2D;
    // lee la dimension viva de la escena por este proveedor.
    editorController->setDimensionProvider(
        [this]() { return getDimensionProyecto(); });
    asegurarGrilla();
    configurarResolverObjetosScripts();
    if (managerGUI) {
        menuBarGUI = managerGUI->getMenuBarGUI(&start);
        if (menuBarGUI) {
            menuBarGUI->setPausa(&simulacionPausada);
            menuBarGUI->setModoJuego(&modoJuego);
        }
        // El menu del editor alterna LOCAL/GLOBAL del gizmo editando este mismo
        // bool (mismo patron que el boton play/stop con toggleBool).
        if (menuBarGUI) menuBarGUI->setGizmoGlobal(gizmoController_->direccionGizmoGlobal());
    }
    // El renderer resuelve la textura de cada Material con el cache de imagenes
    // de la escena (un solo decode por archivo, imagen compartida).
    sceneRenderer->setTextureManager(textureManager.get());
    // El resolvedor decide que material se dibuja en cada region de la malla
    // (materiales por parte) y cual es el material unico del objeto.
    sceneRenderer->setResolvedorMateriales(resolvedorMateriales_.get());
    // Los renderers piden los programas al gestor de shaders.
    sceneRenderer->setGestorShaders(gestorShaders.get());
    // Vista previa de material: la esfera de muestra que pinta el panel del
    // material. Usa el cache de imagenes de la escena (las texturas del
    // material salen de ahi) y su propio renderer, con luces fijas: asi la
    // vista previa no depende de las luces de la escena.
    previewMaterial_ = std::make_unique<PreviewMaterial>(
        backend_, gestorShaders.get(), textureManager.get());
    if (managerGUI) managerGUI->setPreviewMaterial(previewMaterial_.get());
    // Renderer del HUD nativo: usa el backend de la escena para subir a GPU las
    // texturas de los widgets de imagen y el cache de imagenes de la escena
    // para decodificarlas (un solo decode por archivo).
    uiRenderer_ = std::make_unique<UIRenderer>(backend_);
    uiRenderer_->setTextureManager(textureManager.get());
}

GameScene::~GameScene() {
    ReflejoScripts::fijarResolverObjetos(nullptr);
    ReflejoScripts::fijarResolverComponentes({});
    eliminarSnapshotSimulacion();
    // Los renderers sueltan el gestor de shaders antes de que este muera: el
    // renderer de lineas es un unico global y si guardara el puntero, al
    // final del programa usaria un gestor ya destruido.
    if (sceneRenderer) sceneRenderer->setGestorShaders(nullptr);
    // Descarga las vistas web vivas antes de que muera la escena (con un backend
    // real libera sus recursos; con el nulo es no-op).
    overlayInterfaces_.vaciar();
    // Libera las geometrias cacheadas del SceneRenderer (meshes SUV + grilla)
    // si se llegaron a compilar. En el flujo normal main() crea GameScene con
    // new y no la destruye antes de glfwTerminate, asi que esto es higiene
    // defensiva (contexto GL vivo).
    if (sceneRenderer) sceneRenderer->destruir();
    if (editorController) editorController->clearScene();
    if (selecteableGUI) selecteableGUI->bindScene(nullptr, nullptr, nullptr);
}

void GameScene::configurarProyecto(const std::string& nombreProyecto) {
    EditorConfig::asegurarEstructuraProyecto(nombreProyecto);
    // No hay catalogo de audio: cada componente guarda la ruta del archivo que
    // el usuario le asigno, y el motor reproduce esa ruta.
}

AudioEngine* GameScene::getAudioEngine() const noexcept {
    return audioEngine.get();
}

// Reproduce o detiene los AudioSource y los Musica de la escena segun la
// transicion de modo play. Al ENTRAR en play: inyecta el motor y dispara los
// de reproduccion automatica. Al SALIR de play: detiene todo (evita colas de
// audio en el editor). El motor se inyecta siempre, para que el inspector
// pueda probar.
void GameScene::sincronizarAudioPlay(bool entrarEnPlay) {
    auto* gameObjects = getGameObjectsScene();
    if (!gameObjects || gameObjects->isEmpty()) return;
    Position<GameObject*>* pos = gameObjects->first();
    while (pos && pos->getElement()) {
        GameObject* objeto = pos->getElement();
        if (AudioSource* source = objeto->getComponent<AudioSource>()) {
            source->setMotor(audioEngine.get());
            if (entrarEnPlay) {
                if (source->isReproduccionAutomatica() && !source->isReproduciendo())
                    source->reproducir();
            } else {
                source->detener();
            }
        }
        // La musica lleva su propia lista de pistas: al entrar en play suena
        // la primera y el motor va pasando a las siguientes.
        if (Musica* musica = objeto->getComponent<Musica>()) {
            musica->setMotor(audioEngine.get());
            if (entrarEnPlay) {
                if (musica->isReproduccionAutomatica()) musica->reproducir();
            } else {
                musica->detener();
            }
        }
        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
}

ListaDE<GameObject*>* GameScene::getGameObjectsScene() {
    sceneRegistry->refreshGameObjectView();
    return sceneRegistry->getGameObjects();
}

void GameScene::configurarResolverObjetosScripts() {
    auto buscarObjeto = [this](const std::string& nombre) -> GameObject* {
        auto* objetos = getGameObjectsScene();
        if (!objetos || objetos->isEmpty()) return nullptr;
        Position<GameObject*>* pos = objetos->first();
        while (pos && pos->getElement()) {
            if (nombre == pos->getElement()->inputName) return pos->getElement();
            pos = (pos != objetos->last()) ? objetos->next(pos) : nullptr;
        }
        return nullptr;
    };

    ReflejoScripts::fijarResolverObjetos(buscarObjeto);

    // Referencias a componentes (TagTipo::Componente): un campo reflejado puede
    // apuntar al InterfaceComponent (interfaz web) o al UIComponent (interfaz
    // nativa) de otro objeto. El tipo se especifica por nombre.
    ReflejoScripts::fijarResolverComponentes(ReflejoScripts::ResolverComponentes{
        [buscarObjeto](const std::string& nombre,
                       const std::string& tipo) -> void* {
            GameObject* objeto = buscarObjeto(nombre);
            if (!objeto) return nullptr;
            if (tipo == "InterfaceComponent") {
                return static_cast<void*>(
                    objeto->getComponent<InterfaceComponent>());
            }
            if (tipo == "UIComponent") {
                return static_cast<void*>(objeto->getComponent<UIComponent>());
            }
            return nullptr;
        },
        [this](const void* componente, const std::string& tipo) -> std::string {
            if (!componente) return std::string();
            if (tipo != "InterfaceComponent" && tipo != "UIComponent")
                return std::string();
            auto* objetos = getGameObjectsScene();
            if (!objetos || objetos->isEmpty()) return std::string();
            Position<GameObject*>* pos = objetos->first();
            while (pos && pos->getElement()) {
                GameObject* o = pos->getElement();
                const bool coincide =
                    (tipo == "InterfaceComponent")
                        ? (o->getComponent<InterfaceComponent>() ==
                           static_cast<const InterfaceComponent*>(componente))
                        : (o->getComponent<UIComponent>() ==
                           static_cast<const UIComponent*>(componente));
                if (coincide) return o->inputName;
                pos = (pos != objetos->last()) ? objetos->next(pos) : nullptr;
            }
            return std::string();
        }});
}

void GameScene::configurarResolverObjetosEditor() {
    // Mismo resolver que en play mode, pero disponible en editor para
    // serializacion/deserializacion y drag&drop de GameObjects en scripts.
    configurarResolverObjetosScripts();
}

void GameScene::asegurarGrilla() {
    // La grilla de rejilla es un auxiliar del mundo 3D: un proyecto 2D juega
    // sobre el plano XY y no se siembra.
    if (!SiembraEscena::sembrarGrilla(dimensionProyecto)) return;
    if (!sceneRegistry || !editorController) return;
    auto* lista = getGameObjectsScene();

    // Buscar grilla existente en la escena (sin importar si la escena esta vacia o no)
    bool tieneGrilla = false;
    if (lista && !lista->isEmpty()) {
        Position<GameObject*>* pos = lista->first();
        while (pos && pos->getElement()) {
            if (std::string(pos->getElement()->inputName) == "Grilla") {
                tieneGrilla = true;
                break;
            }
            pos = (pos != lista->last()) ? lista->next(pos) : nullptr;
        }
    }

    if (tieneGrilla) return;

    // Usar SimpleObject en lugar de Modelos3D para evitar que el dibujado
    // de la grilla se ancle a un modelo inexistente. SimpleObject no tiene malla.
    // Si no hay root, crear uno implicitamente (SceneRegistry::createObject lo hace
    // si entitys esta vacia). Si entitys no esta vacia pero getRoot() devuelve nullptr
    // (no deberia pasar), usamos nullptr como parent y createObject creara el root.
    GameObject* root = sceneRegistry->getRoot();
    GameObject* crea = editorController->createGameObject(
        std::make_unique<SimpleObject>(), root);
    if (!crea) {
        // Si fallo (p. ej., porque root era nullptr y entitys no estaba vacia),
        // intentar con parent = nullptr para que createObject cree un nuevo root.
        crea = editorController->createGameObject(
            std::make_unique<SimpleObject>(), nullptr);
        if (!crea) return;
    }
    std::snprintf(crea->inputName, sizeof(crea->inputName), "Grilla");
    // Forzar actualizacion de la vista de objetos para incluir la grilla recien creada
    getGameObjectsScene();
    Transform* transform = crea->getComponent<Transform>();
    if (!transform) {
        crea->addComponent(std::make_unique<Transform>());
        transform = crea->getComponent<Transform>();
    }
    if (transform) transform->setTranslatef(0.f, -0.5f, 0.f);
    crea->addComponent(std::make_unique<Grid>());
}
