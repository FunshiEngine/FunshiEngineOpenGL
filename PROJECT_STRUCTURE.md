# Estructura y relaciones del proyecto

## 1. Descripción general

`FunshiEngineGL` es un editor/motor gráfico 3D en C++17. El ejecutable combina:

- Ventana y contexto OpenGL 3.3 **core** mediante GLFW.
- Renderizado con un único pipeline moderno: los modelos usan `MeshRenderer`
  (VBO/VAO + shaders) y las líneas un batch en GPU con shader de ancho en
  píxeles. No queda modo inmediato (`glBegin/glEnd`), ni matrices de
  compatibilidad, ni materiales ni luces de estado fijo. La grilla es un
  componente (`Grid`) en una pasada independiente que ya no usa display lists;
  los marcadores de luz/cámara y los gizmos de los colliders se dibujan con el
  mismo pipeline de líneas.
- Interfaz de editor con Dear ImGui y gizmos con ImGuizmo.
- Jerarquía de entidades basada en árboles enlazados propios.
- Simulación física mediante Bullet Physics detrás de una fachada desacoplada.
- Carga de modelos 3D mediante Assimp.
- Serialización binaria de escenas en preorden (jerarquía completa).
- EventBus para notificaciones desacopladas entre subsistemas.
- Máquina de estados explícita (`ApplicationStateMachine`).
- Explorador de archivos del proyecto con fachada propia y vigilancia de cambios.
- Configuración del editor persistida en JSON (`EditorConfig`).
- Caché compartida de assets (meshes CPU e imágenes) mediante Flyweight
  (`AssetManager` / `TextureManager`).
- Scripts dinámicos (`.so`/`.dll`) con reflexión tipo `SerializeField` y hot
  reload (`ScriptRuntime` + backends C++/Java) sobre `IScriptBehaviour`.
- Estructuras de datos genéricas propias y jerarquía de excepciones.

El punto de entrada es `FunshiEngineGL/src/main.cpp`. La configuración de compilación
está en `FunshiEngineGL/CMakeLists.txt`, que es el único build soportado: CMake
genera la solución de Visual Studio dentro del directorio de build que elijas
(por ejemplo `FunshiEngineGL/build-win/FunshiEngineGL.sln`), así que no hay
proyectos de Visual Studio mantenidos a mano en el repositorio. Las pruebas
headless viven en `tests/` y el CI en
`.github/workflows/ci.yml`.

---

## 2. Árbol de archivos relevante

Se omiten los directorios generados por el build (`build/`, `build_temp/`), los
archivos temporales de editor (`*.swp`, `*.swo`) y el contenido interno de las
dependencias de terceros.

