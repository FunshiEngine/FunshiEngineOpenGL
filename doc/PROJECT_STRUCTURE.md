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
  mismo pipeline de líneas. Los tres ejes (X rojo, Y verde, Z azul) tienen un
  solo color de base en toda la escena —el de la guía de eje, que usa el gizmo
  como referencia— y la grilla y la guía lo ajustan por contraste contra el
  color que tienen debajo.
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

El punto de entrada del **editor** es `FunshiEngineGL/src/main.cpp`. El punto de
entrada de los **juegos exportados** es `FunshiEngineGL/src/Runtime/main_juego.cpp`
(sin código del editor: GUIManager, MenuGUI, paneles). La configuración de compilación
está en `FunshiEngineGL/CMakeLists.txt`, que es el único build soportado: CMake
genera la solución de Visual Studio dentro del directorio de build que elijas
(por ejemplo `FunshiEngineGL/build-win/FunshiEngineGL.sln`), así que no hay
proyectos de Visual Studio mantenidos a mano en el repositorio. Las pruebas
headless viven en `tests/` y el CI en
`.github/workflows/ci.yml`.

`funshi_diagnostics_core` es una biblioteca compartida dentro del motor; no
depende de ImGui y puede consumirla cualquier frente que hable su formato.

`Throwable` guarda el mensaje con el que se lanza y lo devuelve en `what()`,
también cuando la excepción se captura como `std::exception`; `RuntimeException`
conserva la traza de pila y el tipo de excepción. Los límites
que deciden que un fallo es reportable lo envían a `RegistroDiagnosticos`; el
constructor de la excepción no registra automáticamente porque algunas
excepciones se usan como control interno. El registro mantiene una ventana
acotada en memoria para Estado y persiste eventos en JSON Lines para que una
invocación externa al editor pueda consultar el mismo historial. La GUI no
es propietaria del registro y el módulo de diagnósticos no depende de ImGui.
Los publicadores aceptan una instancia explícita del registro para pruebas
aisladas; las llamadas de producción usan la instancia global compartida.
`SceneSerializer::load` devuelve `false` si la reconstrucción queda parcial o
falla. Una escena nueva sin índice se crea vacía sin registrar un error; los
índices corruptos producen advertencias y los objetos que no se pueden leer,
errores con su identificador y ruta.
`Modelos3D` publica los errores tipados de `AssetManager` en el mismo registro;
la clave de ruta evita repetir el evento cuando el cache vuelve a entregar el
fallo.
`Script::cargarSiNecesario` publica cada fallo de compilación/carga como
`SCRIPT_LOAD_FAILED` y conserva el mensaje en el componente para su vista
contextual. Repetir la consulta de un intento fallido no genera otro evento;
un nuevo intento sí se registra de nuevo. El registro puede inyectarse en el
componente para pruebas aisladas.
Los backends C++ y Java contienen las excepciones de callback dentro de su
límite de ejecución, conservan el primer fallo en `ComportamientoCargado` y lo
publican como `SCRIPT_CALLBACK_FAILED`; el comportamiento queda deshabilitado
hasta la recarga.

La terminal del editor se compone de `GUI/Terminal/TerminalInterface` y
`Terminal/TerminalSession`. La primera es una ventana persistente registrada en
`GUIManager`, el menú Ventanas y el dock; la sesión lanza una shell dentro del
directorio fijo del proyecto y entrega su salida mediante un hilo lector que
drena hasta el cierre del pseudo-terminal (incluida la salida emitida al
terminar la shell, que `TerminalInterface` procesa siempre, no solo mientras la
sesión está activa). El backend Windows usa ConPTY y el POSIX usa `forkpty` con
el maestro en modo no bloqueante; la sesión no reutiliza `Proceso`, que es
síncrono y no expone una terminal interactiva.
`TerminalEmulator` mantiene la cuadrícula VT, cursor, atributos ANSI comunes,
pantalla alternativa y respuestas a consultas de cursor; `TerminalInterface`
traduce celdas y teclas a ImGui. La entrada UTF-8 se envía al pseudo-terminal.
La emulación cubre el conjunto usual de secuencias de control, sin extensiones
como color true-color/256, ratón o portapapeles.

---

## 2. Árbol de archivos relevante

Se omiten los directorios generados por el build (`build/`, `build_temp/`), los
archivos temporales de editor (`*.swp`, `*.swo`) y el contenido interno de las
dependencias de terceros.

