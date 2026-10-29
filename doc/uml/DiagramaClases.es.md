# Diagrama de Clases - FunshiEngineOpenGL

Diagrama UML de clases del motor gráfico 3D FunshiEngineGL. Generado a partir del código fuente en `FunshiEngineGL/src/`.

> **Nota importante de sintaxis Mermaid**: Los nombres de clase con caracteres especiales (`::`, `~`, `<`, `>`, `*`) deben simplificarse o usar el alias `as "nombre"` para que GitHub/GitLab los rendered correctamente. He usado nombres simplificados aquí.

Convenciones:
- **Tipos**: PascalCase (`GameObject`, `RenderCommand`)
- **Métodos**: camelCase (`iniciarMotor`, `procesarComandos`)
- **Idioma**: español (identificadores, comentarios, documentación)
- **Relaciones**: `..|>` herencia / implementación, `--` asociación, `*` composición, `o--` agregación

## 1. Jerarquía de Excepciones

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
        -tipoExcepcion: string
        +getStackTrace(): string
        +getTipoExcepcion(): const string&
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

Ubicación: `src/ExcepcionesCPP/`

## 2. Entidades, Componentes y Objetos de Escena

```mermaid
classDiagram
    class Comparable {
        <<template, abstract>>
        +compareTo(p1): int
    }
    class Entity {
        <<abstract>>
        -components: ListaDE<Component*>
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
    GameObject o-- Transform : posee transform
    GameObject "*"-- "*" Component : posee componentes
```

Ubicación: `src/Objetos/`, `src/Objetos/Componentes/`

## 3. Rendering (Backend y Pipeline)

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

Ubicación: `src/Rendering/`

## 4. Física y Audio

```mermaid
classDiagram
    class IPhysicsBackend {
        <<interface>>
        +stepSimulation(deltaTime: float)
        +addRigidBody(body: RigidBody*)
        +removeRigidBody(body: RigidBody*)
        +tomarEventosContacto(): vector<EventoContacto>
        +fijarGravedad(x: float, y: float, z: float)
        +gravedad(x: float&, y: float&, z: float)
        +raycast(ox, oy, oz, dx, dy, dz, alcance: float): ResultadoRaycast
    }
    class PhysicsEngine {
        -backend_: IPhysicsBackend
        +PhysicsEngine(backend: IPhysicsBackend)
        +~PhysicsEngine()
        +stepSimulation(deltaTime: float)
        +addRigidBody(body: RigidBody*)
        +removeRigidBody(body: RigidBody*)
        +tomarEventosContacto(): vector<EventoContacto>
        +fijarGravedad(x: float, y: float, z: float)
        +gravedad(x: float&, y: float&, z: float)
        +raycast(ox, oy, oz, dx, dy, dz, alcance: float): ResultadoRaycast
    }
    class RigidBody { +RigidBody(); }
    class Collider { +Collider(); }
    struct EventoContacto { }
    struct ResultadoRaycast {
        -impacto: bool
        -distancia: float
        -puntoX: float
        -puntoY: float
        -puntoZ: float
        -normalX: float
        -normalY: float
        -normalZ: float
        -collider: Collider*
    }

    IPhysicsBackend <|.. PhysicsEngine : uses
    PhysicsEngine *-- IPhysicsBackend : owns

    class IAudioBackend {
        <<interface>>
        +iniciar(): bool
        +cerrar()
        +reproducirArchivo(ruta: string, volumen: float, bucle: bool, posicion: float*): int
        +detener(id: int)
        +detenerTodo()
        +setVolumenMaestro(volumen: float)
        +setPosicionSonido(id: int, x: float, y: float, z: float)
        +despachar(deltaTime: float)
    }
    class MiniAudioBackend {
        +MiniAudioBackend()
        +~MiniAudioBackend()
        +iniciar(): bool override
        +cerrar() override
    }
    class NullAudioBackend {
        +NullAudioBackend()
        +~NullAudioBackend()
        +iniciar(): bool override
        +cerrar() override
    }
    class AudioEngine {
        -impl_: IAudioBackend
        +AudioEngine(impl: IAudioBackend)
        +~AudioEngine()
        +reproducir(ruta: string, volumen: float, bucle: bool, posicion: float*): int
        +detener(handle: int)
        +detenerTodo()
        +setVolumenMaestro(volumen: float)
        +setPosicionSonido(handle: int, x: float, y: float, z: float)
        +despachar(deltaTime: float)
    }

    IAudioBackend <|.. MiniAudioBackend
    IAudioBackend <|.. NullAudioBackend
    AudioEngine *-- IAudioBackend : owns
```