```text
FunshiEngineGL/                          ← raíz del repo
├── README.md                            ← visión general, build, controles y pendientes
├── PROJECT_STRUCTURE.md                 ← este documento
├── MANUAL_DE_USO.md                     ← manual de usuario (editor + scripting C++/Java)
├── CAMARAS_VISTAS_PREVIAS.md            ← Fase 2: cámaras componente + vistas previas
├── ARQUITECTURA_ESTADOS_GUI.md          ← estados/menú/GUI internas (diseño + Fases 1-3)
├── MANUAL_DE_USO.md                     ← manual de usuario (editor + scripting C++/Java)
├── FLUJO_DE_RAMAS.md                    ← convención de ramas (develop/test/staging/release)
├── .github/workflows/ci.yml             ← CI: engine en Ubuntu + pruebas en Linux/Win/macOS
├── .github/workflows/release.yml        ← instaladores Qt IFW (.run) e Inno (.exe) por tag
├── .github/workflows/windows-release.yml← build+release específico de Windows
├── .gitignore / .gitattributes
├── tests/
│   ├── FileManagerTests.cpp             ← headless del explorador de archivos
│   ├── EditorConfigTests.cpp            ← headless de la configuración JSON
│   ├── EditorEventBusTests.cpp          ← headless del canal tipado de GUI (Fase 2)
│   ├── MenuModelTests.cpp               ← headless del modelo del menú (Fase 3)
│   ├── AssetManagerTests.cpp            ← caché Flyweight de meshes (AssetPath/Mesh)
│   ├── TextureManagerTests.cpp          ← caché Flyweight de imágenes (TextureManager)
│   ├── EstructurasTests.cpp             ← listas, árboles, heaps y ordenamiento propios
│   ├── ScriptsTests.cpp                 ← reflexión SerializeField + round-trip binario
│   ├── ScriptsRuntimeTests.cpp          ← BackendCpp end-to-end (compila y dlopen un .so)
│   ├── ScriptsJavaTests.cpp             ← BackendJava end-to-end (solo con FUNSHI_JAVA)
│   ├── AudioEngineTests.cpp             ← AudioEngine/AudioClipsManager con NullAudioBackend
│   └── UserInterfaceTests.cpp           ← modelo del Creador de interfaces (round-trip JSON)
└── FunshiEngineGL/                      ← proyecto CMake principal
    ├── CMakeLists.txt                   ← GLOB de fuentes, dependencias, sanitizers,
    │                                      pruebas (CTest) y opción BUILD_ENGINE
    ├── Imagenes/                        ← íconos del explorador (cpp, cubo, file, folder, hpp)
    ├── ImGuizmo/                        ← dependencia integrada (ImGuizmo.cpp/.h, etc.)
    ├── External/nlohmann/json.hpp       ← nlohmann/json vendoriado (EditorConfig)
    └── src/
        ├── main.cpp                     ← composition root: ventanas, callbacks, bucle, config
        ├── EngineTime.h / EngineTime.cpp← delta time y limitador de FPS
        ├── Ventana.h / Ventana.cpp      ← inicialización GLFW y contexto 3.3 core
        ├── Assets/                      ← caché Flyweight compartida (meshes e imágenes)
        │   ├── AssetManager.h/.cpp      ← registro de AssetPath→Mesh (loader inyectable)
        │   ├── TextureManager.h/.cpp    ← registro de AssetPath→Image (loader inyectable)
        │   ├── AssetPath.h / Image.h    ← rutas normalizadas y metadatos de imagen
        │   ├── AssetException.h / TextureException.h ← errores de carga con mensaje y ruta
        │   ├── Mesh.h/.cpp              ← geometría CPU (vértices, normales, índices)
        │   ├── AssimpMeshLoader.*       ← loader Assimp→Mesh
        │   └── StbImageLoader.*         ← loader stb_image→Image (solo engine)
        ├── Audio/                       ← audio del motor (backend inyectable)
        │   ├── AudioEngine.h/.cpp       ← fachada thread-safe (cola de comandos + hilo)
        │   ├── AudioClipsManager.h/.cpp ← descubre clips en Sonidos/ y registra por nombre
        │   ├── IAudioBackend.h          ← contrato Strategy del backend de audio
        │   ├── MiniAudioBackend.*       ← backend concreto (miniaudio vendoriado)
        │   ├── NullAudioBackend.h       ← backend nulo para pruebas headless
        │   └── AudioClip.h              ← handle/metadata del clip
        ├── Behaviour/
        │   ├── IScriptBehaviour.h       ← interfaz de scripts (onStart/onUpdate/onStop, campos)
        │   ├── ScriptGameObject.*       ← tabla de acceso al GameObject inyectada al script
        │   ├── ScriptRuntime.*          ← registro de backends, compilación y hot reload
        │   ├── ComportamientoCargado.h  ← instancia compilada (módulo + campos)
        │   ├── Backends/
        │   │   ├── BackendScript.h      ← interfaz de backend (C++/Java)
        │   │   ├── BackendCpp.*         ← compila .cpp→.so/.dll y lo carga (dlopen)
        │   │   └── BackendJava.*        ← Java vía JNI/JVM dinámico (solo con FUNSHI_JAVA)
        │   └── Reflection/
        │       └── BehaviourReflection.* ← reflexión, macros SerializeField y serialización
        ├── Configuracion/
        │   ├── Apariencia.h             ← perfil de apariencia (tema/acento/fondo/B-N) + utilidades
        │   ├── EditorConfig.h/.cpp      ← fachada de la configuración (datos() + cargar*/guardar*, guardado
        │   │                              diferido) y punto de entrada estable del CRUD de proyectos, que
        │   │                              DELEGA en ProjectManager (una sola implementación)
        │   ├── ConfigPersistence.h/.cpp ← JSON puro de la config: general + proyecto (escritura atómica)
        │   ├── ProjectPaths.h/.cpp      ← rutas canónicas del motor (una sola fuente de verdad)
        │   └── ProjectManager.h/.cpp    ← único dueño del ciclo de vida de proyectos (CRUD, migraciones
        │                                  de estructura antigua, fallbacks de copia); delega rutas en ProjectPaths
        ├── Entity/
        │   ├── Entity.h                 ← base: lista de componentes, Transform, serialización
        │   └── Entity.cpp
        ├── Estructuras/                 ← contenedores genéricos, header-only
        │   ├── Comparable.h / Ordenadora.h
        │   ├── ListasEnlazadas/
        │   │   ├── ListasDoblementeEnlazada/ListaDE.h
        │   │   ├── ListasConPrioridad/PriorityListaDE.h
        │   │   └── PositionList.h
        │   ├── MetodosDeOrdenamiento/ListMergeSort.h
        │   ├── Nodos/DNodo.h + TNodo.h + BNodo.h  ← nodos de listas, árbol n-ario y binario
        │   ├── Position/Position.h
        │   └── Trees/
        │       ├── Tree.h
        │       ├── Heap/Heap.h + MinHeap.h + MaxHeap.h  ← colas de prioridad (vector)
        │       └── ArbolesEnlazados/
        │           ├── ArbolEnlazado.h  ← árbol n-ario: jerarquía de GameObjects
        │           └── ArbolBinarioEnlazado.h ← binario con addLeft/addRight y preorden
        ├── Events/
        │   ├── EventBus.h/.cpp          ← pub/sub tipado de escena con token de suscripción
        │   └── EditorEventBus.h/.cpp    ← canal tipado de GUI interna (ventanas; Fase 2,
        │                                  dueño: GUIManager)
        ├── ExcepcionesCPP/              ← Throwable, RuntimeException, excepciones de
        │                                  contenedores (ExcepcionesEstructuras/)
        ├── Fisicas/
        │   ├── IPhysicsBackend.h        ← contrato Strategy del backend de física
        │   ├── PhysicsEngine.h/.cpp     ← fachada PIMPL; el header no expone Bullet
        │   └── BulletPhysicsAdapter.h/.cpp ← adaptador concreto de Bullet (RAII)
        ├── FileManager/
        │   ├── FileManager.h/.cpp       ← fachada del explorador: modelo + operaciones de dominio
        │   │                              Y toda la E/S nativa (diálogos, abrir con la app del
        │   │                              sistema, listado de directorio, plantillas de scripts)
        │   ├── FileSelection.h          ← estado de navegación compartido entre vistas
        │   └── FileSystemWatcher.h/.cpp ← vigilancia de cambios externos (inotify)
        ├── GestorDeArchivos/            ← Binario (streams binarios), File, Carpeta,
        │                                  GestorDeArchivos (exploración del filesystem)
        ├── GUI/
        │   ├── GeneralUserInterface.h/.cpp ← interfaz base de paneles ImGui
        │   ├── WindowNames.h
        │   ├── DockSpaceGUI/               ← dock principal del editor
        │   ├── Estado/
        │   │   └── StatusBarInterface.h/.cpp ← ventana "Estado": toolchain (Compilador/javac/
        │   │                                  libjvm) y estado de scripts (compilando/cargado/error)
        │   ├── Tema/
        │   │   └── TemaEditor.h/.cpp      ← aplica el perfil Apariencia al estilo ImGui en vivo
        │   │                                  (tema claro/oscuro, acento RGB en TODOS los roles de ImGui
        │   │                                  y grises azulados de fábrica a gris neutro; ver tests/TemaEditorTests.cpp)
        │   ├── FileManagerGUI/             ← TreeFilesInterface + ContentFolderInterface
        │   │                                  (vistas del explorador; conversan con FileManager)
        │   ├── MenusGUI/                   ← paquete del menú de inicio (MVP); ver su README.md
        │   │   ├── MenuModel.h/.cpp        ← lógica pura sin ImGui/GLFW
        │   │   ├── MenuView.h/.cpp         ← dibujo ImGui
        │   │   ├── StartMenuPresenter.h/.cpp ← puente motor ↔ modelo
        │   │   └── MenuGUI.h/.cpp          ← fachada pública del paquete
        │   ├── ObjetosGUI/
        │   │   ├── SettingsObjectInterface.h/.cpp ← inspector: crea Settings* por componente
        │   │   ├── SettingsComponent.h
        │   │   ├── Transform/SettingsTransform.h/.cpp ← transform + checkbox "Gizmo activo"
        │   │   ├── Color/  Model/  Script/
        │   │   ├── Material/SettingsMaterial.*  Light/SettingsLight.*
        │   │   ├── Camera/SettingsCamera.*       ← FOV, planos, velocidad, vista previa
        │   │   ├── Grid/SettingsGrid.*           ← visible/color/tamaño/separación de la grilla
        │   │   ├── AudioSource/SettingsAudioSource.* ← dropdown de clip (Sonidos/), volumen, loop
        │   │   ├── Interface/SettingsInterface.* ← dropdown de asset de interfaz (Interfaces/)
        │   │   ├── RigidBody/SettingsRigidBody.*
        │   │   └── Colliders/ (Esfera, Cubo, Malla) ← sync transform/shape con física
        │   ├── CreadorUI/                        ← Creador de interfaces (editor de HUD;
        │   │                                      UserInterfaceCustom, JSON en Interfaces/)
        │   └── SceneGUI/
        │       ├── SceneSelectedInterface.h/.cpp  ← jerarquía y selección; usa EditorController
        │       ├── SceneObjectTree.h/.cpp         ← árbol de objetos (con drag & drop)
        │       └── SceneMenuBarInterface.h/.cpp   ← barra Play/Stop/acciones de escena (flag `start`)
        ├── GUIManager/
        │   └── GUIManager.h/.cpp         ← fábrica y registro de ventanas; posee FileManager
        ├── Herramientas/
        │   ├── TypeUtils.h/.cpp          ← nombres legibles de tipos
        │   ├── PathUtils.h               ← PATH_SEP multiplataforma compartido
        │   ├── MaterialPresets.h         ← presets de Material como datos (reemplaza a materiales.h)
        │   ├── TreeGUI/TreeGUI.h         ← widget de árbol genérico para ImGui
        │   └── IconosGUI/                ← carga de íconos con stb_image
        ├── Iluminacion/
        │   └── LightSystem.h/.cpp        ← dueño del estado GL de luces (GL_LIGHT0..7) por frame
        ├── Input/
        │   └── EditorInput.h/.cpp       ← callbacks GLFW de teclado/mouse + máquina de
        │                                  movimiento de cámara (extraídas de main.cpp)
        ├── ImGui/                        ← Dear ImGui v1.x integrado (+ backends glfw/opengl3)
        ├── Matematicas/
        │   └── StructVec3.h/.cpp         ← vec3 propio
        ├── Rendering/
        │   ├── MeshGPU.h/.cpp            ← malla residente en GPU (buffers VBO/VAO)
        │   ├── MeshRenderer.h/.cpp       ← dibuja MeshGPU con shader program
        │   ├── LineBuilder.h/.cpp        ← geometría CPU de líneas (cada segmento
        │   │                                expandido a un quad; sin OpenGL)
        │   ├── LineBatch.h/.cpp          ← batch de líneas en GPU (VAO+VBO, RAII)
        │   ├── LineRenderer.h/.cpp       ← shader de líneas gruesas + batch; fija las
        │   │                                matrices y el viewport de la pasada actual
        │   │                                (líneas de la grilla, marcadores y gizmos)
         │   ├── GuiaEje.h/.cpp            ← geometría CPU de la guía de eje (X/Y/Z) del
         │   │                                objeto seleccionado: origen + dirección
         │   │                                unitaria, recorte analítico al horizonte y
         │   │                                difuminado por vértice; solo CPU, sin OpenGL

        │   ├── TextureGL.h/.cpp          ← textura OpenGL desde Image
        │   ├── RenderTarget.h/.cpp       ← render a textura (FBO) para vistas previas de cámara
        │   ├── GLFuncs.h                ← punteros de función OpenGL 3.3 core (glad-style)
        │   ├── Backend/                  ← IRenderBackend + OpenGL3Backend (única capa con GL)
        │   └── Shaders/
        │       ├── ShaderProgram.h/.cpp  ← compilación/link de shaders + ShaderSources.h
        │       └── ShaderException.h
        ├── Objetos/
        │   ├── GameObject.h/.cpp         ← id, nombre, estado, update, serialización binaria
        │   ├── GameObjectFactory.h/.cpp
        │   ├── SimpleObject.h            ← GameObject sin geometría (Transform/Grid/Light...)
        │   ├── Modelos3D.h/.cpp          ← carga Assimp y datos de la malla (el dibujado
        │   │                                es de SceneRenderer, vía MeshRenderer)
        │   └── Componentes/
        │       ├── Component.h           ← interfaz base polimórfica (serialize/deserialize)
        │       ├── ComponentFactory.h/.cpp ← creación por nombre (GUI y deserialización)
        │       ├── Transform.h/.cpp      ← local/global con GLM; anti-NaN; flag gizmoHabilitado
        │       ├── CameraComponent.h/.cpp ← cámara componente: vista, FPS, flag de vista previa
        │       ├── Material.h/.cpp       ← AMBIENT/DIFFUSE/SPECULAR/EMISSION/SHININESS
        │       ├── Light.h/.cpp          ← luz puntual serializable
        │       ├── Grid.h/.cpp           ← grilla del suelo: pasada independiente, visible/color/
        │       │                            tamaño/separación + modo blanco y negro
        │       ├── Color.h / Model.h / Script.h
        │       ├── RigidBody/RigidBody.h/.cpp ← cuerpo Bullet sincronizado (RAII)
        │       └── Colliders/
        │           ├── Collider.h/.cpp   ← base abstracta; radio; gizmo del collider (GizmoTarget)
        │           │                        con wireframe por el batch de líneas
        │           ├── EsfereCollider.*  ← btSphereShape
        │           ├── CubeCollider.*    ← btBoxShape (half extents = radio)
        │           └── MallaCollider.*   ← btConvexHullShape a partir de la malla
        ├── Proyectos/
        │   └── GestorDeProyectos.h/.cpp  ← ciclo de vida del proyecto activo (entrar,
        │                                  guardar estado y escena Ctrl+S, renombrar, eliminar,
        │                                  exportar, imgui.ini); extraído de main.cpp
        ├── Comandos/
        │   ├── IComando.h              ← interfaz Command: ejecutar(), deshacer(), descripcion()
        │   ├── GestorComandos.h/.cpp   ← pilas undo/redo (máx 50), ejecuta/deshace/rehace
        │   ├── CrearObjetoComando.h/.cpp
        │   ├── BorrarObjetoComando.h/.cpp
        │   ├── ReparentarComando.h/.cpp
        │   ├── TransformComando.h/.cpp
        │   ├── AgregarComponenteComando.h/.cpp
        │   ├── QuitarComponenteComando.h/.cpp
        │   └── LimpiarEscenaComando.h/.cpp
        └── Scenes/
            ├── GameScene.h/.cpp          ← coordinador del frame: render, GUI, física, gizmo,
            │                                previews y pasada de la grilla
            ├── GizmoController.h/.cpp   ← gizmo ImGuizmo + picking (desde GameScene): estado
            │                                (operación/global/listo), arrastre con
            │                                TransformComando (historial) y congelación de
            │                                hijos; interacción vía ImGuizmo + raycast AABB
            ├── SceneRegistry.h/.cpp      ← ownership único (unique_ptr) + árbol + vista lineal
            ├── EditorController.h/.cpp   ← mutaciones + GizmoTarget + registro de física
            ├── SceneSerializer.h/.cpp    ← save/load binario preorden con marcadores =>/<=
            ├── RutasReescritura.h/.cpp   ← reescribe referencias (model/textura/script) al
            │                                mover/renombrar assets en el explorador
            ├── ManifiestoAssets.h/.cpp   ← manifiesto SceneAssets.json (add-on del .db):
            │                                guarda/carga rutas de asset por objeto con
            │                                precedencia sobre el binario (Ctrl+S)
            ├── ManifiestoAssetsCore.h/.cpp ← núcleo puro headless (JSON + relativizar/
            │                                absolutizar + precedencia); tests propios
        └── States/
            ├── ApplicationStateMachine.h/.cpp ← MainMenu/Editing/Playing/Exiting
            └── OrquestadorEstadoGUI.h/.cpp    ← reglas de transición menú↔editor y de
                                                   simulación: la "función de marco" que fija
                                                   el comportamiento ante F5(Play)/F6(Pausa)/
                                                   F7(Stop) y demás teclas (Escape, Iniciar
                                                   Estudio); headless, con tests propios
                                                   (orquestador-estado-tests)
```

