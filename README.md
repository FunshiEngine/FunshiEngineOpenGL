# FunshiEngineGL

<p align="center">
  <img src="FunshiEngineGL/Imagenes/FunshiEngineGL_Logo_Principal_Blanco.png"
       alt="FunshiEngineGL" width="180">
</p>

Motor y editor 3D en tiempo real escrito en C++17, con interfaz ImGui y renderizado OpenGL construido desde cero.

---

## Características actuales

- Ventana y contexto **OpenGL 3.3 core** con **GLFW**; renderizado íntegramente
  con shaders (VBO/VAO + `ShaderProgram`), sin pipeline inmediato ni GLU. El
  perfil core es un requisito duro: si el driver no da 3.3 core, el arranque
  aborta con un mensaje en consola en vez de mostrar una pantalla negra.
- Interfaz de editor con **Dear ImGui** (docking) y gizmos con **ImGuizmo**.
- Sistema **Entity–Component**: `Transform`, `Color`, `Model`, `Material`, `Light`, `CameraComponent`, `Grid`, colliders (esfera / cubo / malla), `RigidBody`, `AudioSource`, `InterfaceComponent` (HUD por asset JSON del CreadorDeInterfaces) y `Script`.
- **Scripts dinámicos** (`Script` + `IScriptBehaviour`): reflexión por macros con campos `SerializeField` (escalares, arrays y grupos anidados) editables en el inspector; compilación en caliente de C++ a `.so`/`.dll` (`BackendCpp`) y soporte de **Java vía JNI** (`BackendJava`, se activa automáticamente si el build encuentra el JDK). Hot reload por fecha de modificación que reinyecta los valores serializados, y ciclo `onStart`/`onUpdate`/`onStop`.
- **Jerarquía de objetos** con árbol enlazado propio (`ArbolEnlazado<GameObject*>`) y reparentado seguro (rechaza ciclos y la raíz).
- Carga de modelos 3D con **Assimp** (`.obj`, `.fbx` y formatos soportados por Assimp).
- **Física con Bullet** detrás de una fachada desacoplada (`PhysicsEngine` → `IPhysicsBackend` → `BulletPhysicsAdapter`): solo simula en modo Play, sincroniza transformaciones entre objeto, collider y cuerpo, y admite un gizmo dedicado para el collider activo.
- **Serialización binaria de escenas** en preorden con marcadores `=>`/`<=`: guarda y recupera la jerarquía completa (padres e hijos) de forma recursiva.
- **EventBus** con suscripción tipada (creación, eliminación, reparentado, selección y cambios de componentes) + **EditorEventBus**: canal tipado de GUI interna (apariencia, idioma, sensibilidad, cámara activa y visibilidad de ventanas) que median entre el menú, las ventanas del editor y la escena sin pasarse punteros.
- **Máquina de estados** de la aplicación: `MainMenu`, `Editing`, `Playing`, `Exiting`, con reglas de transición centralizadas en `OrquestadorEstadoGUI`.
- **Input modularizado** (`src/Input/EditorInput`): las callbacks de teclado/mouse de GLFW viven en su propio módulo (extraídas de `main.cpp`); traducen los eventos a acciones del editor (E, G, gizmos, Escape, clic derecho para navegar) y mantienen una máquina de estado de teclas WASD/Espacio/Shift con movimiento continuo normalizado por frame (diagonales a la misma velocidad que un eje).
- **Render de un solo pipeline**: los `Modelos3D` se dibujan con `MeshRenderer` (VBO/VAO + shaders vía `ShaderProgram`); una malla sin normales por vértice no se dibuja y se avisa una vez por consola (`Mesh::computeNormals()` las genera). La **grilla** es un componente (`Grid`, con visible/color/tamaño/separación) en una pasada independiente, cuyo color acompaña a la apariencia (incluido el modo blanco y negro); al igual que los marcadores de luz/cámara y los gizmos de los colliders, se dibuja con el pipeline de líneas (batch en GPU + shader de ancho en píxeles, con el difuminado del horizonte resuelto por alpha por vértice).
- **Audio en runtime** (`src/Audio/`): `AudioEngine` (fachada thread-safe con cola + hilo de audio) sobre backends intercambiables (`MiniAudioBackend` con miniaudio, `NullAudioBackend`); `AudioClipsManager` descubre los clips de `Sonidos/` y los registra por nombre; `AudioSource` reproduce con volumen, loop y autoplay.
- **Ventana "Estado"** (`StatusBarInterface`): muestra el toolchain externo (compilador C++, javac, libjvm) y el estado de compilación/carga de los scripts de la escena.
- Explorador de archivos del proyecto con fachada propia (`FileManager`), estado de navegación compartido (`FileSelection`) y vigilancia de cambios externos (`FileSystemWatcher`).
- **Apariencia del editor configurable** (perfil persistido en `Configuracion.json`):
  tema claro/oscuro, **modo blanco y negro** (desatura toda la interfaz y
  acompaña al fondo y la grilla del viewport), color de acento de la interfaz y
  color de fondo de la escena, aplicados en vivo por `TemaEditor`/`AparienciaUtil`.
  El acento se inyecta en **todos** los roles visuales de ImGui (botones,
  solapas del dock, campos de entrada, sliders, checks, enlaces, cabeceras de
  tabla) y los grises azulados de fábrica pasan a gris neutro: la interfaz no
  queda coloreada a medias ni con restos del azul clásico.
