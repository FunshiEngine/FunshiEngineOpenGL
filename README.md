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
- Sistema **Entity–Component**: `Transform`, `Color`, `Model`, `Material`, `Light`, `CameraComponent`, `Grid`, `Skybox` (cubemap de 6 caras que reemplaza al cielo degradado), colliders (esfera / cubo / malla), `RigidBody`, `AudioSource`, `InterfaceComponent` (HUD por asset JSON del CreadorDeInterfaces) y `Script`.
- **Scripts dinámicos** (`Script` + `IScriptBehaviour`): reflexión por macros con campos `SerializeField` (escalares, arrays y grupos anidados) editables en el inspector; compilación en caliente de C++ a `.so`/`.dll` (`BackendCpp`) y soporte de **Java vía JNI** (`BackendJava`, se activa automáticamente si el build encuentra el JDK). Hot reload por fecha de modificación que reinyecta los valores serializados, y ciclo `onStart`/`onUpdate`/`onStop`.
- **Jerarquía de objetos** con árbol enlazado propio (`ArbolEnlazado<GameObject*>`) y reparentado seguro (rechaza ciclos y la raíz).
- Carga de modelos 3D con **Assimp** (`.obj`, `.fbx` y formatos soportados por Assimp).
- **Física con Bullet** detrás de una fachada desacoplada (`PhysicsEngine` → `IPhysicsBackend` → `BulletPhysicsAdapter`): solo simula en modo Play, sincroniza transformaciones entre objeto, collider y cuerpo, y admite un gizmo dedicado para el collider activo.
- **Serialización binaria de escenas** en preorden con marcadores `=>`/`<=`: guarda y recupera la jerarquía completa (padres e hijos) de forma recursiva.
- **EventBus** con suscripción tipada (creación, eliminación, reparentado, selección y cambios de componentes) + **EditorEventBus**: canal tipado de GUI interna (apariencia, idioma, sensibilidad, cámara activa y visibilidad de ventanas) que median entre el menú, las ventanas del editor y la escena sin pasarse punteros.
- **Máquina de estados** de la aplicación: `MainMenu`, `Editing`, `Playing`, `Exiting`, con reglas de transición centralizadas en `OrquestadorEstadoGUI`.
- **Input modularizado** (`src/Input/EditorInput`): las callbacks de teclado/mouse de GLFW viven en su propio módulo (extraídas de `main.cpp`); traducen los eventos a acciones del editor (E, G, gizmos, Escape, clic derecho para navegar) y mantienen una máquina de estado de teclas WASD/Espacio/Shift con movimiento continuo normalizado por frame (diagonales a la misma velocidad que un eje).
- **Render de un solo pipeline**: los `Modelos3D` se dibujan con `MeshRenderer` (VBO/VAO + shaders vía `ShaderProgram`); una malla sin normales por vértice no se dibuja y se avisa una vez por consola (`Mesh::computeNormals()` las genera). La **grilla** es un componente (`Grid`, con `visible` y `color`, en una pasada independiente) que se dibuja como un plano infinito de densidad fija —secundarias cada unidad, una principal cada cinco— recortado a un círculo-horizonte centrado en la cámara cuyo radio elige el usuario (Opciones → *Radio de difuminado*, de 20 a 600 unidades, 150 por defecto; el tramo opaco es siempre la misma fracción del radio, 40 de cada 150, y la caída es cuadrática hasta 0 en el propio horizonte), resuelto por alpha por vértice y compartido con la guía de eje para que ambas se desvanezcan en el mismo círculo; su color efectivo sale del perfil de apariencia (en modo blanco y negro se ignoran tanto el color del componente como el elegido por el usuario, y se usa blanco o negro según el tema). Al igual que los marcadores de luz/cámara y los gizmos de los colliders, se dibuja con el pipeline de líneas (batch en GPU + shader de ancho en píxeles: 1 px las secundarias, 2 px las principales, 3 px los ejes). Los tres ejes (X rojo, Y verde, Z azul) tienen un único color de base en toda la escena, el de la guía de eje —que a su vez usa el gizmo como referencia—, y la grilla y la guía ajustan ese color por contraste contra lo que tienen debajo.
- **Audio en runtime** (`src/Audio/`): `AudioEngine` (fachada thread-safe con cola + hilo de audio) sobre backends intercambiables (`MiniAudioBackend` con miniaudio, `NullAudioBackend`); `AudioClipsManager` descubre los clips de `Sonidos/` y los registra por nombre; `AudioSource` reproduce con volumen, loop y autoplay.
- **Ventana "Estado"** (`StatusBarInterface`): muestra el toolchain externo (compilador C++, javac, libjvm) y el estado de compilación/carga de los scripts de la escena.
- Explorador de archivos del proyecto con fachada propia (`FileManager`), estado de navegación compartido (`FileSelection`) y vigilancia de cambios externos (`FileSystemWatcher`).
- **Apariencia del editor configurable** (perfil persistido en `Configuracion.json`):
  tema claro/oscuro (arranca en **oscuro**), **modo blanco y negro** (desatura toda
  la interfaz y fuerza el fondo del viewport y el color de la grilla a blanco o
  negro según el tema, ignorando los colores elegidos), color de acento de la
  interfaz (solo RGB: la transparencia la define el tema) y los dos colores del
  cielo, `fondoSuperior` (cenit) y `fondoInferior` (suelo), con `radioDifuminado`
  como media anchura de la transicion alrededor del horizonte; todo aplicado en
  vivo por `TemaEditor`/`AparienciaUtil`.
  El acento se inyecta en **todos** los roles visuales de ImGui (botones,
  solapas del dock, campos de entrada, sliders, checks, enlaces, cabeceras de
  tabla) y los grises azulados de fábrica pasan a gris neutro: la interfaz no
  queda coloreada a medias ni con restos del azul clásico.