```text
FunshiEngineGL/                          ← raíz del repo
├── README.md                            ← visión general, build, ejecución y enlaces a la documentación
├── doc/                                 ← documentación del proyecto
│   ├── PROJECT_STRUCTURE.md             ← este documento
│   ├── MANUAL_DE_USO.md                 ← manual de usuario (editor + scripting C++/Java)
│   ├── CAMARAS_VISTAS_PREVIAS.md        ← cámaras componente + vistas previas
│   ├── ARQUITECTURA_ESTADOS_GUI.md      ← estados/menú/GUI internas (diseño + Fases 1-3)
│   ├── DocuTecnicoBugs.md               ← registro técnico de bugs (patrón, síntomas, fix)
│   └── FLUJO_DE_RAMAS.md                ← convención de ramas (develop/test/staging/release)
├── Skills/                              ← convenciones para agentes de código
│   ├── Arquitectura/Arquitectura.md     ← reglas de arquitectura y memoria de trabajo
│   ├── Plan/Plan_De_Trabajo.md          ← convenciones del código y flujo de trabajo
│   ├── Plan/Depuracion.md               ← pruebas, depuración y orden por dependencia
│   └── Repositorio/Repositorio.md       ← comandos esenciales y ejecución del editor
├── .github/workflows/ci.yml             ← CI: engine en Ubuntu y en Windows (MSVC) + pruebas headless
├── .github/workflows/release.yml        ← instaladores Qt IFW (.run) e Inno (.exe) por tag
├── .github/workflows/windows-release.yml← build+release específico de Windows
├── .gitignore / .gitattributes
├── tests/                               ← suites headless (CTest), una por sistema
│   ├── FileManagerTests.cpp             ← headless del explorador de archivos
│   ├── ImportarArchivosTests.cpp        ← headless de la importación de archivos del
│   │                                      sistema (no pisar, carpetas recursivas y los
│   │                                      parsers del portapapeles: uri-list y DROPFILES)
│   ├── EditorConfigTests.cpp            ← headless de la configuración JSON
│   ├── EditorEventBusTests.cpp          ← headless del canal tipado de GUI (Fase 2)
│   ├── MenuModelTests.cpp               ← headless del modelo del menú (Fase 3)
│   ├── AssetManagerTests.cpp            ← caché Flyweight de meshes (AssetPath/Mesh)
│   ├── TextureManagerTests.cpp          ← caché Flyweight de imágenes (TextureManager)
│   ├── EstructurasTests.cpp             ← listas, árboles, heaps y ordenamiento propios
│   ├── ScriptsTests.cpp                 ← reflexión SerializeField + round-trip binario
│   ├── ScriptsRuntimeTests.cpp          ← BackendCpp end-to-end (compila y dlopen un .so)
│   ├── ScriptsJavaTests.cpp             ← BackendJava end-to-end (solo con FUNSHI_JAVA)
│   ├── AudioEngineTests.cpp             ← AudioEngine con NullAudioBackend (por ruta)
│   ├── MusicaTests.cpp                  ← GestorMusica: lista ordenada, fundido y repetición
│   ├── SceneSerializationTests.cpp      ← round-trip de escena + nombres por defecto
│   ├── SerializacionTests.cpp           ← lectura de datos serializados (formatos y tamaños)
│   ├── ModelSerializationTests.cpp      ← round-trip binario del componente Model
│   ├── RenderingTests.cpp               ← geometría de líneas (LineBuilder) y guía de eje
│   ├── EditorInputTests.cpp             ← separación entre el input del editor y la interfaz
│   ├── ProcesoTests.cpp                 ← runner de procesos sin shell (argv, log, entorno)
│   ├── HilosTests.cpp                   ← pool de hilos del motor
│   ├── BarraProgresoTextoTests.cpp      ← formato de la barra de progreso en texto
│   ├── DiagnosticRegistryTests.cpp       ← registro de diagnósticos del motor
│   ├── ComandosTests.cpp                ← undo/redo del patrón Command
│   ├── CatalogoComponentesTests.cpp     ← catálogo declarativo de componentes
│   ├── IconosComponentesTests.cpp       ← iconos de componente (regla y catálogo)
│   ├── MallaRegionesTests.cpp           ← regiones con material propio de una malla
│   ├── MaterialesRegionesTests.cpp      ← materiales por región
│   ├── Primitivas3DTests.cpp            ← primitivas 3D generadas en CPU
│   ├── PreviewMaterialTests.cpp         ← vista previa de un material
│   ├── IluminacionTests.cpp             ← recogida y estado de las luces de la escena
│   ├── SombrasTests.cpp                 ← mapas de sombras
│   ├── ShadersTests.cpp                 ← sistema de shaders
│   ├── TemaEditorTests.cpp              ← TemaEditor (perfil de apariencia del editor)
│   ├── PrefabTests.cpp                  ← prefabs: guardar → instanciar → round-trip
│   ├── TilemapTests.cpp                 ← tileset (JSON estilo Tiled) y componente Tilemap
│   ├── InterfaceComponentTests.cpp      ← round-trip binario del InterfaceComponent
│   ├── InterfacesJuegoTests.cpp         ← recolección de interfaces (HUD) de la escena
│   ├── WebUIRuntimeTests.cpp            ← runtime de interfaces web
│   ├── ManifiestoAssetsTests.cpp        ← núcleo del manifiesto de assets de la escena
│   ├── OrquestadorEstadoTests.cpp       ← orquestador de estados de la GUI
│   ├── ExportacionTests.cpp             ← exportación del juego standalone
│   ├── TerminalEmulatorTests.cpp        ← emulador de terminal (ANSI, rejilla, scrollback)
│   ├── TerminalSessionTests.cpp         ← E/S real del pseudo-terminal
│   ├── ModsTests.cpp                    ← núcleo del sistema de mods (semver, manifiesto, ZIP)
│   ├── ModsTestPlugin.cpp               ← plugin binario de prueba que carga ModsTests
│   └── TempPruebas.h                    ← carpeta temporal por prueba (limpieza al salir)
└── FunshiEngineGL/                      ← proyecto CMake principal
    ├── CMakeLists.txt                   ← GLOB de fuentes, dependencias, sanitizers,
    │                                      pruebas (CTest), opción BUILD_ENGINE y la
    │                                      versión del producto (FUNSHI_VERSION, variable
    │                                      de caché: la de project() y la del
    │                                      VERSIONINFO del .exe salen de ahí; el
    │                                      workflow la fija con -DFUNSHI_VERSION al
    │                                      publicar por tag). Backend gráfico (IRenderBackend),
    │                                      backend audio (IAudioBackend), backend físico
    │                                      (IPhysicsBackend) y backends de scripts (C++/Java)
    │                                      siguen patrón Strategy.
    ├── Imagenes/                        ← íconos del explorador (cpp, cubo, file, folder, hpp)
    ├── ImGuizmo/                        ← dependencia integrada (ImGuizmo.cpp/.h, etc.)
    ├── External/nlohmann/json.hpp       ← nlohmann/json vendoriado (EditorConfig)
    ├── External/miniz/                   ← miniz 3.0.2 vendoriado (módulos separados;
    │                                       lectura/escritura de ZIP de los .fmod)
    ├── External/stb_image/              ← stb_image vendoriado (decodificador de
    │                                       imágenes; su implementación va en una
    │                                       unidad propia, no en un módulo del motor)

    └── src/
        ├── main.cpp                     ← composition root: ventanas, callbacks, bucle, config
        ├── EngineTime.h / EngineTime.cpp← delta time y limitador de FPS
        ├── Ventana.h / Ventana.cpp      ← inicialización GLFW y contexto 3.3 core
        ├── Assets/                      ← caché Flyweight compartida (meshes e imágenes)
        │   ├── AssetManager.h/.cpp      ← registro de AssetPath→Mesh (loader inyectable)
        │   ├── TextureManager.h/.cpp    ← registro de AssetPath→Image (loader inyectable)
        │   ├── Tileset.h/.cpp           ← parseo de tilesets estilo Tiled (image, grilla,
        │   │                                margin/spacing, flag solid) y UV por tile
        │   ├── AssetPath.h / Image.h    ← rutas normalizadas y metadatos de imagen
        │   ├── AssetException.h / TextureException.h ← errores de carga con mensaje y ruta
        │   ├── Mesh.h/.cpp              ← geometría CPU (vértices, normales, índices) y
        │   │                              regiones con material propio (rango de índices
        │   │                              y nombre del material del archivo)
        │   ├── AssimpMeshLoader.*       ← loader Assimp→Mesh (conserva el material de
        │   │                              cada sub-malla como región)
        │   ├── Primitivas2D.h            ← quad 2D unitario (CPU puro, testeable)
        │   ├── Primitivas3D.h            ← esfera UV (CPU puro, testeable): es la
        │   │                              muestra con la que el editor enseña un material
        │   └── StbImageLoader.*         ← loader stb_image→Image (solo engine) +
        │                                   `dimensiones()`: lee la cabecera del
        │                                   archivo sin decodificar; el volteo
        │                                   vertical se hace sobre la imagen
        │                                   decodificada (no con el estado global
        │                                   de la librería)
        ├── Audio/                       ← audio del motor (backend inyectable)
        │   ├── AudioEngine.h/.cpp       ← fachada thread-safe (cola de comandos + hilo)
        │   ├── ExtensionesAudio.h       ← extensiones de audio comparadas normalizadas
        │   ├── IAudioBackend.h          ← contrato Strategy del backend de audio
        │   ├── IMusicaBackend.h         ← contrato del backend de música (pista, bus, fundido)
        │   ├── GestorMusica.h/.cpp      ← reproductor de música (fundido cruzado, bus, playlist)
        │   ├── MiniAudioBackend.*       ← backend concreto (miniaudio vendoriado)
        │   ├── NullAudioBackend.h       ← backend nulo para pruebas headless
        ├── Behaviour/
        │   ├── IScriptBehaviour.h       ← interfaz de scripts (onStart/onUpdate/onStop, campos)
        │   ├── ScriptGameObject.*       ← tablas versionadas de GameObject y servicios
        │   │                                (audio, teclado, mouse y movimiento físico)
        │   ├── ScriptRuntime.*          ← registro de backends, compilación y hot reload
        │   ├── ComportamientoCargado.h  ← instancia compilada (módulo + campos)
        │   ├── Backends/
        │   │   ├── BackendScript.h      ← interfaz de backend (C++/Java)
        │   │   ├── BackendCpp.*         ← compila .cpp→.so/.dll y lo carga (dlopen)
        │   │   ├── RutaCabecerasScript.h ← resuelve include/ junto al ejecutable o la ruta del checkout
        │   │   ├── ResolucionJdk.h    ← empareja libjvm y javac de una misma raíz de JDK
        │   │   └── BackendJava.*        ← Java vía JNI/JVM dinámico (solo con FUNSHI_JAVA)
        │   └── Reflection/
        │       └── BehaviourReflection.* ← reflexión, macros SerializeField y serialización
        │                                    de valores de script
        ├── Configuracion/
        │   ├── Apariencia.h             ← perfil de apariencia (tema/acento/fondo/B-N) + utilidades
        │   ├── EditorConfig.h/.cpp      ← fachada de la configuración (datos() + cargar*/guardar*, guardado
        │   │                              diferido) y punto de entrada estable del CRUD de proyectos, que
        │   │                              DELEGA en ProjectManager (una sola implementación)
        │   ├── ConfigPersistence.h/.cpp ← JSON puro de la config: general + proyecto (escritura atómica)
        │   ├── ProjectPaths.h/.cpp      ← rutas canónicas del motor (una sola fuente de verdad)
        │   └── ProjectManager.h/.cpp    ← ciclo de vida de proyectos; creación estricta para CLI separada
        │                                  del aseguramiento/migración usado por el editor
        ├── Diagnosticos/
        │   └── RegistroDiagnosticos.h/.cpp ← historial acotado en memoria y persistencia JSON Lines
        ├── Terminal/
        │   ├── TerminalSession.h/.cpp      ← shell interactiva mediante ConPTY o pseudo-terminal POSIX
        │   └── TerminalEmulator.h/.cpp     ← cuadrícula VT y parser de secuencias ANSI
        ├── Entity/
        │   ├── Entity.h                 ← base real: componentes, jerarquía, transform y
        │   │                              serialización común (el derivado aporta sus
        │   │                              atributos propios)
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
        ├── Exportador/
        │   ├── GameExporter.h/.cpp      ← exporta el juego standalone: proyecto CMake temporal,
        │   │                              compila el runtime (sin editor), empaqueta ejecutable +
        │   │                              scenes/scripts/assets en estructura simplificada.
        │   │                              Deja su traza en un archivo propio (logs/Exportacion_*.log):
        │   │                              la salida de la compilación queda ahi, y el mensaje
        │   │                              final la menciona. Cada exportación compila en su
        │   │                              propio directorio temporal (reservado de forma atómica:
        │   │                              dos exportaciones a la vez no se pisan)
        │   ├── EstadoExportacion.h/.cpp ← estado de la exportación (fase, resultado, progreso,
        │   │                              mensaje): reglas de dominio, sin ImGui, para que el
        │   │                              resultado no se pierda y se pueda comprobar headless
        │   ├── PrecondicionesExportacion.h/.cpp ← comprueba que la exportación se puede lanzar
        │   │                              (proyecto elegido y con carpeta) y devuelve el motivo
        │   ├── ImportacionesPE.h/.cpp   ← lee la tabla de importaciones de un binario PE (Windows):
        │   │                              las DLL que el juego exportado necesita para arrancar
        │   ├── ConfigJuegoExportado.h   ← manifiesto JuegoExportado.json (header-only)
        │   ├── RaizEngine.h             ← búsqueda de la raíz del engine para add_subdirectory
        │   └── RutasExportacion.h       ← helpers de rutas del proyecto (header-only)
        ├── Fisicas/
        │   ├── IPhysicsBackend.h        ← contrato Strategy del backend de física
        │   ├── ContactoFisico.h           ← transiciones de contacto entre colliders
        │   ├── PhysicsEngine.h/.cpp        ← fachada PIMPL; el header no expone Bullet
        │   └── BulletPhysicsAdapter.h/.cpp ← adaptador concreto de Bullet (RAII)
        ├── FileManager/
        │   ├── FileManager.h/.cpp       ← fachada del explorador (alto nivel): orquesta modelo,
        │   │                              operaciones de dominio y toda la E/S nativa (diálogos,
        │   │                              abrir con la app del sistema, listado de directorio,
        │   │                              plantillas de scripts). Usa GestorDeArchivos internamente.
        │   ├── FileSelection.h          ← estado de navegación compartido entre vistas
        │   ├── FileSystemWatcher.h/.cpp ← vigilancia de cambios externos (inotify
        │   │                              en Linux; polling con mtime en el resto)
        │   ├── IServicioPortapapeles.h  ← contrato: archivos que el sistema tiene en el
        │   │                              portapapeles (el motor no habla con la API del SO)
        │   ├── IServicioDrop.h          ← contrato: archivos soltados sobre la ventana del
        │   │                              editor (el servicio los encola y la vista los drena)
        │   ├── PortapapelesParsers.h    ← parsers puros del portapapeles: text/uri-list
        │   │                              (Linux) y DROPFILES/CF_HDROP (Windows), con la
        │   │                              conversión UTF-16 → UTF-8
        │   ├── PortapapelesSistema.h/.cpp ← lectura del portapapeles en la plataforma del host
        │   │                              (CF_HDROP en Windows; texto del portapapeles en Linux)
        │   ├── ReceptorDropGlfw.h/.cpp  ← recepción del drop con GLFW: encola las rutas y
        │   │                              las entrega al explorador en su frame
        │   └── Proceso.h/.cpp           ← runner de procesos SIN shell (argv propio:
        │                                  CreateProcessW / fork+execvp, log por handles/fd,
        │                                  cwd, entorno extra; el log se trunca por defecto y
        │                                  puede abrirse en modo "agregar al final" para que
        │                                  varias etapas compartan archivo; única vía a
        │                                  `cmd.exe` es la receta cruda del harvest de vcvars)
        ├── GestorDeArchivos/            ← modelo/infra de bajo nivel: Binario (streams binarios),
        │                                  File, Carpeta, GestorDeArchivos (exploración del filesystem).
        │                                  FileManager lo consume como modelo del árbol.
        ├── GUI/
        │   ├── GeneralUserInterface.h/.cpp ← interfaz base de paneles ImGui
        │   ├── WindowNames.h
        │   ├── DockSpaceGUI/               ← dock principal del editor
        │   ├── Estado/
        │   │   ├── StatusBarInterface.h/.cpp ← ventana "Estado": toolchain (Compilador/javac/
        │   │   │                              libjvm) y estado de scripts (compilando/cargado/error)
        │   │   └── EstadoInterfazImGui.h/.cpp ← estado de la interfaz leído de ImGui (si el
        │   │                                  teclado o el mouse pertenecen a un widget)
        │   ├── Navegacion/
        │   │   └── SidebarOrbitaGUI.h/.cpp ← barra lateral con el radio de la órbita de la
        │   │                                  cámara (el input entrega el estado, la GUI dibuja)
        │   ├── Terminal/
        │   │   └── TerminalInterface.h/.cpp ← panel de shell integrado con cwd del proyecto
        │   ├── ModsGUI/                   ← paneles que aportan los mods cargados
        │   │   ├── PanelModInterface.h/.cpp  ← ventana de un panel de mod: abre la ventana de
        │   │   │                              ImGui y llama al callback que entrego el mod
        │   │   └── PanelesModsInterface.h/.cpp ← colección de esos paneles; se sincroniza con
        │   │                                  Mods/RegistroPanelesMods cada frame (los mods
        │   │                                  pueden registrar paneles en caliente)
        │   ├── Tema/
        │   │   └── TemaEditor.h/.cpp      ← aplica el perfil Apariencia al estilo ImGui en vivo
        │   │                                  (tema claro/oscuro, acento RGB en TODOS los roles de ImGui
        │   │                                  y grises azulados de fábrica a gris neutro; ver tests/TemaEditorTests.cpp)
        │   ├── FileManagerGUI/             ← TreeFilesInterface + ContentFolderInterface
        │   │                                  (vistas del explorador; conversan con FileManager)
        │   │   ├── SoltarEnCarpeta.h       ← helper header-only del arrastre: mueve y publica
        │   │   │                              ArchivosReubicados; lo usan las dos vistas (en el
        │   │   │                              grid Ctrl copia; soltar en el árbol siempre mueve)
        │   │   ├── ImportarArchivos.h     ← helper header-only de la importación desde el
        │   │   │                              sistema: copia sin pisar (nombre único) y copia
        │   │   │                              recursiva de carpetas; lo usan el drop del SO y el
        │   │   │                              pegado con Ctrl+V
        │   │   └── RenombrarElemento.h     ← helper header-only del renombre por click derecho:
        │   │                                  modal (campo enfocado al abrir, Enter confirma) y
        │   │                                  camino único de disco + ArchivosReubicados, también
        │   │                                  compartido por el grid y el árbol
        │   ├── MenusGUI/                   ← paquete del menú de inicio (MVP); ver su README.md
        │   │   ├── MenuModel.h/.cpp        ← lógica pura sin ImGui/GLFW
        │   │   ├── MenuView.h/.cpp         ← dibujo ImGui
        │   │   ├── StartMenuPresenter.h/.cpp ← puente motor ↔ modelo
        │   │   └── MenuGUI.h/.cpp          ← fachada pública del paquete
        │   ├── ObjetosGUI/
        │   │   ├── SettingsObjectInterface.h/.cpp ← inspector: crea Settings* por componente
        │   │   │                                 y dibuja el icono de cada componente a la
        │   │   │                                 izquierda de su nombre (IResolutorIconos)
        │   │   ├── SettingsComponent.h
        │   │   ├── Transform/SettingsTransform.h/.cpp ← transform + checkbox "Gizmo activo"
        │   │   ├── Color/  Model/  Script/
        │   │   ├── Material/SettingsMaterial.*  Light/SettingsLight.*
        │   │   ├── Material/EditorMaterialGUI.* ← editor de un material (preset, colores
        │   │   │                                 y texturas), compartido por el panel del
        │   │   │                                 material y el de materiales por region
        │   │   ├── Materiales/SettingsMateriales.* ← una entrada por region con su
        │   │   │                                 material y sus texturas
        │   │   ├── Camera/SettingsCamera.*       ← FOV, planos, velocidad, vista previa
        │   │   ├── Grid/SettingsGrid.*           ← visible y color de la grilla (infinita,
        │   │   │                                 densidad fija: no hay tamaño/separación)
        │   │   ├── Audio/SettingsAudioSource.*    ← ruta del archivo de audio
        │   │   │                                 (drag & drop), volumen, loop
        │   │   ├── Audio/SettingsMusica.*        ← lista ordenada de pistas de música
        │   │   │                                 (drag & drop), volumen, fundido, repetición
        │   │   ├── Interface/SettingsInterface.* ← ruta del .html de la interfaz
        │   │   │                                 (drag & drop) + activa, ancla, tamaño y z
        │   │   ├── Interfaz/SettingsUIComponent.* ← lista de widgets del HUD nativo:
        │   │   │                                 tipo, ancla, desplazamiento, tamaño,
        │   │   │                                 colores, texto, ruta de imagen y orden
        │   │   ├── Skybox/SettingsSkybox.*       ← visible + las seis caras del cubemap
        │   │   │   └── SelectorArchivoCubemap.h ← modal de elección de cara (header-only):
        │   │   │                                la lógica que decide (extensiones válidas,
        │   │   │                                caras faltantes, resolución dispares) son
        │   │   │                                `inline` puras fuera del dibujo, para que
        │   │   │                                las ejercite un target headless sin ImGui
        │   │   ├── RigidBody/SettingsRigidBody.*
        │   │   ├── Prefab/SettingsPrefab.*      ← alta de prefabs desde el inspector
        │   │   ├── Sprite/SettingsSpriteRenderer.* ← tile del sprite: color, mezcla, z
        │   │   ├── Tilemap/SettingsTilemap.*    ← tileset y tamaño del tilemap
        │   │   ├── Animador2D/SettingsAnimador2D.* ← fotogramas, velocidad y loop
        │   │   ├── Sombras/SettingsSombras.*   ← resolución del mapa, sesbio, fuerza, radio
        │   │   │                                y distancia de la sombra de la luz
        │   │   └── Colliders/ (Esfera, Cubo, Malla, Tilemap) ← sync transform/shape con física
        │   ├── Export/
        │   │   └── ExportDialog.h/.cpp           ← diálogo de exportación del juego standalone
        │   │                                      (arma `GameExporter::Config` y lo lanza; el
        │   │                                      estado vive en EstadoExportacion)
        │   └── SceneGUI/
        │       ├── SceneSelectedInterface.h/.cpp  ← jerarquía y selección; usa EditorController
        │       ├── SceneObjectTree.h/.cpp         ← árbol de objetos (con drag & drop)
        │       └── SceneMenuBarInterface.h/.cpp   ← controles Depuración/Juego/Pausa/Reset/Terminar
        ├── GUIManager/
        │   └── GUIManager.h/.cpp         ← fábrica de ventanas y su registro (cada
        │                                  ventana se registra al crearse); posee
        │                                  FileManager
        ├── Herramientas/
        │   ├── TypeUtils.h/.cpp          ← nombres legibles de tipos
        │   ├── PathUtils.h               ← PATH_SEP + regla de cotejo de rutas (rutaBajo)
        │   ├── MaterialPresets.h         ← presets de Material como datos (reemplaza a materiales.h)
        │   ├── TreeGUI/TreeGUI.h         ← widget de árbol genérico para ImGui
        │   └── IconosGUI/                ← carga de íconos (usa stb_image, que ya
        │                                   vive en External/stb_image)
        │       ├── RutasIconos.h        ← dónde se buscan los assets de imagen
        │       │                           (junto al ejecutable y junto al cwd);
        │       │                           el orden de búsqueda es una pieza
        │       │                           testeable en headless
        │       ├── IReglaIcono.h        ← contrato: tipo de componente → ruta de icono
        │       ├── ReglaIconoCatalogo.h/.cpp ← la regla que lee el catálogo (fuente única)
        │       ├── IResolutorIconos.h   ← contrato: tipo de componente → textura
        │       └── CatalogoIconos.h/.cpp← une la regla con la carga en GPU (a demanda)
        ├── Iluminacion/
        │   ├── LightSystem.h/.cpp        ← dueño del estado GL de luces (GL_LIGHT0..7) por frame
        │   │                              (además devuelve el objeto dueño de cada luz)
        │   └── MatrizLuz.h/.cpp          ← matriz de luz del mapa de sombras (CPU puro):
        │                                  vista desde la luz, proyección ortográfica
        │                                  y frustum de foco posicional/direccional
        ├── Input/
        │   ├── EditorInput.h/.cpp       ← callbacks GLFW de teclado/mouse + máquina de
        │   │                              movimiento de cámara (extraídas de main.cpp).
        │   │                              Traduce los eventos y no dibuja: el estado de la
        │   │                              interfaz le llega por IEstadoInterfaz
        │   ├── IEstadoInterfaz.h        ← estado de la interfaz que consulta el input (¿el
        │   │                              teclado es de un widget?) y su motivo
        │   ├── EstadoOrbita.h           ← datos de la órbita de la cámara que la GUI dibuja
        │   └── AtajosEditor.h           ← reglas de los atajos que compiten con el teclado
        │                                  de la interfaz
        ├── ImGui/                        ← Dear ImGui v1.x integrado (+ backends glfw/opengl3)
        ├── Matematicas/
        │   └── StructVec3.h/.cpp         ← vec3 propio
        ├── Mods/
        │   ├── ManifiestoMod.h/.cpp      ← parseo y validación de ManifiestoMod.json
        │   ├── ModVersion.h/.cpp         ← versiones X.Y.Z y rangos (versionMotor/versionMin)
        │   ├── LectorZip.h/.cpp          ← manifiesto desde el .fmod y extracción anti zip-slip
        │   ├── GestorMods.h/.cpp         ← descubrimiento en Mods/, orden topológico, ciclos,
        │   │                                ámbito motor/proyecto y reporte a diagnósticos
        │   ├── ApiMod.h                  ← API pública del mod (POD, tabla de punteros a
        │   │                                función; append-only, y en la tabla de servicios
        │   │                                `version` va primero para que su offset no se mueva)
        │   ├── CargaDinamica.h           ← macros multiplataforma de carga de bibliotecas
        │   │                                compartidas (dlopen/LoadLibrary), compartidas con
        │   │                                Behaviour/Backends/BackendCpp.cpp
        │   ├── CargadorMods.h/.cpp       ← carga nativa: extrae el .fmod a la caché, abre el
        │   │                                binario, pide la interfaz por `funshi_mod_api` e
        │   │                                inicializa; descarga en orden inverso al salir
        │   ├── RegistroPanelesMods.h/.cpp ← registro de los paneles de GUI que aportan los
        │   │                                mods (sin ImGui: testeable headless). Atribuye cada
        │   │                                panel al mod que lo registró y guarda su estado
        │   │                                abierto/cerrado por clave de ventana
        │   ├── ServiciosMods.h/.cpp      ← tabla de servicios que el motor entrega al mod
        │   │                                (POD, punteros a función). Solo inyecta lo que
        │   │                                recibe: fuente de datos de la escena, nombre del
        │   │                                proyecto y contexto de ImGui
        │   └── DatosEscenaMods.h/.cpp    ← fuente de datos real de la escena para los mods
        │                                    (única unidad que conoce GameObject/GameScene)
        ├── Rendering/
        │   ├── SceneRenderer.h/.cpp       ← renderizador de la escena: skybox, modelos,
        │   │                                  iluminación, horizonte común de grilla/render,
        │   │                                  ejes y gizmos (pipeline de líneas) y el pase
        │   │                                  de sombras (elige proyectores, dibuja la escena
        │   │                                  desde cada luz y deja los mapas para el shader)
        │   ├── MeshRenderer.h/.cpp         ← dibujo de meshes con difuminado radial al horizonte
        │   │                                  y el pase de profundidad del mapa de sombras
        │   ├── TilemapRenderer.h/.cpp      ← sube la malla horneada del Tilemap a GPU (caché
        │   │                                  por revisión) y la dibuja con el shader de sprites
        │   ├── MeshGPU.h/.cpp              ← wrapper RAII de VBO/VAO/EBO
        │   ├── DibujoModelo.h/.cpp         ← helpers de render por modelo
        │   ├── LineBatch.h/.cpp            ← batch GPU de líneas expandidas
        │   ├── LineBuilder.h/.cpp          ← expansión de segmentos a quads
        │   ├── LineRenderer.h/.cpp         ← API de dibujo de líneas (grilla, ejes, gizmos,
        │   │                                wireframes de los colliders): es la única vía de
        │   │                                dibujo de líneas (VBO/VAO + shader de líneas
        │   │                                gruesas; el glLineWidth quedó eliminado)
        │   ├── ParticulasRenderer.h/.cpp   ← dibuja el lote de particulas: agrupa por
        │   │                                perfil (cada perfil tiene su textura) y
        │   │                                sube un lote por grupo
        │   ├── UIRenderer.h/.cpp          ← dibuja la interfaz de juego nativa con el
        │   │                                draw list de ImGui (el unico punto del sistema
        │   │                                que sabe de ImGui) y cachea las texturas
        │   ├── GrillaRenderer.h/.cpp       ← geometría/lógica de grilla infinita
        │   ├── GuiaEje.h/.cpp              ← guía de eje con contraste
        │   ├── TextureGL.h/.cpp            ← wrapper de textura 2D + cubemap
        │   ├── RenderTarget.h/.cpp         ← FBO (viewport/vista previa)
        │   ├── Preview/                   ← vista previa de material (la esfera de muestra)
        │   │   ├── IPreviewMaterial.h     ← contrato para la GUI: una vista por clave
        │   │   │                              (la textura y su estado de "sucia")
        │   │   └── PreviewMaterial.h/.cpp ← render target propio + renderer con luces
        │   │                                fijas: dibuja la esfera de muestra con el
        │   │                                material que se está editando
        │   ├── Cielo.h/.cpp                ← cielo degradado
        │   ├── Difuminado.h/.cpp           ← cálculo de difuminado radial
        │   ├── CacheCubemap.h             ← caché de caras del cubemap
        │   ├── FondoEscena.h              ← color de fondo del perfil de apariencia y limpieza
        │   │                                del framebuffer (color y profundidad) al inicio del
        │   │                                frame; lo usan el editor y el juego exportado
        │   ├── Backend/                    ← abstracción de backend gráfico (Strategy)
        │   │   ├── IRenderBackend.h        ← contrato (handles opacos, MeshData, Image2D,
        │   │   │                              DepthFunc, recursos GPU, estados, FBO,
        │   │   │                              objetivo de profundidad del mapa de sombras)
        │   │   ├── OpenGL3Backend.h/.cpp   ← implementación OpenGL 3.3 core
        │   │   └── GLFuncs.h                ← carga perezosa de funciones GL
        │   ├── Sombras/                    ← mapa de sombras de las luces
        │   │   └── MapaSombras.h/.cpp     ← selección de proyectores (luces que proyectan)
        │   │                              y dueño de los objetivos de profundidad por luz
        │   └── Shaders/                    ← sistema de shaders (gestor + GLSL embebido)
        │       ├── GestorShaders.h/.cpp   ← dueño de los programas: uno por nombre,
        │       │                              compilado una vez y compartido
        │       ├── PreprocesadorShaders.h/.cpp ← resuelve #include "..." en CPU
        │       │                              (GL 3.3 no tiene ARB_shading_language_include)
        │       ├── ShaderProgram.h/.cpp    ← programa compilado (backend inyectado)
        │       ├── ShaderException.h      ← errores de compilación e inclusión
        │       └── ShaderSources.h        ← GLSL embebido (inline) de los 6 programas
        │                                  (malla, linea, sprite, cielo, cubemap y el de
        │                                  profundidad del mapa de sombras)
        ├── Objetos/
        │   ├── GameObject.h/.cpp         ← id, nombre, tag, estado, update y serialización binaria
        │   ├── TagRegistry.h             ← registro dinámico de tags para el Inspector
        │   ├── GameObjectFactory.h/.cpp
        │   ├── Prefab.h/.cpp            ← formato .prefab (magic PREB, arbol recursivo con componentes/hijos)
        │   ├── PrefabLibrary.h/.cpp     ← cache + escaneo Assets/Prefabs/, crear/obtener/listar/borrar/renombrar
        │   ├── ClonadorObjetos.h/.cpp   ← copia profunda por roundtrip de serializacion (scripts + Ctrl+C/V)
        │   ├── SimpleObject.h            ← GameObject sin geometría (Transform/Grid/Light...)
        │   ├── Modelos3D.h/.cpp          ← carga Assimp y datos de la malla (el dibujado
        │   │                                es de SceneRenderer, vía MeshRenderer)
        │   └── Componentes/
        │       ├── Component.h           ← interfaz base polimórfica (serialize/deserialize)
        │       ├── ComponentFactory.h/.cpp ← creación por nombre (GUI y deserialización)
        │       ├── CatalogoComponentes.h/.cpp ← tabla única de tipos de componente: nombre,
        │       │                            dimensión, submenú, icono, requisitos, creador y panel
        │       ├── Transform.h/.cpp      ← local/global con GLM; anti-NaN; flag gizmoHabilitado
        │       ├── CameraComponent.h/.cpp ← cámara componente: vista, FPS, flag de vista previa
        │       ├── Material.h/.cpp       ← AMBIENT/DIFFUSE/SPECULAR/EMISSION/SHININESS
        │       ├── Materiales.h/.cpp     ← materiales por region: una entrada por parte de
        │       │                            la malla (el nombre con el que el archivo
        │       │                            declara el material) con su material
        │       ├── IMaterial.h           ← contrato de material (colores y texturas)
        │       ├── IResolutorMateriales.h ← contrato: objeto + region → material
        │       ├── ResolvedorMateriales.h/.cpp ← decide el material de cada region y el
        │       │                            material unico del objeto
        │       ├── Light.h/.cpp          ← luz puntual serializable
        │       ├── Sombras/Sombras.h/.cpp ← ajustes de la sombra que proyecta la luz de su
        │       │                           objeto: resolución, sesbio, fuerza, radio y distancia
        │       ├── Grid.h/.cpp           ← grilla del suelo: pasada independiente, visible/color
        │       │                            (los campos tam/separacion quedan solo para que las
        │       │                            escenas viejas se lean igual; no se dibujan con ellos)
        │       ├── Skybox.h/.cpp         ← cubemap de 6 caras que reemplaza al cielo degradado
        │       ├── SpriteRenderer.h/.cpp ← quad texturizado con UV recortable (planilla/frames)
        │       ├── Animador2D.h/.cpp     ← anima la region UV del SpriteRenderer sobre una grilla
        │       ├── Tilemap.h/.cpp        ← capas de tiles (visible/colision), pintado, malla
        │       │                            horneada cacheada por revision y serial binaria FTM1
        │       ├── Color.h / Model.h / Script.h ← varios scripts por objeto,
        │       │                                    nombre editable y metadatos reflejados
        │       │                                    serializados para el inspector
        │       ├── AudioSource.h/.cpp    ← fuente de sonido: ruta del archivo, volumen y loop
        │       ├── Musica.h/.cpp        ← reproductor de música: lista ordenada de pistas
        │       │                            (rutas), volumen, fundido y repetición de la lista
        │       ├── EmisorParticulas.h/.cpp ← emisor de partículas: tasa, vida, velocidad,
        │       │                            tamaño, color, cono de emisión y perfil de
        │       │                            efecto opcional (serial FPT1)
        │       ├── UIComponent.h/.cpp     ← interfaz de juego nativa (HUD que dibuja el
        │       │                            motor): lista de widgets con su colocacion
        │       │                            (serial UIJ1, con tamaño de bloque acotado)
        │       ├── RigidBody/RigidBody.h/.cpp ← cuerpo Bullet sincronizado, habilitable, con detección de apoyo (RAII); masa, gravedad por cuerpo (uso+escala), friccion y freeze por ejes via setGravity/setFriction/setLinearFactor/setAngularFactor, serial v3 con lectura de v1 y v2
        │       └── Colliders/
        │           ├── Collider.h/.cpp   ← base abstracta; radio; visibilidad persistente y gizmo
        │           │                        con wireframe por el batch de líneas
        │           ├── EsfereCollider.*  ← btSphereShape
        │           ├── CubeCollider.*    ← btBoxShape (half extents = radio)
        │           ├── MallaCollider.*   ← btConvexHullShape a partir de la malla
        │           ├── TilemapCollider.*  ← btCompoundShape (una caja por tramo de tiles solidos)
        │           └── TiposShapeBullet.h ← identifica las shapes por su "shape type",
        │                                   sin RTTI (Bullet se compila sin ella: vcpkg
        │                                   instala bullet3 con USE_MSVC_DISABLE_RTTI=ON)
        ├── Proyectos/
        │   ├── GestorDeProyectos.h/.cpp  ← ciclo de vida del proyecto activo (entrar,
        │   │                              guardar estado y escena Ctrl+S, renombrar, eliminar,
        │   │                              exportar); extraído de main.cpp
        │   └── LayoutInterfazProyecto.h/.cpp ← layout de la interfaz del proyecto (imgui.ini):
        │                                  qué archivo usa ImGui, volcado al abandonar un
        │                                  proyecto y carga diferida del entrante
        ├── Runtime/
        │   └── main_juego.cpp            ← punto de entrada de juegos exportados (sin editor:
        │                                  GUIManager, MenuGUI, paneles). ImGui básico para runtime,
        │                                  carga escenas desde estructura simplificada (scenes/),
        │                                  limpia el framebuffer al inicio de cada frame. Lo
        │                                  compila el target `runtime-compile-check` del motor
        │                                  (el exportador genera su propio proyecto, pero el
        │                                  motor lo compila igual para que sus errores no
        │                                  aparezcan al exportar)
        ├── Comandos/
        │   ├── IComando.h              ← interfaz Command: ejecutar(), deshacer(), descripcion()
        │   ├── GestorComandos.h/.cpp   ← pilas undo/redo (máx 50), ejecuta/deshace/rehace
        │   ├── CrearObjetoComando.h/.cpp ← crea cualquier GameObject (modelo o simple)
        │   ├── BorrarObjetoComando.h/.cpp
        │   ├── DuplicarObjetoComando.h/.cpp ← Ctrl+C/V por la puerta del editor
        │   ├── ReparentarComando.h/.cpp
        │   ├── TransformComando.h/.cpp
        │   ├── AgregarComponenteComando.h/.cpp
        │   ├── QuitarComponenteComando.h/.cpp
        │   ├── AjustarVariableComando.h/.cpp ← escribe una variable de un componente
        │   │                                (los accesores se inyectan: el comando no
        │   │                                depende del catálogo de propiedades)
        │   ├── EstablecerTagComando.h/.cpp ← fija la etiqueta (tag) de un objeto
        │   └── LimpiarEscenaComando.h/.cpp
        ├── Inspeccion/                  ← API publica del motor para herramientas
        │   │                              externas (la CLI, que es un proyecto aparte)
        │   ├── ApiEscena.h              ← contrato (IApiEscena) + versión propia
        │   │                                (versionApiEscena, independiente de la
        │   │                                de los scripts) y los tipos de las vistas
        │   ├── TextoValor.h/.cpp        ← conversiones texto <-> valor (numero,
        │   │                                booleano, vector) sin dependencias del motor
        │   ├── PropiedadesComponentes.h/.cpp ← catálogo de las variables de cada
        │   │                                tipo de componente (getter/setter por
        │   │                                variable); es la lista que antes solo
        │   │                                existía dentro de los paneles de la GUI
        │   └── InspeccionEscena.h/.cpp   ← implementación sobre SceneRegistry y
        │                                    EditorController; toda mutación pasa por
        │                                    el gestor de comandos
        ├── Particulas/                 ← sistema de particulas (CPU puro)
        │   ├── Particula.h             ← estado de una particula viva (dato plano)
        │   ├── ParametrosEmisor.h      ← los datos que edita el inspector
        │   ├── PerfilParticula.h/.cpp  ← perfil de efecto: curvas de tamano y color
        │   ├── LectorPerfil.h/.cpp     ← carga del perfil de efecto (JSON)
        │   ├── SistemaParticulas.h/.cpp ← simulacion: emision, integracion, muerte
        │   │                              y presupuesto de particulas vivas
        │   └── ParticulasBuilder.h/.cpp← expansion de cada particula a un quad
        │                                  (CPU puro, como LineBuilder)
        ├── UI/                          ← interfaz de juego nativa (HUD del motor)
        │   ├── TiposUI.h                ← tipo de widget, anclaje (9), vectores,
        │   │                               colores y rectangulo en pixeles
        │   ├── WidgetUI.h               ← datos de un widget: identificador, tipo,
        │   │                               anclaje, desplazamiento, tamano, colores,
        │   │                               texto, ruta de textura y visibilidad
        │   ├── LienzoUI.h/.cpp          ← lista de widgets y colocacion en pantalla
        │   │                              (se recalcula al cambiar el tamano)
        │   ├── EntradaUI.h              ← posicion y estado del raton de un frame
        │   └── SistemaUI.h/.cpp         ← estado de interaccion: que widget tiene el
        │                                  raton encima, cual se pulso (CPU puro)
        ├── WebUI/
        │   ├── VistaWebUI.h             ← una interfaz web ya montada por un backend
        │   │                               (estado opaco + ruta .html + sello mtime +
        │   │                               textura/handle + área)
        │   ├── IWebUIBackend.h          ← contrato del motor web (Strategy): subproceso,
        │   │                               init/bombeo/apagado globales, cargar,
        │   │                               redimensionar, actualizar, estado/salida,
        │   │                               render, input (ratón/tecla/rueda/foco),
        │   │                               descargar
        │   ├── BackendNull.h            ← backend sin motor (no-op): corre el ciclo
        │   │                               completo headless, sin CEF ni GL (tests/CI)
        │   ├── BackendCef.h             ← backend CEF real (pimpl sin headers CEF,
        │   │                               el motor en C++17 nunca ve a CEF)
        │   ├── BackendCef.cpp           ← única unidad con CEF (C++20, target
        │   │                               funshi_webui_cef): OSR, textura, mapa
        │   │                               GLFW→VK, sentinela de salida JS
        │   ├── WebUIRuntime.h/.cpp      ← registro + selección de backend (por config,
        │   │                               con fallback al nulo) y despacho del ciclo
        │   │                               global (subproceso/init/pump/apagado) y de
        │   │                               cada vista
        │   ├── EstadoInterfaces.h/.cpp  ← estado de runtime por interfaz: dos árboles JSON
        │   │                               (estado motor→JS y salida JS→motor) por rutas JSON
        │   │                               Pointer; lógica sin GL/backend (pimpl + nlohmann)
        │   └── VistasWebUI.h/.cpp       ← vistas vivas de una escena, una por interfaz
        │                                   activa: carga, recarga por cambio de .html
        │                                   o por mtime en disco, redimensiona,
        │                                   entrega el estado y recoge la salida,
        │                                   y descarga (lógica sin GL)
        └── Scenes/
            ├── GameScene.h/.cpp          ← coordinador del frame: render, GUI, física, gizmo,
            │                                previews y pasada de la grilla
            ├── InterfacesJuego.h/.cpp    ← recolección de las interfaces de juego (HUD) de la
            │                                escena: recorre TODOS los objetos y devuelve las
            │                                InterfaceComponent activas y con ruta, ordenadas por
            │                                z (lógica pura, sin GL ni GUI, probada headless)
            ├── GizmoController.h/.cpp   ← gizmo ImGuizmo + picking (desde GameScene): estado
            │                                (operación/global/listo), arrastre con
            │                                TransformComando (historial) y congelación de
            │                                hijos; interacción vía ImGuizmo + raycast AABB
            ├── SceneRegistry.h/.cpp      ← ownership único (unique_ptr) + árbol + vista lineal
            ├── EditorController.h/.cpp   ← mutaciones + GizmoTarget + registro de física
            ├── SceneSerializer.h/.cpp    ← save/load binario preorden con marcadores =>/<=
            ├── RutasReescritura.h/.cpp   ← reescribe referencias (model/textura/script) al
            │                                mover/renombrar assets en el explorador y sanea
            │                                al cargar las que apuntan a un archivo que ya no
            │                                existe (una coincidencia del nombre bajo la raíz
            │                                → repara y guarda; varias/ninguna → avisa)
            ├── ManifiestoAssets.h/.cpp   ← manifiesto SceneAssets.json (add-on del .db):
            │                                guarda/carga rutas de asset por objeto con
            │                                precedencia sobre el binario (Ctrl+S)
            ├── ManifiestoAssetsCore.h/.cpp ← núcleo puro headless (JSON + relativizar/
            │                                absolutizar + precedencia, con las 6 caras del
            │                                Skybox); tests propios
        └── States/
            ├── ApplicationStateMachine.h/.cpp ← MainMenu/Editing/Debugging/Playing/Exiting
            └── OrquestadorEstadoGUI.h/.cpp    ← reglas de transición menú↔editor y de
                                                   Depuración/Juego: pausa, reset, término y
                                                   Escape; headless, con tests propios
                                                   (orquestador-estado-tests)
```