- La **cámara activa** elegida con "Usar" se persiste por id en la configuración
  (default automática si el id ya no existe al cargar).
- **Cámaras como componente** con vistas previas en vivo (render a FBO) — ver [CAMARAS_VISTAS_PREVIAS.md](CAMARAS_VISTAS_PREVIAS.md).
- **Iluminación** gestionada por `LightSystem` (slots `GL_LIGHT0..7`, marcadores de luz y cámara en escena) y **materiales** con presets (`MaterialPresets`).
- Menú de inicio modular (paquete `MenusGUI`, patrón MVP): idioma, nombre del proyecto y sensibilidad de cámara.
- **Configuración del editor persistida en JSON** (nlohmann/json): proyecto, idioma, sensibilidad, gizmo activo, ventana de cámaras, estado de las ventanas y perfil de apariencia. Una sola implementación: `ConfigPersistence` (JSON puro) sobre `ProjectPaths` (rutas), con `EditorConfig` como fachada estable. Dos archivos junto al binario: `<directorioEjecutable>/MotorGrafico/Configuraciones/Configuracion.json` (general) y `MotorGrafico/Proyects/<proyecto>/Memory/ConfiguracionProyecto.json` (por proyecto). **Escritura atómica** (temporal + rename: un corte no corrompe el archivo) y **guardado diferido** de la general (los cambios en vivo de Opciones se escriben como máximo una vez cada 250 ms, y siempre al salir o con Ctrl+S). Tolerante a archivos ausentes o corruptos.
- **Caché de assets compartida** (Flyweight): `AssetManager` (meshes CPU) y `TextureManager` (imágenes), con rutas normalizadas (`AssetPath`) y loader inyectable.
- Estructuras de datos propias (listas, árboles, heaps, mergesort) y jerarquía de excepciones propia: las usadas por el motor (`ListaDE`, `ArbolEnlazado`, `PriorityListaDE`) están corregidas y verificadas, y las implementadas para el motor (`MinHeap`, `MaxHeap`, `ListMergeSort`, `ArbolBinarioEnlazado`) cuentan con pruebas headless.
- **Pruebas headless** (`tests/`, CTest) y **CI multiplataforma** en GitHub Actions.
- Build reproducible en **Linux** y **Windows** (y pruebas en macOS) con **CMake**; ASan+UBSan por defecto en Debug.

---

## Requisitos