---

## 3. Flujo de ejecución

```text
main.cpp
  ├── Ventana (GLFW init: contexto 3.3 core + carga dura de GLFuncs)
  ├── ImGui init (backends glfw + opengl3; imgui.ini gestionado por el gestor)
  ├── GUIManager (crea el menú y las ventanas; posee FileManager)
  ├── GameScene(guiManager)
  │   ├── SceneRegistry            ← ownership de objetos
  │   ├── PhysicsEngine (PIMPL)    ← mundo Bullet vía BulletPhysicsAdapter
  │   ├── EditorController         ← mutaciones, gizmo, registro de física
  │   ├── SceneSerializer          ← persistencia
  │   └── EventBus                 ← notificaciones
  ├── GestorDeProyectos            ← ciclo de vida del proyecto activo (entrar/guardar/
  │                                  renombrar/eliminar/exportar + imgui.ini)
  ├── EditorConfig                 ← carga JSON y aplica a menú/GUI/escena
  ├── ApplicationStateMachine      ← MainMenu / Editing / Playing / Exiting
  └── bucle principal
      ├── glfwPollEvents
      ├── EngineTime::update (deltaTime)
      ├── ImGui::NewFrame
      ├── refleja el estado del menú en la fachada MenuGUI (guardia de cambio)
      ├── si Playing → GameScene::update(dt) = física (start==true, F5) + scripts, con
      │   F6 pausando fisica/scripts sin salir de play y F7 cortando (Playing → Editing)
      ├── pasada de la grilla (batch de líneas + shader de ancho en píxeles; color según apariencia)
      ├── dibujarGameObjects (MeshRenderer VBO/VAO+shader; único pipeline)
      ├── gizmo ImGuizmo sobre el objetivo activo (objeto o collider)
      ├── GUI() de GameScene (paneles) + vistas previas de cámaras (FBO)
      ├── GestorDeProyectos::sincronizar/eliminar (cambios de proyecto desde el menú)
      ├── ImGui::Render + swap buffers
      └── al salir: GestorDeProyectos::guardarProyectoCompleto (escena + config)
```

