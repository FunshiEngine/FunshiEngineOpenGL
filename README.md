# FunshiEngineGL

<p align="center">
  <img src="FunshiEngineGL/Imagenes/FunshiEngineGL_Logo_Principal_Blanco.png"
       alt="FunshiEngineGL" width="180">
</p>

Motor y editor 3D en tiempo real escrito en C++17, con interfaz ImGui y renderizado OpenGL construido desde cero.

---

## Características actuales

- Ventana y contexto OpenGL con **GLFW**; renderizado con **OpenGL / GLU** (pipeline inmediato).
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
- **Render híbrido**: los `Modelos3D` se dibujan con `MeshRenderer` (VBO/VAO + shaders vía `ShaderProgram`) y degradan a `glBegin/glEnd` en contextos legacy o mallas sin normales. La **grilla** es un componente (`Grid`, con visible/color/tamaño/separación) en una pasada independiente, cuyo color acompaña a la apariencia (incluido el modo blanco y negro).
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
| OpenGL / GLU   | cualquiera     | `libglu1-mesa-dev`                                           |
| Bullet Physics | 3.x            | `libbullet-dev`                                              |
| Assimp         | 5.x            | `libassimp-dev`                                              |
| GLM            | 0.9.9+         | Del sistema (`libglm-dev`); el CMake usa `External/glm` si existe y cae al sistema si no |
| nlohmann/json  | —              | Vendoriado en `FunshiEngineGL/External/nlohmann`             |
| ncurses        | cualquiera     | `libncurses-dev` (solo Linux)                                |
| X11            | —              | `libx11-dev`, `libxrandr-dev`, `libxi-dev`                   |
| JDK            | 17+            | Solo scripts **Java**; se auto-habilita si el build encuentra el JDK (opcional en runtime via `JAVA_HOME`) |

### Instalar dependencias en Ubuntu/Debian

```bash
sudo apt install cmake build-essential ninja-build \
    libglfw3-dev libglu1-mesa-dev libglm-dev \
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
```

Build de Release más rápido (sin sanitizers):

```bash
cmake -B build -S FunshiEngineGL -DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF
```

En Windows la misma receta funciona con el generador de Visual Studio. También existe `FunshiEngineGL.sln`, pero es un proyecto heredado con rutas absolutas de una máquina concreta: **prefiere siempre CMake** (`cmake -B build -S FunshiEngineGL`) para un build portable.

> El primer arranque crea su configuración en `MotorGrafico/` junto al binario (la carpeta que contiene el ejecutable): ahí viven la escena serializada, la configuración global en `Configuraciones/Configuracion.json`, la de cada proyecto en `Proyects/<proyecto>/Memory/ConfiguracionProyecto.json` y el layout `imgui.ini` del editor.


---

## Pruebas y CI

Las pruebas son headless (sin pila gráfica), corren con CTest y hay **17 targets**
(dieciséis siempre + `scripts-java-tests` si el build encontró el JDK):

```bash
cmake --build build --target filemanager-tests configuracion-tests eventbus-tests menu-tests tema-tests assetmanager-tests texturemanager-tests estructuras-tests scripts-tests scripts-runtime-tests manifiesto-assets-tests orquestador-estado-tests
ctest --test-dir build --output-on-failure
```