| Dependencia    | Versión mínima | Notas                                                        |
|----------------|----------------|--------------------------------------------------------------|
| CMake          | 3.15           |                                                              |
| Compilador     | C++17          | GCC / Clang / MSVC                                           |
| GLFW           | 3.x            | `libglfw3-dev`                                               |
| OpenGL 3.3 core | 3.3            | Funciones modernas por puntero; **no** requiere GLU |
| Bullet Physics | 3.x            | `libbullet-dev`                                              |
| Assimp         | 5.x            | `libassimp-dev`                                              |
| GLM            | 0.9.9+         | Del sistema (`libglm-dev`); el CMake usa `External/glm` si existe y cae al sistema si no |
| nlohmann/json  | —              | Vendoriado en `FunshiEngineGL/External/nlohmann`             |
| ncurses        | cualquiera     | `libncurses-dev` (solo Linux)                                |
| X11            | —              | `libx11-dev`, `libxrandr-dev`, `libxi-dev`                   |
| JDK            | 17+            | Solo scripts **Java**; se auto-habilita si el build encuentra el JDK. En runtime el motor lo busca solo: `FUNSHI_LIBJVM`, `JAVA_HOME`, un `jre/` junto al `.exe`, el registro de Windows y `/usr/lib/jvm`. El instalador de Windows lo ofrece descargar si falta |

### Instalar dependencias en Ubuntu/Debian

```bash
sudo apt install cmake build-essential ninja-build \
    libglfw3-dev libglm-dev \
    libbullet-dev libassimp-dev \
    libncurses-dev libx11-dev \
    libxrandr-dev libxi-dev
```

(Es el mismo conjunto que instala el job de CI de Ubuntu.)

---

## Compilar y ejecutar

```bash
# Clonar el repositorio
git clone <url-del-repo>
cd FunshiEngineGL

# Configurar (Debug con ASan/UBSan por defecto)
cmake -B build -S FunshiEngineGL

# Compilar
cmake --build build -j$(nproc)

# Ejecutar (Linux)
./build/FunshiEngineGL

# Ejecutar (Windows, con la carpeta de trabajo en la raiz del proyecto:
# los assets relativos, como la carpeta Imagenes/, se resuelven contra el cwd)
cd FunshiEngineGL
build\FunshiEngineGL.exe
```

Build de Release más rápido (sin sanitizers):

```bash
cmake -B build -S FunshiEngineGL -DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF
```

En Windows la misma receta funciona con el generador de Visual Studio, que deja la solución en el directorio de build (por ejemplo `build-win\FunshiEngineGL.sln`) para abrirla desde el IDE. CMake es el **único** build soportado: no hay proyectos de Visual Studio mantenidos a mano en el repositorio, así que siempre conviene `cmake -B build -S FunshiEngineGL` para tener un build portable.

> El primer arranque crea su configuración en `MotorGrafico/`: la escena serializada, la configuración global en `Configuraciones/Configuracion.json`, la de cada proyecto en `Proyects/<proyecto>/Memory/ConfiguracionProyecto.json` y el layout `imgui.ini` del editor.
>
> Esa carpeta va **junto al binario** cuando el motor está en una carpeta donde puede escribir (build de desarrollo, instalación portátil). Si no puede —el caso típico es instalado en `C:\Program Files`, que el proceso no escribe sin elevar—, el motor detecta que no tiene acceso de escritura y usa en su lugar la carpeta de datos del usuario: `%APPDATA%\FunshiEngineGL\MotorGrafico` en Windows y `$XDG_DATA_HOME/FunshiEngineGL/MotorGrafico` (o `~/.local/share/...`) en Linux y macOS. La decisión se toma una vez al arrancar y se informa por consola. Si quedaran datos en la ubicación anterior, se copian una sola vez a la nueva sin pisar nada que ya exista allí.


---

## Pruebas y CI

Las pruebas son headless (sin pila gráfica), corren con CTest y hay **20 targets**
(decinueve siempre + `scripts-java-tests` si el build encontró el JDK):

```bash
# Compilar TODAS las pruebas (la misma lista que compila la CI en el job del
# engine: ver .github/workflows/ci.yml, release.yml y windows-release.yml)
cmake --build build --target filemanager-tests configuracion-tests eventbus-tests menu-tests tema-tests assetmanager-tests texturemanager-tests estructuras-tests rendering-tests scripts-tests scripts-runtime-tests audio-tests userinterface-tests model-serialization-tests manifiesto-assets-tests orquestador-estado-tests comandos-tests escena-serializacion-tests proceso-tests
ctest --test-dir build --output-on-failure
```