Ubicación: `src/Fisicas/`, `src/Audio/`

## 5. Scripting y Comportamientos

```mermaid
classDiagram
    class IScriptBehaviour {
        <<interface>>
        -api: const ApiScriptGameObject*
        -servicios: const ScriptServices*
        +~IScriptBehaviour()
        +onStart(owner: GameObject*)
        +onUpdate(owner: GameObject*, deltaTime: float)
        +onCollisionEnter(owner: GameObject*, propio: Collider*, otro: Collider*)
        +onCollisionStay(owner: GameObject*, propio: Collider*, otro: Collider*)
        +onCollisionExit(owner: GameObject*, propio: Collider*, otro: Collider*)
        +conectarApi(tabla: const ApiScriptGameObject*)
        +onStop(owner: GameObject*)
        +camposReflejados(): vector<DefCampo>
    }
    class BackendScript {
        <<abstract>>
        +~BackendScript()
        +lenguaje(): const char*
        +compilarYCargar(fuente: string, nombreClase: string, salida: ComportamientoCargado&, error: string&): bool
        +descargar(comportamiento: ComportamientoCargado&)
        +descargar(comportamiento: ComportamientoCargado&, owner: GameObject*)
        +llamarInicio(comportamiento: ComportamientoCargado&, owner: GameObject*)
        +llamarActualizar(comportamiento: ComportamientoCargado&, owner: GameObject*, deltaTime: float)
        +llamarDetener(comportamiento: ComportamientoCargado&, owner: GameObject*)
        +llamarContacto(comportamiento: ComportamientoCargado&, owner: GameObject*, propio: Collider*, otro: Collider*, tipo: TipoContacto)
    }
    class BackendCpp {
        +BackendCpp()
        +~BackendCpp()
        +lenguaje(): const char* override
    }
    class BackendJava {
        +BackendJava()
        +~BackendJava()
        +lenguaje(): const char* override
        +inyectar(comportamiento: ComportamientoCargado&, valores: vector<ValorCampo>): void override
        +extraer(comportamiento: ComportamientoCargado&): vector<ValorCampo> override
        +apagarJvm(): static
    }
    class ScriptRuntime {
        <<static>>
        +lenguajeDeFuente(fuente: string): static const char*
        +backendPara(lenguaje: string): static BackendScript*
        +compilarYCargar(fuente: string, nombreClase: string, salida: ComportamientoCargado&, error: string&): static bool
        +descargar(c: ComportamientoCargado&): static void
        +llamarInicio(c: ComportamientoCargado&, owner: GameObject*): static void
        +llamarActualizar(c: ComportamientoCargado&, owner: GameObject*, deltaTime: float): static void
        +llamarDetener(c: ComportamientoCargado&, owner: GameObject*): static void
        +llamarContacto(c: ComportamientoCargado&, owner: GameObject*, propio: Collider*, otro: Collider*, tipo: TipoContacto): static void
        +inyectar(c: ComportamientoCargado&, valores: vector<ValorCampo>): static void
        +extraer(c: ComportamientoCargado&): static vector<ValorCampo>
        +cambioElFuente(c: ComportamientoCargado&): static bool
        +apagarScripts(): static void
    }
    class ComportamientoCargado { }
    class ScriptGameObject { }
    class BehaviourReflection { }

    IScriptBehaviour <|.. BackendScript : uses
    BackendScript <|-- BackendCpp
    BackendScript <|-- BackendJava
    ScriptRuntime ..> BackendScript : dispatches
    ScriptRuntime --> ComportamientoCargado : manages
```

Ubicación: `src/Behaviour/`

## 6. Commands (Undo/Redo)