- La **cámara activa** elegida con "Usar" se persiste por id en la configuración
  del proyecto (default automática si el id ya no existe al cargar).
- **Cámaras como componente** con vistas previas en vivo (render a FBO) — ver [CAMARAS_VISTAS_PREVIAS.md](CAMARAS_VISTAS_PREVIAS.md).
- **Iluminación** gestionada por `LightSystem` (slots `GL_LIGHT0..7`, marcadores de luz y cámara en escena) y **materiales** con presets (`MaterialPresets`).
- Menú de inicio modular (paquete `MenusGUI`, patrón MVP): nombre del proyecto y una vista **Opciones** con idioma, las dos sensibilidades y la apariencia (tema, modo blanco y negro, acento y fondo).
- **Configuración del editor persistida en JSON** (nlohmann/json): una sola implementación, `ConfigPersistence` (JSON puro) sobre `ProjectPaths` (rutas), con `EditorConfig` como fachada estable. Dos archivos junto al binario: `<directorioEjecutable>/MotorGrafico/Configuraciones/Configuracion.json` (general: último proyecto, idioma, las dos sensibilidades y el perfil de apariencia) y `MotorGrafico/Proyects/<proyecto>/Memory/ConfiguracionProyecto.json` (por proyecto: gizmo, ventana de cámaras, estado de las ventanas y cámara activa). **Escritura atómica** (temporal + rename: un corte no corrompe el archivo) y **guardado diferido** de la general (los cambios en vivo de Opciones se escriben como máximo una vez cada 250 ms, y siempre al salir o con Ctrl+S). Tolerante a archivos ausentes o corruptos.
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
> Esa carpeta va **junto al binario** cuando el motor está en una carpeta donde puede escribir: el build de desarrollo y también la instalación de Windows, que es por usuario (`%LOCALAPPDATA%\Programs`, sin UAC). Si el motor está en una carpeta donde no puede escribir —una copia en `Program Files`, una carpeta montada en solo lectura, un proceso corriendo como administrador—, detecta que no tiene acceso de escritura y usa en su lugar la carpeta de datos del usuario: `%APPDATA%\FunshiEngineGL\MotorGrafico` en Windows y `$XDG_DATA_HOME/FunshiEngineGL/MotorGrafico` (o `~/.local/share/...`) en Linux y macOS. La decisión se toma una vez al arrancar y se informa por consola. Si quedaran datos en la ubicación anterior, se copian una sola vez a la nueva sin pisar nada que ya exista allí.


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

