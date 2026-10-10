## 11. Estructura de carpetas MotorGrafico/

El motor organiza sus datos junto al ejecutable (`{app}/MotorGrafico/`):

```
MotorGrafico/
├── Proyects/
│   └── <proyecto>/
│       ├── Memory/
│       │   ├── Binarios/Scene
│       │   ├── Interfaces/
│       │   ├── ConfiguracionProyecto.json
│       │   └── imgui.ini
│       └── src<proyecto>/
│           ├── modelos/
│           ├── Sonidos/
│           └── Scripts/
├── Configuraciones/
│   └── Configuracion.json        ← global (ultimo proyecto, idioma, las dos
│                                   sensibilidades y la apariencia: tema claro/oscuro,
│                                   modo blanco y negro, acento y fondo)
└── Exportaciones/
    └── <nombreExportacion>/
        ├── <Juego>.exe / <Juego>
        ├── Assets/                ← recursos del proyecto (en la raíz)
        ├── scenes/                ← escenas del juego
        │   ├── Scene.db
        │   └── SceneDir/
        ├── scripts/               ← fuentes de usuario (C++, Java)
        ├── cache/                 ← binarios compilados de scripts
        ├── include/               ← cabeceras del motor (scripts en runtime)
        ├── lib/                   ← deps runtime (.so Linux)
        └── JuegoExportado.json    ← metadatos del proyecto
```

Al arrancar, `EditorConfig::asegurarEstructuraProyecto()` crea la estructura
y migra automaticamente proyectos antiguos (directamente bajo `MotorGrafico/`)
a `MotorGrafico/Proyects/`, y la config global a `Configuraciones/`.

## 1. Como lanzar (sin terminal atras de la ventana)

El ejecutable NO necesita una terminal: al arrancar redirige toda la salida de
consola (stdout/stderr, cout/cerr, printf) a un archivo de logs, asi no se
escupe texto a ninguna consola.

- **Doble clic** sobre el binario (o abrirlo con tu gestor de archivos): sin
  terminal.
- **`./ejecutar.sh`** (solo Linux): compila si hace falta y lanza el editor
  desacoplado (en segundo plano) escribiendo al log. En Windows no funciona
  (es un script de bash que espera el binario `build/FunshiEngineGL` sin
  `.exe`); la via equivalente es lanzar `build\FunshiEngineGL.exe` como se
  indica abajo.
- **Escritorio Linux** (`.desktop` en `~/.local/share/applications/`):

  ```
  [Desktop Entry]
  Name=FunshiEngineGL
  Exec=/ruta/a/FunshiEngineGL/build/FunshiEngineGL
  Type=Application
  ```

Solo el lanzamiento **manual desde una terminal** (`./FunshiEngineGL`)
mantiene esa terminal como padre del proceso; incluso ahi la salida ya va al
log y la terminal no muestra basura.

### 1.1 Lanzar en Windows

El binario de Windows (`build\FunshiEngineGL.exe`, construido como app GUI, sin
consola) se lanza igual que en Linux, pero la **carpeta de trabajo (`cwd`)
importa**: los assets relativos —en particular la carpeta `Imagenes/` con los
36 iconos del explorador— se buscan primero junto al ejecutable y despues
relativo al `cwd`. Si ninguna ruta coincide, el editor abre y funciona pero la
grilla de iconos queda vacia, y la unica pista son las lineas `[IconosGUI]` del
log. Por eso:

```bat
cd FunshiEngineGL
build\FunshiEngineGL.exe
```

Lanzar con doble clic desde el Explorador equivale a lo mismo (el `cwd` es la
carpeta del proyecto). No lanzar el `.exe` desde otra carpeta esperando que
"encuentre solo" los assets: en ese caso revisa el log para confirmar donde
resolvio `Imagenes/`.

## 2. Logs

- Carpeta: `logs/` **junto al ejecutable** (se crea sola).
- Archivo: `FunshiEngineGL_AAAAMMDD_HHMMSS.log` (uno por arranque, con
  timestamp UTC), en modo append si ya existia.