- `filemanager-tests` (82 verificaciones): explorador de archivos (`GestorDeArchivos`/`FileManager`/`FileSystemWatcher`) y el contrato de la plantilla de script C++ (la fábrica viaja con el macro portable de exportación, obligatorio en MSVC).
- `proceso-tests` (26): el runner de procesos sin shell `Proceso`: round-trip de argv byte a byte relanzando el propio binario copiado a una carpeta **con espacios** con argumentos hostiles (espacios, operadores de shell, comilla interior, barra final, argumento vacío), exit codes, truncado del log, `cwd`, entorno extra, programa inexistente, la tabla de citación de `citar()` (antes en `scripts-tests`) y, en Windows, la receta cruda de `cmd.exe` del harvest de vcvars.
- `configuracion-tests` (128): `EditorConfig` sobre `ConfigPersistence`/`ProjectPaths` (round-trip general y por proyecto, prioridad de las claves modernas sobre el `menu/*` legacy, tolerancia a archivos ausentes/corruptos/parciales, `restablecer`, escritura atómica sin temporales colgados y guardado diferido con `volcarGuardadoGeneral`) y el cotejo de prefijos de ruta `rutaBajo` (en Windows `/` y `\` equivalen, que es lo que hace posible reescribir las referencias al mover/renombrar).
- `eventbus-tests` (16): canal tipado de GUI interna (`EditorEventBus`).
- `menu-tests` (38): `MenuModel` (traducción en vivo, observer de cambios y reset).
- `tema-tests` (28): `TemaEditor` (aplicación del perfil `Apariencia` al estilo ImGui): el acento llega a **todos** los roles y ningún rol conserva el azul de fábrica de Dear ImGui (regresión "el color de acento no se aplica a toda la interfaz"), el acento por defecto no cambia el aspecto histórico, un acento translúcido no apaga los roles de primer plano, la aplicación es idempotente y el modo B/N deja la paleta monocroma.
- `assetmanager-tests` (82) y `texturemanager-tests` (15): caches Flyweight de meshes (incluido el cálculo de normales por cara, y el que rellena solo las normales que faltan en assets mixtos) e imágenes.
- `estructuras-tests` (87): listas, árboles, heaps y ordenamiento propios.
- `rendering-tests` (131): geometría de las líneas del pipeline moderno (`LineBuilder`: expansión de cada segmento al quad que ensancha el shader, color por extremo, polilíneas, aristas de collider y caja de 12 aristas), sin entrar a OpenGL.
- `scripts-tests` (100): reflexión `SerializeField` (campos, arrays, grupos y round-trip binario), el contrato de flags con el que `BackendCpp` compila el script C++ (mismo CRT dinámico que el engine, `/EHsc`, elección por familia de compilador —MSVC o GCC/Clang—, los ARGV armados sin flags cruzadas ni redirección de shell, y pedido de exportación de la fábrica en el link MSVC, `vcvars64Ruta` que termina en la raíz), el harvest del entorno de vcvars (receta cruda de `cmd` con `/U`, parser UTF-16 del `set /U` y bloque multi-sz ordenado) y el contrato del sondeo de toolchain (dispositivo nulo `NUL`/`/dev/null` abierto por el runner, sin `std::system` ni envoltorios de `cmd.exe`; un hijo `--hijo` ejercita el spawn real).
- `scripts-runtime-tests` (según toolchain): compila un script C++ real con `BackendCpp`, lo carga con `dlopen`/`LoadLibrary` y ejecuta el ciclo; además valida que un segundo componente sobre el **mismo** fuente reutiliza el artefacto al día en vez de volver a enlazarlo (en Windows una `.dll` cargada no se puede reescribir). Se omite si el sondeo del compilador de este build falla (SKIP: con MSVC hace falta `cl.exe` del toolchain de Visual Studio —el entorno del toolset, `INCLUDE`/`LIB`, lo obtiene `BackendCpp` del `vcvars64.bat` al compilar, no del shell—). En Windows con MinGW/GCC corre igual que en Linux.
- `scripts-java-tests` (según toolchain): end-to-end del backend Java (JNI); se compila si el build detecta el JDK (SKIP 77 sin JDK). El sondeo de `javac` resuelve la ruta real —`JAVAC`, `JAVA_HOME`, rutas del JDK— en vez de asumir el `javac` del PATH, así que también corre en Windows.
- `model-serialization-tests` (16): serialización binaria del componente `Model` (path con prefijo de longitud; regresión del core al cargar escenas con paths largos).
- `audio-tests` (16): `AudioEngine`/`AudioClipsManager` con `NullAudioBackend` (contrato de la cola de comandos: clips, handles, encolado, detención, volumen).
- `userinterface-tests` (34): `UserInterfaceCustom` (modelo del Creador de interfaces, `src/GUI/CreadorUI/`): round-trip JSON de los 5 tipos de widget, guardar/cargar y tolerancia a JSON parcial.
- `manifiesto-assets-tests` (29): `ManifiestoAssetsCore` (manifiesto `SceneAssets.json`): JSON round-trip, tolerancia a manifiestos corruptos/inexistentes, relativizar/absolutizar contra la raíz `src<proyecto>`/ y precedencia del manifiesto sobre el `.db`.
- `orquestador-estado-tests` (38): `OrquestadorEstadoGUI` (la "función de marco" de F5/F6/F7 y las teclas): reglas por estado de Play/Pausa/Stop, Escape → menú e "Iniciar Estudio" → editor; y los atajos del editor frente al teclado de ImGui (Ctrl+S guarda siempre; Ctrl+Z/Ctrl+Y ceden al campo de texto).
- `comandos-tests` (77): sistema de comandos (undo/redo) del editor: los 7 comandos con su deshacer/rehacer, la cadena de redo múltiple, el límite de 50 entradas y que `deshacer()`/`rehacer()` devuelvan la descripción del comando aplicado (la que muestra la barra de estado al pulsar `Ctrl+Z`/`Ctrl+Y`).
- `escena-serializacion-tests` (75): round-trip completo de escena (guardar → recargar → conservar nombre, id y jerarquía), el nombre por defecto de los objetos nuevos, que no puedan repetirse dentro del árbol, que guardar con el árbol vacío deje el archivo vacío **avisándolo en el log**, que `Binario` reporte cuando no pudo abrir un archivo (sin `std::remove()` destructivo previo, con valor de retorno `bool` y propagación en `saveEntity`/`loadEntity`), y las defensas del índice de escena: líneas corruptas del `BBDDObjetos.txt` saltadas con aviso en vez de leer la raíz como hijo (objeto fantasma "Scene"), auto-sanado de un hijo con id 0 al guardar, y que cuatro hermanos consecutivos sobrevivan intactos (el look-ahead no pierde la posición de ninguna línea), y la reescritura de referencias al reubicar: mover la carpeta de un script y renombrar las de malla y de textura reescribe exactamente las referencias que caen bajo el prefijo y deja intactas las demás; y el sanado de referencias rotas al cargar: si una
  ruta guardada ya no existe y el nombre base aparece **una sola vez** bajo la
  carpeta del proyecto, se repara y se guarda la escena; con varias o con
  ninguna no se adivina y queda el aviso en el log. Es la suite que faltaba: `model-serialization-tests` cubre el componente `Model` aislado.

Con `-DBUILD_ENGINE=OFF` se compilan **solo** las pruebas que no necesitan el engine: no se requieren GLFW/OpenGL/Bullet/Assimp y funcionan en cualquier plataforma. Quedan fuera las que enlazan `funshi_engine` (`comandos-tests`, `manifiesto-assets-tests`, `orquestador-estado-tests`, `tema-tests` y `escena-serializacion-tests`, que solo se compilan con `BUILD_ENGINE=ON`). `.github/workflows/ci.yml` hace exactamente eso en Linux y Windows (más el backend Java en Ubuntu con JDK), además de un build completo del engine en Ubuntu.

---

## Estructura del repositorio

```text
FunshiEngineGL/            ← raíz del repo
├── README.md                     ← visión general, build, controles y pendientes
├── PROJECT_STRUCTURE.md          ← arquitectura detallada
├── CAMARAS_VISTAS_PREVIAS.md     ← cámaras componente + vistas previas (Fase 2)
├── ARQUITECTURA_ESTADOS_GUI.md   ← estados/menú/GUI internas (diseño + Fases 1-3)
├── .github/workflows/            ← CI (build del engine + pruebas multiplataforma)
├── tests/                        ← pruebas headless: FileManager, EditorConfig,
│                                   EditorEventBus, MenuModel, Assets, Estructuras,
│                                   Scripts (reflexión y runtime C++/Java), Audio
│                                   y Creador de interfaces (UserInterface)
└── FunshiEngineGL/        ← proyecto principal
    ├── CMakeLists.txt
    ├── ImGuizmo/          ← dependencia externa integrada
    ├── External/          ← nlohmann/json vendoriado
    ├── Imagenes/          ← íconos del editor
    └── src/               ← todo el código fuente
        ├── main.cpp       ← composition root y bucle principal
        ├── Assets/        Behaviour/  Entity/  Estructuras/  Events/
        ├── Audio/         ExcepcionesCPP/  Fisicas/  FileManager/
        ├── GestorDeArchivos/  Configuracion/  GUI/  GUIManager/
        ├── Herramientas/  Iluminacion/  Input/  ImGui/
        ├── Matematicas/  Rendering/  Objetos/
        ├── Scenes/  States/  Ventana.*  EngineTime.*
```

Ver **PROJECT_STRUCTURE.md** para la descripción completa de cada módulo, las relaciones entre clases, el flujo de ejecución, las pruebas y los pendientes.

---

## Controles del editor

| Tecla / Acción          | Función                                                       |
|-------------------------|---------------------------------------------------------------|
| `W` / `A` / `S` / `D`   | Mover la cámara activa (con diagonales)                        |
| `Espacio` / `Shift izq` | Subir / bajar la cámara                                        |
| Mouse (sin UI capturada)| Navegación FPS de la cámara activa (sensibilidad de Opciones)  |
| `E`                     | Mostrar / ocultar las interfaces del editor                    |
| `Escape`                | Volver al menú de inicio                                       |
| `1` o `T`               | Gizmo: traslación                                              |
| `2` o `R`               | Gizmo: rotación                                                |
| `3` o `Y`               | Gizmo: escala                                                  |
| Clic en objeto          | Seleccionar objeto en el viewport                              |

> El modo Play/Stop se controla desde la barra de menú de la escena; la física solo simula en Play.

---

## Roadmap / Pendientes conocidos

- [ ] Sistema de animaciones.
- [ ] Cuadro de log de errores en el editor.
- [ ] Resolver IDs duplicados al crear objetos; limpiar binarios huérfanos al eliminar.
- [ ] Puente de input/audio/búsqueda para scripts (la infraestructura existe: `EditorInput`, `AudioEngine`, `SceneRegistry`; falta exponerla en la tabla `ApiScriptGameObject`).
- [ ] Terminar los popups del inspector; prefabs y duplicación de objetos.
- [ ] Portabilidad de rutas de assets (centralizar `HOME` / rutas de Windows).
- [ ] Versionado y validación de la serialización binaria.
- [ ] Extraer `SceneRenderer`/`PhysicsSystem`/`ScriptSystem` de `GameScene`; vistas previas seleccionables con clic.

---

## Licencia

Copyright 2026 Gianfranco Ivan Enrique

El código propio del motor se distribuye bajo la **Apache License 2.0** (ver
[`LICENSE`](LICENSE) y [`NOTICE`](NOTICE)).

Las bibliotecas de terceros integradas (Dear ImGui, ImGuizmo, nlohmann/json,
stb_image, GLM) conservan sus licencias originales —principalmente MIT— y no
están cubiertas por la Apache License 2.0. Consulta
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) para el detalle completo de
licencias, titulares y ubicaciones.
