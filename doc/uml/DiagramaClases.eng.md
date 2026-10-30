# Class Diagram - FunshiEngineOpenGL

UML Class Diagram for the FunshiEngineGL 3D graphics engine. Generated from source code in `FunshiEngineGL/src/`.

> **Important Mermaid syntax note**: Class names with special characters (`::`, `~`, `<`, `>`, `*`) must be simplified or use the `as "name"` alias for proper rendering on GitHub/GitLab. Simplified names are used here.

Conventions:
- **Types**: PascalCase (`GameObject`, `RenderCommand`)
- **Methods**: camelCase (`initEngine`, `processCommands`)
- **Language**: English (identifiers, comments, documentation)
- **Relationships**: `..|>` inheritance/implementation, `--` association, `*` composition, `o--` aggregation

## 1. Exception Hierarchy

```mermaid
classDiagram
    class StdException {
        <<standard>>
        +what()
    }
    class StdRuntimeError {
        <<standard>>
    }
    class Throwable {
        <<abstract>>
        -message: string
        -stackTrace: string
        +what(): const char* override
        +getStackTrace(): const char*
        +~Throwable()
    }
    class RuntimeException {
        -stackTrace: vector<string>
        -exceptionType: string
        +getStackTrace(): string
        +getExceptionType(): const string&
        +~RuntimeException()
    }
    class NullPointerException {
    }
    class ClassCastException {
    }
    class InvalidOperationException {
    }
    class BoundaryViolationException {
    }

    StdException <|-- StdRuntimeError
    StdRuntimeError <|-- Throwable
    Throwable <|-- RuntimeException
    RuntimeException <|-- NullPointerException
    RuntimeException <|-- ClassCastException
    RuntimeException <|-- InvalidOperationException
    RuntimeException <|-- BoundaryViolationException
```

Location: `src/ExceptionsCPP/`

## 2. Entities, Components and Scene Objects

```mermaid
classDiagram
    class Comparable {
        <<template, abstract>>
        +compareTo(p1): int
    }
    class Entity {
        <<abstract>>
        -components: ListaDE<Component*>*
        -myBinario: Binario
        -parentEntity: Entity*
        -childEntities: vector<Entity*>
        -transformOrigin: Transform
        +Entity()
        +~Entity()
        +getComponents(): ListaDE<Component*>*
        +addComponent(component: Component*)
        +deleteComponent(component: Component*)
        +getComponentByName(typeName: string): Component*
    }
    class GameObject {
        -id: int
        -tag_: string
        -dirty_: bool
        +GameObject()
        +~GameObject()
        +setId(id: int)
        +setTag(tag: string)
        +getTag(): const string&
        +isDirty(): bool
        +setDirty(sucio: bool)
    }
    class Component {
        <<abstract>>
        -name: string
        +~Component()
        +onUpdate(owner: GameObject&, deltaTime: float)
    }
    class Transform {
        -objectTranslatef: float[3]
        -objectScalef: float[3]
        -objectRotatef: float[4]
        +Transform()
        +setTranslatef(x, y, z)
        +setRotatef(angle, x, y, z)
        +setScalef(x, y, z)
        +getTranslatef(): float*
        +getScalef(): float*
    }
    class CameraComponent {
        -owner: GameObject*
        -m_pos: float[3]
        -m_dir: float[3]
        -m_fov: float
        -nearPlane: float
        -farPlane: float
        +CameraComponent()
        +onLoaded(owner: GameObject&) override
        +sincronizarConTransform()
    }
    class MeshRenderer {
        +MeshRenderer()
        +~MeshRenderer()
    }
    class Light {
        +Light()
    }
    class Skybox {
        +Skybox()
    }
    class RigidBody {
        +RigidBody()
    }
    class Collider {
        +Collider()
    }

    Comparable <|-- GameObject
    Entity <|-- GameObject
    Component <|-- Transform
    Component <|-- CameraComponent
    Component <|-- MeshRenderer
    Component <|-- Light
    Component <|-- Skybox
    Component <|-- RigidBody
    Component <|-- Collider
    GameObject o-- Transform : owns transform
    GameObject "*"-- "*" Component : owns components
```