- Contiene el diagnostico de GPU, los mensajes `[diag]` del render y cualquier
  error (`[MeshRenderer]`, `[IconosGUI]`, etc.).

En Windows el binario se construye como GUI (`WIN32_EXECUTABLE`): no abre
consola; la salida tambien va al log.

## 3. Controles del editor

| Tecla | Accion |
|-------|--------|
| `E` | Alterna interfaces del editor ON/OFF. En navegacion libre (OFF) el cursor se fija al centro y se oculta |
| `Clic derecho` sostenido sobre la escena | Navegar/mirar desde el editor con interfaces visibles (cursor capturado mientras se sostiene; soltar lo restaura) |
| `W` `A` `S` `D` | Adelante / izquierda / atras / derecha (diagonales permitidas y normalizadas) |
| `Espacio` / `Shift izq.` | Arriba / abajo |
| `G` | Alterna el gizmo entre **Local** (ejes del objeto) y **Global** (ejes del mundo, no rota con el objeto) |
| `1` o `T` | Gizmo: mover (Translate) |
| `2` o `R` | Gizmo: rotar (Rotate) |
| `3` o `U` | Gizmo: escalar (Scale) |
| `X` `Y` `Z` | Guia de eje del objeto seleccionado (ver seccion 4b) |
| `F5` / `F6` / `F7` | Simular / pausar-reanudar / detener la simulacion |
| `Ctrl+S` | Guardar el proyecto en caliente |
| `Ctrl+Z` / `Ctrl+Y` | Deshacer / rehacer |
| `Escape` | En play: detener la simulacion. En edicion: volver al menu principal |

El modo del gizmo (Local/Global) tambien se elige en el menu **"Gizmo"** de la
barra superior del editor, y se persiste por proyecto.

## 4. Configuracion (persistida en JSON)

- **Sensibilidad del mouse-look (camara)**: 0.15 por defecto. Se ajusta en
  **Opciones** del menu de inicio (configuracion general).
- **Sensibilidad de movimiento (WASD)**: 1.0 por defecto. Se ajusta solo en
  **Opciones** del menu de inicio (configuracion general).
- **Gizmo**: operacion (`gizmoOperacion`) y sistema de coordenadas
  (`gizmoGlobal`) se guardan por proyecto (seccion `editor` de
  ConfiguracionProyecto.json).
- **Apariencia** (`apariencia` de Configuracion.json, general): tema claro/oscuro
  (oscuro por defecto), modo blanco y negro, color de acento (RGB) y los colores
  del cielo, que son dos: `fondoSuperior` (cenit) y `fondoInferior` (suelo), con
  `radioDifuminado` como media anchura de la transicion alrededor del horizonte.
  El tema alcanza toda la paleta de ImGui (`TemaEditor`) y el fondo y el color
  efectivo de la grilla (`AparienciaUtil`).

## 4b. Grilla, ejes y guia de eje (viewport del editor)

- El objeto **"Grilla"** lo crea el motor al abrir la escena
  (`GameScene::asegurarGrilla`) si no existe, con el componente `Grid`. Solo se
  dibuja el primer objeto con ese componente, y su panel expone unicamente
  **Visible** y **Color**: la grilla es infinita y de densidad fija
  (secundarias cada `kSeparacionMenor` = 1 unidad, una principal cada
  `kMultiploMayor` = 5). Los campos `tam`/`separacion` del componente se siguen
  serializando para que las escenas viejas se lean igual, pero no se usan para
  dibujar.
- El radio de ese circulo lo elige el usuario (Opciones -> Radio de
  difuminado, 20 a 600, por defecto 150) y llega a la escena en
  `Apariencia::radioDifuminado`. `Difuminado` (modulo CPU puro) es quien lo
  convierte en el tramo opaco y en la curva: el inicio se deriva del radio en
  proporcion constante (`kProporcionInicio` = 40/150) y el radio se acota ahi
  mismo, asi que un valor corrupto en la configuracion no deja la grilla sin
  horizonte. Grilla y guia de eje reciben el MISMO `Difuminado` (con
  subdivisiones distintas) y por eso se desvanecen siempre en el mismo
  circulo-horizonte.