---

## 3. Flujo de ejecución

```text
main.cpp
  ├── Ventana (GLFW init: contexto 3.3 core + carga dura de GLFuncs)
  ├── ImGui init (backends glfw + opengl3; imgui.ini gestionado por el gestor)
  ├── GUIManager (crea el menú y las ventanas y las registra; posee FileManager)
  ├── GameScene(guiManager)
  │   ├── SceneRegistry            ← ownership de objetos
  │   ├── PhysicsEngine (PIMPL)    ← mundo Bullet vía BulletPhysicsAdapter
  │   ├── EditorController         ← mutaciones, gizmo, registro de física
  │   ├── SceneSerializer          ← persistencia
  │   └── EventBus                 ← notificaciones
  ├── GestorDeProyectos            ← ciclo de vida del proyecto activo (entrar/guardar/
  │                                  renombrar/eliminar/exportar + imgui.ini)
  ├── EditorConfig                 ← carga JSON y aplica a menú/GUI/escena
  ├── ApplicationStateMachine      ← MainMenu / Editing / Debugging / Playing / Exiting
  └── bucle principal
      ├── glfwPollEvents
      ├── EngineTime::update (deltaTime)
      ├── ImGui::NewFrame
      ├── refleja el estado del menú en la fachada MenuGUI (guardia de cambio)
      ├── refleja en la escena el modo (Depuración/Juego), start y pausa
      ├── GameScene::update(dt) SIEMPRE: al iniciar guarda un estado serializado
      │   de la escena; Reset lo restaura con la física reconstruida. Al terminar
      │   ambos modos vuelven al estado previo a la simulación.
      │   En ambos modos avanzan física/scripts/audio y F6 pausa sin reinicio.
      ├── pasada de la grilla (Grid + batch de líneas + shader de ancho en
      │   píxeles; solo fuera de Juego, color efectivo según el perfil de
      │   apariencia y referencia de contraste para la guía de eje)
      ├── dibujarGameObjects (MeshRenderer VBO/VAO+shader; único pipeline)
      ├── gizmo ImGuizmo y overlays auxiliares solo fuera de Juego
      ├── GUI() de GameScene + DockSpace siempre enviados durante edición y
      │   simulación; E solo oculta paneles, y las ventanas flotantes se
      │   reanclan a su último dock válido sin reconstruir los splits
      ├── GestorDeProyectos::sincronizar/eliminar (cambios de proyecto desde el menú)
      ├── ImGui::Render + swap buffers
      └── al salir: GestorDeProyectos::guardarProyectoCompleto (escena + config)
```