## 3. Rendering (Backend and Pipeline)

```mermaid
classDiagram
    class IRenderBackend {
        <<interface>>
        +init(): bool
        +available(): bool
        +createMesh(data: MeshData): Handle
        +destroyMesh(mesh: Handle)
        +drawMesh(mesh: Handle, firstIndex: uint, indexCount: uint)
        +createLineBatch(vertices: float*, count: size_t): Handle
        +destroyLineBatch(batch: Handle)
        +updateLineBatch(batch: Handle, vertices: float*, count: size_t)
        +drawLineBatch(batch: Handle, count: uint)
        +createTexture2D(image: Image2D): Handle
        +destroyTexture2D(texture: Handle)
        +bindTexture2D(texture: Handle, unit: int)
        +createTextureCube(image: ImageCube): Handle
        +destroyTextureCube(texture: Handle)
        +bindTextureCube(texture: Handle, unit: int)
        +createProgram(vertexSource: const char*, fragmentSource: const char*): Handle
        +destroyProgram(program: Handle)
        +useProgram(program: Handle)
        +uniformLocation(program: Handle, name: const char*): int
        +setUniformMat4(location: int, value: mat4)
        +setUniformMat3(location: int, value: mat3)
        +setUniformVec3(location: int, value: vec3)
        +setUniformFloat(location: int, value: float)
        +setUniformInt(location: int, value: int)
        +setViewport(x, y, width, height)
        +clearScreen(color: float[3])
        +applyBaseState()
        +setClearColor(color: float[3])
        +diagnosticoGPU(): const char*
        +setBlendEnabled(enabled: bool)
        +setBlendMode(mode: BlendMode)
        +setDepthTestEnabled(enabled: bool)
        +setDepthMask(enabled: bool)
        +setDepthFunc(func: DepthFunc)
        +drawFullscreenTriangle()
        +bindDepthTarget(target: Handle)
        +depthTargetTexture(target: Handle): Handle
    }
    class OpenGL3Backend {
        -meshes: unordered_map<Handle, GpuMesh>
        -lineBatches: unordered_map<Handle, GpuLineBatch>
        -targets: unordered_map<Handle, GpuTarget>
        -depthTargets: unordered_map<Handle, GpuDepthTarget>
        -fboCargadas_: bool
        -modoBlend_: BlendMode
        +OpenGL3Backend()
        +~OpenGL3Backend()
        +init(): bool override
        +available(): bool override
    }

    IRenderBackend <|.. OpenGL3Backend
```

Location: `src/Rendering/`

## 4. Physics and Audio