- `filemanager-tests` (27 verificaciones): explorador de archivos (`GestorDeArchivos`/`FileManager`/`FileSystemWatcher`).
- `configuracion-tests` (99): `EditorConfig` sobre `ConfigPersistence`/`ProjectPaths` (round-trip general y por proyecto, prioridad de las claves modernas sobre el `menu/*` legacy, tolerancia a archivos ausentes/corruptos/parciales, `restablecer`, escritura atómica sin temporales colgados y guardado diferido con `volcarGuardadoGeneral`).
- `eventbus-tests` (16): canal tipado de GUI interna (`EditorEventBus`).
- `menu-tests` (30): `MenuModel` (traducción en vivo, observer de cambios y reset).
- `tema-tests` (28): `TemaEditor` (aplicación del perfil `Apariencia` al estilo ImGui): el acento llega a **todos** los roles y ningún rol conserva el azul de fábrica de Dear ImGui (regresión "el color de acento no se aplica a toda la interfaz"), el acento por defecto no cambia el aspecto histórico, un acento translúcido no apaga los roles de primer plano, la aplicación es idempotente y el modo B/N deja la paleta monocroma.
- `assetmanager-tests` (47) y `texturemanager-tests` (15): caches Flyweight de meshes e imágenes.
- `estructuras-tests` (87): listas, árboles, heaps y ordenamiento propios.
- `scripts-tests` (42): reflexión `SerializeField` (campos, arrays, grupos y round-trip binario).
- `scripts-runtime-tests`: compila un script C++ real con `BackendCpp`, lo carga con `dlopen` y ejecuta el ciclo; se omite en Windows (SKIP, requiere `cl.exe` con entorno de Visual Studio).
- `scripts-java-tests`: end-to-end del backend Java (JNI); se compila si el build detecta el JDK (SKIP sin JDK).
- `audio-tests` (16): `AudioEngine`/`AudioClipsManager` con `NullAudioBackend` (contrato de la cola de comandos: clips, handles, encolado, detención, volumen).
- `userinterface-tests` (34): `UserInterfaceCustom` (modelo del Creador de interfaces, `src/GUI/CreadorUI/`): round-trip JSON de los 5 tipos de widget, guardar/cargar y tolerancia a JSON parcial.
- `manifiesto-assets-tests` (29): `ManifiestoAssetsCore` (manifiesto `SceneAssets.json`): JSON round-trip, tolerancia a manifiestos corruptos/inexistentes, relativizar/absolutizar contra la raíz `src<proyecto>`/ y precedencia del manifiesto sobre el `.db`.
- `orquestador-estado-tests` (34): `OrquestadorEstadoGUI` (la "función de marco" de F5/F6/F7 y las teclas): reglas por estado de Play/Pausa/Stop, Escape → menú e "Iniciar Estudio" → editor.
- `comandos-tests` (77): sistema de comandos (undo/redo) del editor: los 7 comandos con su deshacer/rehacer, la cadena de redo múltiple, el límite de 50 entradas y que `deshacer()`/`rehacer()` devuelvan la descripción del comando aplicado (la que muestra la barra de estado al pulsar `Ctrl+Z`/`Ctrl+Y`).

Con `-DBUILD_ENGINE=OFF` se compilan **solo** las pruebas: no se requieren GLFW/OpenGL/Bullet/Assimp y funcionan en cualquier plataforma. `.github/workflows/ci.yml` hace exactamente eso en Linux, Windows y macOS (más el backend Java en Ubuntu con JDK), además de un build completo del engine en Ubuntu.

---

## Estructura del repositorio

```text
FunshiEngineGL/            ← raíz del repo
├── README.md                     ← visión general, build, controles y pendientes
├── PROJECT_STRUCTURE.md          ← arquitectura detallada
├── CAMARAS_VISTAS_PREVIAS.md     ← cámaras componente + vistas previas (Fase 2)
├── ARQUITECTURA_ESTADOS_GUI.md   ← estados/menú/GUI internas (diseño + Fases 1-3)
├── FunshiEngineGL.sln            ← solución Visual Studio (Windows, heredada)
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
- [x] Scripts dinámicos: `SerializeField` con reflexión, compilación en caliente de C++ (`BackendCpp`) y Java vía JNI (`BackendJava`, auto-activado con JDK en build), ciclo `onStart`/`onUpdate`/`onStop` y hot reload.
- [ ] `CommandManager` para undo/redo.
- [ ] Cuadro de log de errores en el editor.
- [ ] Resolver IDs duplicados al crear objetos; limpiar binarios huérfanos al eliminar.
- [ ] Puente de input/audio/búsqueda para scripts (la infraestructura existe: `EditorInput`, `AudioEngine`, `SceneRegistry`; falta exponerla en la tabla `ApiScriptGameObject`).
- [ ] Terminar los popups del inspector; prefabs y duplicación de objetos.
- [ ] Portabilidad de rutas de assets (centralizar `HOME` / rutas de Windows).
- [ ] Migrar o eliminar el `FunshiEngineGL.vcxproj` (aún arrastra rutas absolutas de una máquina concreta; el build soportado es CMake).
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