`main.cpp` es el composition root; las callbacks de teclado y ratón de GLFW
viven en el módulo `src/Input/EditorInput` (`EditorInput::registrarCallbacks`,
instancia única creada por `main`), que las traduce a acciones del editor y
mantiene la máquina de estado del movimiento de cámara. El estado del menú lo
gobierna el modelo del paquete `MenuGUI`,
sincronizado por frame desde `ApplicationStateMachine`. `GameScene` configura
internamente `GUIManager`, `SceneRegistry`, `EditorController`, `SceneSerializer` y
`EventBus`. El ciclo de vida del proyecto activo (determinar el proyecto al
arrancar, entrar/guardar/renombrar/eliminar, exportar y el `imgui.ini` del
proyecto) vive en `src/Proyectos/GestorDeProyectos`, que inyecta
`EditorConfig`, `GameScene`, `GUIManager` y la fachada `MenuGUI`; `main` queda
solo como orquestador de arranque y bucle.

---

## 4. Relaciones entre los módulos

### Aplicación y escena

- `main.cpp` construye los objetos principales y conecta sus referencias. Es el único composition root.
- `GestorDeProyectos` es la fachada del ciclo de vida del proyecto activo
  (arranque, entrar, guardar estado y escena Ctrl+S, renombrar, eliminar,
  exportar y el `imgui.ini` del proyecto). Se le inyectan `EditorConfig`,
  `GameScene`, `GUIManager` y la fachada `MenuGUI`; el `ImGuiIO` se fija
  después de `ImGui::CreateContext`. Las decisiones de "cuál proyecto" las lee
  de la fachada del menú (misma separación que la escena con la GUI).
- `GameScene` posee mediante `unique_ptr`: `SceneRegistry`, `PhysicsEngine`,
  `EditorController` y `SceneSerializer`. Recibe el `GUIManager` por inyección.
- `SceneRegistry` es el único propietario de los `GameObject`, mediante
  `std::vector<std::unique_ptr<GameObject>>`. El árbol y la lista son vistas no propietarias.
- `EditorController` recibe el registro, la física y el EventBus por inyección.
  Centraliza crear, eliminar, reparentar y modificar componentes; también es el
  **registro central de física** (alta/baja de `RigidBody`) y el dueño del
  `GizmoTarget` (objeto u offset de collider manipulado por el gizmo).
  Publica eventos en `EventBus` después de cada operación exitosa.
- `SceneRegistry::reparent` valida el movimiento: rechaza la raíz, el auto-reparento
  y cualquier ancestro del nuevo padre (no se pueden crear ciclos).
- `EventBus` implementa suscripción tipada mediante tokens (`size_t`). Soporta
  `ObjectCreated`, `ObjectDeleted`, `ObjectReparented`, `ComponentChanged`,
  `SceneCleared` y `ObjectSelected`. No es global: vive dentro de `GameScene`.
- `LightSystem` es el dueño del estado GL de luces: cada frame escanea los objetos,
  toma los componentes `Light` y parametriza los slots `GL_LIGHT0..7`. No queda
  lógica de luz en el bucle ni en los componentes.
- `ApplicationStateMachine` modela los estados `MainMenu`, `Editing`, `Playing` y
  `Exiting`. Las transiciones se deciden en `OrquestadorEstadoGUI` (función de
  marco: Escape → menú, Iniciar Estudio → editor, F5 → Play, F7 → Stop) y solo
  se aplican sobre la máquina desde ahí; conviven con flags de UI legados
  (`menuActivo`, `start`) con roles documentados — `start` lo manejan a la vez el
  botón Activar/Detener del menú de escena y el reflejo de F5/F7.
- `EditorController` posee un `GestorComandos` que envuelve cada mutación
  (crear/borrar/reparentar, cambios de transform, agregar/quitar componentes,
  limpiar escena) en un `IComando`. Las operaciones de la GUI van por el
  gestor, nunca directamente al `SceneRegistry`, de modo que Ctrl+Z/Ctrl+Y
  funcionan de forma transversal. La pila mantiene un máximo de 50 comandos;
  cada nueva acción invalida la pila de redo. La raíz de la escena (id=0)
  está excluida de delete/clear por diseño.

### Entidades, objetos y componentes

- `Entity` contiene una `ListaDE<Component*>` y un `Transform` base. Define la interfaz abstracta de serialización (métodos `serialize*`/`deserialize*`).
- `GameObject` extiende `Entity` e implementa `Comparable<GameObject>`. Añade:
  id entero, nombre (`inputName[25]`), color auxiliar (`auxColor`), estado (`bool`),
  ciclo `update(dt)` y la lógica de serialización binaria concreta.
- `Modelos3D` extiende `GameObject`. Carga geometría (vértices, normales, índices)
  mediante Assimp y serializa adicionalmente la ruta del archivo del modelo. No
  tiene método de dibujado: la entidad no conoce la capa de Rendering, y es
  `SceneRenderer` quien la pinta vía `MeshRenderer`.
- Los componentes concretos son: `Transform`, `Color`, `Model`, `Material`, `Light`,
  `AudioSource`, `InterfaceComponent`, `Grid`, `Script`, `EsfereCollider`,
  `CubeCollider`, `MallaCollider` y `RigidBody`. Todos heredan de `Component`
  y serializan sus datos binarios.
- `ComponentFactory` centraliza la creación por nombre de tipo tanto desde la GUI
  como durante la deserialización (`"CameraComponent"` acepta el alias `"Camera"`).
- `Component::onLoaded(GameObject&)` es el hook polimórfico de post-carga:
  `deserializeEntityComponents` aplica `loadComponent()` y luego el hook, sin
  despachar por nombre de tipo. `Color` lo usa para reflejar su valor en
  `GameObject::auxColor` (el buffer del inspector); cualquier componente nuevo
  con dependencias post-carga lo implementa en vez de abrir el objetos de carga.
- `Transform` provee transformaciones locales y globales usando GLM; expone
  descomposición de matrices para ImGuizmo y blinda su cadena contra NaN
  (valores no finitos no entran al estado ni al render).