```mermaid
classDiagram
    class IPhysicsBackend {
        <<interface>>
        +stepSimulation(deltaTime: float)
        +addRigidBody(body: RigidBody*)
        +removeRigidBody(body: RigidBody*)
        +takeContactEvents(): vector<ContactEvent>
        +setGravity(x: float, y: float, z: float)
        +getGravity(x: float&, y: float&, z: float)
        +raycast(ox, oy, oz, dx, dy, dz, reach: float): RaycastResult
    }
    class PhysicsEngine {
        -backend_: IPhysicsBackend
        +PhysicsEngine(backend: IPhysicsBackend)
        +~PhysicsEngine()
        +stepSimulation(deltaTime: float)
        +addRigidBody(body: RigidBody*)
        +removeRigidBody(body: RigidBody*)
        +takeContactEvents(): vector<ContactEvent>
        +setGravity(x: float, y: float, z: float)
        +getGravity(x: float&, y: float&, z: float)
        +raycast(ox, oy, oz, dx, dy, dz, reach: float): RaycastResult
    }
    class RigidBody { +RigidBody(); }
    class Collider { +Collider(); }
    class ContactEvent <<struct>> { }
    class RaycastResult <<struct>> {
        -hit: bool
        -distance: float
        -pointX: float
        -pointY: float
        -pointZ: float
        -normalX: float
        -normalY: float
        -normalZ: float
        -collider: Collider*
    }

    IPhysicsBackend <|.. PhysicsEngine : uses
    PhysicsEngine *-- IPhysicsBackend : owns

    class IAudioBackend {
        <<interface>>
        +init(): bool
        +close()
        +playFile(path: string, volume: float, looping: bool, position: float*): int
        +stop(id: int)
        +stopAll()
        +setMasterVolume(volume: float)
        +setSoundPosition(id: int, x: float, y: float, z: float)
        +dispatch(deltaTime: float)
    }
    class MiniAudioBackend {
        +MiniAudioBackend()
        +~MiniAudioBackend()
        +init(): bool override
        +close() override
    }
    class NullAudioBackend {
        +NullAudioBackend()
        +~NullAudioBackend()
        +init(): bool override
        +close() override
    }
    class AudioEngine {
        -impl_: IAudioBackend
        +AudioEngine(impl: IAudioBackend)
        +~AudioEngine()
        +play(path: string, volume: float, looping: bool, position: float*): int
        +stop(handle: int)
        +stopAll()
        +setMasterVolume(volumen: float)
        +setSoundPosition(handle: int, x: float, y: float, z: float)
        +dispatch(deltaTime: float)
    }

    IAudioBackend <|.. MiniAudioBackend
    IAudioBackend <|.. NullAudioBackend
    AudioEngine *-- IAudioBackend : owns
```

Location: `src/Physics/`, `src/Audio/`

## 5. Scripting and Behaviors

```mermaid
classDiagram
    class IScriptBehaviour {
        <<interface>>
        -api: const ApiScriptGameObject*
        -services: const ScriptServices*
        +~IScriptBehaviour()
        +onStart(owner: GameObject*)
        +onUpdate(owner: GameObject*, deltaTime: float)
        +onCollisionEnter(owner: GameObject*, own: Collider*, other: Collider*)
        +onCollisionStay(owner: GameObject*, own: Collider*, other: Collider*)
        +onCollisionExit(owner: GameObject*, own: Collider*, other: Collider*)
        +connectApi(table: const ApiScriptGameObject*)
        +onStop(owner: GameObject*)
        +reflectedFields(): vector<FieldDef>
    }
    class BackendScript {
        <<abstract>>
        +~BackendScript()
        +language(): const char*
        +compileAndLoad(source: string, className: string, loaded: LoadedBehavior&, error: string&): bool
        +unload(behavior: LoadedBehavior&)
        +unload(behavior: LoadedBehavior&, owner: GameObject*)
        +callStart(behavior: LoadedBehavior&, owner: GameObject*)
        +callUpdate(behavior: LoadedBehavior&, owner: GameObject*, deltaTime: float)
        +callStop(behavior: LoadedBehavior&, owner: GameObject*)
        +callContact(behavior: LoadedBehavior&, owner: GameObject*, own: Collider*, other: Collider*, type: ContactType)
    }
    class BackendCpp {
        +BackendCpp()
        +~BackendCpp()
        +language(): const char* override
    }
    class BackendJava {
        +BackendJava()
        +~BackendJava()
        +language(): const char* override
        +inject(behavior: LoadedBehavior&, values: vector<FieldValue>): void override
        +extract(behavior: LoadedBehavior&): vector<FieldValue> override
        +shutdownJvm(): static
    }
    class ScriptRuntime {
        <<static>>
        +sourceLanguage(source: string): static const char*
        +backendFor(language: string): static BackendScript*
        +compileAndLoad(source: string, className: string, loaded: LoadedBehavior&, error: string&): static bool
        +unload(c: LoadedBehavior&): static void
        +callStart(c: LoadedBehavior&, owner: GameObject*): static void
        +callUpdate(c: LoadedBehavior&, owner: GameObject*, deltaTime: float): static void
        +callStop(c: LoadedBehavior&, owner: GameObject*): static void
        +callContact(c: LoadedBehavior&, owner: GameObject*, own: Collider*, other: Collider*, type: ContactType): static void
        +inject(c: LoadedBehavior&, values: vector<FieldValue>): static void
        +extract(c: LoadedBehavior&): static vector<FieldValue>
        +changeSource(c: LoadedBehavior&): static bool
        +applScripts(): static void
    }
    class LoadedBehavior { }
    class ScriptGameObject { }
    class BehaviourReflection { }

    IScriptBehaviour <|.. BackendScript : uses
    BackendScript <|-- BackendCpp
    BackendScript <|-- BackendJava
    ScriptRuntime ..> BackendScript : dispatches
    ScriptRuntime --> LoadedBehavior : manages
```

