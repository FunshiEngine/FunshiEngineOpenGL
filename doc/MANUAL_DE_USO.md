<!--
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    SPDX-License-Identifier: Apache-2.0
-->

# Manual de uso de FunshiEngineGL

Guia practica para usuarios del editor: como montar una escena, asignar assets
por arrastre y escribir scripts con la API exacta que expone el motor.

> Documentacion complementaria: [README.md](../README.md) (caracteristicas y
> compilacion), [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) (arquitectura),
> [CAMARAS_VISTAS_PREVIAS.md](CAMARAS_VISTAS_PREVIAS.md) (camaras) y
> [FunshiEngineGL/DOCUMENTACION.md](FunshiEngineGL/DOCUMENTACION.md) (notas
> internas de arquitectura).

## Contenido

1. [Requisitos e inicio rapido](#1-requisitos-e-inicio-rapido)
2. [Proyectos y estructura de carpetas](#2-proyectos-y-estructura-de-carpetas)
3. [Recorrido del editor](#3-recorrido-del-editor)
4. [Objetos y componentes](#4-objetos-y-componentes)
5. [Undo / redo de operaciones del editor](#5-undo--redo-de-operaciones-del-editor)
6. [Assets por drag & drop](#6-assets-por-drag--drop)
7. [Física](#7-física)
8. [Audio](#8-audio)
9. [Interfaces de juego (HUD)](#9-interfaces-de-juego-hud)
10. [Cámaras](#10-cámaras)
11. [Guardar y abrir escenas](#11-guardar-y-abrir-escenas)
12. [Apariencia y configuración del editor](#12-apariencia-y-configuración-del-editor)
13. [Scripts: conceptos y ciclo de vida](#13-scripts-conceptos-y-ciclo-de-vida)
14. [Scripting C++: referencia completa](#14-scripting-c-referencia-completa)
15. [Scripting Java (JNI)](#15-scripting-java-jni)
16. [Hot reload y depuración](#16-hot-reload-y-depuración)

17. [Terminal integrada](#17-terminal-integrada)
18. [Mods del editor (.fmod)](#18-mods-del-editor-fmod)
19. [API de inspeccion de escena](#19-api-de-inspeccion-de-escena)
20. [Particulas y efectos](#20-particulas-y-efectos)

---

## 1. Requisitos e inicio rapido

**Dependencias:** CMake >= 3.16, compilador con C++17, OpenGL 3.3 core (sin GLU),
GLFW, GLM, Assimp y Bullet. Java es opcional (solo scripting Java): si el build
encuentra un JDK, se compila el soporte `FUNSHI_JAVA=ON` automaticamente.

El **perfil core es obligatorio**: el motor pide un contexto OpenGL 3.3 core
(GLFW `GLFW_OPENGL_CORE_PROFILE` + `GLFW_OPENGL_FORWARD_COMPAT`) porque todo su
render es con shaders y no usa estado fijo. Si la GPU no lo soporta, el arranque
se corta con un mensaje en consola y la ventana no se abre; en ese caso hay que
actualizar el driver. El requisito real es GLSL 330, que es lo que imponen los
shaders del engine.

```bash
cmake -S FunshiEngineGL -B FunshiEngineGL/build
cmake --build FunshiEngineGL/build -j$(nproc)
./FunshiEngineGL/build/FunshiEngineGL
```

**Primeros pasos:**

1. En el menu de inicio elegi idioma y sensibilidad de camara. Abri **Config Proyect**: a la izquierda se lista la
   carpeta de proyectos (cada carpeta de `FunshiEngine-DATA/` es un proyecto);
   elegi uno con click o crea uno escribiendo su nombre en el campo de la
   derecha y pulsando **Confirmar**. Con boton derecho sobre un proyecto se
   abre **Editar nombre**: escribi el nuevo nombre y el modal lo confirma,
   renombrando la carpeta en el acto. El mismo menu contextual ofrece
   **Eliminar proyecto**: el modal pide confirmacion y borra de disco la
   escena, los assets y la configuracion de ese proyecto (irreversible). Si
   eliminabas el proyecto abierto, el motor vuelve al estado "sin proyecto".
   "Iniciar Estudio" crea el proyecto y sus carpetas automaticamente.
2. Navega la escena con `W`/`A`/`S`/`D`, `Espacio`/`Shift` y el mouse (nav FPS).
   `E` oculta los paneles; `Escape` vuelve al menu desde edicion y termina la
   simulacion desde Depuración o Juego.

---

## 2. Proyectos y estructura de carpetas

Al crear un proyecto, el motor genera la estructura bajo
`{app}/FunshiEngine-DATA/Proyects/<proyecto>/`:

> **Dónde quedan los datos.** `{app}` es la carpeta del ejecutable cuando el
> motor está en una carpeta donde puede escribir: el build de desarrollo y
> también la instalación de Windows, que es por usuario
> (`%LOCALAPPDATA%\Programs\FunshiEngineGL`, sin pedir permisos de
> administrador). Si el motor está en una carpeta donde no puede escribir —una
> copia en `Program Files`, una carpeta montada en solo lectura, el motor
> lanzado como administrador—, usa en su lugar
> `%APPDATA%\FunshiEngineGL\FunshiEngine-DATA` (Windows) o
> `$XDG_DATA_HOME/FunshiEngineGL/FunshiEngine-DATA`, con respaldo en
> `~/.local/share/FunshiEngineGL/FunshiEngine-DATA` (Linux y macOS). La ruta
> efectiva se imprime por consola al arrancar y la migración de la carpeta
> anterior, si había algo, se avisa en la barra de estado. Los proyectos que
> migran siguen funcionando: las rutas de assets se guardan relativas al
> proyecto, no absolutas.
>
> El log de arranque (`logs/FunshiEngineGL_<AAAAMMDD_HHMMSS>.log`, ver el árbol)
> vive también en esa carpeta de datos, de modo que se escribe aunque el motor
> esté en una carpeta de la que no se pueda escribir. Si esa carpeta tampoco
> admite escritura, el motor cae a la carpeta temporal del sistema
> (`%TEMP%\FunshiEngineGL\logs` o `/tmp/FunshiEngineGL/logs`), y solo si las tres
> opciones fallan se queda sin archivo de log y escribe en la consola.

```
FunshiEngine-DATA/
├── logs/
│   └── FunshiEngineGL_<AAAAMMDD_HHMMSS>.log   ← log de arranque (UTC), uno por ejecucion
├── Proyects/
│   └── <proyecto>/
│       ├── Memory/
│       │   ├── Binarios/Scene        ← escenas binarias
│       │   ├── ConfiguracionProyecto.json
│       │   └── imgui.ini
│       ├── Mods/                     ← mods del proyecto (.fmod, ver seccion 18)
│       └── src<proyecto>/            ← assets del proyecto (raiz del explorador)
│           ├── modelos/              ← .obj/.fbx que arrastra el editor
│           ├── Sonidos/              ← clips de audio (.wav/.mp3/...)
│           └── Scripts/              ← scripts del usuario (.cpp/.java)
# ├── Mods/                             ← mods globales del motor (.fmod, ver seccion 18)
├── Configuraciones/
│   └── Configuracion.json            ← configuracion global (ultimo proyecto, idioma,
│                                       sensibilidades y apariencia: tema, B/N, acento,
│                                       fondo y radio de difuminado)
└── Exportaciones/
    └── <nombreExportacion>/          ← juegos exportados (ver seccion 10.1)
        ├── <Juego>.exe / <Juego>     ← ejecutable standalone
        ├── Assets/                   ← recursos del proyecto (modelos, texturas, etc.)
        ├── scenes/                   ← escenas del juego
        │   ├── Scene.db
        │   └── SceneDir/
        ├── scripts/                  ← fuentes de usuario (C++, Java)
        ├── cache/                    ← binarios compilados de scripts
        ├── include/                  ← cabeceras del motor (para scripts en runtime)
        ├── lib/                      ← dependencias runtime (.so Linux)
        └── JuegoExportado.json       ← metadatos del proyecto
```

En el arbol, las carpetas marcadas con `#` las crea el motor (no hace falta
hacerlas a mano). El motor crea su carpeta global de `Mods/` y, al crear un
proyecto, `Memory/` y `src<proyecto>/`: las carpetas de assets (`Sonidos/`,
`Scripts/`, los `Mods/` del proyecto) las crea el usuario, desde el explorador
de archivos, donde las necesite. Ningun asset depende de una carpeta en
concreto: el motor guarda la **ruta** del archivo, asi que el archivo puede
estar en cualquier carpeta dentro de `src<proyecto>/`. El arbol de
archivos del editor lista la **raiz del proyecto** (`src<nombre>`).

> **Nota de packaging:** en el instalador Windows el explorador apunta a la
> raiz del proyecto; en builds de desarrollo antiguas (Linux) podia apuntar a
> la de scripts. Si un dropdown o drag & drop aparece vacio, verifica que raiz
> lista tu arbol.

**Operaciones sobre los archivos.** En el **grid** de la carpeta seleccionada
(clic derecho sobre un elemento) el menu contextual ofrece "Renombrar",
"Eliminar Archivo" o "Eliminar Carpeta" (segun corresponda); en el **arbol**, el
mismo menú aparece sobre la carpeta (además de "Nueva Carpeta"), y el renombre
pide el nombre en un **diálogo** con el campo ya listo para escribir: `Enter`
confirma y `Escape` (o **Cancelar**) descarta. En los dos paneles el diálogo es
el mismo. Eliminar pide confirmacion en un dialogo y
**no se puede deshacer**: al borrar una carpeta desaparece tambien todo su
contenido, y el arbol y el grid se refrescan en el acto. El **undo/redo**
(`Ctrl+Z` / `Ctrl+Y`) no cubre el borrado de archivos.

**Rutas de la escena.** Las mallas, texturas, fuentes de script y archivos de
audio que usa la escena se persisten **relativas** a la carpeta
`src<proyecto>/`. Por eso, al renombrar un proyecto (menu de inicio) o mover su
carpeta completa, las escenas siguen cargando sin tocar nada: la raiz
`src<nombre>` se desplaza entera con el proyecto. Dentro del explorador, al
**renombrar** o al **mover** un archivo o carpeta el motor reescribe al instante
las referencias de la escena que apuntaban a esa ruta y guarda la escena
modificada. Limitaciones:
una **copia** (arrastre con `Ctrl`) no se rastrea, porque no cambia ninguna de
las dos rutas; los cambios hechos **fuera** del motor (explorador de Windows,
una terminal, un `mv`) solo sirven para refrescar el arbol de archivos, no
reescriben nada, asi que ahi si hay que volver a arrastrar el asset en su
inspector.
Si una referencia quedo apuntando a un archivo que ya no existe, al **abrir la
escena** el motor busca ese archivo por nombre dentro de la carpeta del
proyecto: con **una sola coincidencia** repara la referencia, lo anota en el
log (`[escena] ruta reparada: ...`) y guarda la escena; con **varias** o con
**ninguna** no adivina y deja la ruta como estaba, y ahi si hay que volver a
arrastrar el asset en su inspector (el inspector de script ademas avisa
`El fuente del script no existe` con la ruta que busco).

**Manifiesto de assets (`SceneAssets.json`).** Junto a los binarios de la
escena se mantiene `Memory/Binarios/SceneAssets.json`, un add-on legible que
centraliza las rutas de asset de cada objeto (malla, las 4 texturas del
material y la dll de script). No reemplaza al `.db`: se regenera en **cada**
guardado y, al cargar, sus rutas tienen **precedencia** sobre las del `.db`
(las escenas viejas, sin manifiesto, se cargan igual contra el `.db`). Al
renombrar/mover assets el guardado automatico lo mantiene al dia.

**Guardado sin salir (Ctrl+S).** El editor guarda el proyecto completo
(escena + manifiesto + configuracion de ventanas/gizmo/camara) con
`Ctrl+S`, ademas del guardado automatico al salir. El atajo funciona **siempre**,
tambien mientras escribes en un campo de texto: guardar no le quita ninguna tecla
al campo y asi se evita perder el cambio recien escrito (renombrar un objeto y
guardar sin hacer clic en otro lado). `Ctrl+Z` y `Ctrl+Y` si se ceden al campo,
que ahi tienen su propio deshacer/rehacer. Si no hay proyecto abierto, la barra
de estado avisa en vez de ignorar el atajo.

---

## 3. Recorrido del editor

| Tecla / accion | Funcion |
|---|---|
| `W` `A` `S` `D` | Mover la camara libre del editor (diagonales normalizadas). Solo en Edicion o Depuracion, con las interfaces ocultas (`E`) o el clic derecho sostenido sobre el viewport |
| `Espacio` / `Shift izq.` | Subir / bajar la camara (misma condicion que `WASD`) |
| Mouse / clic der. | El mouse solo mueve la camara con cursor oculto (`C`) o mientras se sostiene clic derecho; el clic derecho navega **sin** esconder las interfaces y con cursor visible no activa la orbita (sensibilidad en Opciones) |
| `E` | Mostrar/ocultar solo los paneles GUI; al ocultarlos el cursor queda visible y no rota la camara; no cambia gizmos, guias ni marcadores visuales |
| `C` | Con la GUI oculta, alterna ocultar/mostrar el cursor. Con cursor oculto el mouse puede mirar libremente y el clic derecho activa orbita |
| `F5` | Iniciar Depuracion: simula fisica, scripts y audio, y permite editar la escena |
| `F6` | Pausar/reanudar la simulacion activa sin reiniciarla (Depuracion o Juego) |
| `F7` | Terminar la simulacion y restaurar la escena al estado previo al inicio |
| `Ctrl+S` | Guardar el proyecto en caliente (escena + manifiesto + config) |
| `Ctrl+Z` | Deshacer ultima accion del editor (undo) |
| `Ctrl+Y` | Rehacer accion deshecha (redo) |
| `Ctrl+C` / `Ctrl+X` / `Ctrl+V` | Copiar / cortar / pegar el objeto seleccionado de la jerarquia (ver seccion 4) |
| `Ctrl+D` | Borrar el objeto seleccionado sin dialogo de confirmacion (se recupera con `Ctrl+Z`) |
| `Escape` | Durante Depuracion o Juego: terminar y restaurar la escena al estado previo al inicio. En edicion: volver al menu; en el menu: no hace nada |
| `1` / `T` | Gizmo: traslacion (apaga la guia de eje) |
| `2` / `R` | Gizmo: rotacion (apaga la guia de eje) |
| `3` / `U` | Gizmo: escala (la `Y` suelta la tomo la guia de eje; apaga la guia) |
| `X` / `Y` / `Z` | Guia de eje del objeto seleccionado (ver abajo) |
| `G` | Gizmo local / mundo (gizmo y guia de eje) |
| Clic en objeto | Seleccionar en viewport |

Fuera de una simulacion, la barra de la escena ofrece **Depuración** y
**Juego**. Durante cualquiera de los dos modos, esos botones se reemplazan por
**Pausa/Reanudar**, **Reset** y **Terminar**. Depuración conserva el modo
editable existente: la física y los scripts avanzan, y los cambios hechos
durante la simulacion se descartan al terminar y la escena vuelve al estado
previo al inicio, igual que en Juego. Los controles se muestran como iconos y
su nombre aparece al pasar el cursor. Juego ejecuta la escena sin
permitir operaciones de edicion y oculta la grilla, gizmos, guias y marcadores
auxiliares; los objetos y componentes de la escena siguen dibujandose. **Reset** restaura
el estado completo guardado al iniciar la simulacion, sin salir de ella.

Clic en un objeto del arbol o del viewport lo selecciona; el Inspector muestra
sus componentes a la derecha. En el arbol, la seleccion se confirma al soltar
el boton: si el puntero se desplaza al menos 6 px mientras esta presionado,
se interpreta como arrastre y se conserva el objeto que ya muestra el Inspector.
Clic en el vacio de la ventana deselecciona (para pegar a la raiz o soltar la
seleccion).

---

## 4. Objetos y componentes

- **Crear objetos:** "New GameObject" (crea un objeto simple en la escena que posee únicamente el componente `Transform`).
- **Menú contextual en la jerarquía:** clic derecho sobre un objeto despliega cuatro
  opciones: **Cambiar ID**, **Renombrar**, **Desanudar a raíz** y **Eliminar**. "Desanudar
  a raíz" solo aparece cuando el objeto cuelga de un padre intermedio: un hijo directo de
  la raíz ya está al nivel superior, así que no hay nada que desenanidar. Clic derecho en
  espacio vacío del panel despliega "New GameObject".
- **Componentes:** `Transform`, `Color`, `Model`, `Material`, `Materiales`, `Light`,
  `CameraComponent`, `Grid`, `Skybox`, `SpriteRenderer`, `Animador2D`, `Tilemap`,
  colliders (esfera / cubo / malla / tilemap), `RigidBody`, `AudioSource`,
  `InterfaceComponent`, `UIComponent`, `EmisorParticulas`, `Sombras` y `Script`.
- **Materiales por region (`Materiales`):** cada parte de la malla (la cara de
  un cubo, la parte de un personaje) puede llevar su propio material y sus
  texturas. El panel lista una entrada por region; "Sincronizar con el modelo"
  agrega las regiones que declara el archivo del modelo, con el nombre del
  material que trae. Si una region no tiene material propio, se dibuja con el
  material único del objeto.
- **Vista previa del material:** el panel de `Material` (y el de cada region de
  `Materiales`) dibuja una **esfera de muestra** con el material que se esta
  editando, con luz propia. Se redibuja sola cuando cambia el material (color,
  brillo o textura), asi que se ve el efecto del cambio sin salir del panel.
  La esfera usa las texturas del slot difuso; el resto de slots se aplican
  igual que en la escena.
- **Inspector:** boton "Agregar componente" abre el popup de componentes; cada
  uno tiene su panel propio (Transform, Luz con tipo/atenuacion/color, etc.).
  Cada componente se lista con su icono a la izquierda del nombre; el icono
  viene del tipo de componente (no se guarda en la escena) y el juego de
  imagenes completo esta en `Imagenes/GUI-Image/Component-Image/`. Se dibuja
  a 18 px de lado, para que se lea el detalle de los icones pequenos.
- **Jerarquia:** arrastra un objeto sobre otro en el arbol para reparentar; el
  motor rechaza ciclos y la raiz. "Childs Freeze" congela la transformacion de
  los hijos durante la edicion del padre.
- **Copiar/pegar en la jerarquia:** `Ctrl+C` copia y `Ctrl+X` corta el objeto
  seleccionado (la raiz no se copia). `Ctrl+V` pega una copia profunda
  (componentes incluidos, con id nuevo): si hay un objeto seleccionado entra
  como su hijo, si no hay seleccion entra a la raiz. El portapapeles se conserva
  para pegar varias veces; cortar lo vacia al pegar (es mover). Pegar sobre el
  propio objeto o sobre uno de sus descendientes entra a la raiz (evita colgar
  el duplicado de su propio subarbol).

### Prefabs: reutilizar jerarquías completas

Un **prefab** guarda un objeto (raiz) con todos sus hijos, componentes y
propiedades (nombre, tag, Transform) en un archivo `.prefab` bajo la carpeta
`Prefabs/` de los assets del proyecto (`src<Proyecto>/Prefabs/`, visible en el
explorador de contenido). Al instanciarlo se crea una copia profunda en la
escena con IDs nuevos; el original no se toca.

**Crear un prefab**

1. Selecciona el objeto raiz en la jerarquía.
2. Abre la ventana **Animaciones y Particulas** (menú **Ventanas**) y entra en
   la pestaña **Prefabs**.
3. Pulsa **Guardar la seleccion como prefab**, escribe el nombre y confirma.
   El archivo aparece en `Prefabs/<nombre>.prefab` dentro de los assets del
   proyecto y se lista en el explorador de contenido.

**Instanciar un prefab**

De cualquiera de las dos formas:

- En la pestaña **Prefabs**, elige el prefab en la lista y pulsa
  **Instanciar**.
- Arrastra el `.prefab` desde el explorador de contenido hasta la zona de
  arrastre de la misma pestaña.

La instancia entra en la raíz de la escena y queda seleccionada para moverla
con el gizmo. Conserva **nombre, tag, Transform, jerarquía completa y todos
los componentes** del prefab original.

**Biblioteca de prefabs (`PrefabLibrary`)**

- Escanea la carpeta `Prefabs/` de los assets del proyecto al abrir la
  pestaña, después de cada operación (guardar, renombrar, borrar) y con el
  botón **Refrescar**.
- Mantiene un **caché en memoria** (`std::unordered_map<string, Prefab>`) para
  que instanciar sea instantáneo tras la primera vez.
- **Validación automática** antes de cachear: rechaza archivos con magic number
  incorrecto, versión incompatible, truncados o sin objetos (numObjetos = 0).
  Los archivos inválidos se loguean en consola y no entran en el caché.

**Formato del archivo (.prefab)**

- Binario, mismo esquema que la serialización de escena (preorden con marcadores
  `=>` / `<=`), pero **solo el subárbol del prefab**, no la escena completa.
- Cabecera: magic `0x42415250` ("PREB") + versión `1`.
- Para cada GameObject: state, id, tam, inputName, tag + lista de componentes
  (length + typeName + datos del componente) + hijos recursivos.
- Los componentes usan `ComponentFactory::create()` + `loadComponent()` para
  deserializar (Transform, Color, Model, etc.). Componentes desconocidos abortan
  la carga.

**Diferencias con copiar/pegar (Ctrl+C / Ctrl+V)**

| Aspecto | Copiar/pegar | Prefab |
|---|---|---|
| **Alcance** | Solo el objeto seleccionado (no hijos) | Raiz + **todos los descendientes** |
| **Persistencia** | Solo en sesión actual (portapapeles) | **Archivo en disco** (`src<Proyecto>/Prefabs/`) |
| **Reutilización** | Una vez por pegado | **Ilimitada** (múltiples instancias) |
| **IDs** | Nuevos en cada pegado | **Nuevos en cada instanciación** (SceneRegistry) |
| **Workflow** | Rápido, ad-hoc | **Predefinido**, versionable, compartible |

**Buenas prácticas**

- Usa nombres descriptivos: `Enemigo_Base`, `Arbol_Pino`, `Item_Medkit`.
- **Renombrar** y **Borrar** están junto a la lista del prefab elegido; borrar
  pide confirmación porque elimina el archivo del disco.
- Si mueves o renombras la carpeta `Prefabs/` desde el explorador, los
  archivos `.prefab` se mueven con ella; el botón **Refrescar** actualiza la
  lista.
- Los IDs se regeneran al instanciar: no colisionan con el original ni entre
  instancias.

---

### Grilla del suelo

La escena trae un objeto llamado **"Grilla"** con el componente `Grid`: es el
piso del editor y se dibuja siempre que el componente este visible, excepto
durante Juego, cuando se oculta junto con las demas ayudas visuales del editor.
Su panel en el Inspector tiene solo dos controles:

- **Visible**: enciende o apaga la grilla.
- **Color**: el color de las lineas. El que trae de fabrica es un blanco hielo
  casi blanco (`0.88, 0.91, 0.89`).

No hay tamano ni separacion: la grilla es **infinita** y de densidad fija
(secundarias cada unidad, una linea principal cada cinco). El circulo en el que
se dibuja persigue a la camara y las lineas se **difuminan con la distancia**:
opacas hasta 40 unidades de la camara y desvanciendose por completo en el
horizonte, a 150 unidades, que es tambien el limite: mas alla no se dibuja
nada. El render de la escena y las vistas previas de camara usan ese mismo
horizonte como limite radial, con el mismo difuminado hasta el borde, aunque
la camara tenga un `Far Plane` mayor; el valor configurable de la camara no se
modifica. Los tres ejes de la
grilla (X rojo, Y verde, Z azul, los mismos colores
que el gizmo) se dibujan mas gruesos y se ajustan de brillo para que siempre
se vean contra el color de la grilla.

El componente `Grid` se puede agregar a cualquier objeto desde "Agregar
componente", pero solo se dibuja el **primer** objeto de la escena que lo tenga,
y siempre con la misma densidad y el mismo horizonte.

### Guia de eje (`X` / `Y` / `Z`)

Con un objeto seleccionado, `X`, `Y` o `Z` dibujan la recta sobre la que se
puede mover ese objeto: la tecla pulsada es el eje que varia y **las otras dos
coordenadas quedan fijadas** a las del objeto.

Por ejemplo, un objeto en `(3, 2, -5)` y se pulsa `X`: la recta es `(t, 2, -5)`
para todo `t`. Es decir, el objeto solo se desplaza en X y su altura (Y) y su
profundidad (Z) no se mueven. Con `Y` el efecto es el inverso: quedan fijos X y Z.

- Cada eje tiene su color (X rojo, Y verde, Z azul, los mismos que los ejes de
  la grilla y que el gizmo) y la recta llega hasta el
  **horizonte**, difuminandose con el mismo criterio que la grilla: opaca cerca de
  la camara y desvanciendose en el mismo punto donde el piso se acaba. Asi la
  guia se lee como un eje que atraviesa la escena entera, no como un palo corto
  pegado al objeto. Si el color de la grilla se parece al del eje, el motor
  ajusta el brillo de la guia hasta que se los distingue.
- Es un interruptor: apretar dos veces la misma tecla la apaga. Al activarla o
  apagarla aparece un aviso en la barra de estado que dice que guia quedo
  prendida y sobre que eje se puede mover el objeto. Tambien se apaga sola al
  seleccionar otro objeto, al ocultar las interfaces con `E` y al volver al menu
  con `Escape`.
- Con la guia activa el gizmo **no se oculta**: se queda solo la flecha del eje
  elegido, que es el punto de agarre. Se arrastra esa flecha y el objeto se
  mueve por ese unico eje, sin salir de la recta.
- Elegir otra operacion del gizmo (`1`/`T`, `2`/`R`, `3`/`U`) apaga la guia: si
  el usuario pide rotar, escalar o mover en los tres ejes, el "solo este eje" ya
  no aplica.
- `G` alterna entre **mundo** (la recta sigue el eje del mundo) y **local** (la
  recta sigue el eje del objeto, rotado con el), igual que el gizmo. La escala
  del objeto no alarga la recta.
- Solo aparece en el viewport principal, no en las vistas previas de camara, y
  no necesita `Ctrl` (`Ctrl+Y`/`Ctrl+Z` siguen siendo redo/undo).

---

### Skybox (cubemap de seis caras)

El componente `Skybox` reemplaza el cielo degradado por una imagen de seis
caras. Se agrega desde "Agregar componente"; no hay limite de cuantos Skybox
puede haber en la escena, pero el que se dibuja es el **primer objeto visible**
que lo tenga, igual que con `Grid`.

- **Visible**: enciende o apaga el cubemap. Apagado, o con una casilla sin
  asignar, vuelve a dibujarse el cielo degradado de la seccion 12.
- Seis campos de texto con la ruta de cada cara: `Cara +X (Right)`,
  `Cara -X (Left)`, `Cara +Y (Top)`, `Cara -Y (Bottom)`, `Cara +Z (Front)` y
  `Cara -Z (Back)`. Las rutas son **relativas al proyecto** y se guardan
  relativas a la raíz de assets, igual que la malla, las texturas y el script:
  la escena sigue siendo válida si renombrás o movés el proyecto entero, y las
  referencias se actualizan solas si movés o renombrás una cara desde el
  explorador. Si escribís una ruta con `/` o `\` indistinto, también funciona.
  Los formatos admitidos son los que carga el motor de imagenes (PNG, JPG, TGA,
  BMP, PSD, HDR).
- **Elegir el archivo con el explorador**: cada campo tiene un botón `...` que
  abre un selector dentro del editor, con las carpetas primero y un filtro de
  texto. Arranca en la carpeta de la cara que estás editando (si no tiene ruta,
  en la raíz de assets del proyecto), se navega con doble clic o con el botón
  `Arriba`, y se elige con doble clic o con el botón `Elegir`. No hace falta
  escribir ni recordar la ruta.
- **Arrastrar y soltar**: también podés arrastrar el archivo desde el
  explorador y soltarlo directamente sobre el campo de su cara, igual que la
  malla y el script.
- El panel avisa de las dos condiciones que hacen que el motor descarte el
  cubemap sin explicar nada en la pantalla: cuántas caras quedan sin asignar, y
  si alguna no mide lo mismo que la `+X`. Cuando es así te dice la resolución de
  cada una. La medida se hace en **píxeles**, igual que la comprobación del
  render, y leyendo solo la cabecera del archivo: no decodifica la imagen. No se
  compara el peso del archivo, porque dos imágenes idénticas con distinta
  compresión pesan distinto sin que midan distinto.
- Las seis caras deben ser del **mismo tamano**: si una falta, no es legible o
  no coincide con las demas, el motor avisa una vez por conjunto de caras y
  dibuja el degradado en su lugar.
- La imagen se decodifica y se sube a la tarjeta de video **una sola vez** por
  conjunto de caras; se vuelve a subir solo si cambia alguna de las rutas o si
  se reescribe algun archivo, asi que editar la escena con un Skybox cargado no
  tiene costo extra por frame.
- El cubemap **acompania a la camara**: se dibuja como si estuviera a distancia
  infinita, de modo que **desplazarse** no lo acerca ni lo aleja, pero **girar**
  si lo recorre, asi que las nubes y el sol se ven desde el angulo correcto
  segun donde mires. Es lo que hace que un cielo con imagen se sienta como un
  cielo y no como un fondo de pantalla. Aparece tanto en el viewport principal
  como en las vistas previas.

### Sombras (sombra proyectada por una luz)

El componente `Sombras` se agrega **al mismo objeto que lleva la luz** y le dice
que projecute sombra sobre la escena. El motor dibuja la escena desde el punto
de vista de la luz, guarda esa profundidad en un mapa y, al dibujar las mallas,
consulta el mapa para oscurecer lo que queda tapado.

- Solo se ofrece en objetos **3D que ya tienen una luz**; el panel aparece en
  el Inspector de ese objeto.
- **Proyecta sombra**: enciende o apaga la sombra de esa luz. Apagada, la luz
  sigue iluminando igual, solo que sin sombra.
- **Resolucion**: lado del mapa de sombras en píxeles (de 256 a 4096). Un mapa
  mas grande da bordes mas nitidos a cambio de mas memoria y mas tiempo de
  dibujado.
- **Sesgo**: desplazamiento de la profundidad al comparar. Sin el, la propia
  superficie que proyecta la sombra se auto-sombrea (el "acne" de las sombras).
  Si la sombra sale moteada, subilo un poco; si las sombras se despegan de los
  objetos, bajalo.
- **Fuerza**: cuanto se oscurece la zona en sombra. 0 deja la sombra sin efecto
  y 1 la deja completa.
- **Radio**: semi-lado del frustum ortografico de la luz. La sombra abarca una
  caja de lado `2 * radio` alrededor del foco: un radio grande cubre mas
  escena, con menos resolucion efectiva.
- **Distancia**: alcance de la sombra, en unidades desde la luz.

Dos limites del motor:

- Solo proyectan sombra las luces **direccionales** y **spot**. Las puntuales
  proyectan en todas las direcciones, asi que necesitarian un mapa por cara; el
  motor las cuenta aparte y avisa por consola de que no proyectan.
- Como mucho **4 mapas de sombra** a la vez. Si hay mas luces con sombra, las
  que se quedan fuera se avisan por consola (las primeras 4 son las que
  proyectan).

La sombra es de la luz que la lleva: si hay dos luces en la escena, cada una
sombrea solo lo que ilumina ella.

## 5. Undo / redo de operaciones del editor

Todas las operaciones del editor estan encapsuladas en comandos (patron
Command) que admiten undo y redo. `EditorController` posee un
`GestorComandos` que mantiene dos pilas (undo/redo) con un maximo de 50
entradas; cada nueva accion invalida la pila de redo.

| Accion | Comando que la envuelve |
|---|---|
| Crear objeto | `CrearObjetoComando` |
| Borrar objeto | `BorrarObjetoComando` |
| Duplicar objeto (pegar) | `DuplicarObjetoComando` |
| Reparentar (arrastrar en jerarquia) | `ReparentarComando` |
| Modificar transform (posicion/rotacion/escala) | `TransformComando` |
| Agregar componente | `AgregarComponenteComando` |
| Quitar componente | `QuitarComponenteComando` |
| Ajustar una variable de un componente | `AjustarVariableComando` |
| Cambiar la etiqueta (tag) de un objeto | `EstablecerTagComando` |
| Limpiar escena (borrar todos los objetos no-raiz) | `LimpiarEscenaComando` |

**Atributos de los comandos:** cada comando captura el estado antes de mutar
(posicion del padre, id del objeto, transform anterior, componentes) y lo
almacena por valor para poder restaurarlo exactamente. La raiz de la escena
(id=0) esta excluida de delete/clear por diseno.
Los comandos mutan por la puerta del editor (`EditorController`): desregistran
y re-registran los cuerpos en fisica y limpian la seleccion, asi que el undo
nunca deja cuerpos fantasma en el mundo. Al entrar o salir de una simulacion
el historial se limpia (los ids guardados caducan con la restauracion).

**Mover un objeto con el gizmo tambien se deshace:** al iniciar un arrastre se
toma una foto del transform y, al soltarlo, se registra **un solo**
`TransformComando` con el estado inicial y el final (no uno por frame), asi que
`Ctrl+Z` devuelve el objeto a donde estaba antes del movimiento. Un clic sobre el
gizmo sin arrastrar no genera historial.

**Aviso en la barra de estado:** `Ctrl+Z` y `Ctrl+Y` dejan un mensaje
momentaneo (4 s) con la descripcion del comando aplicado, por ejemplo
`Deshacer: Transformar: Cubo`, o `Nada que deshacer` / `Nada que rehacer` si la
pila estaba vacia: el atajo nunca falla en silencio.

**Limitacion conocida:** el arrastre del *offset local de un collider* (el gizmo
que edita el componente, no el transform del objeto) todavia no genera comando.

---

## 6. Assets por drag & drop

### 6.1 Mover y copiar dentro del explorador

El panel **Vista de contenido** lleva el nombre de la carpeta que esta
mostrando en su barra superior (pasa el raton por encima para ver la ruta
completa). Hay dos vistas del mismo arbol -- el grid de esa ventana y el arbol
de la izquierda -- y arrastrar un archivo o una carpeta sirve para las dos:

| Destino del drop | Resultado |
|---|---|
| Una celda de **carpeta** del grid | Mueve el elemento dentro de esa carpeta |
| Una fila de **carpeta** del arbol | Mueve el elemento dentro de esa carpeta |
| El espacio vacio del grid | Mueve el elemento a la carpeta que se esta viendo |

Soltar sobre una fila del arbol **siempre mueve** (cortar y pegar): hacia el
explorador no hay opcion de copia, con o sin Ctrl. Dentro del grid —sobre una
celda de carpeta o sobre el espacio vacio— manteniendo `Ctrl` (o `Cmd` en
macOS) el arrastre **copia** en vez de mover; el tooltip indica cual de las dos
va a ocurrir antes de soltar.

Al mover se actualizan al instante las referencias de la escena que apuntaban
a la ruta anterior (mallas, texturas, fuentes de script) y se guarda la escena;
copiar no cambia ninguna ruta, asi que no hay nada que reescribir. El
`SceneAssets.json` se regenera con el guardado.

Dos casos se rechazan a proposito, sin tocar disco:

- **Destino ocupado:** si en la carpeta destino ya existe un elemento con ese
  nombre, el movimiento se cancela y el original queda intacto. No se pisa nada.
- **Carpeta dentro de si misma:** soltar `Assets` sobre `Assets/Modelos` (o
  sobre si misma) se cancela, tanto al mover como al copiar. Si se dejara, la
  copia se encontraria a si misma mientras avanza y dejaria el arbol a medias en
  disco.

### 6.2 Arrastrar un asset a un componente

El explorador de archivos (arbol + grid) emite el payload ImGui
`ARCHIVO_PATH` (path completo del asset) al arrastrar. Receptores del editor:

| Destino del drop | Componente | Que guarda |
|---|---|---|
| Panel **Model** del Inspector | `Model` | path completo (buffer 4096) |
| Panel **Script** del Inspector | `Script` | path completo del fuente (`std::string`) |
| Panel **Fuente de audio** del Inspector | `AudioSource` | ruta del archivo de audio |
| Panel **Musica** del Inspector | `Musica` | ruta de la pista (se anade a la lista) |
| Campo **Interfaz** de InterfaceComponent | `InterfaceComponent` | ruta del `.html` (no la asigna si no es `.html`) |
| Panel **Tilemap** del Inspector | `Tilemap` | ruta del tileset JSON (no lo asigna si no valida) |
| Panel **Prefab** del Inspector | `Prefab` | instancia el prefab completo (jerarquía + componentes) |

Todos los receptores almacenan el valor en `std::string` o en buffers de al
menos 4096 bytes, de modo que paths largos no se truncan (el componente
`Model` era el unico con buffer de 100 y fue ampliado a 4096).

**Como usar:**

1. Copia tus assets a las carpetas del proyecto (o crealas con "New Folder" /
   "New File"); el FileSystemWatcher rescanea al detectar cambios externos.
2. Selecciona el objeto y arrastra el asset sobre el control correspondiente
   del panel del componente ("Arrastra modelo", campo Fuente del script,
   boton de la fuente de audio, zona de pistas de Musica, dropdown de
   Interfaz).
3. La escena guarda el path; al recargar se resuelve de nuevo.

**Instanciar prefab por drag & drop:**
1. En el explorador, localiza un archivo `.prefab` en la carpeta `Prefabs/`
   de los assets del proyecto.
2. Arrástralo hasta la pestaña **Prefabs** de la ventana **Animaciones y
   Particulas** (menú **Ventanas**): la zona "Arrastra aqui un .prefab para
   instanciarlo" lo recibe.
3. La instancia se crea en la raíz de la escena y se selecciona
   automáticamente.

 **Crear y pintar un Tilemap:**

 1. **Prepara el tileset**: un archivo `.json` estilo Tiled con campos
    `image` (ruta relativa al PNG), `tilewidth`/`tileheight`, `imagewidth`/
    `imageheight`, `margin`/`spacing` opcionales, y `tiles[]` con propiedad
    `solid: true` en los tiles que deben colisionar. Guárdalo en
    `Assets/Tilesets/`.
 2. En la jerarquía: clic derecho → **New GameObject** → con el objeto
    seleccionado, **Add Component → Tilemap**.
 3. En el **panel Tilemap del Inspector**: arrastra el `.json` del tileset al
    drop zone (valida antes de asignar).
 4. Click **Agregar capa** → pon nombre (ej. "Suelo"), dimensiones (ej. 20×15),
    marca **Colisión** si esa capa debe chocar → **OK**.
 5. En la **paleta** verás los IDs de tile (fondo rojo = sólido). Click en un
    ID → se vuelve el pincel activo.
 6. **Click izquierdo** en la grilla → pinta el tile; **click derecho** → borra.
    Puedes cambiar de capa y pincel a voluntad.
 7. Para colisión física: **Add Component → TilemapCollider** (requiere Tilemap
    en el mismo objeto) → en su panel click **Recargar colisión** (reconstruye
    las cajas desde los tiles sólidos actuales).

### 6.3 Traer archivos del sistema operativo

El explorador del editor tambien acepta archivos y carpetas que vienen de
**fuera** del proyecto: del explorador de archivos del sistema o del
portapapeles del sistema.

| Como | Donde cae |
|---|---|
| **Arrastrar** archivos desde el explorador del sistema sobre la ventana del editor | Se copian en la carpeta que se esta viendo en **Vista de contenido** |
| **Ctrl+V** con la ventana **Vista de contenido** enfocada | Se copian en la carpeta que se esta viendo |

Detalles que conviene saber:

- **El destino es la carpeta visible**, la que aparece en la barra superior de
  la ventana. Si no hay ninguna carpeta visible, el arrastre se ignora.
- **No se pisa nada.** Si en la carpeta destino ya existe un archivo con ese
  nombre, el nuevo recibe un nombre libre: `notas.txt`, `notas (1).txt`,
  `notas (2).txt`... El archivo que ya estaba conserva su contenido. Es la
  misma politica que aplica el explorador del sistema.
- **Las carpetas se copian enteras**, con todo su contenido.
- **El resultado se avisa en la barra de estado** (ventana *Estado*), tanto si
  la importacion funciono como si algun elemento no se pudo copiar. Un
  elemento que falla no impide que se copien los demas.
- Importar no cambia ninguna ruta de la escena (no hay ningun asset que se
  mueva de sitio), asi que la escena no se reescribe ni se guarda sola.
- Copiar una **carpeta** hace que el arbol de carpetas de la izquierda se
  actualice; copiar solo archivos no lo necesita, porque los archivos no
  aparecen en el arbol.

**Limitaciones por plataforma:**

- En **Windows** el portapapeles de archivos es una lista de rutas del shell:
  `Ctrl+V` funciona con lo que se haya copiado o cortado en el explorador de
  archivos.
- En **Linux** el portapapeles de archivos es texto (`text/uri-list`). Depende
  de que el explorador del sistema lo deje disponible: si no hay nada, el
  motor avisa "El portapapeles del sistema no tiene archivos" en vez de fallar.
  Arrastrar archivos sobre la ventana si funciona en cualquier escritorio.
- En una maquina **sin escritorio** (un servidor) no hay portapapeles: el motor
  lo dice y sigue funcionando con normalidad.

---

## 7. Física

- Colliders de esfera, cubo, malla o `TilemapCollider` (cajas por tramo de
  tiles solidos); con `RigidBody` participan de la simulacion Bullet. Solo simula en **Depuración** o **Juego**: en edicion el
  gizmo mueve el objeto y el motor sincroniza collider/cuerpo/objeto con la
  matriz global compuesta del dueño, para que mover un collider no
  desincronice el cuerpo.
- Cada collider tiene **Visible en escena**, que mantiene su wireframe visible
  como ayuda de depuracion aunque su objeto no este seleccionado. La opcion
  se guarda con la escena y no depende del gizmo del offset.
- **Activo** en `RigidBody` habilita o pausa la participacion de ese cuerpo en
  Bullet. El estado se guarda con la escena; las escenas anteriores cargan los
  cuerpos como activos.
- Panel `RigidBody`: **Masa** (`0` = estatico), **Usa gravedad** + **Escala
  gravedad** (multiplica la gravedad global: `1` = igual, `0` = flota),
  **Friccion** de contacto (`0` = desliza) y **Freeze posicion/rotacion** por
  ejes X/Y/Z. Todo se guarda con la escena; las escenas anteriores toman los
  valores por defecto (cae normal, friccion `0.5`, sin freeze).
- La **gravedad global** del mundo es manipulable (tambien desde scripts) y se
  re-aplica a cada cuerpo segun su escala al cambiar.
- Mientras se manipula el gizmo, `stepSimulation` se pausa (la gravedad podria
  "eyectar" el objeto); al soltar, la simulacion sigue.
- Gizmo dedicado de fisica para el collider activo.

---

## 8. Audio

- Los archivos de audio se referencian **por ruta**: no hay catálogo ni carpeta
  reservada. El archivo puede estar en cualquier carpeta dentro de
  `src<proyecto>/` (wav/ogg/mp3/flac).
- Agrega `AudioSource` a un objeto y **arrastra el archivo de audio** desde el
  explorador de archivos al panel del componente, igual que se hace con los
  scripts. El panel muestra la ruta asignada; si el archivo arrastrado no es
  un formato de audio, el motor lo dice en el log y no lo asigna.
- Ajusta volumen, loop y "reproduccion automatica". En Depuración y Juego, el
  `AudioEngine` reproduce el archivo en su hilo de audio.
- La ruta se guarda en la escena **relativa** a `src<proyecto>/`: al mover o
  renombrar el proyecto entero, la escena sigue funcionando. En el juego
  exportado se resuelve contra la carpeta del juego (allí se copia todo
  `src<proyecto>/`).
- Si el archivo no existe, el motor avisa en el log y no reproduce: el
  componente queda sin sonido.

### 8.1 Musica: lista de pistas

- Agrega `Musica` a un objeto y **arrastra los archivos de musica** al panel:
  cada uno se anade a la **lista ordenada** de pistas del componente. El
  archivo puede estar en cualquier carpeta dentro de `src<proyecto>/`, igual
  que los efectos de sonido.
- El orden de la lista es el orden en que suenan: `^` y `v` mueven una pista y
  `x` la quita.
- Ajusta **Volumen**, **Fundido (s)** (cuanto dura el cruce entre dos pistas),
  **Repetir la lista** (al terminar la ultima vuelve a la primera) y
  **Reproducir al activar**.
- En Depuracion y Juego el motor reproduce la primera pista y, cuando se
  acaba, pasa a la siguiente con un fundido cruzado. El boton **Reproducir**
  del inspector arranca la lista en cualquier momento; **Detener** la para con
  el mismo fundido.
- La lista se guarda en la escena **relativa** a `src<proyecto>/`, como la ruta
  del `AudioSource`: al mover o renombrar el proyecto entero sigue funcionando,
  y en el juego exportado se resuelve contra la carpeta del juego.

---

## 9. Interfaces de juego (HUD)

Esta seccion describe la interfaz **web** (una pagina `.html` dentro del
juego). La interfaz **nativa**, que dibuja el motor sin navegador de por medio,
esta en la seccion 21.

- Agrega `InterfaceComponent` a un GameObject y arrastra un archivo `.html` al
  campo del inspector. El `.html` referencia su CSS y su JS por rutas relativas,
  como cualquier otro asset del proyecto.
- El inspector permite configurar: **Activa** (si la interfaz entra en el HUD),
  **Ancla** (nueve posiciones respecto al area de juego), **Desplazamiento**
  desde el ancla, **Tamaño automático** (el que fija el CSS del `.html`) o un
  **Tamaño** fijo, y **Z** (orden de apilado entre varias interfaces).
- La escena reúne las interfaces activas y las ordena por z para el overlay del
  juego, delante de la camara principal.
- La ventana dockable **Vista de interfaz** muestra la vista web de la interfaz
  del objeto seleccionado (preview del editor): ratón y rueda funcionan sobre
  la preview, el foco sigue al ratón y el teclado llega a los campos de la
  página. Sin backend web real la ventana avisa que no hay textura.
- **Recarga en caliente**: al guardar el `.html` en un editor externo, la vista
  se recarga sola en el siguiente frame (por `mtime`), sin tocar el componente.
- El motor web es CEF (Chromium): se compila con `-DFUNSHI_CEF=ON` (requiere
  `-DFUNSHI_WEBUI=ON`, que viene activado por defecto). Sin CEF el motor sigue
  corriendo con las vistas sin textura (backend nulo): el ciclo y los scripts
  funcionan igual, solo no se dibuja nada.
- Un script puede manipular una interfaz: declara un campo `InterfaceComponent*`
  con `REFLECT_COMPONENTE(campo, "InterfaceComponent")` y **arrastra la interfaz**
  desde su inspector al campo (igual que con un objeto); en el inspector de la
  interfaz hay un botón que hace de origen del arrastre. Desde el script, con el
  handle del campo o `servicios->interfazDeObjeto("nombre")`, se cambia la
  visibilidad, se fija el estado que recibe la página (`interfazEstablecerEstado`
  / `interfazEstablecerValor`) y se lee la salida que la página devuelve
  (`interfazValor`). Ver la tabla de servicios de la sección 13.4.

---

## 10. Cámaras

- `CameraComponent` con **vistas previas en vivo** (render a FBO); detalle
  completo en [CAMARAS_VISTAS_PREVIAS.md](CAMARAS_VISTAS_PREVIAS.md).
- "Usar" en el panel de la camara la marca como activa; el id se persiste en la
  configuracion del proyecto (`ConfiguracionProyecto.json`), con default
  automatica si el id ya no existe al cargar.

---

## 11. Guardar y abrir escenas

El guardado se hace desde la barra de menu de la escena. Las escenas son
binarias (`Memory/Binarios/Scene`), con serializacion en preorden y marcadores
`=>`/`<=` que recupera la jerarquia completa (padres e hijos) de forma
recursiva. Al iniciar, el editor recupera la escena del proyecto.

Cada escena tiene un objeto raiz automatico llamado **"Scene"** (tipo
`ObjetoEscena`, sin geometria) que agrupa como hijos a todas las entidades
(objetos, luces, camaras). Al crear objetos sin padre explicitamente, cuelgan
de esta raiz. La raiz no aparece en la lista plana del inspector pero
estructura la jerarquia y se serializa (id 0).

> Las escenas no tienen versionado aun (pendiente en el roadmap); al cambiar
> el formato binario de un componente, las escenas viejas pueden leer campos
> truncados con seguridad (los lectores acotan con `std::min`), pero el valor
> largo se pierde.

### 10.1 Exportar juego (distribucion standalone)

Menu **Archivo > Exportar juego** (barra superior del editor). Abre un dialogo
modal para configurar la exportacion. El dialogo muestra de que proyecto se
exporta (el proyecto activo en ese momento):

| Campo | Descripcion |
|---|---|
| **Nombre del ejecutable** | Nombre del binario final (sin extension). |
| **Nombre del proyecto exportado** | Nombre de la carpeta bajo `FunshiEngine-DATA/Exportaciones/`. |
| **Plataforma objetivo** | La que admita la maquina: en Windows, **Windows (nativo)** (sale un `.exe` con sus DLL al lado); en Linux, **Linux (nativo)** o **Windows (cross-compile MinGW)** (si esta instalado el toolchain cruzado). |

Al pulsar **Exportar**, el motor:

1. Genera un proyecto CMake temporal con `BUILD_RUNTIME=ON`: solo la libreria
   del motor (sin ejecutable del editor ni pruebas) y exportacion con soporte
   de scripts Java via JNI (`FUNSHI_JAVA=ON`; los scripts `.java` del proyecto
   viajan y se ejecutan en tiempo de ejecucion).
2. Compila la libreria y el ejecutable del juego. El binario usa un punto de
   entrada dedicado (`main_juego.cpp`) que NO incluye código del editor
   (GUIManager, MenuGUI, paneles): solo el runtime del juego (escena, física,
   scripts, render, input). Al arrancar lee `JuegoExportado.json` junto a si
   mismo y entra directo en modo juego.
3. Empaqueta en `FunshiEngine-DATA/Exportaciones/<nombre>/` con estructura
   **simplificada** (sin replicar la jerarquía del editor):
   - Ejecutable (`<nombre>.exe` en Windows, `<nombre>` en Linux).
   - `Assets/` directamente en la raíz (modelos, texturas, etc.).
   - `scenes/` con la escena del proyecto (`Scene.db` y `SceneDir/`).
   - `scripts/` con los fuentes de usuario (C++ y Java).
   - `cache/` con binarios compilados de scripts (si existen).
   - `include/` con las cabeceras del motor para compilar scripts en runtime.
   - `lib/` con las `.so` de sistema en Linux; en Windows las DLL quedan
     junto al `.exe`.
   - `JuegoExportado.json` (metadatos del proyecto).

> El juego exportado **lleva consigo las DLL de las que depende**: el
> exportador las lee de la tabla de importaciones del binario (y de las que
> importan esas a su vez) y las copia desde la carpeta del toolchain (el
> runtime de C++ y las bibliotecas con las que enlaza el motor). Asi el juego
> arranca en otra maquina sin instalar nada.
   
   **NO se incluyen** archivos del editor:
   `ConfiguracionProyecto.json`, `imgui.ini` ni la estructura
   `FunshiEngine-DATA/Proyects/`.

El dialogo muestra un **spinner indeterminado** (barra de progreso falsa) mientras
se ejecuta la compilacion; el proceso no bloquea el editor. Mientras la
exportacion corre el dialogo **no se puede cerrar** (ni con la X ni con Cerrar);
al terminar queda abierto mostrando el resultado hasta que el usuario lo cierre.

**Que se ve en cada momento:**

- **Configuracion**: el formulario y, si el intento anterior no llego a
  exportarse, el motivo en rojo (por ejemplo, que no hay proyecto o que su
  carpeta no existe).
- **Exportando**: el spinner, la barra de progreso, la etapa en curso
  (compilando, empaquetando...) y la ultima linea de la bitacora.
- **Resultado**: el mensaje final de la exportacion (la carpeta donde quedo
  empaquetada, o el motivo del fallo), con los botones **Cerrar** y **Exportar
  otra vez** (este ultimo vuelve al formulario con la misma configuracion).

> Si el dialogo muestra «Proyecto: (ninguno)» no hay ningun proyecto activo
> desde el que exportar: elige o abre un proyecto antes de exportar.

**Traza de la exportacion:** cada exportacion deja su propio archivo de log
en la carpeta `logs/` del motor (`FunshiEngine-DATA/logs/Exportacion_<AAAAMMDD_HHMMSS>.log`,
timestamp en UTC). Ahi queda **todo lo que se imprimio en la terminal durante
la compilacion** (el configure de CMake, el progreso de la compilacion, los
avisos del compilador y del enlazador) junto con la bitacora de la
exportacion. El mensaje final del dialogo dice la ruta exacta del archivo, y
las etapas se agregan al final unas de otras (el configure no borra lo que
escribio la compilacion). Es el sitio donde mirar cuando la exportacion falla
o cuando el compilador avisa de algo.

> La compilacion ocurre en un directorio propio, en la carpeta temporal del
> sistema: dos exportaciones simultaneas no se pisan (cada una compila en el
> suyo) y ahi no quedan restos de las exportaciones fallidas.

> Los scripts C++ del proyecto viajan como fuentes y se compilan en la
> **primera ejecucion** del juego en la maquina que lo corre: hace falta un
> compilador C++ ahi (`g++`/`cl` en el PATH o la variable `FUNSHI_CXX`).
> Las ejecuciones siguientes reutilizan el artefacto ya compilado.

**Requisitos para cross-compile Windows:** toolchain MinGW
(`x86_64-w64-mingw32-g++`, `x86_64-w64-mingw32-gcc`, `windres`) y las
dependencias del motor construidas para MinGW (GLFW, Bullet, Assimp).

**Lanzar el juego exportado:**
```bash
# Linux
./FunshiEngine-DATA/Exportaciones/MiJuego/MiJuego

# Windows
FunshiEngine-DATA\Exportaciones\MiJuego\MiJuego.exe
```

> El binario exportado arranca directo en modo juego (doble clic, sin
> argumentos). Es un ejecutable standalone, **no es el editor**: no acepta
> `--proyecto` ni tiene la estructura del editor.

---

## 12. Apariencia y configuración del editor

La vista **Opciones** del menú de inicio tiene tres bloques:

- **Juego:** idioma (Espanol / English), *Radio de difuminado* (20 a 600
  unidades), *Sensibilidad de camara* (mouse-look, 0.02 a 5.0) y
  *Sensibilidad de movimiento* (velocidad de `WASD`, 0.1 a 5.0).
- **Apariencia:**
  - *Tema claro de la interfaz* — arranca en **oscuro**.
  - *Modo blanco y negro (interfaz y viewport)* — desatura la interfaz completa y
    fuerza el fondo del viewport y el color de la grilla a **blanco con el tema
    claro o negro con el oscuro**: en ese modo el color de fondo y el de la
    grilla que se hayan elegido se ignoran, y con el fondo blanco la grilla se
    pone negra (y al reves), para que siempre se vea.
  - *Color de acento de la interfaz* — solo RGB: la transparencia de cada
    elemento la define el tema, no el color elegido.
  - *Color de la parte superior del cielo* — color del degradado en la zona
    alta del viewport (por defecto gris oscuro).
  - *Color de la parte inferior del cielo* — color del degradado en la zona
    baja del viewport (por defecto gris oscuro).
  - *Restablecer apariencia* — vuelve el perfil completo a los valores de
    fabrica (tema oscuro, sin modo blanco y negro, acento azul, cielo gris
    oscuro).
- **Configuracion:** *Restablecer configuracion* — vuelve **toda** la
  configuracion a los defaults, conservando el nombre del proyecto.

*Radio de difuminado* es el radio del circulo-horizonte del piso, en unidades de
mundo: la grilla se dibuja hasta ahi y se difumina hacia su borde, y la guia de
eje se desvanece en ese mismo circulo. Con un radio corto el piso llega menos
lejos y se dibujan menos lineas (mas fluido); con uno largo llega mas lejos y
cuesta mas. La grilla se rearma solo cuando cambia algo que la altera (camara,
radio o color): con la camara quieta editar la escena no cuesta geometria
nueva, aunque conviene igual no pasarse del radio necesario. El tramo
completamente opaco es siempre la misma fraccion del radio
(40 de cada 150), asi que al agrandarlo el degradado se agranda con el, en vez
de estirarse. El valor por defecto es 150 y *Restablecer apariencia* lo vuelve
ahi.

Los cambios se aplican **en vivo**, sin reiniciar: el acento alcanza **todos** los
roles de la interfaz (botones, solapas del dock, campos de entrada, sliders,
checkboxes, enlaces, bordes, separadores y tablas) y los grises azulados de
fabrica quedan en gris neutro, así que al cambiar de color no quedan restos del
azul clasico.

- El **cielo** se renderiza como un degradado entre *Color de la parte
  superior del cielo* y *Color de la parte inferior del cielo*. Los dos colores
  se guardan tal como los elige el usuario —un cielo claro es una eleccion
  valida— y el degradado los mezcla. En modo blanco y negro ambas partes se
  fuerzan a blanco (tema claro) o negro (tema oscuro). El degradado se dibuja
  como primera pasada del viewport, con la prueba de profundidad activa pero sin
  escribir en ella, asi que queda por detras de la grilla y de los objetos. Si un
  objeto tiene el componente **Skybox** visible con sus seis caras asignadas, su
  cubemap reemplaza al degradado (ver "Skybox (cubemap de seis caras)" en la
  seccion 4).

- El color del cielo depende de **hacia donde mira la camara**, no de la
  posicion del pixel en la pantalla: el shader des-proyecta cada pixel al plano
  lejano y usa la componente vertical de ese rayo de vista. Por eso el cielo va
  con la camara en vez de quedar clavado a la pantalla como un fondo de
  escritorio. En la practica:

  - Mirando al **cenit** se ve el color superior; mirando al **suelo**, el
    inferior.
  - En el **horizonte** queda la banda de mezcla, y los colores puros aparecen a
    unos 20 grados por encima y por debajo de el.
  - **Girar** la camara de un lado a otro recorre el degradado igual que mirar
    arriba o abajo; **desplazarse** (translation) no lo cambia, porque la
    direccion de vista es la misma.

- Persistencia: la **apariencia**, el idioma y las dos sensibilidades se guardan
  en la configuracion general, en la raiz de datos del motor
  (`FunshiEngine-DATA/Configuraciones/Configuracion.json` —junto al binario si el
  motor puede escribir ahi, o en la carpeta de datos del usuario si no (ver
  "Donde quedan los datos" en la seccion 2)—). El **gizmo** (operacion y sistema
  de coordenadas), la **ventana de camaras**, la **visibilidad de las ventanas**
  y la **camara activa** se guardan por proyecto, en
  `Proyects/<proyecto>/Memory/ConfiguracionProyecto.json`. La escritura es
  **atomica** (archivo temporal + rename: un corte no deja el JSON cortado) y
  la configuracion general se guarda de forma **diferida**: mientras cambias
  opciones en vivo se escribe como maximo una vez cada 250 ms, y siempre al salir
  o con Ctrl+S. Tolera archivos ausentes o corruptos.

---

## 13. Scripts: conceptos y ciclo de vida

Los comportamientos del juego se escriben como **scripts dinamicos**: archivos
`.cpp` o `.java` dentro del proyecto que el editor compila en caliente y
ejecuta durante Depuración y Juego.

- Se crean desde el explorador: clic derecho sobre la carpeta actual > "New
  Script" y, en el dialogo que se abre, elegir **C++ (`.cpp`)** o **Java
  (`.java`)** (este ultimo disponible si el motor se compilo con soporte JNI).
- **El nombre del archivo debe ser `<ClassName>.cpp`** (la clase == nombre del
  archivo). El backend compila la clase como `FUNSHI_<ClassName>` mediante
  `-DFUNSHI_NOMBRE_CLASE=<ClassName>`.
- Ciclo de vida en C++ (`IScriptBehaviour`): `onStart(owner)` al entrar en
  Depuración o Juego, `onUpdate(owner, deltaTime)` cada frame de simulación, y
  `onStop(owner)` opcional al terminar.
- **SerializeField:** los campos declarados con macros `REFLECT_*` aparecen
  editables en el inspector, se guardan con la escena y sobreviven al hot
  reload (se reinyectan por nombre de campo).
- Un GameObject puede tener varios componentes **Script**. Cada componente
  conserva su propia fuente, campos reflejados y estado; todos se muestran en
  el inspector y participan por separado del ciclo de vida de la simulacion.
- Desde el menú contextual del encabezado de cada componente Script se puede
  cambiar su nombre; queda guardado y organiza los paneles sin alterar el nombre
  de clase del archivo fuente. Los campos SerializeField aparecen dentro del
  mismo panel, debajo de la fuente, y sus nombres y tipos permanecen disponibles
  después de recargar la escena.
- Los ángulos de rotación de la API de scripts usan radianes aunque el
  componente `Transform` los almacene internamente en grados.
- Hot reload por mtime del fuente: durante la simulación, guardar el `.cpp`/`.java`
  recompila y recarga el comportamiento conservando los valores.
- La ventana **Estado** muestra el toolchain (compilador C++, javac, libjvm,
  cache) y el resultado de compilacion/carga de cada script de la escena.
- Los errores de carga/compilacion se informan en la ventana **Estado** y en el
  log del motor (`logs/FunshiEngineGL_*.log` en la carpeta de datos, seccion 2);
  el panel del componente no los repite: queda con el fuente asignado y sus
  SerializeField.

---

## 14. Scripting C++: referencia completa

### 13.1 Plantilla (identica a la que genera el editor)

```cpp
#include "Behaviour/IScriptBehaviour.h"

// El archivo debe llamarse <ClassName>.cpp. La macro FUNSHI_NOMBRE_CLASE
// se define en compilacion con el nombre real de tu clase.
class FUNSHI_NOMBRE_CLASE : public IScriptBehaviour {
public:
    // Campos SerializeField editables en el inspector:
    float velocidad = 5.0f;

    REFLECT_INICIO(FUNSHI_NOMBRE_CLASE)
        REFLECT_CAMPO(velocidad)
    REFLECT_FIN

    void onStart(GameObject* owner) override {
        (void)owner;
    }

    void onUpdate(GameObject* owner, float deltaTime) override {
        (void)owner; (void)deltaTime;
    }

    void onStop(GameObject* owner) override {
        (void)owner;
    }

    std::vector<::ReflejoScripts::DefCampo>
    camposReflejados() const override { return reflexion(); }
};

// Export requerida por el backend del motor; NO renombrar. Va FUERA de la
// clase, al final del archivo.
// En Windows/MSVC la fabrica tiene que viajar marcada con
// FUNSHI_COMPORTAMIENTO_EXPORT: sin ese atributo la .dll compila pero no
// exporta el simbolo y el motor no la encuentra (GetProcAddress). En
// MinGW/Linux/macOS el macro queda vacio (ahi se exporta todo solo).
extern "C" FUNSHI_COMPORTAMIENTO_EXPORT IScriptBehaviour* FUNSHI_CREAR_COMPORTAMIENTO(
    const MotorScript::ApiScriptGameObject* api) {
    (void)api;
    return new FUNSHI_NOMBRE_CLASE();
}
```

### 13.2 Campos SerializeField (macros REFLECT_*)

Declaralos entre `REFLECT_INICIO(<Clase>)` y `REFLECT_FIN` (pueden ir en zona
`public` o `private`; la macro abre `public`). Tipos soportados:

| Macro | Tipo del campo | Ejemplo |
|---|---|---|
| `REFLECT_CAMPO(nombre)` | `int`, `float`, `double`, `bool`, `std::string`, `vec3` | `int vidas = 3;` |
| `REFLECT_CAMPO(objetivo)` | `GameObject*` | referencia asignable arrastrando un objeto desde la jerarquia |
| `REFLECT_COMPONENTE(hud, "InterfaceComponent")` | `T*` de un componente | referencia a un componente de otro objeto; se asigna arrastrando el componente desde su inspector. El tipo se escribe como texto (`"InterfaceComponent"` para la interfaz web o `"UIComponent"` para la nativa) |
| `REFLECT_ARRAY(puntos)` | `std::vector<T>` de primitivas / `vec3` / `std::vector<GameObject*>` | `std::vector<float> ratios;` |
| `REFLECT_GRUPO(misil)` | struct anidado con su propio bloque `REFLECT_*` | `Misil misil;` |
| `REFLECT_GRUPOS(oleadas)` | `std::vector<S>` de structs reflejados | `std::vector<Oleada> oleadas;` |

Ejemplo con todos los casos:

```cpp
struct Oleada {
    int conteo = 1;
    float espaciado = 0.5f;

    REFLECT_INICIO(Oleada)
        REFLECT_CAMPO(conteo)
        REFLECT_CAMPO(espaciado)
    REFLECT_FIN
};

class FUNSHI_NOMBRE_CLASE : public IScriptBehaviour {
public:
    int vidas = 3;
    float velocidad = 5.0f;
    bool activo = true;
    std::string nombre = "jugador";
    vec3 direccion = { 0.0f, 1.0f, 0.0f };  // Float x3 en el inspector
    std::vector<float> puntos;
    std::vector<std::string> tags;
    Oleada primera;
    std::vector<Oleada> oleadas;
    GameObject* objetivo = nullptr;             // dropdown de objetos
    std::vector<GameObject*> enemigos;          // multi-seleccion

    REFLECT_INICIO(FUNSHI_NOMBRE_CLASE)
        REFLECT_CAMPO(vidas)
        REFLECT_CAMPO(velocidad)
        REFLECT_CAMPO(activo)
        REFLECT_CAMPO(nombre)
        REFLECT_CAMPO(direccion)
        REFLECT_ARRAY(puntos)
        REFLECT_ARRAY(tags)
        REFLECT_GRUPO(primera)
        REFLECT_GRUPOS(oleadas)
        REFLECT_CAMPO(objetivo)
        REFLECT_CAMPO(enemigos)
    REFLECT_FIN

    void onStart(GameObject* owner) override { (void)owner; }
    void onUpdate(GameObject* owner, float d) override { (void)owner; (void)d; }
    void onStop(GameObject* owner) override { (void)owner; }

    std::vector<::ReflejoScripts::DefCampo>
    camposReflejados() const override { return reflexion(); }
};
```

Notas:
- Las referencias `GameObject*` se asignan arrastrando un objeto desde la
  jerarquia sobre el campo del inspector. Se serializan por el **nombre** del
  objeto (no por puntero), sobreviven al hot reload y pueden limpiarse con `x`.
- Para asignar una camara, arrastra el objeto que contiene su componente
  `CameraComponent`; el campo recibe el `GameObject*` de ese objeto.
- El editor muestra cada campo con su widget (InputFloat, Checkbox, InputText,
  ColorEdit para `vec3` cuando aplica y destino de drag & drop para referencias).
- Si el inspector muestra "Sin campos SerializeField", verificá que el bloque
  `REFLECT_*` este dentro de la clase y que `camposReflejados()` devuelva
  `reflexion()`.

### 13.3 Acceso al GameObject: tabla `api`

El motor inyecta en cada instancia la tabla `MotorScript::ApiScriptGameObject`
(via `conectarApi`, que el backend llama al crear el comportamiento). Los
scripts la usan como `this->api->...` y **deben comprobar `if (api)`** antes
de usarla.

**Versionado APPEND-ONLY:** los miembros nuevos se agregan siempre al final de
la struct, sin reordenar ni cambiar tipos. El campo `version` (al final)
permite guardas:
`if (api->version >= 2) { float y = api->rotacionEjeY(owner); }`.
No se promete compatibilidad binaria entre versiones del motor: al cambiar la
API o el runtime, el cache invalida los artefactos y recompila los scripts
contra la version actual.

| Funcion | Version | Firma | Descripcion |
|---|---|---|---|
| `api->nombre(owner)` | 1 | `const char* (const void*)` | nombre del objeto |
| `api->posicionX/Y/Z(owner)` | 1 | `float (const void*)` | posicion local por eje |
| `api->fijarPosicion(owner,x,y,z)` | 1 | `void (void*, float, float, float)` | fija la posicion local |
| `api->fijarEscala(owner,x,y,z)` | 1 | `void (void*, float, float, float)` | fija la escala |
| `api->fijarRotacionEjes(owner,ang,x,y,z)` | 1 | `void (void*, float, float, float, float)` | rota `ang` rad sobre el eje `(x,y,z)` |
| `api->imprimirConsola("texto")` | 1 | `void (const char*)` | log a la consola del editor |
| `api->rotacionAngulo(owner)` | 2 | `float (const void*)` | angulo de rotacion (radianes) |
| `api->rotacionEjeX/Y/Z(owner)` | 2 | `float (const void*)` | eje de rotacion por componente |
| `api->escalaX/Y/Z(owner)` | 2 | `float (const void*)` | escala por eje |
| `api->fijarVelocidadHorizontal(owner,x,z)` | 3 | `bool (void*, float, float)` | fija la velocidad horizontal mundial del `RigidBody`; `false` si no hay cuerpo |
| `api->saltar(owner,velocidad)` | 3 | `bool (void*, float)` | salta con el `RigidBody` si esta apoyado; `false` si esta en el aire o no hay cuerpo |
| `api->etiqueta(owner)` | 4 | `const char* (const void*)` | devuelve el tag del objeto |
| `api->tieneEtiqueta(owner,tag)` | 4 | `bool (const void*, const char*)` | compara el tag del objeto |
| `api->objetoDeCollider(collider)` | 4 | `void* (const void*)` | devuelve el GameObject dueño del collider; `nullptr` si no tiene dueño |
| `api->masa(owner)` | 5 | `float (const void*)` | masa del `RigidBody`; `0` si no hay cuerpo |
| `api->fijarMasa(owner,masa)` | 5 | `bool (void*, float)` | fija la masa (`0` = estatico); `false` si no hay cuerpo |
| `api->usaGravedad(owner)` | 5 | `bool (const void*)` | si el cuerpo responde a la gravedad |
| `api->fijarUsoGravedad(owner,usar)` | 5 | `bool (void*, bool)` | activa o apaga la gravedad por cuerpo; `false` si no hay cuerpo |
| `api->escalaGravedad(owner)` | 5 | `float (const void*)` | factor sobre la gravedad global; `0` si no hay cuerpo |
| `api->fijarEscalaGravedad(owner,escala)` | 5 | `bool (void*, float)` | fija el factor (`1` = igual, `0` = flota); `false` si no hay cuerpo o no es finito |
| `api->friccion(owner)` | 5 | `float (const void*)` | friccion de contacto; `0` si no hay cuerpo |
| `api->fijarFriccion(owner,friccion)` | 5 | `bool (void*, float)` | fija la friccion (`0` = desliza); `false` si no hay cuerpo o es negativa |
| `api->posicionCongelada(owner,eje)` | 5 | `bool (const void*, int)` | freeze de posicion en el eje (`0` = X, `1` = Y, `2` = Z) |
| `api->fijarFreezePosicion(owner,x,y,z)` | 5 | `bool (void*, bool, bool, bool)` | congela la posicion por ejes; `false` si no hay cuerpo |
| `api->rotacionCongelada(owner,eje)` | 5 | `bool (const void*, int)` | freeze de rotacion en el eje |
| `api->fijarFreezeRotacion(owner,x,y,z)` | 5 | `bool (void*, bool, bool, bool)` | congela la rotacion por ejes; `false` si no hay cuerpo |

Los tags son cadenas dinámicas, no un `enum`: se pueden asignar desde el
Inspector y el motor registra los valores usados para ofrecerlos en los demás
objetos. Los objetos de escenas guardadas conservan su tag; las escenas
anteriores a este dato cargan como `Untagged`.

Los scripts reciben los cambios de contacto en `onCollisionEnter`,
`onCollisionStay` y `onCollisionExit`, con el `GameObject` propio, el collider
propio y el collider opuesto. El collider opuesto permite obtener su dueño con
`api->objetoDeCollider(otro)` y consultar su tag. Java tiene los callbacks
equivalentes `colisionInicio`, `colisionPersistencia` y `colisionFin`; sus
colliders se reciben como handles `long`.

```cpp
void onCollisionEnter(GameObject*, Collider*, Collider* otro) override {
    if (!api || api->version < 4) return;
    void* objetoSuelo = api->objetoDeCollider(otro);
    if (api->tieneEtiqueta(objetoSuelo, "suelo"))
        sobreSuelo = true;
}
```

El plano de gravedad interno de Bullet no es un GameObject y no tiene
`Collider` ni tag. Para detectar `"suelo"` desde scripts, el piso debe ser un
objeto de escena con collider y RigidBody estático (masa `0`), etiquetado
`"suelo"`. El salto también requiere que el RigidBody del jugador esté apoyado.

Ejemplo de uso combinado:

```cpp
void onUpdate(GameObject* owner, float deltaTime) override {
    if (!api) return;
    float x = api->posicionX(owner);
    api->fijarPosicion(owner, x + velocidad * deltaTime,
                       api->posicionY(owner), api->posicionZ(owner));
    if (api->version >= 2) {
        // Leer la rotacion actual y girar un poco mas cada frame:
        float ang = api->rotacionAngulo(owner);
        api->fijarRotacionEjes(owner, ang + 0.5f * deltaTime,
                               0.0f, 1.0f, 0.0f);
    }
    if (x > 10.0f) api->imprimirConsola("llego al limite");
}
```

### 13.4 Servicios de escena: tabla `servicios` (v9)

Ademas de `api`, `IScriptBehaviour` expone `this->servicios`: acceso a los
servicios del motor que **no son del objeto** sino de la escena (audio,
busqueda de objetos y teclado). GameScene la inyecta al iniciar la simulacion,
antes del primer `onStart`. Al terminar la simulacion, los servicios siguen
disponibles durante `onStop`; despues se detienen los sonidos iniciados desde
scripts y los `AudioSource` de componentes, y se desconecta la tabla.
Fuera de simulacion las funciones son no-ops tolerantes: devuelven
`false`/`nullptr`/`-1`, sin bloquear.
Misma convencion APPEND-ONLY con `servicios->version` al final.

| Funcion | Firma | Descripcion |
|---|---|---|
| `servicios->reproducirSonido(ruta, vol, loop)` | `int (const char*, float, bool)` | reproduce el archivo de audio de la **ruta** (relativa a `src<proyecto>/` o absoluta); devuelve handle >= 0, o -1 si el archivo no existe |
| `servicios->detenerSonido(handle)` | `void (int)` | detiene la reproduccion del handle |
| `servicios->objetoPorNombre("Enemigo")` | `void* (const char*)` | busca un GameObject por nombre en la escena; `nullptr` si no existe. El puntero vale mientras el objeto viva |
| `servicios->objetoPorId(7)` (v3) | `void* (int)` | busca un GameObject por id de escena; `nullptr` si no existe |
| `servicios->objetoPorEtiqueta("suelo")` (v3) | `void* (const char*)` | busca el primer GameObject con ese tag; `nullptr` si ninguno lo tiene. Util para referencias opcionales (ej. la camara de un controlador) |
| `servicios->teclaSostiene("W")` | `bool (const char*)` | tecla mantenida apretada |
| `servicios->teclaPresionada("SPACE")` | `bool (const char*)` | tecla apretada este frame (edge press) |
| `servicios->teclaSoltada("F")` | `bool (const char*)` | tecla soltada este frame (edge release) |
| `servicios->deltaMouseX()` | `float ()` | movimiento horizontal del mouse en pixeles en el frame actual |
| `servicios->deltaMouseY()` | `float ()` | movimiento vertical del mouse en pixeles en el frame actual |
| `servicios->fijarGravedadGlobal(x,y,z)` (v4) | `void (float, float, float)` | fija la gravedad del mundo fisico; cada cuerpo la recibe por su escala |
| `servicios->gravedadGlobalX/Y/Z()` (v4) | `float ()` | gravedad global por eje |
| `servicios->crearObjeto("Enemigo",padre)` (v4) | `void* (const char*, void*)` | crea un objeto vacio como hijo de `padre` (`nullptr` = raiz). Devuelve el puntero de inmediato para configurarlo, pero entra a la escena al final del frame; vale mientras el objeto viva |
| `servicios->destruirObjeto(obj)` (v4) | `bool (void*)` | marca para borrar al final del frame (seguro incluso sobre si mismo); `false` si no existe |
| `servicios->clonarObjeto(orig,padre)` (v4) | `void* (const void*, void*)` | copia profunda (componentes incluidos) como hija de `padre`; misma validez diferida que crear. Los scripts copiados compilan a demanda |
| `servicios->agregarColliderEsfera(obj,radio)` (v4) | `bool (void*, float)` | agrega collider esfera; `false` si ya tiene uno o el radio no es valido |
| `servicios->agregarColliderCubo(obj,radio)` (v4) | `bool (void*, float)` | agrega collider cubo; mismas condiciones |
| `servicios->agregarRigidBody(obj,masa)` (v4) | `bool (void*, float)` | agrega cuerpo con masa; `false` si no hay collider o ya tiene cuerpo |
| `servicios->instanciarPrefab("Enemigo",padre)` (v5) | `void* (const char*, void*)` | instancia un prefab por nombre como hijo de `padre` (`nullptr` = raiz). Devuelve la raiz de la instancia (valida al final del frame). |
| `servicios->listarPrefabs()` (v5) | `const char** ()` | lista los nombres de prefabs disponibles; array terminado en `nullptr`. |
| `servicios->interfazDeObjeto("HUD")` (v6) | `void* (const char*)` | handle de la interfaz web (HUD) del objeto con ese nombre, o `nullptr` si no existe o no tiene `InterfaceComponent`. El handle es opaco: identifica el componente |
| `servicios->interfazActiva(iface)` (v6) | `bool (const void*)` | estado de visibilidad de la interfaz |
| `servicios->interfazEstablecerActiva(iface,activa)` (v6) | `void (void*, bool)` | muestra/oculta la interfaz |
| `servicios->interfazEstablecerEstado(iface,json)` (v6) | `bool (void*, const char*)` | reemplaza el arbol de estado que el motor entrega a la interfaz; `false` si el texto no es un objeto JSON valido |
| `servicios->interfazEstablecerValor(iface,ruta,valorJson)` (v6) | `bool (void*, const char*, const char*)` | fija un valor en la ruta (JSON Pointer, p. ej. `"/jugador/vida"`; se acepta sin la barra inicial), creando los objetos intermedios; `valorJson` es un literal JSON de cualquier tipo |
| `servicios->interfazValor(iface,ruta)` (v6) | `const char* (void*, const char*)` | valor que la interfaz devolvio al motor en esa ruta, como texto JSON, o `nullptr` si no existe |
| `servicios->reproducirMusica(ruta,vol,fundido)` (v8) | `bool (const char*, float, float)` | reproduce una pista de musica por **ruta** (relativa a `src<proyecto>/` o absoluta); `false` si el archivo no existe. Si la pista no esta en la lista del componente `Musica`, suena sola |
| `servicios->detenerMusica(fundido)` (v8) | `void (float)` | detiene la musica (con fundido de salida opcional) |
| `servicios->volumenMusica(vol)` (v8) | `void (float)` | volumen del bus de musica (todas las pistas juntas, de 0 a 1) |
| `servicios->interfazNativaDeObjeto(nombre)` (v9) | `void* (const char*)` | handle de la interfaz **nativa** (HUD que dibuja el motor) del objeto con ese nombre, o `nullptr` si no existe o no tiene `UIComponent`. El handle es opaco: identifica el componente |
| `servicios->interfazNativaVisible(iface)` (v9) | `bool (const void*)` | si la interfaz nativa esta activa (visible) |
| `servicios->interfazNativaEstablecerVisible(iface,visible)` (v9) | `void (void*, bool)` | muestra u oculta la interfaz nativa |
| `servicios->interfazNativaEstablecerTexto(iface,id,texto)` (v9) | `bool (void*, const char*, const char*)` | cambia el texto del widget con ese identificador; `false` si no hay ningun widget con ese identificador |
| `servicios->interfazNativaEstablecerVisibleWidget(iface,id,visible)` (v9) | `bool (void*, const char*, bool)` | muestra u oculta un widget; `false` si no hay ningun widget con ese identificador |
| `servicios->interfazNativaBotonPulsado(iface,id)` (v9) | `bool (void*, const char*)` | si ese boton recibio el clic en el frame actual (flanco de pulsacion) |

Los deltas del mouse se acumulan durante el frame y se reinician al avanzar al
siguiente. Se capturan en Juego y cuando el cursor esta bloqueado durante
Depuracion; la primera posicion tras iniciar la simulacion solo establece el
origen y no produce un salto.

Las teclas usan nombres de tecla GLFW sin el prefijo `GLFW_KEY_`:
`"A"`..`"Z"`, `"D0"`..`"D9"`, `"F1"`..`"F12"`, `"SPACE"`, `"ENTER"`, `"TAB"`,
`"ESCAPE"`, `"LEFT_SHIFT"`, `"RIGHT_SHIFT"`, `"LEFT_CONTROL"`, `"UP"`,
`"DOWN"`, `"LEFT"`, `"RIGHT"`, entre otras. Las letras son mayusculas.

Ejemplo: control por teclado + sonido + busqueda de objetos:

```cpp
void onUpdate(GameObject* owner, float deltaTime) override {
    if (!api || !servicios) return;

    // Movimiento con teclado (A/D + flechas):
    float dx = 0;
    if (servicios->teclaSostiene("A") || servicios->teclaSostiene("LEFT"))
        dx -= velocidad * deltaTime;
    if (servicios->teclaSostiene("D") || servicios->teclaSostiene("RIGHT"))
        dx += velocidad * deltaTime;
    api->fijarPosicion(owner, api->posicionX(owner) + dx,
                       api->posicionY(owner), api->posicionZ(owner));

    // Efecto de sonido al disparar (una sola vez por pulsacion):
    if (servicios->teclaPresionada("SPACE"))
        servicios->reproducirSonido("sonidos/disparo.wav", 0.8f, false);

    // Alcanzar otro objeto por nombre (o usar el campo GameObject*
    // expuesto en el inspector, que es lo recomendado):
    void* meta = servicios->objetoPorNombre("Meta");
    if (meta) api->imprimirConsola("meta encontrada");
}
```

> **Notas de modularidad:** la tabla `servicios` es distinta de `api` a
> proposito: audio/busqueda/teclado son servicios de escena, no del objeto, y
> se inyectan como punteros opacos (`AudioEngine*`, `SceneRegistry*`,
> `InputScripts*`) que el modulo de scripts envuelve sin conocer sus
> cabeceras. Java usa las funciones estaticas equivalentes de `Nativo`,
> descritas en la seccion 15.

### 13.5 Notas del backend C++

- El fuente se compila a `.so` (Linux), `.dll` (Windows) o `.dylib` (macOS). El
  juego de flags lo decide la **familia del compilador**, no el sistema
  operativo: MSVC (`cl.exe`) recibe
  `/nologo /LD /std:c++17 /O2 /MD /EHsc` y GCC/Clang (`g++`, `c++`, `clang++`,
  incluido MinGW en Windows) recibe `-std=c++17 -shared -fPIC -O2`; en los dos
  casos se agrega `-DFUNSHI_NOMBRE_CLASE=<ClassName>`.
- **Runtime de C++ compartido (Windows/MSVC):** el `.dll` del script se compila
  con el **mismo CRT dinamico que el engine** (`/MD` en Release, `/MDd` en
  Debug) y con `/EHsc`. No es una preferencia de estilo: los `SerializeField`
  cruzan la frontera del `.dll` con `std::string`, `std::vector` y
  `std::function`, asi que la memoria se aloca en un modulo y se libera en el
  otro. Compilar el script con el CRT estatico (`/MT`) le da a la `.dll` su
  propio heap, y liberar memoria del otro lado corrompe el heap: el motor muere
  al asignar el script, sin log ni backtrace. El runtime del engine se declara
  en `CMakeLists.txt` (`CMAKE_MSVC_RUNTIME_LIBRARY`) y llega al script como
  `FUNSHI_CXX_RUNTIME_FLAG`; el cache de artefactos incluye ese contrato y la
  version del runtime de scripts, asi que cambiar los flags o la API recompila
  los binarios solos (no hay que borrar
  `%TEMP%/funshi_scripts` a mano).
- El `.so` del script **no enlaza contra el motor**: todo el acceso pasa por
  la tabla `api` de punteros a funcion. No incluyas cabeceras del motor mas
  alla de `IScriptBehaviour.h`; en particular evita arrastrar Bullet/Assimp
  (opcional: usa `<cmath>`, `<string>`, `<vector>` y demas STL).
- La clase compilada se llama `FUNSHI_<ClassName>`: el mismo template
  (`FUNSHI_NOMBRE_CLASE`) compila para cualquier `<ClassName>.cpp`.
- El cache de artefactos compilados y la ruta del compilador se muestran en
  la ventana Estado.
- **Windows:** el motor invoca `cl.exe` a traves de `vcvars64.bat` del mismo
  toolset MSVC, porque `cl.exe` resuelve los headers del CRT (incluido
  `<cstddef>`) y las librerias por `INCLUDE`/`LIB`. Con esto el editor funciona
  igual si se lanza desde el Explorador o desde Visual Studio. El toolset se
  busca en este orden: el que se deduce de la ruta del compilador (un build de
  desarrollo, donde esa ruta es de este equipo) y, si no hay ninguno, el que
  tenga instalado el usuario en sus carpetas de Visual Studio, con lo que el
  paquete instalado compila scripts C++ sin tocar nada. Con Visual Studio
  presente se usa `cl` a secas (el nombre lo resuelve el propio entorno), de
  modo que el motor no queda atado a la versión instalada. Si el compilador
  configurado es MinGW/g++ (`FUNSHI_CXX`, o el horneado por el build), se
  emiten los flags de GCC: ese camino tambien funciona y no necesita `cl.exe`
  en el entorno. Los scripts Java no tienen este requisito (javac se invoca por
  ruta absoluta).
- **Que compilador se usa:** primero la variable de entorno `FUNSHI_CXX` si
  esta, que manda siempre; despues el compilador con el que se compilo este
  motor, pero solo si esa ruta existe en este equipo (en el paquete instalado no
  existe, porque es la del equipo que lo publico); y si no, `cl` en Windows con
  Visual Studio instalado o `g++` en Linux, macOS y Windows con MinGW. La ruta
  final se muestra en la ventana Estado. Si `FUNSHI_CXX` apunta a algo que no
  existe, el motor lo avisa por log y lo usa tal cual.
- **Export de la fabrica en Windows/MSVC:** la funcion
  `FUNSHI_CREAR_COMPORTAMIENTO` tiene que declararse con
  `FUNSHI_COMPORTAMIENTO_EXPORT` (asi la genera el editor). En MSVC un
  `extern "C"` pelado **no se exporta solo**: la `.dll` compila, pero
  `GetProcAddress` no la encuentra. Para los scripts escritos con el template
  viejo (sin el macro), el motor pide el export tambien en el link
  (`/EXPORT:`), asi que siguen funcionando; los `.dll` compilados antes de ese
  cambio simplemente se recompilan solos. En MinGW/Linux/macOS el macro queda
  vacio porque ahi los simbolos se exportan por defecto.

---

## 15. Scripting Java (JNI)

Requiere que el motor se haya compilado con el JDK disponible
(`FUNSHI_JAVA=ON`); la ventana Estado muestra `javac`, `libjvm` y si el
soporte esta activo.

En runtime hace falta un **JDK** (no un JRE) porque el motor compila el
`.java` del proyecto con `javac` antes de cargarlo en la JVM. El motor lo
busca solo, en este orden:

1. `FUNSHI_LIBJVM` (ruta explicita a la biblioteca de la JVM).
2. Un `jre/` junto al ejecutable (reservado para empaquetar un runtime).
3. `JAVA_HOME` (`<JAVA_HOME>/bin/server/jvm.dll` en Windows,
   `<JAVA_HOME>/lib/server/libjvm.so` en Linux y macOS).
4. La ruta con la que se compiló el binario, que solo existe si el juego se
   corre en la misma máquina donde se compiló.
5. Las instalaciones típicas: el registro de Windows (`JavaSoft\JDK`,
   `JavaSoft\Java Development Kit`, `Eclipse Adoptium\JDK`) y las carpetas
   `Program Files\{Java,Eclipse Adoptium,Microsoft,Amazon Corretto,Zulu}`, o
   `/usr/lib/jvm` en Linux.

El `javac` se busca en la **misma** raíz que la JVM, así que el `.java` se
compila siempre con el mismo JDK que después lo ejecuta; es importante porque
un `javac` de otra versión genera un `.class` que su JVM rechaza. Si esa raíz
no trae `javac` (es un JRE), el motor **no** busca otro: los `.class` ya
compilados siguen cargando y al compilar avisa de que hace falta un JDK, no un
JRE. Se puede forzar el compilador con la variable `JAVAC` (y la biblioteca de
la JVM con `FUNSHI_LIBJVM`; si se dan las dos, la raíz de `FUNSHI_LIBJVM` es la
que manda para el emparejamiento).

Cuando una clase no carga, el mensaje dice **por qué**: la excepción que lanzó
la JVM (`motivo:`), la carpeta de caché, dónde se buscaron las clases y con qué
`javac` y `libjvm`. Con eso se distingue de un vistazo entre "el `.java` no
compiló", "la clase no existe" y "el `.class` es de otra versión de Java".

El instalador de Windows (`FunshiEngineGL_setup.iss`) comprueba si hay un JDK
antes de instalar —exige `bin\server\jvm.dll` **y** `bin\javac.exe`, los dos, que
es lo mismo que necesita el motor— y, si no lo encuentra, ofrece descargar e
instalar Temurin JDK 17. El paquete de Linux (Qt IFW) no puede encadenar
instaladores, así que declara el requisito en la descripción: en la mayoría de
distros el JDK ya viene instalado.

### 15.1 Plantilla generada por el editor

```java
// El nombre de la clase debe coincidir con el del archivo (MiScript.java).
public class MiScript implements Comportamiento {
    // Campos publicos = SerializeField editables en el inspector:
    public float velocidad = 5.0f;

    @Override
    public void iniciar(long objeto) {}

    @Override
    public void actualizar(long objeto, double deltaTime) {}

    @Override
    public void detener(long objeto) {}
}
```

### 15.2 API del motor y servicios en Java

Java tiene acceso a las mismas operaciones de objeto y servicios de escena que
C++. El objeto recibido como `long` es un handle opaco: se pasa a los metodos
que operan sobre GameObject y no se interpreta ni se modifica desde el script.
Las posiciones consultadas o fijadas por `Nativo.posicionX/Y/Z` y
`Nativo.fijarPosicion` son **locales** al transform del objeto.

| Operacion | Metodos Java |
|---|---|
| Nombre y transform local | `Nativo.nombre`, `posicionX/Y/Z`, `fijarPosicion`, `fijarEscala`, `fijarRotacion` |
| Getters de rotacion y escala | `Nativo.rotacionAngulo`, `rotacionEjeX/Y/Z`, `escalaX/Y/Z` |
| Movimiento con fisica | `Nativo.fijarVelocidadHorizontal`, `Nativo.saltar` |
| Propiedades fisicas | `Nativo.masa`, `fijarMasa`, `usaGravedad`, `fijarUsoGravedad`, `escalaGravedad`, `fijarEscalaGravedad`, `friccion`, `fijarFriccion`, `posicionCongelada`, `fijarFreezePosicion`, `rotacionCongelada`, `fijarFreezeRotacion` |
| Tags y colliders | `Nativo.etiqueta`, `tieneEtiqueta`, `objetoDeCollider` |
| Contactos | `colisionInicio`, `colisionPersistencia`, `colisionFin` |
| Consola | `Nativo.imprimir` |
| Audio de script | `Nativo.reproducirSonido(ruta, volumen, bucle)`, `Nativo.detenerSonido(handle)` |
| Musica | `Nativo.reproducirMusica(ruta, volumen, fundido)`, `Nativo.detenerMusica(fundido)`, `Nativo.volumenMusica(volumen)` |
| Busqueda de objetos | `Nativo.objetoPorNombre(nombre)`, `Nativo.objetoPorId(id)`, `Nativo.objetoPorEtiqueta(etiqueta)`; devuelven `0` si no existe |
| Gravedad global | `Nativo.fijarGravedadGlobal(x, y, z)`, `Nativo.gravedadGlobalX/Y/Z` |
| Gestion de objetos | `Nativo.crearObjeto(nombre, padre)`, `Nativo.destruirObjeto(objeto)`, `Nativo.clonarObjeto(original, padre)`, `Nativo.agregarColliderEsfera/cubo(objeto, radio)`, `Nativo.agregarRigidBody(objeto, masa)` |
| Prefabs | `Nativo.instanciarPrefab(nombre, padre)`, `Nativo.listarPrefabs()` (devuelve `String[]`) |
| Interfaces web (HUD) | `Nativo.interfazDeObjeto(nombre)` (handle opaco, `0` si no existe), `Nativo.interfazActiva(iface)`, `Nativo.interfazEstablecerActiva(iface, activa)`, `Nativo.interfazEstablecerEstado(iface, json)`, `Nativo.interfazEstablecerValor(iface, ruta, valorJson)`, `Nativo.interfazValor(iface, ruta)` (texto JSON o `null`) |
| Interfaz nativa (HUD del motor) | `Nativo.interfazNativaDeObjeto(nombre)` (handle opaco, `0` si no existe), `Nativo.interfazNativaVisible(iface)`, `Nativo.interfazNativaEstablecerVisible(iface, visible)`, `Nativo.interfazNativaEstablecerTexto(iface, id, texto)`, `Nativo.interfazNativaEstablecerVisibleWidget(iface, id, visible)`, `Nativo.interfazNativaBotonPulsado(iface, id)` (`true` si ese boton se pulso en este frame) |
| Teclado | `Nativo.teclaSostiene`, `teclaPresionada`, `teclaSoltada` |
| Mouse | `Nativo.deltaMouseX`, `Nativo.deltaMouseY` (pixeles por frame) |

El handle devuelto por `objetoPorNombre`, `objetoPorId` u `objetoPorEtiqueta`
puede pasarse a los getters y setters de `Nativo`. Un campo `public long` del
comportamiento es una referencia a GameObject: en el inspector se asigna por
drag & drop y en disco se guarda por nombre, igual que `GameObject*` en C++.
Los nombres de teclas usan las mismas cadenas GLFW de C++, incluidos
los digitos `D0`..`D9`. Los servicios toleran no estar conectados; durante
`detener` siguen conectados y los sonidos iniciados por scripts se detienen al
terminar la simulacion, sin detener sonidos propios de componentes antes de
ejecutar `detener`.

```java
@Override
public void actualizar(long objeto, double deltaTime) {
    if (Nativo.teclaPresionada("SPACE")) {
        int sonido = Nativo.reproducirSonido("sonidos/disparo.wav", 0.8f, false);
        Nativo.detenerSonido(sonido);
    }
    long meta = Nativo.objetoPorNombre("Meta");
    if (meta != 0) {
        float x = Nativo.posicionX(meta);
        Nativo.fijarPosicion(objeto, x, Nativo.posicionY(objeto),
                             Nativo.posicionZ(objeto));
    }
}
```

### 15.3 Diferencias con C++

- El ciclo es `iniciar` / `actualizar` / `detener` y reciben el objeto como
  `long` (handle nativo), no como `GameObject*`.
- Los callbacks de contacto reciben los handles del objeto y de ambos colliders;
  `Nativo.objetoDeCollider` resuelve el dueño del collider opuesto.
- Los campos `public` del comportamiento son SerializeField automaticos; no se
  usan macros.
- Las operaciones del motor se llaman como metodos estaticos de `Nativo`; en
  C++ se accede mediante las tablas `api` y `servicios`.
- Cada carga usa un classloader nuevo (child-first: delega al padre solo
  `Nativo`, `Comportamiento` y clases del JDK), lo que permite hot reload sin
  reiniciar la JVM.
- Utiles para prototipado rapido; para rendimiento, preferi C++.

---

## 16. Hot reload y depuración

- **C++:** guardar el `.cpp` durante la simulacion recompila; el editor compara el mtime del
  fuente con el del artefacto cargado. Los valores SerializeField se extraen
  antes de descargar y se reinyectan por nombre de campo al terminar, de modo
  que reordenar campos en el fuente no pierde valores.
- **Varios objetos sobre el mismo script:** si dos objetos apuntan al mismo
  `.cpp` (o al mismo `.java`), solo se compila una vez: el segundo componente
  usa el artefacto que ya está al día en vez de compilarlo otra vez, que en
  Windows sería reescribir una `.dll` que está cargada (y el sistema lo
  rechaza).
- **Java:** igual, con el classloader child-first; la JVM se reutiliza.
- **Ventana Estado:** para cada script muestra nombre, ok/error y mensaje
  (errores de compilacion incluidos), ademas del toolchain detectado.
- **Errores de carga/compilacion C++ y Java** aparecen en la ventana Estado y
  se publican en el registro estructurado compartido. El mensaje
  contextual del componente conserva el error de su fuente; corregilo y vuelve
  a cargar el script.
- Si un callback del script arroja una excepción durante la ejecución, el
  motor conserva el detalle en el estado del componente, lo publica como
  `SCRIPT_CALLBACK_FAILED` y deja de invocar ese comportamiento hasta que se
  recargue. El resto de la escena continúa ejecutándose.
- Al cerrar la aplicacion los comportamientos se descargan sin disparar
  `onStop`; los backends (incluida la JVM) se apagan despues.

---

## 17. Terminal integrada

El panel **Terminal** abre una shell del sistema dentro del editor. Se acopla
por defecto en la zona inferior y también se puede mostrar u ocultar desde
**Ventanas → Terminal**. Su directorio de trabajo es la raíz del proyecto
activo; al cambiar de proyecto se cierra la sesión anterior.
Ocultar el panel conserva la sesión en ejecución.

Selecciona una shell en el menu del panel y presiona **Iniciar**. Haz clic en
el area de la terminal y escribe directamente; **Enter** ejecuta el comando,
**Ctrl+C** envía la señal de interrupción, y las flechas, **Tab**, **Backspace**,
**Delete**, **Home**, **End** y **Escape** se envían como teclas de terminal.
El botón **Interrumpir** también envía Ctrl+C; **Cerrar sesión** termina la
shell y los procesos asociados; la salida que la shell alcance a emitir al
cerrarse igual se muestra en el panel, y la sesión arranca con la cuadrícula
del tamaño real del panel.

En Windows se puede elegir `cmd.exe`, Windows PowerShell (`powershell.exe`) o
PowerShell (`pwsh.exe`, si está instalado). En Linux se puede elegir Bash, `sh`
o Zsh (si está instalado). Windows requiere ConPTY, disponible desde Windows 10
versión 1809 ([requisito oficial de CreatePseudoConsole](https://learn.microsoft.com/en-us/windows/console/createpseudoconsole)).

El panel mantiene una cuadricula de celdas y procesa UTF-8, movimiento del
cursor, borrado y desplazamiento de pantalla, colores ANSI de 16 entradas,
visibilidad del cursor y pantalla alternativa. La terminal responde consultas
de estado y posicion del cursor. La emulacion cubre secuencias VT de uso comun;
no implementa extensiones como colores true-color/256, raton, portapapeles ni
todas las convenciones de cada emulador. Algunas aplicaciones TUI pueden
requerir funciones que el panel todavia no ofrece.

---

## 18. Mods del editor (.fmod)

Un **mod** es un paquete que extiende el editor: agrega paneles y herramientas
que el motor no trae de fábrica. Los mods se aplican **solo al editor**; los
juegos exportados no llevan mods.

### 18.1 Carpetas de mods

El motor lee dos carpetas y las dos son opcionales:

- `<raizDeDatos>/Mods/` — mods **globales**, disponibles para cualquier
  proyecto. La carpeta se crea al arrancar el editor.
- `<proyecto>/Mods/` — mods del **proyecto activo**; se leen solo mientras ese
  proyecto está abierto. La carpeta se crea al crear el proyecto.

Ambas se recorren al arrancar el editor. El resultado se reporta en el panel de
estado/log: un resumen por ámbito (`mods de motor encontrados: N` y `mods de
proyecto encontrados: M`, cada uno solo si hay al menos uno) y, por cada paquete
rechazado, el nombre del mod y la causa.

Después, los paquetes válidos se **cargan**: su binario se extrae a la caché
(`<raizDeDatos>/cache/mods/<id del mod>/`) y se abre, y el mod recibe su
inicialización. La carga se reporta aparte, porque un paquete puede ser válido
y su binario no: `mods nativos cargados: N` dice cuántos quedaron en memoria, y
cada mod que no se pudo cargar aparece con su causa (`mod no cargado: ...`).
Al salir del editor los mods se apagan en orden inverso al de carga y sus
binarios se cierran.

### 18.2 Instalación

La instalación es **manual**: copia o arrastra el archivo `.fmod` a la carpeta
de mods que corresponda. No hay descarga desde red desde el motor.

### 18.3 Formato del paquete

Un `.fmod` es un archivo **ZIP** con extensión propia (se puede abrir con
cualquier herramienta ZIP). El nombre del archivo es libre; la única fuente de
verdad es el manifiesto. El paquete contiene:

```
autor.mimod_1.0.0.fmod
├── ManifiestoMod.json       ← obligatorio, en la raíz del ZIP
├── icono.png                ← opcional
└── bin/                     ← el binario del mod (.dll en Windows, .so en Linux)
```

La extracción rechaza rutas que escapen de la carpeta destino (`..` o rutas
absolutas): un paquete no puede escribir fuera de su caché.

### 18.4 Manifiesto

`ManifiestoMod.json` declara la identidad del mod y con qué puede convivir:

```json
{
  "esquemaVersion": 1,
  "id": "Autor.Nombre",
  "nombre": "Nombre visible",
  "descripcion": "Qué hace el mod",
  "autor": "Autor",
  "version": "1.0.0",
  "licencia": "MIT",
  "icono": "icono.png",
  "tipo": "nativo",
  "versionMotor": ">=1.0.0 <2.0.0",
  "puntoEntrada": "bin/mod.dll",
  "dependencias": [
    { "id": "Autor.Otro", "versionMin": "1.2.0", "requerida": true, "orden": "antes" }
  ],
  "conflictos": ["Autor.Incompatible"]
}
```

- `id` es estable, con formato `Autor.Nombre`. Un mismo `id` **dos veces** (en
  la misma carpeta o entre la global y la del proyecto) es error: se rechazan
  todas las instancias con ese id.
- `version` y `versionMin` usan `X.Y.Z`. `versionMotor` es un rango contra la
  versión del motor (`>=1.0.0 <2.0.0`), para no cargar binarios incompatibles.
- `tipo` solo admite `"nativo"` en v1. `"contenido"` está reservado para el
  futuro.
- `orden` de una dependencia es `antes`, `despues` o `ninguno`: los mods se
  carga en orden topológico y un ciclo entre dependencias se rechaza.
- `icono`, `dependencias` y `conflictos` son opcionales.
- `puntoEntrada` es la ruta del binario **dentro del paquete**. Es una sola
  cadena: el paquete lleva un binario por plataforma, así que un `.fmod` se
  arma para el sistema al que se instala.

### 18.5 La API del mod

Un mod nativo es una biblioteca compartida (`.dll` en Windows, `.so` en Linux)
que exporta una función de entrada:

```cpp
extern "C" FUNSHI_MOD_EXPORT int funshi_mod_api(FunshiMods::IMod** salida);
```

Devuelve la versión de la API con la que se compiló, o `0` si no puede
trabajar con el motor; el motor rechaza los que declaran una versión de API
posterior a la que ofrece. La interfaz del mod es una tabla de punteros a
función (`init`, `update`, `dibujar`, `shutdown`, `nombre`, `versionStr`), todas
opcionales. El motor no expone objetos: le pasa una tabla de servicios y, en
cada dibujo, un contexto con el contexto de ImGui y el tamaño del área. El mod
que dibuja hace `ImGui::SetCurrentContext(ctxImgui)` antes de pintar.

La tabla de servicios es *append-only*: los campos nuevos van al final. La
única diferencia con las tablas de API de scripts es que **la versión va al
principio**, para que un mod compilado contra una versión anterior lea siempre
su versión en el mismo offset. Los servicios que ofrece son:

| Servicio | Qué hace |
|---|---|
| `version` | Versión de la API de mods que ofrece el motor (primero, offset fijo). |
| `registrarPanel(id, titulo, dibujar, datos)` | Registra un panel de GUI del mod. Devuelve si se registró. |
| `contarObjetos()` | Objetos que hay en la escena abierta. |
| `obtenerObjeto(indice, salida)` | Datos de un objeto (nombre, id, si tiene collider, cuántos componentes tiene). |
| `contarComponentes(indiceObjeto)` | Componentes que tiene un objeto de la escena. |
| `obtenerComponente(indiceObjeto, indiceComponente, salida)` | Datos de un componente (su tipo). |
| `nombreProyecto()` | Nombre del proyecto abierto (cadena vacía si no hay ninguno). |
| `contextoImGui()` | Contexto de ImGui del motor, opaco: el mod lo pasa a `ImGui::SetCurrentContext()` para dibujar con él. |

### 18.6 Paneles de los mods

Un mod puede abrir sus propias ventanas dentro del editor. Se registran con
`registrarPanel()` desde `init()` (o desde `update()`, en caliente): cada panel
recibe un identificador (único dentro del mod), un título visible y el callback
que dibuja su contenido.

El motor abre la ventana del panel, le entrega el contexto de dibujo (el
contexto de ImGui, el tamaño del área disponible y el delta del frame) y guarda
el estado abierto/cerrado junto al resto de ventanas del editor. Por eso las
ventanas de los mods aparecen también en el menú **Ventanas** de la barra
principal, con su casilla, y su estado se guarda con el proyecto.

Un mod que implementa `dibujar()` no necesita registrar nada: el motor le abre
una ventana propia, titulada con el nombre visible del mod, y llama a
`dibujar()` dentro en cada frame.

Cuando el mod se descarga, sus paneles se retiran: el editor nunca dibuja una
ventana cuyo binario ya está cerrado.

### 18.7 Cómo se crea un mod

Un mod es un **proyecto aparte**: no se compila con el motor ni entra en su
repositorio ni en el instalador. Se compila por separado y su resultado es un
`.fmod` que se copia a la carpeta de mods del motor.

**1. El proyecto.** El mod usa solo la API pública del motor
(`src/Mods/ApiMod.h`, una cabecera con tablas POD) y **su propia copia de
ImGui**: no puede enlazar contra el motor (es un ejecutable, no una biblioteca)
y el contexto de ImGui que usa es el del motor, que se le entrega opaco. Por
eso tiene que compilar **las mismas fuentes de ImGui que usa el motor**, para
que las estructuras del contexto coincidan en memoria; si el motor actualiza
ImGui, el mod se recompila.

**2. El código.** El binario exporta un único símbolo, con la interfaz del mod
en una tabla de punteros a función:

```cpp
extern "C" FUNSHI_MOD_EXPORT int funshi_mod_api(FunshiMods::IMod** salida);
```

Devuelve la versión de la API con la que se compiló (`0` si no puede trabajar
con ese motor). La interfaz tiene `init`, `update`, `dibujar`, `shutdown`,
`nombre` y `versionStr`, todas opcionales. Desde `init()` el mod registra sus
paneles con `registrarPanel()` y los dibuja con widgets de ImGui, después de
fijar el contexto que le entrega el motor:

```cpp
ImGui::SetCurrentContext(static_cast<ImGuiContext*>(contexto->ctxImgui));
```

**3. El manifiesto.** El paquete lleva un `ManifiestoMod.json` en la raíz con
los campos del § 18.4. El campo `puntoEntrada` es la ruta del binario **dentro
del paquete** (por ejemplo `bin/MiPrimerMod.dll`), con `/` como separador.

**4. El empaquetado.** El `.fmod` es un ZIP (se puede inspeccionar con
cualquier herramienta ZIP) con esta estructura:

```
MiPrimerMod.fmod
├── ManifiestoMod.json
└── bin/
    └── MiPrimerMod.dll
```

**5. La instalación.** Copiar el `.fmod` a la carpeta de mods del motor (junto
al ejecutable) y reiniciar el editor: los mods se cargan al arrancar.

Un `.fmod` es para un sistema: el paquete lleva un binario por plataforma, así
que hay que compilarlo en la plataforma destino.

### 18.8 Confianza

Los mods se cargan con **confianza total**: no hay firma ni sandbox, igual que
en otros motores con mods. Un mod es código que corre dentro del editor, con
los mismos permisos. Instala solo paquetes de autores en los que confíes; el
motor no puede distinguir un mod benigno de uno malicioso.

### 18.9 Estado activo

El estado activo de cada mod se guarda por ámbito: los mods del motor en la
configuración general del editor (`Configuracion.json`) y los del proyecto en
la del proyecto (`ConfiguracionProyecto.json`). Un mod que no está en el mapa
se considera activo por defecto.

### 18.10 Actualización y alcance de esta versión

No hay hot-reload: los mods se cargan al arrancar y se descargan al salir.
Actualizar un mod es reemplazar su `.fmod` y reiniciar el editor. Esta versión
cubre el **envase, la carga y los paneles**: descubre, valida y ordena los
paquetes, extrae el binario de cada uno a la caché, lo abre y lo inicializa; el
mod recibe la tabla de servicios, se actualiza una vez por frame y puede
registrar paneles de GUI propios.

---

---

## 19. API de inspeccion de escena

El motor publica un contrato para que una herramienta externa pueda leer y
modificar una escena sin abrir el editor. La herramienta que lo consume hoy es
la **CLI** (`FunshiEngineCLI`), que es un **proyecto aparte**: el motor
publica la API y la CLI la usa; no hay ningun ejecutable de linea de comandos
dentro de este repositorio.

El contrato vive en `FunshiEngineGL/src/Inspeccion/`:

| Archivo | Que aporta |
|---|---|
| `ApiEscena.h` | El contrato `IApiEscena`, las vistas y `versionApiEscena` |
| `PropiedadesComponentes.h/.cpp` | Las variables de cada tipo de componente |
| `TextoValor.h/.cpp` | Las conversiones entre texto y valor |
| `InspeccionEscena.h/.cpp` | La implementacion sobre la escena y el editor |

### 19.1 Version del contrato

`versionApiEscena` versiona el contrato de inspeccion. Es **independiente** de
la version de los scripts (`versionRuntimeScript`): subir una no recompila los
scripts de los proyectos ni invalida su cache. Una herramienta comprueba el
numero al arrancar y avisa si no coincide con el que espera.

### 19.2 Consultar la escena

`listarObjetos(filtro)` devuelve los objetos de la escena que cumplen el
filtro. Los filtros se acumulan (se combinan entre si):

| Filtro | Que selecciona |
|---|---|
| `componente` | Objetos que tienen ese componente (se acepta el nombre canonico y los alias, como `Camera` por `CameraComponent`) |
| `etiqueta` | Objetos con esa etiqueta (tag) exacta |
| `nombre` | Por nombre exacto, o parcial si `nombreParcial` esta activo (no distingue mayusculas) |
| `id` | Un objeto concreto |
| `tipoObjeto` | Por tipo: `SimpleObject`, `Modelos3D`, `GameObject`... |
| `limite` | Maximo de objetos devueltos (0 = sin limite) |

De cada objeto se devuelve su id, nombre, etiqueta, tipo, id del padre,
posicion local, posicion global (compuesta con la de sus padres), cantidad de
componentes y el numero de componentes que tiene.

`componentesDe(id)` devuelve los componentes del objeto con sus variables y el
valor actual de cada una. `objetoPorId(id)` devuelve el objeto, o nada si no
existe.

### 19.3 Modificar la escena

Toda mutacion pasa por el gestor de comandos del editor, asi que se deshace
igual que en el editor. Las operaciones que fallan no dejan la escena a medias:
devuelven `false` y el motivo.

| Operacion | Que hace |
|---|---|
| `crearObjeto(nombre, idPadre)` | Crea un objeto simple. Si el nombre ya esta en uso, recibe un sufijo numerico (`Enemigo 2`) |
| `establecerEtiqueta(id, etiqueta)` | Fija la etiqueta (tag) |
| `crearComponente(id, tipo)` | Agrega un componente. Respeta los prerrequisitos del catalogo (un `RigidBody` necesita un `Collider`) y no admite dos componentes del mismo tipo |
| `quitarComponente(id, tipo)` | Quita un componente |
| `asignarVariable(id, tipoComponente, variable, valor)` | Escribe una variable de un componente |
| `guardarEscena(prefijo)` / `cargarEscena(prefijo)` | Guarda y carga la escena |

La escena se guarda con un **prefijo** de ruta, igual que el motor al guardar
un proyecto: el indice queda en `<prefijo>BBDDObjetos.txt` y los binarios de
cada objeto en la carpeta `<prefijo>/`.

### 19.4 Variables y su formato

El valor de una variable es texto, y el motor lo interpreta segun el tipo de
la variable:

| Tipo | Como se escribe | Ejemplo |
|---|---|---|
| `Entero` | Un entero | `12` |
| `Flotante` | Un numero (se rechazan `nan` e infinitos) | `1.5` |
| `Booleano` | `true`/`false` (o `1`/`0`) | `true` |
| `Texto` | El texto tal cual | `assets/sonido.ogg` |
| `Vec2`, `Vec3`, `Vec4` | Los numeros separados por espacios o comas | `1 2 3` |
| `Enumeracion` | Uno de los valores admitidos | `Puntual` |

Las variables disponibles por tipo de componente:

| Componente | Variables |
|---|---|
| `Transform` | `position`, `scale`, `rotationAngle`, `rotationAxis` |
| `RigidBody` | `mass`, `active`, `useGravity`, `gravityScale`, `friction`, `sensor`, `freezePositionX/Y/Z`, `freezeRotationX/Y/Z` |
| `Light` | `type` (`Direccional`, `Puntual`, `Spot`), `ambient`, `diffuse`, `specular`, `attenuationConstant`, `attenuationLinear`, `attenuationQuadratic`, `spotCutOff` |
| `CameraComponent` | `fov`, `projection` (`Perspectiva`, `Ortografica`), `orthoSize`, `nearPlane`, `farPlane`, `speed` |
| `Material` | `ambient`, `diffuse`, `specular`, `emission`, `shininess`, `diffuseMap`, `specularMap`, `normalMap`, `emissionMap` |
| `Materiales` | `selectedRegion` |
| `Color` | `color` |
| `Grid` | `visible`, `color`, `size`, `separation` |
| `SpriteRenderer` | `texturePath`, `tint`, `size`, `uvRect` |
| `AudioSource` | `clipPath`, `volume`, `loop`, `autoplay` |
| `Musica` | `volume`, `fadeDuration`, `autoplay`, `repeatList` |
| `InterfaceComponent` | `htmlPath`, `active`, `anchor`, `offset`, `autoSize`, `size`, `z` |
| `UIComponent` | `active`, `z` |
| `EsfereCollider`, `CubeCollider` | `radius`, `visibleInScene` |
| `MallaCollider`, `TilemapCollider` | `visibleInScene` |
| `Skybox` | `visible`, `facePositiveX/NegativeX/PositiveY/NegativeY/PositiveZ/NegativeZ` |
| `Animador2D` | `columns`, `rows`, `firstFrame`, `lastFrame`, `fps`, `loop`, `playing` |
| `Model` | `path` |
| `Script` | `sourcePath`, `componentName`, `deferredLoad` |
| `Tilemap` | `tilesetPath` |

Notas:

- La rotacion del motor es un angulo global mas un eje: por eso se expone en
  dos variables (`rotationAngle` en grados y `rotationAxis`) en vez de una
  sola.
- Los colores del material son RGB: el motor fuerza el canal alfa a 1.
- Cambiar la variable `sourcePath` de un `Script` descarta el comportamiento
  cargado, asi que el motor lo vuelve a compilar.
- Los valores se **validan antes** de aplicarse: un texto que no encaja con
  el tipo (por ejemplo `abc` en una variable numerica) se rechaza con su
  motivo y la escena no cambia.

---

---

## 20. Particulas y efectos

Un **emisor de particulas** es un componente: se agrega a cualquier objeto
desde el inspector (pestaña "Agregar componente") y suelta particulas en la
posicion del objeto. Sin mas, cada particula es un quad de color plano que
sale disparado en la direccion del emisor y se apaga al terminar su vida.

### 20.1 Que se puede editar

El panel del emisor tiene:

| Campo | Que hace |
|---|---|
| Activo | Apaga o enciende la emision sin quitar el componente |
| Particulas por segundo | Cuantas particulas suelta por segundo |
| Vida minima / maxima | Cuanto viven (en segundos) |
| Velocidad minima / maxima | A que velocidad salen (unidades de mundo por segundo) |
| Tamano minimo / maximo | De que tamano nacen (unidades de mundo) |
| Color | El color con el que nacen |
| Direccion | El eje del cono de emision |
| Dispersion | Cuanto se abre el cono (0 = todas en la direccion, 1 = cono de 90 grados) |
| Gravedad | La aceleracion constante que se les aplica |
| Rozamiento | La fraccion de velocidad que pierden por segundo |
| Rotacion / Velocidad de rotacion | El giro del quad |
| Ruta (perfil de efecto) | El JSON del efecto (opcional): escrito a mano o asignado arrastrando el `.json` desde el explorador de contenido; **Quitar** lo limpia |
| Textura de particula | Imagen que sobreescribe la textura del perfil al dibujar (se asigna arrastrándola desde el explorador; opcional) |

Las particulas se dibujan despues de los sprites, con mezcla aditiva, y no
escriben profundidad: el orden de dibujo decide el solapamiento.

### 20.2 El perfil de efecto

Un perfil es un archivo JSON que describe un efecto completo: la textura, la
vida, la velocidad, la gravedad y las **curvas de tamano y color** a lo largo
de la vida. Con perfil, sus valores mandan sobre los del emisor (salvo la
tasa, que sigue siendo del emisor).

El formato:

```json
{
  "version": 1,
  "nombre": "Humo",
  "textura": "particulas/humo.png",
  "vida": [0.6, 1.2],
  "velocidad": [1.0, 2.0],
  "aceleracion": [0.0, -1.0, 0.0],
  "rozamiento": 0.2,
  "tamanos": [0.4, 1.0, 1.2],
  "colores": [[1, 1, 1, 0.6], [0.5, 0.5, 0.5, 0.0]]
}
```

- `vida` y `velocidad` son pares `[minimo, maximo]`.
- `aceleracion` es una terna (x, y, z).
- `tamanos` son los multiplicadores de tamano por punto de la curva: el
  primero es el nacimiento y el ultimo la muerte; entre ellos se interpola de
  forma lineal. Un tamano de 0.4 que sube a 1.2 hace que la particula crezca.
- `colores` son los colores RGBA por punto de la curva, con la misma
  convencion. Un color que se va a alfa 0 hace que la particula se apague.

Todos los campos son opcionales. Si el JSON esta mal escrito (un rango
invertido, un color con menos de cuatro componentes), el emisor se queda sin
perfil y el panel muestra el motivo.

La ruta del perfil se guarda **relativa a la carpeta de assets** del proyecto,
igual que las texturas de los materiales.

### 20.3 La ventana Animaciones y Particulas

La ventana **Animaciones y Particulas** (menú **Ventanas**) es el taller de
los efectos: configura el perfil sin necesidad de tener un objeto delante y
genera el archivo que después se arrastra sobre un emisor.

- **Pestaña Particulas**: edita el perfil en curso (nombre, vida, velocidad,
  aceleración, rozamiento, textura y las curvas de tamaño y color, un punto
  por fila). El botón **Generar perfil** lo escribe como
  `Particulas/<nombre>.json` dentro de los assets del proyecto: aparece en el
  explorador de contenido y desde ahí se arrastra sobre el campo de perfil de
  un `EmisorParticulas`. Generar exige nombre válido y no pisa un efecto
  existente.
- Arrastrar un `.json` de perfil sobre la zona de carga de la pestaña lo trae
  de vuelta al editor para retocarlo y regenerarlo.
- **Pestaña Prefabs**: la biblioteca de prefabs del proyecto (guardar la
  selección, instanciar por lista o por arrastre, renombrar y borrar). El
  flujo completo está en la sección de prefabs (capítulo 4, "Prefabs:
  reutilizar jerarquías completas").
- **Pestaña Animaciones**: referencia a dónde se configura una animación 2D
  (el inspector del componente `Animador2D`); el taller propio de animaciones
  llegará con un formato de asset para ellas.

### 20.4 Como se ejecuta

La simulacion corre en el editor y en juego: un efecto se ve mientras se
edita. El numero de particulas vivas esta acotado (4000 por defecto), asi que
un efecto con muchos emisores no puede hundir el frame: cuando se llega al
limite, se descarta la particula mas vieja.

Las particulas son **cinematicas**: no chocan con la fisica. Si un efecto
necesita rebotar, habria que consultarlo contra el mundo.

### 20.5 Desde la consola

El emisor es un componente mas, asi que la API de inspeccion (seccion 19) lo
altera como a cualquier otro: `scene.component.create` con el tipo
`EmisorParticulas`, y `scene.component.set` con sus variables (`rate`,
`lifeMin`, `lifeMax`, `speedMin`, `speedMax`, `sizeMin`, `sizeMax`, `color`,
`direction`, `dispersion`, `gravity`, `drag`, `rotation`, `rotationSpeed`,
`profilePath` y `active`).

---

## 21. Interfaz de juego nativa (HUD del motor)

La seccion 9 describe la interfaz **web** (una pagina `.html` que se muestra
dentro del juego). Esta describe la interfaz **nativa**: un conjunto de widgets
(etiquetas, botones, imagenes y paneles) que dibuja el motor, sin navegador de
por medio y sin que el usuario tenga que saber HTML.

Las dos conviven: una escena puede tener una interfaz web y una nativa, o
varias nativas.

### 21.1 Que se puede editar

- Agrega `UIComponent` a un GameObject desde el inspector (pestaña "Agregar
  componente", entrada "Interfaz de juego (nativa)").
- El panel del componente tiene:
  - **Activa**: apaga o enciende el HUD sin quitar el componente.
  - **Z**: el orden de apilado entre varias interfaces nativas (la de mayor z
    queda encima).
  - **Anadir widget**: el menu para agregar un widget del tipo que corresponda.
- Cada widget se edita en su propia entrada de la lista (la flecha del titulo
  la abre):
  - **Identificador**: el nombre con el que un script consulta el widget. Es
    lo que permite referirse a "vida" o "botonStart" sin conocer el orden de la
    lista. No tiene que ser unico, pero conviene que lo sea.
  - **Tipo**: `Panel`, `Etiqueta`, `Imagen` o `Boton`.
  - **Ancla**: la posicion de la pantalla a la que se pega (las nueve
    posiciones, como la interfaz web).
  - **Desplazamiento**: pixeles de separacion respecto al ancla.
  - **Tamano**: ancho y alto del widget, en pixeles.
  - **Color**: el color de fondo (panel y boton) o del texto (etiqueta).
  - **Color del texto**: el color del texto (boton y etiqueta).
  - **Texto**: lo que escribe la etiqueta o el boton.
  - **Tamano del texto**: el cuerpo con el que se dibuja.
  - **Ruta de la imagen** (solo en las imagenes): se arrastra un archivo del
    proyecto; la ruta se guarda relativa a la carpeta de assets.
  - **Visible**: apaga el widget sin quitarlo de la lista.
  - **Subir** / **Bajar**: cambian el orden de apilado (el ultimo de la lista se
    dibuja encima).
  - **Quitar widget**: lo saca de la lista.

### 21.2 Colocacion

Cada widget se ancla a una de las nueve posiciones de la pantalla y crece desde
ahi hacia el lado que le corresponde: el de arriba centro queda centrado en
horizontal, el de abajo derecha pegado a la esquina, y asi con los demas. El
desplazamiento lo separa del punto de anclaje y el tamano decide cuanto
ocupa.

La colocacion se recalcula sola cuando cambia el tamano de la ventana.

### 21.3 El raton

El boton es el unico widget interactivo: es el que recibe el clic. Cuando el
puntero esta encima de un boton, el boton se aclara y **el juego deja de girar
la camara con el raton** (el boton se queda el raton).

El clic se entrega **por consulta**, no por callback: un script pregunta en su
actualizacion si el boton se pulso en este frame. El estado se borra al
terminar el frame, asi que un clic no se repite.

### 21.4 Desde los scripts

La interfaz nativa se maneja con el handle del objeto. En C++ se puede
declarar el campo y **arrastrar la interfaz** desde su inspector al campo
(igual que con la interfaz web):

```cpp
// El campo apunta al UIComponent del objeto arrastrado.
REFLECT_COMPONENTE(hud, "UIComponent")
```

Y tambien se puede obtener por el nombre del objeto:

```cpp
// C++
void* hud = servicios->interfazNativaDeObjeto("HUD");
servicios->interfazNativaEstablecerTexto(hud, "vida", "80");
if (servicios->interfazNativaBotonPulsado(hud, "botonStart")) {
    servicios->interfazNativaEstablecerVisible(hud, false);
}
```

```java
// Java
long hud = Nativo.interfazNativaDeObjeto("HUD");
Nativo.interfazNativaEstablecerTexto(hud, "vida", "80");
if (Nativo.interfazNativaBotonPulsado(hud, "botonStart")) {
    Nativo.interfazNativaEstablecerVisible(hud, false);
}
```

Las seis funciones estan en la tabla de servicios de la seccion 13.4 (C++) y
en la tabla de la seccion 15.2 (Java). Todas toleran un handle nulo (no hace
nada) y devuelven `false` si no hay ningun widget con ese identificador.

### 21.5 Diferencias con la interfaz web

- La nativa no necesita ningun motor de navegador: el motor dibuja los widgets
  con su propio dibujado de pantalla.
- La tipografia es la de la fuente que trae el motor, escalada por widget. No
  hay TTF propia todavia.
- La colocacion es respecto a la **ventana**, no respecto al viewport 3D: en el
  editor, con los paneles visibles, el HUD se ve encima de ellos.
- El texto y la visibilidad de un widget se cambian por su identificador; no
  hay un arbol de estado con JSON como en la interfaz web.

---