- La cadena física: los colliders crean la `btCollisionShape` (reconstruida si
  cambia el radio), `RigidBody` la conecta al mundo de `PhysicsEngine`, y el sync
  de transformaciones entre objeto, collider y cuerpo se hace con matrices
  compuestas (global T del dueño + offset del collider), no con deltas sueltos.

### Jerarquía y contenedores

- `ArbolEnlazado<GameObject*>` representa la jerarquía padre-hijo dentro de
  `SceneRegistry`. No libera los objetos apuntados (la ownership está en el vector).
- `ListaDE<GameObject*>` es una vista lineal reconstruible de `SceneRegistry`,
  usada por GUI, render, iluminación y actualización. Tampoco es propietaria.
- `TNodo` contiene sus hijos mediante una `ListaDE<TNodo<E>*>`;
  `DNodo` implementa los nodos de listas doblemente enlazadas y `BNodo` los del
  binario (con destructor recursivo de subárbol).
- Las estructuras genéricas son header-only para permitir la instanciación de plantillas.
- Estado de las estructuras (ver `tests/EstructurasTests.cpp`):
  - **Usadas por el motor**: `ListaDE`, `PriorityListaDE`, `ArbolEnlazado`.
    ListaDE y ArbolEnlazado fueron corregidas (inicialización de nodos,
    `addFirst`, liberación de nodos en `remove` y destructores, orden de
    borrado en `deleteRoot`/`deleteInternalNode`/`deleteExternalNode`) y su
    comportamiento está cubierto por la suite.
  - **Disponibles y probadas, sin consumidores todavía**: `MinHeap`, `MaxHeap`,
    `ListMergeSort` y `ArbolBinarioEnlazado`. Fueron implementadas (antes eran
    stubs vacíos que el README daba por hechos) y validadas con las mismas
    pruebas, aptas para su uso por el motor cuando hagan falta.

### GUI

- `GeneralUserInterface` define la interfaz común (`initGUI`, `contentGUI`, `printGUI`, `endGUI`).
- `GUIManager` actúa como fábrica y registro centralizado de todas las ventanas
  del editor. Expone métodos tipados (`getMenuGUI`, `getTreeFilesGUI`, etc.),
  guarda/restaura el estado abierto/cerrado de las ventanas y **posee el
  `FileManager`** del proyecto.
- El explorador de archivos es una arquitectura de tres piezas: `FileManager`
  (fachada dueña del modelo `GestorDeArchivos` y de las operaciones de dominio
  —crear/renombrar/eliminar/copiar— **más toda la E/S nativa del sistema**:
  diálogos de selección de carpeta/archivo, abrir con la app predeterminada,
  listado de directorio con symlink-safe y plantillas de scripts C++/Java),
  `FileSelection` (estado de navegación compartido) y las vistas
  `TreeFilesInterface`/`ContentFolderInterface`, que solo conversan con la
  fachada (sin `system()`/`popen()`/`ShellExecute*` ni `directory_iterator`
  propios).
  `FileSystemWatcher` avisa de cambios externos (inotify) para re-escanear.
- `SceneSelectedInterface` observa la escena inyectada y delega las
  operaciones de edición a `EditorController`. Usa `GameObjectFactory` para crear objetos.
- `SceneObjectTree` dibuja el árbol de objetos (con drag & drop para reparentar).
- `SettingsObjectInterface` inspecciona un `GameObject` seleccionado y crea/actualiza
  los paneles `Settings*` específicos de cada componente presente. El checkbox
  **"Gizmo activo"** de `SettingsTransform` enciende/apaga el gizmo de ese
  transform (del objeto o del offset del collider) sin deseleccionar.
- `SceneMenuBarInterface` gestiona la barra de menú de escena y comunica el
  estado Play/Stop mediante un `bool*` (`start`) que consume `GameScene`.
- `MenuGUI` es la fachada del paquete `MenusGUI` (menú principal): `MenuModel`
  (lógica pura) + `MenuView` (ImGui) + `StartMenuPresenter` (puente motor).
  Ver `src/GUI/MenusGUI/README.md`.

### Persistencia

- `SceneSerializer` es el único responsable del archivo `BBDDObjetos.txt` y de los
  `.db` individuales. Guarda en preorden: escribe el binario de cada objeto y
  registra la ruta; inserta `=>` para abrir un bloque de hijos y `<=` para cerrarlo.
- La carga reconstruye la escena **de forma recursiva** (`loadPreOrder` con
  look-ahead de marcadores), restaurando padres e hijos; inserta cada objeto
  directamente en `SceneRegistry` vía `EditorController`.
- `Binario` encapsula los streams binarios usados por las entidades.
- `GameObject::saveEntity/loadEntity` coordina la serialización binaria propia
  (atributos globales, locales, componentes).
- La configuración del editor se persiste en JSON con **una sola fuente de
  verdad**: `Configuracion/ConfigPersistence.{h,cpp}` (JSON puro, sin estado)
  sobre `Configuracion/ProjectPaths.{h,cpp}` (rutas), con
  `EditorConfig.{h,cpp}` como fachada estable que expone `datos()` y
  `cargar*/guardar*` a main, escenas y tests. El **ciclo de vida de los
  proyectos** (`asegurarEstructuraProyecto`, `crearProyectoPorDefecto`,
  `renombrarProyecto`, `eliminarProyecto`) no se implementa en EditorConfig:
  son **delegaciones a `ProjectManager`**, único dueño del CRUD, las
  migraciones de estructura antigua y los fallbacks de copia entre
  dispositivos. La orquestación de todo el flujo de proyectos sobre esta
  fachada (qué hará al arrancar, entrar, guardar, renombrar, eliminar,
  exportar y cuál es el `imgui.ini` vigente) vive en
  `Proyectos/GestorDeProyectos` (extraído de `main.cpp`). Guarda: menú (proyecto, idioma,
  sensibilidad de cámara), gizmo, ventana de cámaras, ventanas (estado
  abierto/cerrado de GUIManager), cámara activa por id y perfil de apariencia,
  en dos archivos junto al binario (Linux y Windows):
  `<directorioEjecutable>/MotorGrafico/Configuraciones/Configuracion.json`
  (general) y `Proyects/<proyecto>/Memory/ConfiguracionProyecto.json`
  (por proyecto). Tolerante a archivos ausentes o corruptos: los defaults
  viven en `EditorConfig.h`/`ConfigPersistence.h`.
  - **Escritura atómica**: `ConfigPersistence::escribirJson` escribe a
    `<archivo>.tmp` y renombra encima; un corte a mitad de escritura no deja
    el JSON cortado ni temporales colgados.
  - **Guardado diferido de la general**: `EditorConfig::solicitarGuardadoGeneral`
    encola los cambios en vivo de Opciones y `volcarGuardadoGeneral()` (llamado
    por main una vez por frame) escribe como máximo una vez cada
    `kIntervaloEscritura` (250 ms); `guardarGeneral()` (Ctrl+S, salida, reset)
    vuelca el pendiente sin esperar.
  - El layout `imgui.ini` también se guarda junto al proyecto (no en el CWD).
- Limitación conocida: la serialización binaria no tiene versionado ni validación
  de tamaños; un cambio en la estructura de atributos invalida escenas guardadas.

### Scripts dinámicos

- `Script` (componente) orquesta el ciclo de vida del módulo y guarda los valores de
  los campos (`SerializeField`) en la serialización binaria de la escena.