Location: `src/Behaviour/`

## 6. Commands (Undo/Redo)

```mermaid
classDiagram
    class ICommand {
        <<interface>>
        +~ICommand()
        +execute()
        +undo()
        +description(): string
    }
    class CommandManager {
        -commands: vector<unique_ptr<ICommand>>
        -currentIndex: int
        +CommandManager()
        +~CommandManager()
        +executeCommand(command: unique_ptr<ICommand>)
        +undo()
        +redo()
        +canUndo(): bool
        +canRedo(): bool
    }
    class AddComponentCommand { +execute() override +undo() override +description(): string override }
    class RemoveComponentCommand { +execute() override +undo() override +description(): string override }
    class CreateObjectCommand { +execute() override +undo() override +description(): string override }
    class DeleteObjectCommand { +execute() override +undo() override +description(): string override }
    class ReparentCommand { +execute() override +undo() override +description(): string override }
    class TransformCommand { +execute() override +undo() override +description(): string override }
    class SetTagCommand { +execute() override +undo() override +description(): string override }
    class DuplicateObjectCommand { +execute() override +undo() override +description(): string override }
    class ClearSceneCommand { +execute() override +undo() override +description(): string override }
    class AdjustVariableCommand { +execute() override +undo() override +description(): string override }

    ICommand <|.. CommandManager : uses
    ICommand <|-- AddComponentCommand
    ICommand <|-- RemoveComponentCommand
    ICommand <|-- CreateObjectCommand
    ICommand <|-- DeleteObjectCommand
    ICommand <|-- ReparentCommand
    ICommand <|-- TransformCommand
    ICommand <|-- SetTagCommand
    ICommand <|-- DuplicateObjectCommand
    ICommand <|-- ClearSceneCommand
    ICommand <|-- AdjustVariableCommand
```

Location: `src/Commands/`

## 7. Scene, Serialization and Registry