`main.cpp` es el composition root; las callbacks de teclado y ratón de GLFW
viven en el módulo `src/Input/EditorInput` (`EditorInput::registrarCallbacks`,
instancia única creada por `main`), que las traduce a acciones del editor y
mantiene la máquina de estado del movimiento de cámara. Ese módulo no dibuja
nada: para saber si el teclado o el mouse pertenecen a un widget consulta el
estado de la interfaz por `IEstadoInterfaz` (implementado sobre ImGui en
`GUI/Estado/EstadoInterfazImGui`), y el dibujo de la barra de la órbita vive en
`GUI/Navegacion/SidebarOrbitaGUI`, que recibe el estado (`Input/EstadoOrbita`)
en lugar de leer el input. El estado del menú lo
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
  `SceneCleared`, `ObjectSelected` y `ComponentStructureChanged` (alta o baja de
  componente; el inspector reconcilia solo con este último). No es global: vive
  dentro de `GameScene`.
- `LightSystem` es el dueño del estado GL de luces: cada frame escanea los objetos,
  toma los componentes `Light` y parametriza los slots `GL_LIGHT0..7`. No queda
  lógica de luz en el bucle ni en los componentes.
- `ApplicationStateMachine` modela `MainMenu`, `Editing`, `Debugging`, `Playing`
  (Juego) y `Exiting`. `OrquestadorEstadoGUI` decide Escape, Iniciar Estudio,
  F5/Depuración, Juego, pausa, Reset y término; `GameScene` conserva un snapshot
  serializado al iniciar y reconstruye física y referencias al restaurarlo.
  `menuActivo` solo gobierna paneles GUI; el flag de simulación refleja los dos
  modos en `GameScene`.
- `EditorController` posee un `GestorComandos` que envuelve cada mutación
  (crear/borrar/reparentar, cambios de transform, agregar/quitar componentes,
  limpiar escena) en un `IComando`. Las operaciones de la GUI van por el
   gestor, nunca directamente al `SceneRegistry`, de modo que Ctrl+Z/Ctrl+Y
   funcionan de forma transversal. La pila mantiene un máximo de 50 comandos;
   cada nueva acción invalida la pila de redo. La raíz de la escena (id=0)
   está excluida de delete/clear por diseño.
- Equivalencia GUI ↔ CLI (contrato): toda operación de la GUI de edición debe
  pasar por un `IComando` con parámetros explícitos (los que ya existen:
  crear/borrar/reparentar, transform, agregar/quitar componentes, ajustar una
  variable, etiquetar, limpiar escena), de modo que cada acción del editor
  tenga un equivalente invocable sin GUI. Esto habilita un editor por línea de
  comandos que manipula la creación del videojuego con los mismos comandos, sin
  interactuar con la GUI. Regla: ninguna mutación de escena desde la GUI salta
  el gestor; un comando nuevo se agrega con su clase en `Comandos/` y su caso
  en `tests/ComandosTests.cpp`.
- Sistema de partículas (`src/Particulas/`): la simulación es **CPU pura**
  (no toca OpenGL ni glm), así que se puede probar entera en los targets
  headless, igual que `LineBuilder`.
  - `SistemaParticulas` emite, integra (rozamiento y gravedad), retira las
    partículas sin vida y acota cuántas hay vivas: al llegar al límite se
    descarta la más vieja, de modo que el coste por frame no depende de lo que
    se edite en el inspector. El azar sale de un `mt19937` con semilla, y el
    número real se arma a mano (no con `uniform_real_distribution`, que no
    está especificada y difiere entre implementaciones): la simulación es
    reproducible.
  - `ParticulasBuilder` expande cada partícula a un quad de dos triángulos con
    el tamaño y la rotación ya aplicados: el vertex shader solo proyecta. Es
    el mismo patrón que `LineBuilder` (un VBO interleaved, sin EBO).
  - El **perfil de efecto** es el guion de una partícula a lo largo de su
    vida (textura, vida, velocidad, gravedad y las curvas de tamaño y color).
    Lo guarda el componente emisor y la partícula lo comparte, así que sigue
    vivo aunque el emisor desaparezca de la escena.
  - El componente `EmisorParticulas` guarda los datos del emisor; su estado de
    ejecución (el acumulador de emisión) no se serializa, igual que hace
    `Animador2D` con su fotograma. `GameScene` es quien avanza el sistema y
    pide a cada emisor que suelte las suyas: la simulación corre también en el
    editor, no solo en juego.
  - El renderer (`src/Rendering/ParticulasRenderer.*`) no conoce el emisor:
    recibe el sistema y dibuja lo que hay, como los demás renderers.
  - El backend graphics incorporaron un lote de partículas y un **modo de
    mezcla aditivo** (`BlendMode`): el fuego y las chispas suman el color en
    vez de fundirse con el fondo.
- Interfaz de juego nativa (`src/UI/`): el HUD que dibuja el motor, sin
  navegador web. Convive con la interfaz web (`InterfaceComponent`): son dos
  sistemas distintos y una escena puede tener de los dos.
  - El módulo es **CPU puro** (no toca OpenGL, glm ni ImGui), así que se
    prueba entero en los targets headless, igual que `src/Particulas/`.
    `LienzoUI` coloca los widgets (los nueve anclajes, desplazamiento y
    tamaño) y `SistemaUI` responde qué widget tiene el ratón encima y cuál se
    pulsó. El estado de interacción se reconstruye cada frame: no se serializa.
  - El componente `UIComponent` guarda el lienzo (lo que se serializa) y el
    estado vivo (lo que no). Es un tipo nuevo, no una ampliación de
    `InterfaceComponent`: el nombre del tipo persistido **es** el formato de
    datos, así que ampliar el componente web habría obliged a versionar su
    payload binario.
  - El renderer (`src/Rendering/UIRenderer.*`) es el **único** punto del
    sistema que sabe de ImGui: traduce los rectángulos del layout a
    `AddRectFilled` / `AddText` / `AddImage` del draw list de primer plano.
    No conoce el componente: recibe el lienzo ya colocado.
  - El ratón llega al HUD por el `ImGuiIO` y lo procesa `GameScene` antes de
    que corran los scripts, para que un script vea el clic en el mismo frame.
    Cuando el puntero está encima de un botón, el juego deja de girar la
    cámara con el ratón (lo consulta `EditorInput` a la escena).
  - La notificación de los clics es **por consulta**, no por callback: el
    motor solo llama a los scripts, nunca al revés.
- API de inspección (`src/Inspeccion/`): contrato público del motor para
  herramientas externas. La CLI (`FunshiEngineCLI`) es un **proyecto aparte**
  que consume este contrato; el motor publica la API y la CLI la usa.
  - `IApiEscena` (`ApiEscena.h`) expone: listar objetos con filtros
    combinables (componente, etiqueta, id, nombre exacto o parcial, tipo de
    objeto y límite), ver un objeto, ver los componentes de un objeto con sus
    variables y valores, crear/quitar componentes, asignar una variable,
    consultar la posición global, crear objetos, etiquetarlos, guardar y
    cargar la escena. `versionApiEscena` versiona este contrato y es
    **independiente** de `versionRuntimeScript` (la caché de scripts): subir una
    no invalida la otra.
  - `PropiedadesComponentes` es el catálogo de las variables de cada tipo de
    componente (nombre, tipo, valores admitidos y el par getter/setter). Antes
    esa lista no existía: cada panel de la GUI editaba su componente con su
    propio cast, y no había forma de leer o escribir una variable por nombre.
    El acceso es siempre por los getters y setters públicos del componente,
    nunca por el dato interno: `Transform` guarda dos copias de su estado
    (la que serializa y la que usa el render) y solo los setters las
    mantienen sincronizadas.
  - `TextoValor` traduce entre texto y valor (entero, flotante, booleano,
    vectores y enumeraciones por su nombre). Un valor se **valida antes** de
    encolar el comando, así que un texto inválido no deja la escena a medias.
  - Toda mutación de la API pasa por el `GestorComandos`, de modo que se
    deshace igual que en el editor. Como `GestorComandos::ejecutar` no puede
    fallar, la API valida antes de encolar (objeto existente, tipo de
    componente conocido, prerrequisito del catálogo cumplido, valor
    interpretable) y devuelve el motivo del rechazo.
  - La escena se guarda y se carga con un **prefijo** de ruta
    (`<prefijo>BBDDObjetos.txt` es el índice y `<prefijo>/` la carpeta con un
    binario por objeto), igual que hace el motor al guardar un proyecto.
- Settings manipulables por código: los datos públicos que muestran los
  paneles de componentes (`Settings*`) deben tener su contraparte escribible
  por código: setters del componente y/o entradas en las tablas de scripts
  (`ApiScriptGameObject`/`ScriptServices`, append-only). Así lo que se ajusta
  con sliders/checkboxes en el inspector también se genera y manipula desde
  scripts, y la creación de objetos por código no depende de la GUI.

### Entidades, objetos y componentes

- `Entity` es la base real de la escena: es duena de sus componentes
  (`ListaDE<Component*>` más los propietarios), conoce su lugar en la jerarquía
  y su transform de origen, e implementa la parte común de la serialización
  (el orden de los bloques, el transform y la lista de componentes) y el
  transform global. El derivado solo aporta los atributos que son suyos
  (`serializeLocalAtributes`/`deserializeLocalAtributes`).
- `GameObject` extiende `Entity` e implementa `Comparable<GameObject>`. Añade:
  id entero, nombre (`inputName[25]`), color auxiliar (`auxColor`), estado (`bool`),
  ciclo `update(dt)` y la lógica de serialización binaria concreta.