- `GrillaRenderer` recorta la grilla a ese circulo centrado en la camara: es a
  la vez el horizonte y el limite de dibujado. El difuminado es radial y por
  vertice (alpha por vertice, con `LineBuilder` interpolando entre extremos):
  opacidad plena hasta `dif.inicio` y caida cuadratica hasta 0 en `dif.fin`. Un
  batch de lineas por ancho (secundarias 1 px, principales 2 px, ejes 3 px) =
  3 draws por frame.
- Los tres ejes (X rojo, Y verde, Z azul) salen de `GuiaEje::colorEje`: una sola
  convencion para la grilla, la guia de eje y el gizmo. Grilla y guia ajustan
  ese color por contraste contra lo que tienen debajo
  (`AparienciaUtil::ejeContraste`): el color efectivo de la grilla para la
  grilla, y ese mismo color (o el fondo, si no hay grilla visible) para la guia,
  de modo que en modo blanco y negro el contraste se mide contra el blanco o el
  negro real. El ajuste escala el brillo, asi que con un eje saturado puede no
  alcanzar el minimo de 0.35 de diferencia de luminancia.

- **Cielo degradado**: se dibuja como primera pasada (antes que la grilla y los
  objetos) mediante un fullscreen triangle y un shader que interpola entre
  `fondoSuperior` (cenit) y `fondoInferior` (suelo) segun la **direccion de
  vista**, no segun la posicion del pixel: el fragment shader des-proyecta el NDC
  al plano lejano, resta la camara y normaliza, de modo que el cielo va con la
  camara (al mirar hacia arriba, el color superior ocupa mas pantalla aunque el
  pixel este en la parte baja). Los colores se resuelven con
  `Cielo::coloresEfectivos`, que aplica
  la logica B/N (ambos blanco/negro segun tema) y modo normal (respeta los
  colores elegidos). El pase usa depth test ON + depth mask OFF para que el
  cielo quede "detras" de toda la geometria sin escribir profundidad. El
  triangulo se dibuja por `IRenderBackend::drawFullscreenTriangle` (el backend
  posee el VAO vacio que el core profile exige para cualquier dibujo).
- **Skybox (cubemap de seis caras)**: si hay un componente `Skybox` visible con
  las 6 caras asignadas, su cubemap reemplaza al degradado. Las caras se
  decodifican y se suben una sola vez por conjunto de rutas y fechas de
  modificacion (`CacheCubemap::claveDeCaras` decide la invalidacion) y el
  intento fallido no se repite hasta que cambie un archivo; el aviso es una vez
  por clave, no por frame.
- La guia de eje (`X`/`Y`/`Z` sobre el objeto seleccionado) reutiliza las
  mismas constantes de difuminado (`SceneRenderer::dibujarGuiaEje` las lee de
  `GrillaRenderer`), con mas subdivisiones (24) porque la recta es mucho mas
  larga que una linea de la grilla.

## 5. Movimiento por maquina de estado (Input/EditorInput)

- Las callbacks GLFW (teclado/mouse) viven en `src/Input/EditorInput.{h,cpp}`,
  fuera de `main.cpp`.
- `onKey` SOLO registra el estado de las teclas (PRESS/RELEASE). Cada frame el
  bucle llama a `EditorInput::aplicarMovimiento(dt)`, que combina las
  direcciones activas en un vector `[derecha, arriba, adelante]` y lo
  normaliza: las diagonales (`W+A`, `W+D`, ...) se mueven a la misma velocidad
  que un solo eje.
- El movimiento solo aplica en estado **Editing** y cuando ImGui no captura el
  teclado (escribir en un campo de texto no desplaza la camara).
- El pitch de la camara esta limitado a +-89 grados (`kPitchMaxGrados` en
  `CameraComponent.cpp`) para que no se "dé vuelta".

## 6. GameObject "Scene" (raiz de la escena)