```mermaid
classDiagram
    class SceneRegistry {
        -ownedGameObjects: vector<unique_ptr<GameObject>>
        -entitys: LinkedTree<GameObject*>
        -gameObjects: DoublyLinkedList<GameObject*>
        -viewDirty: bool
        +SceneRegistry()
        +~SceneRegistry()
        +getEntitysTree(): LinkedTree<GameObject*>*
        +getGameObjects(): DoublyLinkedList<GameObject*>*
        +refreshGameObjectView()
        +getRoot(): GameObject*
        +contains(object: GameObject*): bool
        +createObject(object: unique_ptr<GameObject>, parent: GameObject*): GameObject*
        +availableId(): int
        +replaceRoot(root: unique_ptr<GameObject>): bool
        +deleteObject(object: GameObject*): bool
        +deleteObjectByID(id: int): bool
        +reparent(object: GameObject*, parent: GameObject*): bool
        +clear()
        +getObjectByID(id: int): GameObject*
        +takeObject(object: GameObject*): unique_ptr<GameObject>
        +takeSubtree(object: GameObject*): vector<unique_ptr<GameObject>>
        +takeAllNonRoot(): vector<unique_ptr<GameObject>>
        +restoreSubtree(objects: vector<unique_ptr<GameObject>>, parent: GameObject*): GameObject*
    }
    class GameScene {
        -managerGUI: GUIManager*
        -backend_: IRenderBackend
        -sceneRegistry: unique_ptr<SceneRegistry>
        -physics: unique_ptr<PhysicsEngine>
        -assetManager: unique_ptr<AssetManager>
        -shaderManager: unique_ptr<ShaderManager>
        -sceneRenderer: unique_ptr<SceneRenderer>
        -particleSystem: ParticleSystem
        -editorController: unique_ptr<EditorController>
        +GameScene(backend: IRenderBackend)
        +~GameScene()
        +init()
        +update(deltaTime: float)
        +render()
        +setActiveCamera(object: GameObject*)
        +getParticleSystem(): ParticleSystem&
        +getEditorController(): EditorController*
        +getAppearance(): const Appearance&
    }

    SceneRegistry *-- GameObject : owns
    GameScene *-- SceneRegistry : owns
    GameScene *-- PhysicsEngine : owns
    GameScene *-- AssetManager : owns
    GameScene *-- SceneRenderer : owns

    class SceneSerializer {
        -scene: SceneRegistry*
        -controller: EditorController*
        -assets: AssetManager*
        -diagnostics: DiagnosticsRegistry*
        +SceneSerializer(scene, controller, assetsManager, registry)
        +save(filename: string)
        +load(pathTxt: string, semiPath: string): bool
    }
    class AssetManager {
        -meshes: unordered_map<string, shared_ptr<Mesh>>
        -failures: unordered_map<string, string>
        -loader_: unique_ptr<IMeshLoader>
        +AssetManager(loader: unique_ptr<IMeshLoader>)
        +~AssetManager()
        +getMesh(path: string): shared_ptr<const Mesh>
        +putMesh(path: string, mesh: shared_ptr<Mesh>)
        +hasMesh(path: string): bool
        +meshCount(): size_t
        +removeMesh(path: string)
        +clearUnusedMeshes()
        +clearFailedMeshes()
        +hasFailedMesh(path: string): bool
        +failedMeshCount(): size_t
        +reloadMesh(path: string)
        +setLoader(loader: unique_ptr<IMeshLoader>)
    }
    class IMeshLoader { <<interface>> +~IMeshLoader() +load(path: string): shared_ptr<Mesh> }
    class AssimpMeshLoader {
        +AssimpMeshLoader()
        +~AssimpMeshLoader()
        +load(path: string): shared_ptr<Mesh> override
    }
    class EditorController { +EditorController() +~EditorController(); }
    class DiagnosticsRegistry { +DiagnosticsRegistry() +~DiagnosticsRegistry(); }
    class ParticleSystem {
        +ParticleSystem()
        +~ParticleSystem()
        +update(deltaTime: float)
        +emit(config: EmitterConfig)
    }

    SceneSerializer --> SceneRegistry : uses
    SceneSerializer --> EditorController : uses
    SceneSerializer --> AssetManager : uses
    SceneSerializer --> DiagnosticsRegistry : uses
    AssetManager --> IMeshLoader : injects (Strategy)
    AssimpMeshLoader ..|> IMeshLoader : implements
```

Location: `src/Scenes/`, `src/Assets/`

## 8. GUI and Application States