- `ScriptRuntime` registra los backends por extensión, compila/carga el fuente y
  aplica hot reload comparando la fecha de modificación (conserva y reinyecta los
  valores por nombre de campo).
- `IScriptBehaviour` define la interfaz: `onStart`/`onUpdate`/`onStop` y
  `camposReflejados()`; el motor inyecta la tabla `MotorScript::ApiScriptGameObject`
  (nombre, transform completo con getters de rotacion/escala, log) y la tabla
  `MotorScript::ScriptServices` (audio, busqueda de objetos por nombre y
  consulta de teclado via `InputScripts`, inyectadas por `GameScene` al entrar
  en Play) para que el script no enlace contra el motor. Ambas tablas siguen
  versionado APPEND-ONLY con campo `version` final para guardas en runtime.
- `BehaviourReflection` implementa la reflexión por macros (`REFLECT_INICIO`,
  `CAMPO`, `ARRAY`, `GRUPO`, `GRUPOS`, `FIN`), la conversión de valores tipados y la
  serialización binaria autodescriptiva de los campos.
- `BackendCpp` compila el `.cpp` a `.so`/`.dll` con el compilador configurado y lo
  carga con `dlopen`/`LoadLibrary`; `BackendJava` (opcional, `-DFUNSHI_JAVA=ON`)
  compila con `javac` y ejecuta sobre un JVM cargado dinámicamente vía JNI.
- El editor (`SettingsScript`) dibuja los campos reflejados (escalares, arrays,
  grupos y referencias a `GameObject`) y dispara la recompilación.

---

## 5. Dependencias externas

| Dependencia | Uso | Integración |
|---|---|---|
| GLFW | Ventana, contexto OpenGL y eventos de input | `find_package(glfw3)` |
| OpenGL 3.3 core | Renderizado (shaders, VBO/VAO, FBO) vía `GLFuncs` | `find_package(OpenGL)` (sin GLU) |
| Bullet Physics | Física y colisiones (btDiscreteDynamicsWorld) | `find_package(Bullet)` con fallback a targets clásicos |
| Assimp | Carga de modelos 3D (opcional via `USE_ASSIMP`) | `find_package(assimp)` |
| GLM | Matemáticas de Transform y cámaras (`GLM_ENABLE_EXPERIMENTAL`) | Usa `External/glm` si existe; si no, `find_package(glm)` del sistema |
| nlohmann/json | `EditorConfig` (persistencia de configuración) | Vendoriado en `External/nlohmann` |
| Dear ImGui | Interfaz del editor | Integrada en `src/ImGui/` |
| ImGuizmo | Gizmos de transformación | Integrada en `FunshiEngineGL/ImGuizmo/` |
| ncurses / X11 | Enlace en Linux | `find_package(Curses)` + X11/Xrandr/Xi |

En Windows se enlazan además `opengl32` y `dbghelp` (stack traces de
`RuntimeException`). `glu32` ya no se enlaza: sin modo inmediato no queda ningún
`glu*` en uso.

---

## 6. Organización del código

La convención general es un par `.h`/`.cpp` por clase. Las excepciones son:

- Estructuras de datos genéricas, que deben ser header-only para la instanciación de plantillas.
- Componentes pequeños y algunas interfaces GUI, parcialmente inline por el acoplamiento actual.
- Headers de terceros, que conservan la organización original.

`CMakeLists.txt` descubre automáticamente los `.cpp` y `.h` bajo `src/` mediante
`GLOB_RECURSE CONFIGURE_DEPENDS`. Los nuevos archivos de implementación se incorporan
al build sin enumerarlos manualmente. Los archivos de ImGui se recopilan por separado
desde `src/ImGui/` y los de ImGuizmo desde `ImGuizmo/` (fuera de `src/`).

Además del ejecutable, el proyecto define **doce targets de prueba headless**
registrados en CTest (compilan en cualquier plataforma con `BUILD_ENGINE=OFF`;
`scripts-java-tests` solo se registra con `-DFUNSHI_JAVA=ON`):

- `filemanager-tests` (27): ejercita `GestorDeArchivos`/`FileManager`/`FileSystemWatcher`
  contra un proyecto temporal, sin ventanas ni pila gráfica.
- `configuracion-tests` (99): round-trip del JSON de `EditorConfig` (general y
  por proyecto, con `ConfigPersistence`/`ProjectPaths`), carga tolerante ante
  archivos ausentes/corruptos/parciales, prioridad de las claves modernas sobre
  el `menu/*` legacy, `restablecer`, escritura atómica y guardado diferido.
- `eventbus-tests` (16): suscripción/publicación/unsubscribe del canal tipado de GUI.
- `menu-tests` (30): lógica pura del menú (traducción, observer de cambios y reset).
- `assetmanager-tests` (82): caché Flyweight de meshes (rutas `AssetPath`, geometría
  `Mesh` con `computeBounds`, `computeNormals` —incluido el modo `soloFaltantes`
  para assets que mezclan sub-mallas con y sin normales— y `computeTangents`) y el
  registro compartido con un loader artificial.
- `texturemanager-tests` (15): caché Flyweight de imágenes CPU (sin entrar la pila gráfica).
- `estructuras-tests` (87): `ListaDE`, `ArbolEnlazado`, `PriorityListaDE`,
  `MinHeap`/`MaxHeap`, `ListMergeSort` y `ArbolBinarioEnlazado`.
- `rendering-tests` (131): geometría de las líneas del pipeline moderno
  (`LineBuilder`): expansión de cada segmento al quad que ensancha el shader,
  color por extremo (difuminado de la grilla), polilíneas, aristas con índices
  fuera de rango y caja de 12 aristas; más la guía de eje (`GuiaEje`): origen y
  dirección en mundo, dirección local rotada con el objeto, normalización frente
  a la escala, colores por eje, recorte de la recta hasta el horizonte con la
  distancia 3D a la cámara, difuminado por vértice y los rechazos defensivos
  (eje inválido, NaN/Inf, eje degenerado, horizonte degenerado, recta fuera del
  horizonte). Solo CPU, sin OpenGL.
- `scripts-tests` (42): reflexión `SerializeField` (escalares, arrays, grupos
  anidados) y el round-trip binario del árbol de valores.
- `scripts-runtime-tests`: compila un `.cpp` real con `BackendCpp`, lo carga con
  `dlopen` y ejecuta el ciclo + hot reload (en Windows sale con 77/SKIP).
- `scripts-java-tests`: end-to-end del backend Java (JNI); solo con `FUNSHI_JAVA=ON`.
- `audio-tests` (16): `AudioEngine`/`AudioClipsManager` con `NullAudioBackend`
  (contrato de la cola de comandos: clips, handles, encolado, detención, volumen).
- `userinterface-tests` (34): `UserInterfaceCustom` (modelo del Creador de
  interfaces, `src/GUI/CreadorUI/`): round-trip JSON de los 5 tipos de widget,
  guardar/cargar y tolerancia a JSON parcial.

La opción `BUILD_ENGINE=OFF` compila solo las pruebas (útil en CI y plataformas
sin las librerías gráficas), y `ENABLE_ASAN` (ON por defecto en Debug) activa
ASan+UBSan en GCC/Clang.

---

## 7. Flujo de datos principal