Cada escena tiene ahora un objeto raiz llamado **"Scene"** (tipo
`ObjetoEscena`, derivado de `SimpleObject` sin geometria). Este objeto
es el padre estructural de **todas las entidades** de la escena:
objetos 3D, luces, camaras, etc. Se crea automaticamente al abrir o
crear una escena (`SceneRegistry::createDefaultRoot`) y se serializa
como cualquier GameObject (id 0, sin malla). La jerarquia visible en el
explorador/inspector refleja esta estructura: al crear objetos sin
padre, cuelgan del "Scene". Es compatible con escenas anteriores:
al cargar un binario raiz antiguo (guardado con malla), los bytes
sobrantes quedan sin leer (EOF) y no rompen el formato.

## 7. Sistema de audio (AudioEngine + AudioSource)

- **AudioEngine** (`src/Audio/`): motor de audio concurrente con hilo
  dedicado (`bucleAudio`). Recibe comandos por cola protegida con
  mutex (`reproducir`, `detener`, `detenerTodo`). Usa **MiniAudio**
  como backend; si no hay dispositivo de audio al iniciar, el motor
  queda en modo mudo (todas las llamadas devuelven -1) sin romper nada.
- **AudioSource** (Component): adjunto a un GameObject. Guarda el
  nombre del clip (string, coincide con un archivo en `Sonidos/` del
  proyecto), volumen (0-1), bucle y reproduccion automatica. Al entrar
  en modo **Play** (`GameScene::sincronizarAudioPlay`), la escena
  inyecta el `AudioEngine` en cada `AudioSource` y dispara los de
  reproduccion automatica. Al salir de play, detiene todos.
- **Inspector**: panel `SettingsAudioSource` permite editar clip,
  volumen, bucle, auto-play y probar reproduccion/detencion en editor.
- **Persistencia**: se serializa length-prefixed (string + floats + bools)
  junto al GameObject; compatible con escenas viejas (campos nuevos
  leen 0/false al final).

## 8. Creador de Interfaces (CreadorDeInterfaces)

Ventana del editor (accesible desde menu **Ventanas > Creador UI**)
para crear assets de UI en `Memory/Interfaces/<nombre>.json` del
proyecto actual. Funciones:

- **Lista de interfaces**: muestra todas las del proyecto; click para
  cargar en el area de edicion.
- **Edicion de widgets**: 5 tipos — Etiqueta, Boton, Checkbox, Slider,
  EntradaTexto. Cada widget puede tener un **Sonido** (dropdown con
  clips de `Sonidos/`).
- **Guardar**: escribe el JSON en disco.
- **Activar en canvas**: la interfaz del borrador se vuelve la "activa"
  y se refleja en el `CanvasInterface` (ventana dockable para probar).
- **Uso en juego**: ver seccion 9 (InterfaceComponent).

Formato JSON: nombre, titulo, ancho/alto (px), array de widgets
(tipo, etiqueta, valores, sonido). El `CanvasInterface` pinta en vivo
el estado de checkbox/slider/texto.

## 9. InterfaceComponent (UI del jugador / HUD)

Componente `InterfaceComponent` que se adjunta a CUALQUIER GameObject.
Tiene un campo: **Nombre de interfaz** (asset en `Memory/Interfaces/`).
Al entrar en **modo Play** (`start == true`), `GameScene::GUI` busca el
primer objeto con `InterfaceComponent` cuyo nombre no este vacio,
llama a `CreadorDeInterfaces::activarInterfaz(nombre)` (carga el JSON
si no esta activo) y pasa esa `UserInterfaceCustom` al
`CanvasInterface`. Como el canvas en modo play es un **overlay a
pantalla completa** (`setModoPlay(true)`), la interfaz se dibuja
**directamente frente a la camara principal** (HUD del juego).

El estado editable (checkbox, slider, texto) se conserva entre frames
mientras la misma interfaz siga activa (la instancia `interfazActiva_`
del creador persiste).

Para agregar: click derecho en inspector > **Agregar Interfaz** > edita
el nombre del asset.

## 10. Exportar juego (distribucion standalone)

Desde el editor: menu **Archivo > Exportar juego** (barra superior).
Abre un dialogo modal con configuracion:

| Campo | Descripcion |
|---|---|
| **Nombre del ejecutable** | Nombre del binario final (sin extension). |
| **Nombre del proyecto exportado** | Nombre de la carpeta bajo `MotorGrafico/Exportaciones/`. |
| **Plataforma objetivo** | Linux (nativo) o Windows (cross-compile MinGW). |

Al pulsar **Exportar**, el motor ejecuta en hilo separado (no bloquea el
editor, spinner indeterminado en el dialogo):

1. Genera un proyecto CMake temporal con `BUILD_RUNTIME=ON`: la raiz del
   engine construye **solo su libreria** (sin ejecutable del editor ni
   pruebas) y `FUNSHI_JAVA=ON` incluye soporte de scripts Java via JNI (los
   scripts `.java` del proyecto viajan y se ejecutan en tiempo de ejecucion).
2. Compila la libreria y el ejecutable del juego. El binario usa un punto de
   entrada dedicado (`main_juego.cpp`) que **NO incluye código del editor**
   (GUIManager, MenuGUI, paneles): solo el runtime del juego (escena, física,
   scripts, render, input). Al arrancar lee `JuegoExportado.json` junto a si
   mismo y entra directo en modo juego. ImGui se inicializa para el runtime
   (contexto básico), sin la infraestructura GUI del editor.
3. Empaqueta en `MotorGrafico/Exportaciones/<nombre>/` con estructura
   **simplificada** (sin replicar la jerarquía del editor):
   - Ejecutable (`<nombre>.exe` en Windows, `<nombre>` en Linux).
   - `Assets/` directamente en la raíz (modelos, texturas, etc. del proyecto).
   - `scenes/` con la escena del proyecto (`Scene.db` y `SceneDir/`).
   - `scripts/` con los fuentes de usuario (C++ y Java).
   - `cache/` con binarios compilados de scripts (si existen).
   - `include/` con las cabeceras del motor que `BackendCpp` usa al compilar
     los scripts C++ en la primera ejecucion.
   - `lib/` con las `.so` de sistema en Linux (Bullet, miniaudio, GLFW); en
     Windows las DLL van junto al `.exe`, que es donde las busca el loader.
   - `JuegoExportado.json` con metadatos del proyecto.
   
   **NO se incluyen** archivos del editor: `Memory/Interfaces/`,
   `ConfiguracionProyecto.json`, `imgui.ini` ni la estructura
   `MotorGrafico/Proyects/`.

Mientras la exportacion corre, el dialogo **no se puede cerrar** (ni con la X
ni con Cerrar); al terminar queda abierto mostrando el resultado hasta que el
usuario lo cierre.

**Scripts C++ del juego exportado:** los fuentes viajan con el proyecto y
`BackendCpp` los compila en la **primera ejecucion** en la maquina que corre
el juego, con la clase que declara la escena. Hace falta un compilador C++ en
esa maquina (`g++`/`cl` en el PATH o la variable `FUNSHI_CXX`); las cabeceras
del motor estan en `include/`. Las ejecuciones siguientes reutilizan el
artefacto ya compilado.

**Requisitos para cross-compile Windows:** toolchain MinGW
(`x86_64-w64-mingw32-g++`, `x86_64-w64-mingw32-gcc`, `windres`) y las
dependencias del motor (GLFW, Bullet, Assimp, ncurses) construidas para MinGW
y alcanzables por `find_package`. El exportador escribe el toolchain
(`toolchain-mingw.cmake`) y lo pasa al configure con
`-DCMAKE_TOOLCHAIN_FILE`.

**Lanzar el juego exportado:**
```bash
# Linux
./MotorGrafico/Exportaciones/MiJuego/MiJuego

# Windows
MotorGrafico\Exportaciones\MiJuego\MiJuego.exe
```

El binario exportado arranca directo en modo juego (doble clic, sin
argumentos). En un binario del motor tambien valen los flags de siempre:
`--proyecto <nombre>` abre el proyecto en modo editor y `--juego` fuerza el
modo juego sin manifiesto.