```mermaid
classDiagram
    class IComando {
        <<interface>>
        +~IComando()
        +ejecutar()
        +deshacer()
        +descripcion(): string
    }
    class GestorComandos {
        -comandos: vector<unique_ptr<IComando>>
        -indiceActual: int
        +GestorComandos()
        +~GestorComandos()
        +ejecutarComando(comando: unique_ptr<IComando>)
        +deshacer()
        +rehacer()
        +puedeDeshacer(): bool
        +puedeRehacer(): bool
    }
    class AgregarComponenteComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class QuitarComponenteComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class CrearObjetoComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class BorrarObjetoComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class ReparentarComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class TransformComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class EstablecerTagComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class DuplicarObjetoComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class LimpiarEscenaComando { +ejecutar() override +deshacer() override +descripcion(): string override }
    class AjustarVariableComando { +ejecutar() override +deshacer() override +descripcion(): string override }

    IComando <|.. GestorComandos : uses
    IComando <|-- AgregarComponenteComando
    IComando <|-- QuitarComponenteComando
    IComando <|-- CrearObjetoComando
    IComando <|-- BorrarObjetoComando
    IComando <|-- ReparentarComando
    IComando <|-- TransformComando
    IComando <|-- EstablecerTagComando
    IComando <|-- DuplicarObjetoComando
    IComando <|-- LimpiarEscenaComando
    IComando <|-- AjustarVariableComando
```

Ubicación: `src/Comandos/`

## 9. Ventana, Tiempo y Input

```mermaid
classDiagram
    class Ventana {
        +Ventana()
        +~Ventana()
        +crear(ancho: int, alto: int, titulo: string)
        +debeCerrarse(): bool
        +procesarEventos()
        +intercambiarBuffers()
        +obtenerContexto(): GLFWwindow*
    }
    class EngineTime {
        <<static>>
        +deltaTime(): float
        +tiempoTotal(): float
        +actualizar()
    }
    class EditorInput {
        +EditorInput()
        +~EditorInput()
        +procesarInput(window: GLFWwindow*)
        +descartarDeltaLook()
        +aplicarModoCursor(window: GLFWwindow*)
    }
    class TerminalEmulator { +TerminalEmulator() +~TerminalEmulator(); }
    class TerminalSession { +TerminalSession() +~TerminalSession(); }

    Ventana --> EngineTime : uses
    Ventana --> EditorInput : uses
```

## 10. Exportador y Proyectos

```mermaid
classDiagram
    class GameExporter {
        +GameExporter()
        +~GameExporter()
        +exportarJuego(config: ConfigJuegoExportado)
    }
    class ConfigJuegoExportado {
        -nombreEjecutable: string
        -nombreProyectoExportado: string
        -plataformaObjetivo: string
    }
    class GestorDeProyectos {
        +GestorDeProyectos()
        +~GestorDeProyectos()
        +crearProyecto(nombre: string)
        +eliminarProyecto(nombre: string)
        +listarProyectos(): vector<string>
    }
    class EstadoExportacion { +EstadoExportacion() +~EstadoExportacion(); }

    GameExporter --> ConfigJuegoExportado : uses
    GestorDeProyectos --> ConfigJuegoExportado : uses
```

## 11. Estructuras de Datos

```mermaid
classDiagram
    class ListaDE<T> {
        <<template>>
        -cabeza: DNodo<T>*
        -cola: DNodo<T>*
        -tamano: int
        +ListaDE()
        +~ListaDE()
        +isEmpty(): bool
        +first(): Position<T>*
        +last(): Position<T>*
        +next(pos: Position<T>*): Position<T>*
        +getElement(): T
    }
    class ArbolEnlazado<T> {
        <<template>>
        +ArbolEnlazado()
        +~ArbolEnlazado()
    }
    class DNodo<T> {
        <<template>>
        -elemento: T
        -anterior: DNodo<T>*
        -siguiente: DNodo<T>*
    }
    class PriorityListaDE<T> {
        <<template>>
        +PriorityListaDE()
        +~PriorityListaDE()
    }
    class Heap<T> {
        <<template>>
        +Heap()
        +~Heap()
    }
    class MaxHeap<T> { +MaxHeap() +~MaxHeap(); }
    class MinHeap<T> { +MinHeap() +~MinHeap(); }
    class ArbolBinarioEnlazado<T> {
        <<template>>
        +ArbolBinarioEnlazado()
        +~ArbolBinarioEnlazado()
    }

    DNodo<T> <|.. ListaDE<T> : uses
    MaxHeap<T> ..|> Heap<T>
    MinHeap<T> ..|> Heap<T>
```

Ubicación: `src/Estructuras/```

Ubicación: `src/States/`, `src/GUIManager/`, `src/Configuracion/`