```mermaid
classDiagram
    class ApplicationStateMachine {
        -state: ApplicationState
        +getState(): ApplicationState
        +is(value: ApplicationState): bool
        +transitionTo(value: ApplicationState)
    }
    class ApplicationState <<enumeration>> {
        MainMenu
        Editing
        Debugging
        Playing
        Exiting
    }
    class GUIOrchestrator {
        -machine: ApplicationStateMachine*
        -pendingCloseRequest: bool
        -pausedSimulation_: bool
        -resetRequest_: bool
        +GUIOrchestrator(machine: ApplicationStateMachine*)
        +handleEscapeKey()
        +startStudy(): bool
        +handleSimulationKey(key: SimulationKey)
        +toggleSimulation()
        +startGame()
        +requestReset()
        +consumeResetRequest(): bool
        +shouldMenuBeVisible(): bool
        +shouldSceneRun(): bool
        +pendingClose(): bool
        +inSimulation(): bool
        +inDebug(): bool
        +inGameMode(): bool
        +pausedSimulation(): bool
        +insideEditor(): bool
        +getState(): ApplicationState
    }
    class GUIManager {
        -fileManager: unique_ptr<FileManager>
        -menuGUI: unique_ptr<MenuGUI>
        -settingsModel: unique_ptr<GameObject>
        -settingsGUI: unique_ptr<SettingsInterface>
        -selectableGUI: unique_ptr<SceneSelectionInterface>
        -menuBarGUI: unique_ptr<SceneMenuBarInterface>
        -treeFilesGUI: unique_ptr<TreeFilesInterface>
        -dockSpaceGUI: unique_ptr<DockSpaceInterface>
        -iconsGUI: unique_ptr<IconsGUI>
        -iconRule: unique_ptr<IconRuleCatalog>
        -iconResolver: unique_ptr<IconCatalog>
        -editor: EditorController*
        -currentFolder: unique_ptr<ContentFolderInterface>
        -statusBarGUI: unique_ptr<StatusBarInterface>
        -terminalGUI: unique_ptr<TerminalInterface>
        -modsPanelsGUI: unique_ptr<ModsPanelsInterface>
        -particleAnimationsGUI: unique_ptr<ParticleAnimationsInterface>
        -editorEvents: EditorEventBus
        -windows_: vector<GeneralUserInterface*>
        +GUIManager(window: GLFWwindow*)
        +~GUIManager()
        +bindScene(scene, editor, events, dimensionProvider)
        +getEditorEventBus(): EditorEventBus*
        +getMenuGUI(): MenuGUI*
        +getSettingsGUI(gameObject: GameObject*): SettingsInterface*
        +getMenuBarGUI(): SceneMenuBarInterface*
        +getSelectableGUI(): SceneSelectionInterface*
        +getDockSpaceGUI(): DockSpaceInterface*
        +registerWindow(ventana: GeneralUserInterface*)
        +restoreWindowStates(states: map<string, bool>)
        +getWindowStates(): map<string, bool>
        +setWindowState(name: string, abierta: bool)
    }
    class MenuGUI { +MenuGUI() +~MenuGUI() +SetMenuActive(active: bool) +ConsultMenu(): bool +updateProjects() }
    class DockSpaceInterface { +DockSpaceInterface() +~DockSpaceInterface(); }
    class SceneSelectionInterface { +SceneSelectionInterface() +~SceneSelectionInterface(); }
    class SceneMenuBarInterface { +SceneMenuBarInterface() +~SceneMenuBarInterface(); }

    GUIOrchestrator --> ApplicationStateMachine : uses
    GUIManager --> MenuGUI : owns
    GUIManager --> DockSpaceInterface : owns
    GUIManager --> SceneSelectionInterface : owns
    GUIManager --> SceneMenuBarInterface : owns
    GUIManager --> GeneralUserInterface : registers windows

    class EditorEventBus {
        -listeners: unordered_map<size_t, Callback>
        -nextToken: size_t
        +subscribe(callback: Callback): size_t
        +unsubscribe(token: size_t)
        +publish(event: SceneEvent)
    }
    class SceneEventType <<enumeration>> {
        ObjectCreated
        ObjectDeleted
        ObjectReparented
        ComponentChanged
        SceneCleared
        ObjectSelected
        ComponentStructureChanged
    }
    class SceneEvent <<struct>> {
        -type: SceneEventType
        -object: GameObject*
        -relatedObject: GameObject*
    }
    class EditorConfig {
        -data_: Data
        -generalPending: bool
        -generalPath: string
        -lastGeneralWrite: time_point
        +EditorConfig()
        +~EditorConfig()
        +load(path: string)
        +save(path: string)
        +saveGeneral(path: string)
        +loadGeneral(path: string)
        +saveProject(name: string, path: string)
        +loadProject(name: string, path: string)
        +requestGeneralSave(path: string)
        +dumpGeneralSave()
        +restore()
        +data(): Data&
    }
    class Data <<struct>> {
        -version: int
        -projectName: string
        -language: string
        -cameraSensitivity: float
        -movementSensitivity: float
        -masterVolume: float
        -camerasWindowOpen: bool
        -gizmoOperation: int
        -gizmoGlobal: bool
        -activeCameraId: int
        -dimension: ProjectDimension
        -windowStates: map<string, bool>
        -motorMods: map<string, bool>
        -projectMods: map<string, bool>
    }
```