```text
main.cpp
  │
  ├─ input GLFW ──► EditorInput (callbacks + máquina de teclas) ──► cámara activa (movimiento continuo)
  │                                       ──► tecla E: toggleEditorInterfaces()
  │                                       ──► 1/T, 2/R, 3/U: operación del gizmo (y apagan la guía)
  │                                       ──► X/Y/Z: guía de eje del objeto seleccionado
  │                                       ──► Escape: volver al menú (máquina de estados)
  │
  ├─ GameScene::GUI()
  │     ├─ SceneMenuBarInterface ──► flag start (Play/Stop) ──► GameScene::update
  │     ├─ SceneSelectedInterface ──► EditorController (crear/borrar/reparentar GO)
  │     │                         ──► EventBus.publish(ObjectCreated/Deleted/Selected)
  │     ├─ SceneObjectTree ──► selección y reparentado por drag & drop
  │     └─ GUIManager paneles
  │           └─ SettingsObjectInterface ──► EditorController (add/removeComponent,
  │                                          sync colliders, "Gizmo activo")
  │
  ├─ GameScene::update(dt)
  │     ├─ transición editor→play: empuja la pose visual a los cuerpos Bullet
  │     ├─ si start y gizmo libre: PhysicsEngine::stepSimulation(dt)
  │     │               └─ btDiscreteDynamicsWorld::stepSimulation
  │     └─ scripts: IScriptBehaviour::onUpdate (si compilados)
  │
  └─ GameScene::gameScene()
        ├─ LightSystem::beginFrame() [solo CPU: datos de luz para el shader]
        ├─ pasada de la grilla (Grid + batch de líneas, color según apariencia)
        ├─ dibujarGameObjects (MeshRenderer shader; único pipeline)
        ├─ marcadores de luz y cámara (wireframes auxiliares, batch de líneas)
        ├─ ImGuizmo::Manipulate sobre el GizmoTarget activo (objeto o collider)
        ├─ dibujarViewportsPrevios (FBO de cámaras) + paneles ImGui
        └─ pickObject con el mouse para seleccionar en el viewport
```

---

## 8. Patrones aplicados y límites actuales

| Patrón | Aplicación actual | Efecto arquitectónico |
|---|---|---|
| RAII / ownership explícito | `unique_ptr` en `GameScene`, `SceneRegistry`, `GUIManager`, `FileManager`; colliders y cuerpos Bullet gestionados por `Collider`/`RigidBody` | Propietarios claros y liberación automática. |
| Composite | Árbol n-ario de `GameObject` en `ArbolEnlazado` | Modela la jerarquía padre-hijo y las transformaciones anidadas. |
| Registry / Repository | `SceneRegistry` | Centraliza búsqueda, ownership y operaciones de escena. |
| Controller | `EditorController` | La GUI no coordina mutaciones directamente; la física se registra aquí. |
| Factory | `ComponentFactory`, `GameObjectFactory` | Reduce `new` concretos en GUI y serialización. |
| Facade | `PhysicsEngine` (PIMPL) y `FileManager` | Ocultan Bullet y el filesystem detrás de operaciones del dominio. |
| Adapter | `BulletPhysicsAdapter` | Permite sustituir el backend de física sin modificar `PhysicsEngine`. |
| Strategy | `IPhysicsBackend` | Contrato de backend inyectado por constructor (con nullptr se puede montar la fachada sin mundo). |
| MVP | Paquete `MenusGUI` (`MenuModel`/`MenuView`/`StartMenuPresenter`) | Lógica del menú testeable, independiente de ImGui/GLFW. |
| Observer | `EventBus` (token-based) | Desacopla notificaciones de escena y selección entre subsistemas. |
| State | `ApplicationStateMachine` | Explicita los modos principales de la aplicación. |
| Flyweight / Cache | `AssetManager` (meshes) y `TextureManager` (imágenes) | Recursos CPU compartidos por ruta (`AssetPath`) con loader inyectable y liberación por refcount. |

No están implementados todavía:
- `Command` para undo/redo.
- `Prototype` para duplicación y prefabs.
- `Visitor` para inspectores extensibles.

---

## 9. Observaciones de arquitectura

- `SceneRegistry` establece ownership único mediante `std::vector<unique_ptr<GameObject>>`.
  GUI, selección, física y vistas lineales solo mantienen referencias raw no propietarias.
- `PhysicsEngine` es una fachada PIMPL con el backend inyectado (`IPhysicsBackend`):
  el header no expone Bullet y la fachada puede existir sin mundo (útil para pruebas
  y para el editor sin simulación).
- `GameScene` sigue siendo un coordinador amplio: mezcla ciclo de frame, render,
  GUI, física, gizmo y vistas previas. La extracción de `SceneRenderer`,
  `PhysicsSystem` y `ScriptSystem` aliviaría esa responsabilidad.
- `ApplicationStateMachine` explicita los estados, pero las transiciones se realizan
  desde `main.cpp` y conviven con flags de UI legados (`menuActivo`, `start`).
- `EventBus` transporta `GameObject*` crudos. Un consumidor no debe conservar esos
  punteros después de recibir `ObjectDeleted`. `EditorController::deleteGameObject`
  publica el evento **antes** de destruir el objeto, para que los observadores
  invaliden sus referencias a tiempo (fix de un use-after-free histórico).
- La manipulación del gizmo pausa `stepSimulation` mientras el usuario arrastra y
  la física solo corre en Play; el sync collider↔rigidbody↔objeto usa la matriz
  global compuesta del dueño, de modo que mover un collider no desincroniza el cuerpo.
- Las rutas de usuario (`<directorioEjecutable>/MotorGrafico`) están
  centralizadas en `EditorConfig` para la configuración y el layout, pero los assets
  del proyecto todavía se resuelven a mano; `PathUtils.h` solo comparte el separador.
- La serialización binaria no tiene versionado ni validación formal de tamaños. Un
  cambio en la estructura de atributos invalida escenas guardadas.
- `SettingsObjectInterface` y algunos componentes todavía incluyen y construyen
  detalles concretos; el siguiente paso de desacoplamiento es completar el uso de
  `EditorController` y descriptors de componentes.
- El renderer es de un solo pipeline: `MeshRenderer` (VBO/VAO + shaders) para
  mallas y el batch de líneas (GPU + shader de ancho en píxeles) para grilla,
  marcadores y gizmos de collider. No hay modo inmediato ni estado fijo: la
  iluminación viaja como uniforms y las transformaciones como uniforms de
  matriz, así que el contexto es OpenGL 3.3 core estricto.
- El perfil core es un **requisito duro**, no una preferencia: `Ventana::
  initVentana` lo pide con `GLFW_OPENGL_CORE_PROFILE` + versión 3.3 +
  `GLFW_OPENGL_FORWARD_COMPAT`, y si `glfwCreateWindow` falla (driver sin 3.3
  core) o si a `GLFuncs::init()` le falta alguna función, aborta el arranque con
  un mensaje en consola en vez de seguir hasta fallar en el primer draw (que se
  vería como pantalla negra sin explicación). El requisito real es GLSL 330,
  que es lo que imponen los shaders (`layout in/out`).

---

## 10. Capas de la aplicación