- `Modelos3D` extiende `GameObject`. Carga geometría (vértices, normales, índices)
  mediante Assimp y serializa adicionalmente la ruta del archivo del modelo. No
  tiene método de dibujado: la entidad no conoce la capa de Rendering, y es
  `SceneRenderer` quien la pinta vía `MeshRenderer`.
- `GameObject` no incluye los componentes concretos: los declara y quien los
  usa incluye su cabecera. Asi el dominio no arrastra el componente de fisica
  (ni Bullet) ni el de modelo (ni Assimp). El tipo plano `color` (RGBA) vive en
  `TypeUtils.h` para que el objeto no dependa del componente `Color`.
- Los componentes concretos son: `Transform`, `Color`, `Model`, `Material`, `Light`,
  `AudioSource`, `Musica`, `InterfaceComponent`, `Grid`, `Script`,
  `EsfereCollider`, `CubeCollider`, `MallaCollider` y `RigidBody`. Todos heredan
  de `Component` y serializan sus datos binarios.
- `ComponentFactory` centraliza la creación por nombre de tipo tanto desde la GUI
  como durante la deserialización (`"CameraComponent"` acepta el alias `"Camera"`).
- El catálogo declara además el **icono** de cada tipo de componente (la ruta
  del PNG dentro de `Imagenes/GUI-Image/Component-Image/`). El icono se
  **deriva del tipo** y no se serializa: el nombre del tipo ya es el dato
  persistido de la escena, así que guardarlo también sería duplicar la fuente
  de verdad. El inspector lo pinta a la izquierda del nombre del componente,
  resuelto por `IResolutorIconos` (la regla lee el catálogo; la textura se
  carga en GPU a demanda).