## 9. Window, Time and Input

```mermaid
classDiagram
    class Window {
        +Window()
        +~Window()
        +create(width: int, height: int, title: string)
        +shouldClose(): bool
        +processEvents()
        +swapBuffers()
        +getContext(): GLFWwindow*
    }
    class EngineTime {
        <<static>>
        +deltaTime(): float
        +totalTime(): float
        +update()
    }
    class EditorInput {
        +EditorInput()
        +~EditorInput()
        +processInput(window: GLFWwindow*)
        +resetDeltaLook()
        +applyCursorMode(window: GLFWwindow*)
    }
    class TerminalEmulator { +TerminalEmulator() +~TerminalEmulator(); }
    class TerminalSession { +TerminalSession() +~TerminalSession(); }

    Window --> EngineTime : uses
    Window --> EditorInput : uses
```

## 10. Exporter and Projects

```mermaid
classDiagram
    class GameExporter {
        +GameExporter()
        +~GameExporter()
        +exportGame(config: GameConfig)
    }
    class GameConfig {
        -executableName: string
        -projectName: string
        -targetPlatform: string
    }
    class ProjectManager {
        +ProjectManager()
        +~ProjectManager()
        +createProject(name: string)
        +deleteProject(name: string)
        +listProjects(): vector<string>
    }
    class ExportStatus { +ExportStatus() +~ExportStatus(); }

    GameExporter --> GameConfig : uses
    ProjectManager --> GameConfig : uses
```

## 11. Data Structures

```mermaid
classDiagram
    class DoublyLinkedList as "DoublyLinkedList<T>" {
        <<template>>
        -head: Node*
        -tail: Node*
        -size: int
        +DoublyLinkedList()
        +~DoublyLinkedList()
        +isEmpty(): bool
        +first(): Node*
        +last(): Node*
        +next(pos: Node*): Node*
        +getElement(): T
    }
    class LinkedTree as "LinkedTree<T>" {
        <<template>>
        +LinkedTree()
        +~LinkedTree()
    }
    class Node as "Node<T>" {
        <<template>>
        -element: T
        -prev: Node*
        -next: Node*
    }
    class PriorityQueue as "PriorityQueue<T>" {
        <<template>>
        +PriorityQueue()
        +~PriorityQueue()
    }
    class BinaryHeap as "BinaryHeap<T>" {
        <<template>>
        +BinaryHeap()
        +~BinaryHeap()
    }
    class MaxBinaryHeap as "MaxBinaryHeap<T>" { +MaxBinaryHeap() +~MaxBinaryHeap(); }
    class MinBinaryHeap as "MinBinaryHeap<T>" { +MinBinaryHeap() +~MinBinaryHeap(); }
    class BinaryTree as "BinaryTree<T>" {
        <<template>>
        +BinaryTree()
        +~BinaryTree()
    }

    Node <|.. DoublyLinkedList : uses
    MaxBinaryHeap ..|> BinaryHeap
    MinBinaryHeap ..|> BinaryHeap
```

Location: `src/DataStructures/`