```text
Aplicación:      main.cpp, MiAPP, ApplicationStateMachine, GUIManager, EditorConfig
Dominio:         GameScene, SceneRegistry, EditorController, GameObject, Component,
                 EventBus, FileManager, LightSystem
Infraestructura: BulletPhysicsAdapter, OpenGL/GLFW (Ventana), Assimp (Modelos3D),
                 Binario/SceneSerializer, Script (dlopen/LoadLibrary), FileSystemWatcher
Dependencias:    ImGui, ImGuizmo, nlohmann/json, GLM (integradas o vendoriadas)
```

Relaciones clave:

```text
GUI → EditorController → SceneRegistry
                       └─ PhysicsEngine → IPhysicsBackend → BulletPhysicsAdapter → Bullet
                       └─ EventBus → suscriptores (SceneSelectedInterface, etc.)
Vistas del explorador → FileManager → GestorDeArchivos / FileSystemWatcher
SceneSerializer → SceneRegistry / EditorController
GameScene → coordina todos los subsistemas del frame
```

---

## 11. Pruebas y CI

- `tests/FileManagerTests.cpp`: construcción y re-resolución del árbol de archivos,
  operaciones de dominio (crear, renombrar, copiar, eliminar, búsqueda) y
  `FileSystemWatcher` (detección de cambios externos, en Linux via inotify).
- `tests/EditorConfigTests.cpp`: round-trip del JSON (general, por proyecto y
  legacy `menu/*`), tolerancia a archivos ausentes/corruptos/parciales,
  escritura atómica (sin temporales colgados) y guardado diferido
  (`solicitarGuardadoGeneral`/`volcarGuardadoGeneral` con `kIntervaloEscritura`).
- `tests/AssetManagerTests.cpp` y `tests/TextureManagerTests.cpp`: caches
  Flyweight con loader artificial; validan rutas normalizadas, compartición y ciclo
  de vida de los recursos.
- `tests/EstructurasTests.cpp`: suite de las estructuras propias. Cubre la regresión
  de `ListaDE::addFirst`, la liberación de nodos en `remove`, el borrado de
  `ArbolEnlazado` (hoja, nodo interno y raíz), `PriorityListaDE`, la extracción
  ordenada de `MinHeap`/`MaxHeap`, la estabilidad de `ListMergeSort` y el árbol
  binario (addLeft/addRight, childsOf, preorden RID, borrado de hoja e interno).
- `tests/ModelSerializationTests.cpp`: serialización binaria del componente
  `Model` (path con prefijo de longitud). Cubre la regresión del core al cargar
  escenas: verifica que un path más largo que el buffer de lectura no desalinee
  el stream, además de round-trip corto/largo/vacío y archivos truncados.
- `tests/TemaEditorTests.cpp`: aplicación del perfil `Apariencia` al estilo de ImGui
  (`TemaEditor::aplicarEstilo`, solo contexto de ImGui, sin pila gráfica). Cubre la
  regresión "el color de acento no llega a toda la interfaz": con un acento no azul
  verifica que **ningún** rol de la paleta conserve el azul de fábrica de Dear ImGui
  (`FrameBg` —campos y pista del slider—, `Tab`/`TabDimmed`, `Border`/`Separator`,
  `TableHeaderBg`, `TextLink`, `DragDropTarget`), que el acento por defecto mantenga
  el aspecto y las transparencias históricas, que un acento translúcido no apague
  los roles de primer plano (el alpha del perfil no participa del tema), que
  aplicar el mismo perfil dos veces sea idempotente y que el modo blanco y negro
  deje la paleta monocroma.
- `tests/ComandosTests.cpp`: los 7 comandos del editor (`CrearObjetoComando`,
  `BorrarObjetoComando`, `ReparentarComando`, `TransformComando`,
  `AgregarComponenteComando`, `QuitarComponenteComando`, `LimpiarEscenaComando`)
  con deshacer/rehacer, la cadena de redo múltiple, el límite del historial y la
  descripción que el historial devuelve para avisar en la barra de estado.
- Los dieciocho targets compilan en cualquier plataforma y se ejecutan con `ctest`.
- `.github/workflows/ci.yml` compila el engine completo en Ubuntu (Release, sin
  ASan) y ejecuta las pruebas; además ejecuta las headless en
  Linux/Windows con `BUILD_ENGINE=OFF` y el backend Java en Ubuntu con JDK.
- `.github/workflows/release.yml` y `windows-release.yml` también ejecutan la
  suite (y `estructuras-tests`) al generar los instaladores por tag.

Los bugs de la Fase 2 (cámaras/vistas previas) y sus fixes están documentados en
[CAMARAS_VISTAS_PREVIAS.md](CAMARAS_VISTAS_PREVIAS.md).

---

## 12. Pendientes conocidos

- [ ] Sistema de animaciones.
- [ ] `CommandManager` para undo/redo.
- [ ] Cuadro de log de errores en el editor.
- [ ] Resolver IDs duplicados al crear objetos; limpiar binarios huérfanos al eliminar.
- [ ] Clase `Input` de gameplay (el input del editor ya está modularizado en `src/Input/EditorInput`; falta exponer teclado/mouse a los scripts vía la tabla `api`).
- [ ] Terminar los popups del inspector.
- [ ] Prefabs y duplicación de objetos.
- [ ] Portabilidad de rutas de assets (centralizar `HOME` / rutas de Windows).
- [ ] Versionado y validación de la serialización binaria.
- [ ] Extraer `SceneRenderer`, `PhysicsSystem` y `ScriptSystem` de `GameScene`.
- [ ] Encapsular las estructuras internas de `SceneRegistry` (eliminar getters raw de compatibilidad).
- [ ] Vistas previas de cámara seleccionables con clic (hoy son pasivas).

---

## 13. Análisis: expresiones regulares (veredicto)

Se buscó uso de `std::regex` / `boost::regex` en el código del motor (búsqueda
por `regex|regular_expression` en `src/`, tests y herramientas). **No existe
ningún consumidor de expresiones regulares** (la única aparición es un comentario
dentro de `ImGui/imgui.cpp`, código de terceros), y los puntos donde un motor
suele necesitarlas están resueltos con otras herramientas:

- **Rutas de assets**: `AssetPath` normaliza con `std::filesystem` (carga sobre
  `std::path` con separadores y síndromes `.`/`..` del propio API), sin patrones.
- **Extensión de archivos**: predicados directos (`entrada.path().extension()`,
  comparaciones de `std::string`) en `GestorDeArchivos` y `AssetManager`.
- **Configuración del editor**: `ConfigPersistence` (invocada por la fachada
  `EditorConfig`) parsea JSON con **nlohmann/json** (librería ya vendoriada en
  `External/`); las claves se validan por acceso estructurado, no por patrones.
- **Serialización de escenas**: `SceneSerializer` usa un formato binario en
  preorden con marcadores literales `=>`/`<=`, decididos con comparaciones de
  `std::string` exactas (búsqueda del look-ahead), no con matching.
- **Nombres de tipos**: `ComponentFactory`/`TypeUtils` comparan huesos
  exactamente (`"CameraComponent"` y alias `"Camera"`), sin patrones.

**Conclusión**: no se incorpora ninguna dependencia de regex. Las necesidades
actuales de parsing son estructuradas (filesystem, JSON, binario, comparación
literal) y una implementación con `std::regex` añadiría coste de compilación y
de runtime sin cubrir ningún caso real. Si más adelante se necesitara validar
nombres de archivos o campos con patrones (p. ej. prefijos de prefabs), la
decisión sería revistarla puntualmente con `std::regex` (sin dependencia extra).