- `Component` declara dos hooks de serialización (`serializeComponent` y
  `deserializeComponent`): son los dos métodos que implementa cada componente. La
  base ofrece `saveComponent` y `loadComponent` como entrada del motor, que
  reenvían a los hooks y no se reimplementan, así que la serialización de un
  componente vive en un solo sitio.
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
  —crear/renombrar/eliminar/copiar/**mover**— **más toda la E/S nativa del
  sistema**:
  diálogos de selección de carpeta/archivo, abrir con la app predeterminada,
  listado de directorio con symlink-safe y plantillas de scripts C++/Java),
  `FileSelection` (estado de navegación compartido) y las vistas
  `TreeFilesInterface`/`ContentFolderInterface`, que solo conversan con la
  fachada (sin `system()`/`popen()`/`ShellExecute*` ni `directory_iterator`
  propios).
  `FileSystemWatcher` avisa de cambios externos (inotify) para re-escanear.
  El **arrastre** lo resuelve `SoltarEnCarpeta.h`, compartido por las dos
  vistas: dentro del grid soltar mueve (con `Ctrl` copia) y sobre una fila del
  árbol siempre mueve (cortar y pegar, sin opción de copia); al mover, publica
  `ArchivosReubicados` para que el gestor de proyectos reescriba y guarde las
  referencias de la escena; el cotejo de ese prefijo vive en
  `PathUtils::rutaBajo` y trata `/` y `\` como el mismo separador en Windows,
  porque el explorador trae las rutas de `std::filesystem` con `\` y
  la escena las resuelve con `/`. El **renombre por click derecho** (grid y
  árbol) vive en `RenombrarElemento.h`: mismo modal en los dos paneles y un
  único camino de disco que, si el cambio se hizo, publica el mismo
  `ArchivosReubicados`; quien renombra sigue siendo dueño de su navegación (el
  árbol reubica `rutaVisible` y la expansión por ruta de la rama antes del
  rescaneo). `GestorDeArchivos::renombrar` también se niega a pisar un destino
  existente (mismo criterio que mover, permitido solo el MISMO elemento, p. ej.
  para cambios de mayúsculas); `GestorDeArchivos::mover` usa `rename`, que es
  atómico, y solo cae a copiar+borrar si el destino está en otro volumen;
  rechaza pisar un destino ocupado y meter una carpeta en sí misma. Al cargar
  la escena, `GameScene::loadScene` invoca el mismo módulo para sanar las
  referencias rotas (busca el nombre bajo la raíz del proyecto): con una sola
  coincidencia repara y guarda; con varias o con ninguna avisa en el log y no
  adivina.
  Lo que viene de **fuera** del proyecto (el arrastre desde el explorador del
  sistema y el pegado con `Ctrl+V` desde el portapapeles del sistema) entra por
  dos contratos, `IServicioDrop` e `IServicioPortapapeles`, que el editor
  inyecta en la fachada: el módulo de archivos no conoce ni la API del sistema
  ni GLFW. `ReceptorDropGlfw` encola las rutas del drop (el sistema las
  entrega fuera del frame de la GUI) y `PortapapelesSistema` las lee del
  portapapeles de la plataforma (`CF_HDROP` en Windows, `text/uri-list` en
  Linux), con los parsers puros en `PortapapelesParsers.h`. La política de
  importación vive en `ImportarArchivos.h`: copia sin pisar (genera un nombre
  único), copia las carpetas recursivamente y un origen fallido no aborta el
  resto.
- `SceneSelectedInterface` observa la escena inyectada y delega las
  operaciones de edición a `EditorController`. Usa `GameObjectFactory` para crear objetos.
- `SceneObjectTree` dibuja el árbol de objetos (con drag & drop para reparentar).
- `SettingsObjectInterface` inspecciona un `GameObject` seleccionado y crea/actualiza
  los paneles `Settings*` específicos de cada componente presente. El checkbox
  **"Gizmo activo"** de `SettingsTransform` enciende/apaga el gizmo de ese
  transform (del objeto o del offset del collider) sin deseleccionar.
- `SceneMenuBarInterface` ofrece los controles de Depuración/Juego y, durante
  una simulación, Pausa/Reset/Terminar; las transiciones pasan por el orquestador.
- `MenuGUI` es la fachada del paquete `MenusGUI` (menú principal): `MenuModel`
  (lógica pura) + `MenuView` (ImGui) + `StartMenuPresenter` (puente motor).
  Ver `src/GUI/MenusGUI/README.md`.

### Persistencia

- `SceneSerializer` es el único responsable del archivo `BBDDObjetos.txt` y de los
  `.db` individuales. Guarda en preorden: escribe el binario de cada objeto y
  registra la ruta; inserta `=>` para abrir un bloque de hijos y `<=` para cerrarlo.
- La carga reconstruye la escena **de forma recursiva** (`loadPreOrder` con
  look-ahead de marcadores), restaurando padres e hijos; inserta cada objeto
  directamente en `SceneRegistry` vía `EditorController`. El índice se abre en
  **binario** (el `seekg(tellg())` del look-ahead no es fiable en modo texto
  con CRLF, ver `DocuTecnicoBugs.md` §7) y cada línea se valida contra el
  patrón estricto `ObjectN<entero>.db`: lo que no cumple — o un hijo con
  id 0, que leería `ObjectN0.db`, el binario de la raíz — se salta con aviso.
- `Binario` encapsula los streams binarios usados por las entidades.
- `GameObject::saveEntity/loadEntity` coordina la serialización binaria propia
  (atributos globales, locales, componentes).
- La configuración del editor se persiste en JSON con **una sola fuente de
  verdad**: `Configuracion/ConfigPersistence.{h,cpp}` (JSON puro, sin estado)
  sobre `Configuracion/ProjectPaths.{h,cpp}` (rutas), con
  `EditorConfig.{h,cpp}` como fachada estable que expone `datos()` y
  `cargar*/guardar*` a main, escenas y tests. El **ciclo de vida de los
  proyectos** (`asegurarEstructuraProyecto`, `asegurarEstructuraBase`,
  `renombrarProyecto`, `eliminarProyecto`) no se implementa en EditorConfig:
  son **delegaciones a `ProjectManager`**, único dueño del CRUD, las
  migraciones de estructura antigua y los fallbacks de copia entre
  dispositivos. La política de qué proyecto queda abierto al arrancar (menú
  salvo `--proyecto`, y si es primer arranque) vive aislada y sin
  disco ni UI en `Configuracion/ProyectoInicial.h`. La orquestación de todo el flujo de proyectos sobre esta
  fachada (qué hará al arrancar, entrar, guardar, renombrar, eliminar,
  exportar y cuál es el `imgui.ini` vigente) vive en
  `Proyectos/GestorDeProyectos` (extraído de `main.cpp`). Guarda dos archivos
  junto al binario (Linux y Windows), y cada campo va a uno u otro según si es
  una preferencia de la máquina o del proyecto abierto:
  `<directorioEjecutable>/FunshiEngine-DATA/Configuraciones/Configuracion.json`
  (**general**: último proyecto, idioma, sensibilidad de cámara, sensibilidad de
  movimiento y perfil de apariencia) y
  `Proyects/<proyecto>/Memory/ConfiguracionProyecto.json` (**por proyecto**:
  operación y sistema de coordenadas del gizmo, ventana de cámaras, estado
  abierto/cerrado de las ventanas de `GUIManager` y cámara activa por id).
  Tolerante a archivos ausentes o corruptos: los defaults
  viven en `EditorConfig.h`/`ConfigPersistence.h`.
  La raíz `FunshiEngine-DATA` no es siempre `<directorioEjecutable>/FunshiEngine-DATA`:
  `ProjectPaths::directorioBase()` la resuelve probando si puede escribirse y, si
  no, cae a la carpeta de datos del usuario (`%APPDATA%` en Windows,
  `$XDG_DATA_HOME` o `~/.local/share` en Linux). Ver "Resolución de la raíz de
  datos" más abajo.
  - **Escritura atómica**: `ConfigPersistence::escribirJson` escribe a
    `<archivo>.tmp` y renombra encima; un corte a mitad de escritura no deja
    el JSON cortado ni temporales colgados.
  - **Guardado diferido de la general**: `EditorConfig::solicitarGuardadoGeneral`
    encola los cambios en vivo de Opciones y `volcarGuardadoGeneral()` (llamado
    por main una vez por frame) escribe como máximo una vez cada
    `kIntervaloEscritura` (250 ms); `guardarGeneral()` (Ctrl+S, salida, reset)
    vuelca el pendiente sin esperar.
  - El layout `imgui.ini` también se guarda junto al proyecto (no en el CWD).
- Carpeta de un proyecto: `Proyects/<proyecto>/` con `Memory/` (escena,
  configuración del proyecto e `imgui.ini`) y la raíz de
  assets `src<proyecto>/` (meshes, texturas, materiales y las carpetas de
  assets que cree el usuario). Esas rutas salen de `ProjectPaths`
  (`directorioSrc`, `directorioScripts`), incluido el
  exportador del juego standalone, que las pide ahí mediante
  `Exportador/RutasExportacion.h` (lógica pura, header-only) en vez de
  componerlas desde la carpeta del proyecto: componiéndolas a mano las copias
  se saltaban en silencio y el juego exportado salía sin sonidos, sin scripts
  del usuario y sin configuración.
- El motor solo crea `Memory/`, `src<proyecto>/` y su carpeta global de
  `Mods/`: las carpetas de assets (`Sonidos/`, `Scripts/`, los `Mods/` del
  proyecto) las crea el usuario desde el explorador. Los assets se referencian
  por ruta, así que ninguno depende de una carpeta en concreto.
- Limitación conocida: la serialización binaria tiene versionado **parcial**
  (varios componentes llevan magic y versión y la escena valida el índice de
  forma estricta, pero no hay versionado global por tipo de componente ni
  validación formal de tamaños): un cambio en la estructura de atributos de un
  componente sin versión sigue invalidando escenas guardadas.

### Scripts dinámicos

- `Script` (componente) orquesta el ciclo de vida del módulo y guarda los valores de
  los campos (`SerializeField`) en la serialización binaria de la escena.
- `ScriptRuntime` registra los backends por extensión, compila/carga el fuente y
  aplica hot reload comparando la fecha de modificación (conserva y reinyecta los
  valores por nombre de campo).
- `IScriptBehaviour` define la interfaz: `onStart`/`onUpdate`/`onStop` y
  `camposReflejados()`; el motor inyecta la tabla `MotorScript::ApiScriptGameObject`
  (nombre, transform completo con getters de rotacion/escala, log y control
  horizontal/salto sobre `RigidBody`) y la tabla
  `MotorScript::ScriptServices` (audio y musica por ruta, busqueda de objetos
  por nombre, id o etiqueta y
  consulta de teclado via `InputScripts`, inyectadas por `GameScene` al iniciar
  la simulacion) para que el script no enlace contra el motor. Ambas tablas siguen
  versionado APPEND-ONLY con campo `version` final para guardas en runtime.
  Las posiciones de la API de transform son locales. Durante el cierre, GameScene
  conserva los servicios hasta completar `onStop`, detiene los handles de audio
  iniciados por scripts y luego desconecta el contexto, sin afectar la propiedad
  de audio de los componentes. `InputScripts` expone los edges del teclado y el
  delta del mouse en pixeles durante el frame actual.
-   `BehaviourReflection` implementa la reflexión por macros (`REFLECT_INICIO`,
  `CAMPO`, `ARRAY`, `GRUPO`, `GRUPOS`, `COMPONENTE`, `FIN`), la conversión de
  valores tipados y la serialización binaria autodescriptiva de los campos.
  `CAMPO`, `ARRAY`, `GRUPO`, `GRUPOS`, `COMPONENTE`, `FIN`), la conversión de valores
  tipados y la serialización binaria autodescriptiva de los campos.
- Una referencia a un componente de otro objeto (`TagTipo::Componente`, macro
  `REFLECT_COMPONENTE(campo, "Tipo")`) se serializa por el **nombre del objeto
  dueño**. El motor la resuelve con dos ganchos que inyecta `GameScene`
  (`fijarResolverComponentes`): nombre+tipo → puntero, y puntero → nombre dueño.
- La API cubre propiedades fisicas (masa, gravedad por cuerpo, friccion,
  freeze), gravedad global y gestion de objetos (crear/clonar/destruir,
  agregar collider y cuerpo) con cola diferida a fin de frame; `ClonadorObjetos`
  (copia profunda por roundtrip de serializacion) lo comparten los scripts y
  el Ctrl+C/V del editor (`EditorController::duplicarObjeto`).
- `BackendCpp` compila el `.cpp` a `.so`/`.dll` con el compilador configurado y lo
  carga con `dlopen`/`LoadLibrary`; `BackendJava` (opcional, `-DFUNSHI_JAVA=ON`)
  compila con `javac` y ejecuta sobre un JVM cargado dinámicamente vía JNI.
  Java accede a las mismas funciones de objeto y servicios de escena mediante
  las funciones nativas de `Nativo`, sin enlazarse directamente con los módulos.
  Los caches C++ y Java incluyen la version del runtime de scripts, por lo que
  un cambio de API/runtime recompila los fuentes y no ofrece ABI binaria entre
  versiones del motor.
  La eleccion del toolchain Java vive en `Behaviour/Backends/ResolucionJdk.h`, que
  es codigo puro (sin JNI, para poder probarlo en un test headless): recorre las
  raices candidatas en un orden fijo y **empareja** `libjvm` y `javac` de una misma
  raiz, para que el `.class` se compile con el mismo JDK que lo ejecuta (con dos
  toolchains distintos, un `javac` mas nuevo que la JVM produce
  `UnsupportedClassVersionError`, que el motor|reportaba como "clase no
  encontrada"). `FUNSHI_LIBJVM` y `JAVAC` siguen mandando por encima de la
  busqueda; si la raiz elegida no trae `javac` (es un JRE) no se busca otro
  compilador. La biblioteca que entrega `FindJNI` llega horneada como la ruta
  del **archivo**, no como una raiz: se resuelve aparte y solo si ese archivo
  existe en la maquina, porque pasada por la lista de raices se descartaba
  siempre (buscaba `<archivo>/lib/server/libjvm.so`). El texto que cruza la
  frontera JNI (`NewStringUTF`) se convierte a
  UTF-8 con `std::filesystem::path::u8string`, que en Windows convierte la
  codificacion ANSI nativa a UTF-8; las rutas que van a `JavaVMOption` y a
  `Proceso::ejecutar` se dejan estrechas a proposito, porque esas APIs usan la
  codificacion nativa.
  Los flags con los que se compila el script viven en
  `Behaviour/Backends/ComandoCompilacionCpp.h` y cumplen el mismo contrato de
  CRT que el engine (`/MD` o `/MDd` según la configuración, más `/EHsc`): la
  reflexión cruza la frontera de la `.dll` con objetos STL que aloca un módulo y
  libera el otro, así que un CRT distinto (p. ej. el estático `/MT`) separa los
  heaps y corrompe la memoria.
- El editor (`SettingsScript`) dibuja los campos reflejados (escalares, arrays,
  grupos, referencias a `GameObject` y a componentes como el `InterfaceComponent`,
  asignables arrastrando desde su inspector) y dispara la recompilación.
- Qué compilador se invoca lo decide `BackendCpp::compilador()` con el mismo
  criterio que el resto: `FUNSHI_CXX` del entorno si está (manda siempre), el
  valor que hornea CMake **solo si existe en disco** —la ruta horneada es la del
  equipo que compiló, y en el del usuario no existe—, y si no `cl` del PATH
  cuando hay un toolset MSVC en la máquina o `g++` en el resto de plataformas.
  El entorno de MSVC sale de `vcvars64EnRaices`, que busca `vcvars64.bat` a dos
  niveles bajo las raíces de Visual Studio (`<raiz>/<año>/<edición>` y también
  las Build Tools, que cuelgan igual); con Visual Studio presente se invoca
  `cl` a secas porque el propio entorno de vcvars lo pone en el PATH, así que el
  motor no queda atado a la versión instalada. Las dos decisiones viven como
  funciones puras en `ComandoCompilacionCpp.h` (`elegirCompilador` y
  `vcvars64EnRaices`, esta con las raíces por parámetro) para poder probarlas
  con rutas falsas, sin depender de un Windows.
- Las cabeceras que el script incluye al compilarse (`Behaviour/IScriptBehaviour.h`
  y las tres que arrastra) se resuelven con `RutaCabecerasScript`: el motor recibe
  la carpeta de fuentes por `FUNSHI_SRC_DIR`, que el paquete deja como el nombre
  relativo `include` y el editor resuelve contra la carpeta del ejecutable (el
  build de desarrollo lo sobreescribe con la ruta absoluta del checkout). Sin esa
  carpeta el backend avisa antes de invocar al compilador, en vez de dejar que
  falle con un `-I` inexistente.

### Interfaces de juego web

Las interfaces gráficas de los juegos (HUD) se escriben en HTML/CSS/JS y se
montan sobre un motor web embebido. El subsistema vive en `src/WebUI/` y sigue
el patrón Strategy + registro de los backends de audio y de scripts:

- El `InterfaceComponent` guarda solo la ruta del `.html` (con su colocación:
  ancla, desplazamiento, tamaño y `z`); el motor web resuelve los recursos
  relativos (CSS, JS, imágenes) por sí mismo.
- `IWebUIBackend` es el contrato del motor web: `ejecutarSubproceso` (rol de
  subproceso del motor web, antes de todo en `main`), `inicializar` (arranque
  una vez por proceso, con GL listo), `bombear` (bucle de mensajes por frame),
  `cargar`, `redimensionar`, `actualizar`, `establecerEstado` (entrega el
  estado JSON a la página), `leerSalida` (recoge `FunshiUI.salida`), `render`
  (que devuelve el handle de textura para ImGui), el input neutral (ratón con
  `BotonRatonWebUI`, rueda, teclado por código GLFW y foco), `descargar` y
  `apagar`. Ninguno de esos métodos tiene comportamiento por defecto: cada
  backend los declara, de modo que un backend incompleto no compila en vez de
  heredar un silencio que el motor no ve.
- `WebUIRuntime` es la fachada de despacho: mantiene un registro de backends y
  selecciona el activo (por configuración, con fallback al nulo ante un nombre
  desconocido). Expone el ciclo global (`ejecutarSubproceso`, `inicializar`,
  `bombear`, `apagar`, que `main.cpp` y `main_juego.cpp` invocan) y reenvía el
  ciclo de cada vista al backend activo. Es la única puerta que usa el resto
  del motor.
- `BackendNullWebUI` corre el ciclo completo sin motor web ni GL, para tests
  headless y CI.
- `BackendCefWebUI` es el backend real sobre CEF 154 (Chromium): una ventana
  off-screen (OSR) por vista, bitmap BGRA a textura vía `updateTexture2D`,
  salida JS por sentinela en `console.log`, teclado por mapa GLFW→VK. Vive en
  su propia unidad C++20 (`BackendCef.cpp`, pimpl sin headers CEF en el resto)
  y solo se compila con `-DFUNSHI_CEF=ON` (requiere `-DFUNSHI_WEBUI=ON`); CMake
  descarga la distribución pineada bajo el build dir y copia el runtime junto
  al binario. Sin CEF el runtime cae al nulo.
- `EstadoInterfaces` guarda, por interfaz viva, dos árboles JSON: el **estado**
  que el motor entrega a la página y la **salida** que la página devuelve. Se
  manejan por rutas estilo JSON Pointer. Es lógica pura (pimpl + nlohmann), sin
  GL ni backend, y los scripts la manipulan por la tabla de servicios.
- `VistasWebUI` gobierna las vistas vivas de una escena (una por interfaz
  activa): las da de alta, recarga al cambiar la ruta del `.html` o al cambiar
  el archivo en disco (sello `mtime`, patrón `cambioElFuente` de scripts),
  redimensiona, entrega el estado pendiente, recoge la salida, avanza y
  descarta las que dejan de estar activas.
- `GameScene` recolecta las interfaces activas (`InterfacesJuego`), sincroniza
  `VistasWebUI` con el tamaño del área de juego y su `EstadoInterfaces`, y
  dibuja cada vista con su textura vía el *draw list* de ImGui, respetando el
  orden por `z`. En el editor además pinta la ventana dockable `Vista de
  interfaz` con la vista de la interfaz seleccionada (preview + input local
  con foco por hover).

---

## 5. Dependencias externas

| Dependencia | Uso | Integración |
|---|---|---|
| GLFW | Ventana, contexto OpenGL y eventos de input | `find_package(glfw3)` |
| OpenGL 3.3 core | Renderizado (shaders, VBO/VAO, FBO) vía `GLFuncs` | `find_package(OpenGL)` (sin GLU) |
| Bullet Physics | Física y colisiones (btDiscreteDynamicsWorld) | `find_package(Bullet)` con fallback a targets clásicos. Se compila **sin RTTI** (vcpkg instala `bullet3` con `USE_MSVC_DISABLE_RTTI=ON`), así que las shapes se identifican por su "shape type" (`TiposShapeBullet`) y nunca con `dynamic_cast`. |
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

Además del ejecutable, el proyecto define **cuarenta y nueve targets de prueba
headless** registrados en CTest (`scripts-java-tests` solo se registra con
`-DFUNSHI_JAVA=ON`; veinticinco de ellos solo existen con `BUILD_ENGINE=ON`
—`catalogo-componentes-tests`, `comandos-tests`, `emisor-particulas-tests`,
`escena-serializacion-tests`, `exportacion-tests`, `iconos-componentes-tests`,
`iluminacion-tests`, `inspeccion-escena-tests`, `interface-component-tests`,
`interfaces-juego-tests`, `malla-regiones-tests`, `manifiesto-assets-tests`,
`materiales-regiones-tests`, `orquestador-estado-tests`, `prefab-tests`,
`preview-material-tests`, `primitivas3d-tests`, `scripts-java-tests`,
`serializacion-tests`, `shaders-tests`, `sombras-tests`, `tema-tests`,
`tilemap-tests`, `ui-component-tests` y `webui-runtime-tests`—; los veinticuatro
restantes compilan también con `BUILD_ENGINE=OFF`):

- `filemanager-tests` (190 en Linux; 181 en Windows con MinGW, 183 con MSVC): ejercita `GestorDeArchivos`/`FileManager`/`FileSystemWatcher`
  contra un proyecto temporal, sin ventanas ni pila gráfica; incluye el arrastre
  con invalidación explícita de caché del grid en carpeta origen y destino, el
  renombre por click derecho de las vistas del explorador y la creación de
  carpetas (`CrearCarpeta`, compartida por árbol y grid: valida el nombre, avisa
  del nombre repetido sin cerrar el modal y sube el contador una sola vez).
  También la lógica
  pura de `SelectorArchivoCubemap` (filtro de extensiones del cubemap y aviso
  de caras faltantes o de resolución dispares), que al vivir fuera del dibujo del
  modal se ejercita aquí sin crear contexto de ImGui.
- `importar-archivos-tests` (45): la importación de archivos que vienen de fuera
  del proyecto (arrastrados desde el explorador del sistema o pegados con Ctrl+V):
  la copia no pisa nada y genera un nombre único (`a.txt` → `a (1).txt`), las
  carpetas se copian recursivamente, un origen inexistente da error sin abortar el
  resto, y los parsers del portapapeles se prueban como funciones puras (el
  `text/uri-list` de Linux con rutas con espacios y no-ASCII, y el bloque
  `DROPFILES` de Windows en ANSI y en UTF-16, con su conversión a UTF-8).
- `proceso-tests` (30): el runner de procesos sin shell `Proceso`: round-trip
  de argv byte a byte (el binario se relanza a sí mismo copiado en una carpeta
  con espacios, con argumentos hostiles), exit codes, truncado del log, el modo
  "agregar al final" (varias etapas escriben en el mismo archivo sin pisarse),
  `cwd`, entorno extra, tabla de `citar()` y, en Windows, la receta cruda de
  `cmd.exe` del harvest de vcvars.
- `configuracion-tests` (174): round-trip del JSON de `EditorConfig` (general y
  por proyecto, con `ConfigPersistence`/`ProjectPaths`), carga tolerante ante
  archivos ausentes/corruptos/parciales, prioridad de las claves modernas sobre
  el `menu/*` legacy, `restablecer`, escritura atómica y guardado diferido, la
  política de proyecto inicial `ProyectoInicial::resolver`, los colores del cielo
  (se conservan tal como se guardaron —un cielo claro incluido— y solo se
  acotan los componentes fuera de `[0, 1]`), el cotejo de prefijos `rutaBajo` (en
  Windows `/` y `\` equivalen y se ignoran mayúsculas y minúsculas; en Linux se
  mantiene sensible al caso), y la elección de la raíz de datos
  (`ProjectPathsDetalle::elegirRaizDeDatos`: junto al ejecutable si ahí se puede
  escribir; si no, la carpeta de datos del usuario, migrando lo que ya hubiera
  junto al ejecutable).
- `eventbus-tests` (17): suscripción/publicación/unsubscribe del canal tipado de GUI.
- `menu-tests` (40): lógica pura del menú (traducción, observer de cambios —sin
  notificar al reaplicar una apariencia idéntica— y reset).
- `assetmanager-tests` (95): caché Flyweight de meshes (rutas `AssetPath`, con los prefijos
  UNC y `\\?` preservados al normalizar) y geometría de `Mesh` con `computeBounds`,
  `computeNormals` —incluido el modo `soloFaltantes` para assets que mezclan sub-mallas con
  y sin normales— y `computeTangents`; registro compartido con un loader artificial.
- `texturemanager-tests` (15): caché Flyweight de imágenes CPU (sin entrar la pila gráfica).
- `estructuras-tests` (91): `ListaDE`, `ArbolEnlazado`, `PriorityListaDE`,
  `MinHeap`/`MaxHeap`, `ListMergeSort` y `ArbolBinarioEnlazado`; más el
  contrato de `Throwable` (el mensaje llega intacto a quien captura la
  excepción como `std::exception`).
- `rendering-tests` (178): geometría de las líneas del pipeline moderno
  (`LineBuilder`): expansión de cada segmento al quad que ensancha el shader,
  color por extremo (difuminado de la grilla), polilíneas, aristas con índices
  fuera de rango y caja de 12 aristas; más la guía de eje (`GuiaEje`): origen y
  dirección en mundo, dirección local rotada con el objeto, normalización frente
  a la escala, colores por eje, el color efectivo que aleja el eje de una grilla
  de su mismo color sin perder la convención (X roja, Y verde, Z azul) ni su
  opacidad, recorte de la recta hasta el horizonte con la
  distancia 3D a la cámara, difuminado por vértice y los rechazos defensivos
  (eje inválido, NaN/Inf, eje degenerado, horizonte degenerado, recta fuera del
  horizonte); más el difuminado del piso (`Difuminado`): el inicio derivado en
  proporción constante del radio elegido, el radio acotado (finito, enorme,
  negativo y nulo) y la forma de la curva, que no depende del radio; más los
  colores efectivos del cielo degradado (`Cielo`): los dos extremos con B/N y
  tema resueltos; y la identidad del cubemap del Skybox (`CacheCubemap`): la
  clave que decide cada cuanto volver a subirlo a GPU cambia solo si cambia una
  ruta o su fecha de modificación. Solo CPU, sin OpenGL.
- `scripts-tests` (173): reflexión `SerializeField` (escalares, arrays, grupos
  anidados y referencias a componentes por `REFLECT_COMPONENTE`, con su
  resolución por nombre del objeto dueño) y su round-trip binario; el contrato
  de flags con el que
  `BackendCpp` compila los scripts (CRT, `/EHsc`, familia de compilador, los
  ARGV armados sin shell ni redirección); la resolución de la carpeta de
  cabeceras del script (`RutaCabecerasScript`: absoluta del checkout o nombre
  relativo `include` resuelto contra la carpeta del ejecutable en la app
  instalada); el descubrimiento del toolset MSVC de la máquina del usuario
  (`vcvars64EnRaices`, con las raíces por parámetro para poder probarla con un
  árbol falso: dos niveles, que es donde cuelgan las Build Tools, y primera
  raíz por preferencia) y la decisión de qué compilador se invoca
  (`elegirCompilador`: override del entorno, valor horneado solo si existe en
  disco, `cl` del PATH si hay toolset o `g++`); el harvest del entorno de vcvars
  (receta cruda de `cmd`, parser UTF-16, bloque multi-sz); y el contrato del
  sondeo de toolchain (dispositivo nulo `NUL`/`/dev/null` abierto por el
  runner, sin `std::system`); el emparejamiento de `libjvm` y `javac` de una
  misma raíz de JDK (`ResolucionJdk`, con sus overrides por entorno y la
  dedución de la raíz desde la ruta de la biblioteca en los layouts de POSIX y
  Windows, que el compilador que se pasa al proceso sea ejecutable, y que la
  biblioteca horneada por CMake —que es la ruta del archivo, no una raíz— solo
  se use si ese archivo existe en la máquina, como último recurso) y la paridad
  del requisito de JDK del instalador (que exige
  `jvm.dll` **y** `javac.exe`).
- `scripts-runtime-tests`: compila un `.cpp` real con `BackendCpp`, lo carga con
  `dlopen`/`LoadLibrary` y ejecuta el ciclo + hot reload, comprueba que un
  segundo componente sobre el **mismo** fuente reutiliza el artefacto ya al
  día en vez de volver a enlazarlo, y que sin las cabeceras del motor el
  backend avisa con su propio mensaje (SKIP 77 solo si el
  sondeo del compilador del build falla; con MSVC el entorno del toolset lo
  obtiene `BackendCpp` del `vcvars64.bat`).
- `scripts-java-tests`: end-to-end del backend Java (JNI); solo con `FUNSHI_JAVA=ON`.
- `model-serialization-tests` (16): serialización binaria del componente `Model`
  (path con prefijo de longitud; regresión del core al cargar escenas con paths
  largos).
- `audio-tests` (25): `AudioEngine` con `NullAudioBackend` (contrato de la cola
  de comandos: reproducción por ruta, handles, encolado, detención, volumen) y
  la cola de música con un backend falso (la lista de pistas por ruta, su
  reproducción, el bus, el silencio, la detención y el tick de la escena).
- `interface-component-tests` (16): serialización binaria del `InterfaceComponent`
  (ruta del `.html` + colocación): round-trip del formato nuevo y lectura del
  formato viejo (solo el nombre) sin consumir bytes de más, que desalinearía el
  resto del objeto.
- `interfaz-ui-tests` (67): la interfaz de juego nativa (CPU pura, corre también
  en el build sin motor): la colocación de los widgets con los nueve anclajes y
  su desplazamiento, el recálculo del layout cuando cambia el tamano de la
  pantalla, la búsqueda por identificador, el cambio de texto y de visibilidad,
  el acierto del ratón sobre un widget y el flanco de pulsación (un clic se
  marca solo en el frame en que ocurre), el apilado (el widget de encima recibe
  el ratón) y el widget invisible (no se dibuja ni recibe el ratón).
- `ui-component-tests` (27): serialización binaria del `UIComponent`: round-trip
  de la lista de widgets (identificador, tipo, anclaje, desplazamiento, tamaño,
  colores, texto, ruta de textura y visibilidad), los valores por defecto, un
  payload ajeno que no debe consumirse de más (el centinela posterior sobrevive)
  y los valores fuera de rango (tipo, anclaje y cantidad de widgets) acotados
  sin desalinear la lectura.
- `barra-progreso-tests` (8): `BarraProgresoTexto` (formato de la barra de la
  ventana Estado, acotado ante `hecha > total` y `total == 0`).
- `webui-runtime-tests` (57): `WebUIRuntime`, `VistasWebUI` y `EstadoInterfaces`
  sobre el backend nulo: selección de backend con fallback al nulo ante un nombre
  desconocido, ciclo de vida de una vista sin motor web (`cargar`→`actualizar`→
  `render`→`descargar`), el gestor de vistas de una escena (alta, recarga al
  cambiar la ruta del `.html` o al cambiar el archivo en disco por `mtime`,
  redimensionado y descarte de las que dejan de estar activas), el estado por
  interfaz por rutas JSON Pointer y el intercambio del estado pendiente/olvido
  en el ciclo de vistas.
- `shaders-tests` (35): el preprocesador de `#include` del GLSL embebido (el
  include se inserta en su sitio, los anidados, el include inexistente, el ciclo
  y el tope de anidamiento dan error limpio) y el gestor de shaders (un programa
  por nombre, compilado una vez y compartido entre los que lo piden).
- `sombras-tests` (75): la matriz de la luz (vista desde la luz y proyección
  ortográfica, con los casos degenerados: luz sin dirección, radio o alcance
  inválidos), el componente `Sombras` con sus ajustes y la selección de
  proyectores (las luces puntuales no soportadas, el límite de mapas y el orden
  en que se eligen).
- `musica-tests` (84): el reproductor de música con un backend falso: la lista
  ordenada de pistas (por ruta), la reproducción, el fundido cruzado (la pista
  que sale baja mientras la que entra sube), el paso automático a la pista
  siguiente cuando la actual se acaba o cuando le queda justo el fundido, el
  final de la lista (con o sin repetición), la detención con y sin fundido y el
  volumen del bus.
- `iluminacion-tests` (17): la recogida de las luces de la escena (el orden en
  que se recorren, los datos que salen del `Transform` de cada objeto y el límite
  de slots) y el paralelismo entre la lista de luces y la de dueños: el pase de
  sombras usa el índice de la luz para saber a qué luz pertenece cada mapa, así
  que las dos listas no pueden separarse.
- `tema-tests` (28): `TemaEditor` (aplicación del perfil `Apariencia`): el acento
  llega a todos los roles de ImGui y el modo B/N deja la paleta monocroma.
- `comandos-tests` (77): undo/redo del editor (7 comandos, cadena de redo
  múltiple, límite de 50 entradas y descripción del comando aplicado).
- `manifiesto-assets-tests` (37): manifiesto `SceneAssets.json` (JSON round-trip de
  malla, texturas, script y las seis caras del cubemap, tolerancia a manifiestos
  corruptos y precedencia sobre el `.db`).
- `mods-tests` (243): sistema de mods `.fmod`: parseo y
  validación de `ManifiestoMod.json` (campos obligatorios, `esquemaVersion`,
  `id` con formato `Autor.Nombre`, `tipo` nativo), semver y rangos
  (`versionMotor`, `versionMin`), lectura del manifiesto desde el ZIP y
  extracción con rechazo de rutas `../`/absolutas (anti zip-slip), y
  descubrimiento en `Mods/` con orden topológico, detección de ciclos, rechazo
  de `id` duplicado, chequeos de dependencia/conflicto/versión de motor, ámbito
  motor vs proyecto y reporte a `RegistroDiagnosticos`. Los `.fmod` de prueba se
  generan con el escritor de miniz. La carga nativa se ejercita con binarios
  reales: los targets `mods-test-plugin-a/b` (bibliotecas compartidas que
  exportan `funshi_mod_api`) y `mods-test-plugin-sin-simbolo` se empaquetan
  como `.fmod` y se cargan de verdad, cubriendo la extracción a la caché, la
  inicialización, el orden inverso de descarga y los rechazos (sin punto de
  entrada, sin binario en el paquete, sin el símbolo exportado). El registro de
  paneles y la tabla de servicios se cubren aparte: alta de paneles (con su
  clave de ventana única por mod y panel), atribución al mod en ejecución,
  estado abierto/cerrado por ventana, retirada de los paneles al descargar el
  mod, `version` en offset fijo, y el reparto de las consultas de datos de la
  escena a la fuente inyectada.
- `orquestador-estado-tests`: transiciones de Depuración/Juego, pausa, solicitud
  consumible de Reset, fin/Escape por estado, teclas del editor y atajos frente
  a ImGui.
- `escena-serializacion-tests` (358): round-trip completo de escena (guardar →
  recargar → conservar nombre, id y jerarquía), defensas del índice de escena
  (líneas corruptas saltadas con aviso, auto-sanado de hijos con id 0),
  apertura avisada de archivos `Binario` inexistente sin `std::remove()`
  destructivo, la reescritura de referencias al mover/renombrar (script,
  malla y textura bajo el prefijo reubicado, los demás intactos), el
  sanado de referencias rotas al cargar (una sola coincidencia del nombre bajo
  la raíz → repara; varias o ninguna → no adivina y avisa), la resolucion del
  componente `Model` con la matriz mundial del objeto y la separacion del
  evento estructural (`ComponentStructureChanged`) del de propiedad en el bus
  de escena: editar un campo no reconstruye los paneles del Inspector y
  agregar un componente crea unicamente el que falta. Cubre ademas el
  reparentado: preserva la pose mundial reescribiendo el local como
  `inverse(padre) × mundo` (los descendientes no saltan), refresca el cuerpo
  fisico del objeto y deja el local consistente en disco. Comprueba tambien la
  tabla de servicios de scripts: la música se reproduce por ruta (con y sin
  motor de audio conectado) y el volumen del bus y la deteccion se aplican.
- `diagnostic-registry-tests` (26): `RegistroDiagnosticos`: registro de eventos
  de diagnóstico (nivel, módulo, timestamp UTC), historial JSONL con retención y
  lectura acotada, deduplicación por clave y el volcado de `RuntimeException`.
- `catalogo-componentes-tests` (52): la tabla única que declara, por componente,
  su dimensión (2D/3D), requisitos, creador y panel: descriptores completos y sin
  nombres repetidos, `ComponentFactory` creando desde el catálogo (tolerando el
  prefijo de MSVC y el alias retrocompatible), tipos desconocidos y requisitos
  incumplidos devolviendo nullptr, y el filtro por dimensión y los submenús.
- `tilemap-tests` (57): tileset JSON estilo Tiled (parseo, geometría de la
  grilla, flag de solidez por tile), capas y pintado por celda, horneado de la
  malla (una malla por tilemap, un quad por tile), round-trip binario y la shape
  compuesta del `TilemapCollider`.
- `interfaces-juego-tests` (9): la recolección de interfaces de juego (HUD):
  `InterfacesJuego::recolectarDeEscena` sobre la lista de objetos de la escena
  (lista nula o vacía sin resultados, objetos sin `InterfaceComponent` que no
  aportan, una escena con varias interfaces que las devuelve TODAS ordenadas por
  z y las interfaces sin ruta o inactivas omitidas sin cortar el recorrido).
- `prefab-tests` (82): sistema de Prefabs: round-trip guardar → instanciar
  conservando nombre, tag, jerarquía, componentes y `Transform`, más la caché de
  `PrefabLibrary` y la validación de archivos corruptos; la API de scripts
  `listarPrefabs` con storage estable (válido hasta la llamada siguiente).
- `terminal-emulator-tests` (24): cuadrícula VT del `TerminalEmulator`: CR/LF,
  posicionamiento y consulta del cursor (`H`/`DSR`), secuencias SGR (color,
  negrita, fondo) y borrado; CSI 1K con cursor en wrap pendiente, rejilla
  recién redimensionada a las dimensiones por defecto, y pantalla alternativa
  con resize (cursor restaurado acotado).
- `terminal-session-tests` (14): E/S real del pseudo-terminal (`forkpty` en
  POSIX, ConPTY en Windows): rechazos deterministas, arranque, eco del comando y
  su salida por el PTY, y que `detener()` no se cuelga ni pierde la salida
  emitida durante el cierre (SKIP 77 si ConPTY no está disponible).

Los **veinticinco** targets que solo existen con `BUILD_ENGINE=ON` son los que
enlazan `funshi_engine` (`comandos-tests`, `escena-serializacion-tests`,
`serializacion-tests`, `catalogo-componentes-tests`, `tilemap-tests`,
`interfaces-juego-tests`, `prefab-tests`, `iluminacion-tests`,
`exportacion-tests`, `iconos-componentes-tests`, `malla-regiones-tests`,
`materiales-regiones-tests`, `preview-material-tests`, `primitivas3d-tests`,
`emisor-particulas-tests`, `inspeccion-escena-tests` y `ui-component-tests`),
los que necesitan glm (`interface-component-tests`, `webui-runtime-tests`,
`shaders-tests` y `sombras-tests`), los que compilan las fuentes de ImGui
(`tema-tests`), los que usan solo el core de diagnóstico
(`manifiesto-assets-tests` y `orquestador-estado-tests`) y el backend Java
(`scripts-java-tests`). El resto compila sus
fuentes directamente (`barra-progreso-tests` entre ellas) y corre también con
`BUILD_ENGINE=OFF`: con el motor hay 47 suites que se ejecutan (49
registradas, dos deshabilitadas) y sin él 23 (24 registradas, una
deshabilitada).

La opción `BUILD_ENGINE=OFF` compila solo las pruebas (útil en CI y plataformas
sin las librerías gráficas), y `ENABLE_ASAN` (ON por defecto en Debug) activa
ASan+UBSan en GCC/Clang.

---

## 7. Flujo de datos principal

```text
main.cpp
  │
  ├─ input GLFW ──► EditorInput (callbacks + máquina de teclas) ──► cámara activa (movimiento continuo)
  │                 (consulta IEstadoInterfaz: ¿el teclado es de un widget?)
  │                                       ──► tecla E: toggleEditorInterfaces()
  │                                       ──► 1/T, 2/R, 3/U: operación del gizmo (y apagan la guía)
  │                                       ──► X/Y/Z: guía de eje del objeto seleccionado
  │                                       ──► Escape: en simulación termina; en editor
  │                                         vuelve al menú (regla en el orquestador)
  │
  ├─ GameScene::GUI()
  │     ├─ SceneMenuBarInterface ──► Depuración/Juego o Pausa/Reset/Terminar
  │     │                          ──► OrquestadorEstadoGUI
  │     ├─ SceneSelectedInterface ──► EditorController (crear/borrar/reparentar GO)
  │     │                         ──► EventBus.publish(ObjectCreated/Deleted/Selected)
  │     ├─ SceneObjectTree ──► selección y reparentado por drag & drop
  │     └─ GUIManager paneles
  │           └─ SettingsObjectInterface ──► EditorController (add/removeComponent,
  │                                          sync colliders, "Gizmo activo")
  │
  ├─ GameScene::update(dt)  (siempre, también con start en false: es donde viven los
  │                          flancos de transición; física/scripts se auto-gatean)
  │     ├─ transición editor→simulación: empuja la pose visual a los cuerpos Bullet
  │     ├─ transición simulación→edición: cola vacía, servicios de script
  │     │   desconectados, audio cortado y onStop a cada script
  │     │   (ambos modos restauran el estado previo a la simulación)
  │     ├─ si start y gizmo libre: PhysicsEngine::stepSimulation(dt)
  │     │               └─ btDiscreteDynamicsWorld::stepSimulation
  │     └─ scripts: IScriptBehaviour::onUpdate (si compilados)
  │
  └─ GameScene::gameScene()
        ├─ LightSystem::beginFrame() [solo CPU: datos de luz para el shader]
        ├─ pasada de la grilla (Grid + batch de líneas; solo fuera de Juego)
        ├─ dibujarGameObjects (MeshRenderer shader; único pipeline)
        ├─ marcadores de luz y cámara (wireframes auxiliares, batch de líneas)
        ├─ ImGuizmo::Manipulate sobre el GizmoTarget activo (objeto o collider)
        ├─ dibujarViewportsPrevios (FBO de cámaras) + paneles ImGui
        ├─ pintarInterfacesJuego (overlay HUD; también corre sin GUI de editor)
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
| Service (inyección) | `GestorShaders` (dueño de los programas) y `TextureManager` | Los renderers reciben los recursos por setter en vez de buscarlos por su cuenta: un programa por nombre, compilado una vez y compartido, y el backend de render se inyecta en lugar de localizarlo por un global. |

El backend de render (`IRenderBackend`) llega **inyectado** por el camino que
va de `main` a la escena: `main` lo pasa a `GameScene`, esta al
`SceneRenderer` y de ahí a los renderers y a los recursos de GPU
(`MeshGPU`, `TextureGL`, `LineBatch` y `RenderTarget`, que lo reciben al
construirse). Así esos recursos no dependen de un global y se pueden probar
con un backend falso. El global `activeBackend()` queda solo en los puntos
que aun no están migrados (la ventana, el arranque, los iconos de la GUI, el
backend web y el dibujado del wireframe de los colliders).

No están implementados todavía:
- `Visitor` para inspectores extensibles (hoy los paneles del inspector se
  construyen con `dynamic_cast` sobre cada tipo de componente, y el catálogo
  declarativo `CatalogoComponentes` cubre lo demás).

### 8.1 Resolución de la raíz de datos

Todos los datos del usuario (proyectos, escenas, configuraciones, `imgui.ini`)
cuelgan de una única raíz, `FunshiEngine-DATA`, y toda la jerarquía de rutas sale de
`ProjectPaths::directorioBase()`. Esa función **resuelve** la raíz en vez de
calcularla:

1. Parte de `<directorioEjecutable>/FunshiEngine-DATA`, la ubicación histórica.
2. Comprueba si puede **escribir** ahí. No basta con que el directorio exista y
   sea legible: se abre y se cierra un archivo de prueba, porque en
   `C:\Program Files` la carpeta es legible y aun así el proceso no puede crear
   nada dentro (la creó el instalador con los ACL de `Program Files`; el
   ejecutable no lleva manifiesto de elevación).
3. Si no puede, cae a la carpeta de datos del usuario: `%APPDATA%\FunshiEngineGL`
   en Windows, `$XDG_DATA_HOME/FunshiEngineGL` o `~/.local/share/FunshiEngineGL`
   en Linux y macOS. El instalador de Windows es por usuario
   (`PrivilegesRequired=lowest`), así que en una instalación normal este punto
   2 se cumple y la carpeta queda en `%LOCALAPPDATA%\Programs\FunshiEngineGL`:
   el paso 3 es la red que cubre una copia en `Program Files`, una carpeta en
   solo lectura o el motor lanzado como administrador.
4. Si tampoco hay carpeta de usuario utilizable, conserva la ruta histórica: es
   preferible una ruta conocida que falle de forma visible a dejar al motor sin
   rutas válidas.

La decisión se cachea por proceso (es una decisión de inicio, no por llamada) y
se expone con `ProjectPaths::datosEnRutaDeUsuario()`,
`directorioBaseOriginal()` y `migrarDatosDesdeRutaOriginal()`. Esta última copia
una sola vez lo que hubiera quedado en la ruta histórica, y solo si el destino
está vacío.

Por qué importa: sin este paso, las escrituras fallaban **en silencio** (nadie
comprobaba el retorno de `crearArchivo` ni de los demás), de modo que el motor
leía bien pero no podía guardar nada. Esa divergencia entre lo leído y lo que
se creía haber escrito desincronizaba el árbol de `SerializeField` de los
scripts y terminaba en un `std::bad_variant_access` al abrir el inspector.

Las rutas de assets dentro de la escena se guardan **relativas** a la raíz de
assets del proyecto (`EditorConfig::relativizarRuta`) y se absolutizan al
cargar, así que mover la raíz no invalida las escenas existentes.

---

## 9. Observaciones de arquitectura

- `SceneRegistry` establece ownership único mediante `std::vector<unique_ptr<GameObject>>`.
  GUI, selección, física y vistas lineales solo mantienen referencias raw no propietarias.
- `PhysicsEngine` es una fachada PIMPL con el backend inyectado (`IPhysicsBackend`):
  el header no expone Bullet y la fachada puede existir sin mundo (útil para pruebas
  y para el editor sin simulación). El contrato no tiene comportamiento por
  defecto: el backend declara los contactos que entrega y la gravedad global que
  maneja, así que uno incompleto no compila.
- `GameScene` sigue siendo un coordinador amplio: mezcla ciclo de frame, GUI,
  física, gizmo y estado de la simulación. El render ya está extraído en
  `SceneRenderer` (que también dibuja las vistas previas); la extracción de
  `PhysicsSystem` y `ScriptSystem` aliviaría el resto.
- `ApplicationStateMachine` explicita los estados y las transiciones pasan por
  `OrquestadorEstadoGUI`, que decide en que modo se está (`main.cpp` solo
  refleja la decisión en la escena); los flags de UI legados (`menuActivo`)
  viven en `GameScene`.
- `EventBus` transporta `GameObject*` crudos. Un consumidor no debe conservar esos
  punteros después de recibir `ObjectDeleted`. `EditorController::deleteGameObject`
  publica el evento **antes** de destruir el objeto, para que los observadores
  invaliden sus referencias a tiempo (fix de un use-after-free histórico).
- La manipulación del gizmo pausa `stepSimulation` mientras el usuario arrastra y
  la física solo corre en Depuración o Juego; el sync collider↔rigidbody↔objeto usa la matriz
  global compuesta del dueño, de modo que mover un collider no desincroniza el cuerpo.
- Las rutas de usuario (`<directorioEjecutable>/FunshiEngine-DATA`) están
  centralizadas en `EditorConfig` para la configuración y el layout, y las del
  proyecto en `ProjectPaths` (que es la única fuente de verdad: `directorioSrc`,
  `directorioScripts` y el resto); `PathUtils.h` comparte el separador
  (`PATH_SEP`) y la regla única de cotejo de rutas (`rutaBajo`/`esSeparadorPath`):
  en Windows `/` y `\` equivalen, que es lo que permite reescribir las
  referencias de la escena cuando el explorador trae rutas con `\` y
  `ProjectPaths` arma las suyas con `/`.
- La serialización binaria tiene versionado **parcial**: varios componentes
  llevan magic y versión (`RigidBody` v1/v2/v3, `InterfaceComponent`,
  `Animador2D`, `Tilemap`, `Prefab`) y la escena valida el índice de forma
  estricta, pero no hay versionado global por tipo de componente ni validación
  formal de tamaños: un cambio en la estructura de atributos de un componente
  sin versión sigue invalidando escenas guardadas.
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
  operaciones de dominio (crear, renombrar, copiar, eliminar archivo y carpeta,
  búsqueda) y `FileSystemWatcher` (detección de cambios externos: inotify en
  Linux; en el resto, el polling de mtime con su semilla en el ctor y el
  intervalo inyectable para probarlo).
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
- `tests/SceneSerializationTests.cpp`: round-trip completo de escena (guardar →
  recargar → conservar nombre, id y jerarquía), el nombre por defecto de los
  objetos nuevos (`GameObjectFactory` + `NombreUnico.h`, que no puede repetirse
  en el árbol), el guardado con el árbol vacío (archivo vacío **con aviso en el
  log**, nunca un trunc silencioso) y el reporte de fallos de apertura en
  `Binario` (sin `std::remove` destructivo previo, con valor de retorno `bool` y
  propagación en `saveEntity`/`loadEntity`). También verifica que el puntero a
  una cámara eliminada durante la UI se invalide antes de usarlo en el gizmo.
  Es la suite que faltaba: hasta ahora solo existía `ModelSerializationTests`,
  que cubre el componente `Model` aislado.
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
- `tests/ComandosTests.cpp`: los 8 comandos del editor (`CrearObjetoComando`,
  `BorrarObjetoComando`, `ReparentarComando`, `TransformComando`,
  `AgregarComponenteComando`, `QuitarComponenteComando`, `LimpiarEscenaComando`,
  `DuplicarObjetoComando`)
  con deshacer/rehacer, la cadena de redo múltiple, el límite del historial, la
  descripción que el historial devuelve para avisar en la barra de estado y el
  registro de cuerpos en fisica al borrar/duplicar/agregar/quitar.
- `tests/IconosComponentesTests.cpp`: el catálogo de componentes es la fuente
  única del icono de cada tipo. La suite verifica que todo componente declare
  icono, que la imagen exista en el disco, que la regla resuelva por nombre de
  tipo, por alias ("Camera") y con el prefijo que MSVC antepone ("class
  Transform"), y que un tipo desconocido no devuelva icono.
- `tests/MallaRegionesTests.cpp`: un archivo con varios materiales (un `.obj`
  con dos `usemtl`) produce una región por material: un rango contiguo de
  índices con el nombre del material. La suite escribe el `.obj` y su `.mtl` en
  una carpeta temporal, los carga con el importador real y comprueba que las
  regiones salen en orden, sin huecos y cubriendo todos los índices.
- `tests/MaterialesRegionesTests.cpp`: el componente `Materiales` (una region por
  parte de la malla con su material) y el resolvedor que decide qué material se
  dibuja en cada region: el de la region, el material único del objeto o
  ninguno. La suite comprueba el alta y la baja de regiones, la sincronización
  con las que declara el modelo, el round-trip binario (con el material de cada
  region dentro) y las tres respuestas del resolvedor.

- `tests/ExportacionTests.cpp`: el diálogo de exportación, el estado que lleva
  (`EstadoExportacion`) y las dependencias del juego exportado. La suite
  comprueba que las precondiciones digan el motivo cuando no se puede
  exportar (sin proyecto, o con su carpeta inexistente), que el resultado
  (éxito o error) se conserve al terminar la exportación en vez de perderse,
  que la barra de menú le pase al diálogo el proyecto activo, que el lector de
  importaciones PE encuentre las DLL que pide el binario (sobre un PE
  construido a propósito), que el cierre de dependencias alcance las DLL de
  segundo nivel y que la traza de la exportación se cree en un archivo propio
  que una segunda exportación no reutiliza.
- `tests/Primitivas3DTests.cpp`: la esfera de muestra de un material es
  geometría pura (sin OpenGL): la suite comprueba que los vértices están en la
  esfera, que las normales son unitarias, que las UVs están en [0,1], que los
  índices apuntan a vértices válidos y que ningún triángulo es degenerado (un
  vértice repetido dejaría un trozo invisible).
- `tests/PreviewMaterialTests.cpp`: la vista previa de un material (la esfera
  de muestra del panel). La suite la ejercita con un backend falso (sin GPU):
  cada material editado tiene su vista propia, la vista se dibuja en un render
  target de su tamaño con su viewport y vuelve al framebuffer de la ventana, el
  material editado llega al shader (el color difuso se sube) y al invalidarla
  queda sucia para redibujarse.
- `tests/InterfazUITests.cpp`: la interfaz de juego nativa (el modulo es CPU
  puro, asi que la suite corre tambien en el build sin motor). Comprueba la
  colocacion de los widgets con los nueve anclajes y su desplazamiento, el
  recalculo del layout cuando cambia el tamano de la pantalla, la busqueda por
  identificador, el cambio de texto y de visibilidad, el acierto del raton
  sobre un widget y el flanco de pulsacion, el apilado (el widget de encima
  recibe el raton) y que un widget invisible no se dibuja ni recibe el raton.
- `tests/UIComponentTests.cpp`: la serializacion binaria del `UIComponent` (el
  HUD nativo): el round-trip de la lista de widgets, los valores por defecto, un
  payload ajeno que no debe consumirse de mas (el centinela posterior sobrevive)
  y los valores fuera de rango acotados sin desalinear la lectura.
- Las suites de prueba se ejecutan con `ctest`: el build con motor registra
  **49** tests (47 habilitados y 2 deshabilitados) y el build headless
  (`BUILD_ENGINE=OFF`) registra **24** (23 habilitados y 1 deshabilitado).
- `.github/workflows/ci.yml` compila el engine completo en Ubuntu (Release, sin
  ASan) y ejecuta las pruebas; además ejecuta las headless en
  Linux/Windows con `BUILD_ENGINE=OFF`, el backend Java en Ubuntu con JDK y
  las headless en Windows con el **generador multi-config** (Visual Studio),
  donde cada configuración compila en su propia carpeta: lo que el proyecto
  genera en tiempo de configuración tiene que ser por configuración.
- La CI compila y **enlaza** el engine también en Windows con MSVC (generador
  por defecto y dependencias de vcpkg), que es lo que hace el job que publica
  el instalador: sin ese enlace, un fallo que solo aparece con Visual Studio
  (por ejemplo, un símbolo cuyo nombre depende del *class-key* del tipo) se
  descubre al publicar, cuando corregirlo obliga a rehacer el tag.
- El *class-key* (`class` o `struct`) con el que se declara un tipo forma parte
  del nombre del símbolo en MSVC: una declaración adelantada con un key y la
  definición con el otro generan dos símbolos distintos y el enlazado falla
  con `LNK2019` aunque el código sea idéntico (GCC y Clang lo ignoran). Por
  eso el build avisa y el aviso es error: C4099 en MSVC (`/we4099`) y
  `-Werror=mismatched-tags` en GCC y Clang.
- `typeid(x).name()` no es el nombre del tipo: MSVC escribe `"class Transform"`
  y GCC/Clang escriben el *mangling* (`"9Transform"`). Por eso el nombre con el
  que se **persiste** un componente (prefabs, escenas, catálogo) lo produce un
  único helper, `nombreSimpleTipo()` de `Herramientas/TypeUtils`, que quita el
  prefijo de class-key y se queda con el último componente del namespace. Es el
  mismo helper al escribir, al leer y al comparar, de modo que un artefacto
  escrito en una plataforma se lee en la otra.
- `.github/workflows/release.yml` y `windows-release.yml` también ejecutan la
  suite (y `estructuras-tests`) al generar los instaladores por tag. El job de
  Windows usa el generador por defecto (multi-config), que es el mismo que
  comprueba el job multi-config de la CI.
- La lista de targets que compila cada job tiene que cubrir todas las suites
  registradas en ese modo de build (41 con `BUILD_ENGINE=ON`, 21 con
  `BUILD_ENGINE=OFF`): `ctest` ejecuta todos los tests registrados en el build,
  no solo los compilados, y los que faltan quedan como `Not Run`, lo que hace
  fallar el job. Las suites con `DISABLED TRUE` se pueden omitir porque `ctest`
  las salta.
- El punto de entrada del juego exportado (`src/Runtime/main_juego.cpp`) no
  forma parte de la librería del motor: lo compila el proyecto que genera el
  exportador, y ese proyecto solo existe cuando el usuario exporta. Para que
  sus errores no aparezcan en ese momento, el motor lo compila en el target
  `runtime-compile-check`, que la CI construye en Linux y en Windows.

Los bugs de la Fase 2 (cámaras/vistas previas) y sus fixes están documentados en
[CAMARAS_VISTAS_PREVIAS.md](CAMARAS_VISTAS_PREVIAS.md).

---

## 12. Pendientes conocidos

- [ ] Animación 3D: máquina de estados, esquelética y por interpolación (la
      animación 2D por fotogramas, con su panel, ya está hecha:
      `Animador2D`).
- [ ] Filtros y navegación avanzada del historial de diagnósticos en el editor.
- [ ] Limpiar los binarios huérfanos al eliminar un objeto (al crear, los ids
      duplicados ya se evitan: se asigna el siguiente libre).
- [ ] Terminar los popups del inspector.
- [ ] Versionado global por componente en la serialización binaria (varios
      componentes ya llevan magic y versión, y la escena valida el índice de
      forma estricta).
- [ ] Extraer `PhysicsSystem` y `ScriptSystem` de `GameScene` (el render ya
      está extraído en `SceneRenderer`).
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
  `std::string` exactas (búsqueda del look-ahead), no con matching; el patrón
  de las líneas del índice (`ObjectN<entero>.db`) se valida a mano, sin
  regex.
- **Nombres de tipos**: `ComponentFactory`/`TypeUtils` comparan huesos
  exactamente (`"CameraComponent"` y alias `"Camera"`), sin patrones.

**Conclusión**: no se incorpora ninguna dependencia de regex. Las necesidades
actuales de parsing son estructuradas (filesystem, JSON, binario, comparación
literal) y una implementación con `std::regex` añadiría coste de compilación y
de runtime sin cubrir ningún caso real. Si más adelante se necesitara validar
nombres de archivos o campos con patrones (p. ej. prefijos de prefabs), la
decisión sería revistarla puntualmente con `std::regex` (sin dependencia extra).