- `filemanager-tests` (190 verificaciones; 186 en Windows, donde una barra invertida no puede formar parte de un nombre): explorador de archivos (`GestorDeArchivos`/`FileManager`/`FileSystemWatcher`), el arrastre (`soltarEnCarpeta`) con invalidación explícita de caché del grid en carpeta origen y destino y con el nombre del elemento resuelto con el separador que separa en cada plataforma (en Linux una barra invertida es parte del nombre y no inventa una carpeta), la copia de carpetas (`copiarCarpeta`, que rechaza un destino dentro de la propia carpeta de origen en vez de reproducirse al copiar), el renombre por click derecho (`RenombrarElemento`, compartido por el árbol y el grid: ruta nueva hermana de la vieja, sin pisar destinos existentes, y un único `ArchivosReubicados` sólo si el cambio se hizo), la creación de carpetas (`CrearCarpeta`, compartida por el árbol y el grid: nombre válido, aviso de nombre repetido en vez de reportar un éxito que no ocurrió, y una sola subida del contador de cambios) y el contrato de la plantilla de script C++ (la fábrica viaja con el macro portable de exportación, obligatorio en MSVC), más la lógica pura del selector de caras del cubemap (`SelectorArchivoCubemap`: filtro de extensiones, aviso de caras faltantes y comparación de resolución entre caras (en píxeles, con el mismo criterio que el render)).
- `proceso-tests` (24): el runner de procesos sin shell `Proceso`: round-trip de argv byte a byte relanzando el propio binario copiado a una carpeta **con espacios** con argumentos hostiles (espacios, operadores de shell, comilla interior, barra final, argumento vacío), exit codes, truncado del log, `cwd`, entorno extra, programa inexistente, la tabla de citación de `citar()` (antes en `scripts-tests`) y, en Windows, la receta cruda de `cmd.exe` del harvest de vcvars.
- `configuracion-tests` (169): `EditorConfig` sobre `ConfigPersistence`/`ProjectPaths` (round-trip general y por proyecto, prioridad de las claves modernas sobre el `menu/*` legacy, tolerancia a archivos ausentes/corruptos/parciales, `restablecer`, escritura atómica sin temporales colgados, guardado diferido con `volcarGuardadoGeneral` y política de proyecto inicial `ProyectoInicial::resolver`), los colores del cielo (se conservan tal como se guardaron —un cielo claro incluido— y solo se acotan los componentes fuera de `[0, 1]`) el cotejo de prefijos de ruta `rutaBajo` (en Windows `/` y `\` equivalen y el cotejo ignora mayúsculas y minúsculas, como el sistema de archivos; en Linux se mantiene sensible al caso) y las rutas que el exportador del juego standalone copia al `Data/` del proyecto (`RutasExportacion.h`, que las pide a `ProjectPaths` en vez de armarlas a mano: sonidos y fuentes de script bajo `src<nombre>/`, configuración dentro de `Memory/`).
- `eventbus-tests` (17): canal tipado de GUI interna (`EditorEventBus`).
- `menu-tests` (40): `MenuModel` (traducción en vivo, observer de cambios —sin notificar al reaplicar una apariencia idéntica— y reset).
- `tema-tests` (28): `TemaEditor` (aplicación del perfil `Apariencia` al estilo ImGui): el acento llega a **todos** los roles y ningún rol conserva el azul de fábrica de Dear ImGui (regresión "el color de acento no se aplica a toda la interfaz"), el acento por defecto no cambia el aspecto histórico, un acento translúcido no apaga los roles de primer plano, la aplicación es idempotente y el modo B/N deja la paleta monocroma.
- `assetmanager-tests` (95) y `texturemanager-tests` (15): caches Flyweight de meshes (incluido el cálculo de normales por cara, el que rellena solo las normales que faltan en assets mixtos, la cache de fallos de carga y la normalización de rutas UNC y de prefijo `\\?`) e imágenes.
- `estructuras-tests` (87): listas, árboles, heaps y ordenamiento propios.
- `rendering-tests` (178): geometría de las líneas del pipeline moderno (`LineBuilder`: expansión de cada segmento al quad que ensancha el shader, color por extremo, polilíneas, aristas de collider y caja de 12 aristas), la guía de eje (`GuiaEje`: rectas hasta el horizonte con difuminado por vértice, convención de color por eje y el color efectivo que ajusta el contraste contra la grilla), el difuminado del piso (`Difuminado`: radio configurable, inicio proporcional y curva de opacidad) y el cielo degradado (`Cielo`: colores superior/inferior con B/N y tema) y la identidad del cubemap del Skybox (`CacheCubemap`: clave de las 6 caras por ruta y fecha de modificacion que decide cuando re-subirlo), sin entrar a OpenGL.
- `scripts-tests` (137): reflexión `SerializeField` (campos, arrays, grupos y round-trip binario), el contrato de flags con el que `BackendCpp` compila el script C++ (mismo CRT dinámico que el engine, `/EHsc`, elección por familia de compilador —MSVC o GCC/Clang—, los ARGV armados sin flags cruzadas ni redirección de shell, y pedido de exportación de la fábrica en el link MSVC, `vcvars64Ruta` que termina en la raíz, el descubrimiento del toolset MSVC en la máquina del usuario —que busca `vcvars64.bat` a dos niveles, cubre las Build Tools y respeta el orden de preferencia de raíces— y la decisión de qué compilador invocar —override del entorno, valor horneado solo si existe en disco, `cl` del PATH con toolset o `g++`—), la resolución de la carpeta de cabeceras del script (`RutaCabecerasScript`, que resuelve `FUNSHI_SRC_DIR` —el paquete deja el nombre relativo `include`— contra la carpeta del ejecutable en la app instalada, sin devolver rutas muertas), el harvest del entorno de vcvars (receta cruda de `cmd` con `/U`, parser UTF-16 del `set /U` y bloque multi-sz ordenado) y el contrato del sondeo de toolchain (dispositivo nulo `NUL`/`/dev/null` abierto por el runner, sin `std::system` ni envoltorios de `cmd.exe`; un hijo `--hijo` ejercita el spawn real). El emparejamiento de `libjvm` y `javac` de una misma raíz de JDK (`ResolucionJdk`) y la paridad del requisito de JDK del instalador (que exige `jvm.dll` **y** `javac.exe`, no solo la JVM).
- `scripts-runtime-tests` (según toolchain): compila un script C++ real con `BackendCpp`, lo carga con `dlopen`/`LoadLibrary` y ejecuta el ciclo; además valida que un segundo componente sobre el **mismo** fuente reutiliza el artefacto al día en vez de volver a enlazarlo (en Windows una `.dll` cargada no se puede reescribir) y que, sin las cabeceras del motor, el backend avisa con su propio mensaje en vez de lanzar el compilador contra un `-I` inexistente. Se omite si el sondeo del compilador de este build falla (SKIP: con MSVC hace falta `cl.exe` del toolchain de Visual Studio —el entorno del toolset, `INCLUDE`/`LIB`, lo obtiene `BackendCpp` del `vcvars64.bat` al compilar, no del shell—). En Windows con MinGW/GCC corre igual que en Linux.
- `scripts-java-tests` (según toolchain): end-to-end del backend Java (JNI); se compila si el build detecta el JDK (SKIP 77 sin JDK). El sondeo de `javac` resuelve la ruta real —`JAVAC`, `JAVA_HOME`, rutas del JDK— en vez de asumir el `javac` del PATH, así que también corre en Windows. Cuando una clase no carga, el mensaje incluye la causa real que lanzó la JVM en vez de reportarlo todo como "clase no encontrada".
- `model-serialization-tests` (16): serialización binaria del componente `Model` (path con prefijo de longitud; regresión del core al cargar escenas con paths largos).
- `audio-tests` (16): `AudioEngine`/`AudioClipsManager` con `NullAudioBackend` (contrato de la cola de comandos: clips, handles, encolado, detención, volumen).
- `userinterface-tests` (42): `UserInterfaceCustom` (modelo del Creador de interfaces, `src/GUI/CreadorUI/`): round-trip JSON de los 5 tipos de widget, guardar/cargar y tolerancia a JSON parcial; y `BarraProgresoTexto` (formato de la barra de la ventana Estado, acotado ante `hecha > total` y `total == 0`).
- `manifiesto-assets-tests` (37): `ManifiestoAssetsCore` (manifiesto `SceneAssets.json`): JSON round-trip (incluidas las seis caras del cubemap del Skybox), tolerancia a manifiestos corruptos/inexistentes, relativizar/absolutizar contra la raíz `src<proyecto>`/ y precedencia del manifiesto sobre el `.db`.
- `orquestador-estado-tests` (54): `OrquestadorEstadoGUI` (la "función de marco" de F5/F6/F7, del botón Activar/Detener y de las teclas): reglas por estado de Play/Pausa/Stop, Escape por estado (en play detiene la simulación, en editor vuelve al menú), la condición compartida por las teclas del editor (editor o play) e "Iniciar Estudio" → editor; y los atajos del editor frente al teclado de ImGui (Ctrl+S guarda siempre; Ctrl+Z/Ctrl+Y ceden al campo de texto).
- `comandos-tests` (77): sistema de comandos (undo/redo) del editor: los 7 comandos con su deshacer/rehacer, la cadena de redo múltiple, el límite de 50 entradas y que `deshacer()`/`rehacer()` devuelvan la descripción del comando aplicado (la que muestra la barra de estado al pulsar `Ctrl+Z`/`Ctrl+Y`).
- `escena-serializacion-tests` (151): round-trip completo de escena (guardar → recargar → conservar nombre, id y jerarquía), el nombre por defecto de los objetos nuevos, que no puedan repetirse dentro del árbol, que guardar con el árbol vacío deje el archivo vacío **avisándolo en el log**, que `Binario` reporte cuando no pudo abrir un archivo (sin `std::remove()` destructivo previo, con valor de retorno `bool` y propagación en `saveEntity`/`loadEntity`), y las defensas del índice de escena: líneas corruptas del `BBDDObjetos.txt` saltadas con aviso en vez de leer la raíz como hijo (objeto fantasma "Scene"), auto-sanado de un hijo con id 0 al guardar, y que cuatro hermanos consecutivos sobrevivan intactos (el look-ahead no pierde la posición de ninguna línea), y las rutas de las seis caras del Skybox: se persisten relativas a la raíz de assets (como la malla, las texturas y el script), una relativa se resuelve a absoluta al cargar, una absoluta del formato viejo se deja intacta y una cara sin asignar sigue vacía; y la reescritura de referencias al reubicar: mover la carpeta de un script y renombrar las de malla y de textura reescribe exactamente las referencias que caen bajo el prefijo y deja intactas las demás; y el sanado de referencias rotas al cargar: si una
  ruta guardada ya no existe y el nombre base aparece **una sola vez** bajo la
  carpeta del proyecto, se repara y se guarda la escena; con varias o con
  ninguna no se adivina y queda el aviso en el log; la resolución del componente `Model` con la matriz mundial del objeto; y la separación del evento estructural (`ComponentStructureChanged`) del de propiedad en el bus de escena: editar un campo no reconstruye los paneles del Inspector y agregar un componente crea solo el que falta; el reparentado que preserva la pose mundial (el local pasa a `inverse(padre) × mundo`, los descendientes no saltan), refresca el cuerpo físico del objeto y sobrevive el guardado; y el predicado que decide si "Desanidar a raíz" aplica (un hijo directo de la raíz ya está al nivel superior). Es la suite que faltaba: `model-serialization-tests` cubre el componente `Model` aislado.

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
| `F5` / `F6` / `F7`      | Simular / pausar-reanudar / detener la simulación              |
| `Escape`                | En play: detener. En edición: volver al menú de inicio        |
| `Ctrl+S`                | Guardar el proyecto en caliente                                |
| `Ctrl+Z` / `Ctrl+Y`     | Deshacer / rehacer la última acción del editor                |
| `1` o `T`               | Gizmo: traslación                                              |
| `2` o `R`               | Gizmo: rotación                                                |
| `3` o `U`               | Gizmo: escala                                                  |
| `G`                     | Gizmo local / mundo (también la guía de eje)                   |
| `X` / `Y` / `Z`         | Guía de eje del objeto seleccionado                            |
| Clic en objeto          | Seleccionar objeto en el viewport                              |

> El movimiento con `WASD` y `Espacio` solo actúa con las interfaces ocultas
> (`E`) o con el clic derecho sostenido sobre el viewport, y funciona tanto en
> edición como durante el play.
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
