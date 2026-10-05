<!--
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    SPDX-License-Identifier: Apache-2.0
-->

# Manual de uso de FunshiEngineGL

Guia practica para usuarios del editor: como montar una escena, asignar assets
por arrastre y escribir scripts con la API exacta que expone el motor.

> Documentacion complementaria: [README.md](README.md) (caracteristicas y
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
   carpeta de proyectos (cada carpeta de `MotorGrafico/` es un proyecto);
   elegi uno con click o crea uno escribiendo su nombre en el campo de la
   derecha y pulsando **Confirmar**. Con boton derecho sobre un proyecto se
   abre **Editar nombre**: escribi el nuevo nombre y el modal lo confirma,
   renombrando la carpeta en el acto. El mismo menu contextual ofrece
   **Eliminar proyecto**: el modal pide confirmacion y borra de disco la
   escena, los assets y la configuracion de ese proyecto (irreversible). Si
   eliminabas el proyecto abierto, el motor vuelve al estado "sin proyecto".
   "Iniciar Estudio" crea el proyecto y sus carpetas automaticamente.
2. Navega la escena con `W`/`A`/`S`/`D`, `Espacio`/`Shift` y el mouse (nav FPS).
   `E` oculta la UI; `Escape` vuelve al menu (y durante el play, detiene la
   simulacion y deja el editor).

---

## 2. Proyectos y estructura de carpetas

Al crear un proyecto, el motor genera la estructura bajo
`{app}/MotorGrafico/Proyects/<proyecto>/`:

> **Dónde quedan los datos.** `{app}` es la carpeta del ejecutable cuando el
> motor está en una carpeta donde puede escribir: el build de desarrollo y
> también la instalación de Windows, que es por usuario
> (`%LOCALAPPDATA%\Programs\FunshiEngineGL`, sin pedir permisos de
> administrador). Si el motor está en una carpeta donde no puede escribir —una
> copia en `Program Files`, una carpeta montada en solo lectura, el motor
> lanzado como administrador—, usa en su lugar
> `%APPDATA%\FunshiEngineGL\MotorGrafico` (Windows) o
> `$XDG_DATA_HOME/FunshiEngineGL/MotorGrafico`, con respaldo en
> `~/.local/share/FunshiEngineGL/MotorGrafico` (Linux y macOS). La ruta
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
MotorGrafico/
├── logs/
│   └── FunshiEngineGL_<AAAAMMDD_HHMMSS>.log   ← log de arranque (UTC), uno por ejecucion
├── Proyects/
│   └── <proyecto>/
│       ├── Memory/
│       │   ├── Binarios/Scene        ← escenas binarias
│       │   # ├── Interfaces/           ← assets JSON del CreadorDeInterfaces
│       │   ├── ConfiguracionProyecto.json
│       │   └── imgui.ini
│       └── src<proyecto>/            ← assets del proyecto (raiz del explorador)
│           ├── modelos/              ← .obj/.fbx que arrastra el editor
│           # ├── Sonidos/              ← clips de audio (.wav/.mp3/...)
│           └── Scripts/              ← scripts del usuario (.cpp/.java)
├── Configuraciones/
│   └── Configuracion.json            ← configuracion global (ultimo proyecto, idioma,
│                                       sensibilidades y apariencia: tema, B/N, acento,
│                                       fondo y radio de difuminado)
└── Exportaciones/
    └── <nombreExportacion>/          ← juegos exportados (ver seccion 10.1)
        ├── <Juego>.exe / <Juego>     ← ejecutable standalone
        ├── Data/
        │   ├── Memory/
        │   # ├── Sonidos/
        │   └── ConfiguracionProyecto.json
        └── lib/                      ← dependencias runtime (Bullet, miniaudio, GLFW, etc.)
```

En el arbol, las carpetas marcadas con `#` las crea el motor al crear el proyecto
(no hace falta hacerlas a mano). La convencion de assets por nombre usa las carpetas
`src<proyecto>/Sonidos/` y `Memory/Interfaces/`, con mayuscula inicial. El arbol de
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

**Rutas de la escena.** Las mallas, texturas y fuentes de script que usa la
escena se persisten **relativas** a la carpeta `src<proyecto>/`. Por eso, al
renombrar un proyecto (menu de inicio) o mover su carpeta completa, las
escenas siguen cargando sin tocar nada: la raiz `src<nombre>` se desplaza
entera con el proyecto. Dentro del explorador, al **renombrar** o al **mover**
un archivo o carpeta el motor reescribe al instante las referencias de la
escena que apuntaban a esa ruta y guarda la escena modificada. Limitaciones:
una **copia** (arrastre con `Ctrl`) no se rastrea, porque no cambia ninguna de
las dos rutas; los cambios hechos **fuera** del motor (explorador de Windows,
una terminal, un `mv`) solo sirven para refrescar el arbol de archivos, no
reescriben nada, asi que ahi si hay que volver a arrastrar el asset en su
inspector; y los sonidos de `src<proyecto>/Sonidos/` e interfaces de `Memory/Interfaces/` se
referencian por **nombre**: un move con el mismo nombre conserva la referencia
y un rename la rompe (vuelve a seleccionar el clip/interfaz en su dropdown).
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
| `W` `A` `S` `D` | Mover la camara activa (diagonales normalizadas). Solo con las interfaces del editor ocultas (`E`) o con el clic derecho sostenido sobre el viewport. Funciona en edicion y tambien durante el play |
| `Espacio` / `Shift izq.` | Subir / bajar la camara (misma condicion que `WASD`) |
| Mouse / clic der. | Nav FPS; el clic derecho sostenido sobre el viewport navega **sin** esconder las interfaces (sensibilidad en Opciones) |
| `E` | Mostrar/ocultar interfaces del editor (en edicion y durante el play; no en el menu de inicio) |
| `F5` | Simular (Play): arranca la simulacion de la escena (fisica, scripts y audio) desde el editor |
| `F6` | Pausar/reanudar la simulacion (solo durante el play; congela fisica y scripts sin salir) |
| `F7` | Detener la simulacion y volver al modo edicion |
| `Ctrl+S` | Guardar el proyecto en caliente (escena + manifiesto + config) |
| `Ctrl+Z` | Deshacer ultima accion del editor (undo) |
| `Ctrl+Y` | Rehacer accion deshecha (redo) |
| `Escape` | Durante el play: detener la simulacion y volver al modo edicion (igual que `F7`). En edicion: volver al menu de inicio. En el menu: no hace nada |
| `1` / `T` | Gizmo: traslacion (apaga la guia de eje) |
| `2` / `R` | Gizmo: rotacion (apaga la guia de eje) |
| `3` / `U` | Gizmo: escala (la `Y` suelta la tomo la guia de eje; apaga la guia) |
| `X` / `Y` / `Z` | Guia de eje del objeto seleccionado (ver abajo) |
| `G` | Gizmo local / mundo (gizmo y guia de eje) |
| Clic en objeto | Seleccionar en viewport |

El modo Play/Stop tambien se controla con el boton **Activar/Detener** de la
barra de menu de la escena, que hace exactamente lo mismo que `F5` y `F7`; la
fisica y los scripts solo se ejecutan en Play. Clic en un objeto del arbol o del
viewport lo selecciona; el Inspector muestra sus componentes a la derecha.

---

## 4. Objetos y componentes

- **Crear objetos:** "New GameObject" (crea un objeto simple en la escena que posee únicamente el componente `Transform`).
- **Menú contextual en la jerarquía:** clic derecho sobre un objeto despliega cuatro
  opciones: **Cambiar ID**, **Renombrar**, **Desanudar a raíz** y **Eliminar**. "Desanudar
  a raíz" solo aparece cuando el objeto cuelga de un padre intermedio: un hijo directo de
  la raíz ya está al nivel superior, así que no hay nada que desenanidar. Clic derecho en
  espacio vacío del panel despliega "New GameObject".
- **Componentes:** `Transform`, `Color`, `Model`, `Material`, `Light`,
  `CameraComponent`, `Grid`, `Skybox`, colliders (esfera / cubo / malla),
  `RigidBody`, `AudioSource`, `InterfaceComponent` y `Script`.
- **Inspector:** boton "Agregar componente" abre el popup de componentes; cada
  uno tiene su panel propio (Transform, Luz con tipo/atenuacion/color, etc.).
- **Jerarquia:** arrastra un objeto sobre otro en el arbol para reparentar; el
  motor rechaza ciclos y la raiz. "Childs Freeze" congela la transformacion de
  los hijos durante la edicion del padre.
- **Gizmos** (ImGuizmo): traslacion/rotacion/escala con `1`/`2`/`3` o
  `T`/`R`/`U`, local/mundo con `G`; la fisica tiene su gizmo propio para el
  collider activo. El checkbox **"Gizmo activo"** del panel `Transform` apaga
  el gizmo de ese objeto (y el del collider, si el que se edita es el offset de
  un collider) sin sacarlo de la seleccion.

### Grilla del suelo

La escena trae un objeto llamado **"Grilla"** con el componente `Grid`: es el
piso del editor y se dibuja siempre que el componente este visible. Su panel en
el Inspector tiene solo dos controles:

- **Visible**: enciende o apaga la grilla.
- **Color**: el color de las lineas. El que trae de fabrica es un blanco hielo
  casi blanco (`0.88, 0.91, 0.89`).

No hay tamano ni separacion: la grilla es **infinita** y de densidad fija
(secundarias cada unidad, una linea principal cada cinco). El circulo en el que
se dibuja persigue a la camara y las lineas se **difuminan con la distancia**:
opacas hasta 40 unidades de la camara y desvanciendose por completo en el
horizonte, a 150 unidades, que es tambien el limite: mas alla no se dibuja
nada. Los tres ejes de la grilla (X rojo, Y verde, Z azul, los mismos colores
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

## 5. Undo / redo de operaciones del editor

Todas las operaciones del editor estan encapsuladas en comandos (patron
Command) que admiten undo y redo. `EditorController` posee un
`GestorComandos` que mantiene dos pilas (undo/redo) con un maximo de 50
entradas; cada nueva accion invalida la pila de redo.

| Accion | Comando que la envuelve |
|---|---|
| Crear objeto | `CrearObjetoComando` |
| Borrar objeto | `BorrarObjetoComando` |
| Reparentar (arrastrar en jerarquia) | `ReparentarComando` |
| Modificar transform (posicion/rotacion/escala) | `TransformComando` |
| Agregar componente | `AgregarComponenteComando` |
| Quitar componente | `QuitarComponenteComando` |
| Limpiar escena (borrar todos los objetos no-raiz) | `LimpiarEscenaComando` |

**Atributos de los comandos:** cada comando captura el estado antes de mutar
(posicion del padre, id del objeto, transform anterior, componentes) y lo
almacena por valor para poder restaurarlo exactamente. La raiz de la escena
(id=0) esta excluida de delete/clear por diseno.

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
| Dropdown **Sonido** de AudioSource | `AudioSource` | nombre del clip |
| Dropdown **Interfaz** de InterfaceComponent | `InterfaceComponent` | nombre del asset JSON |

Todos los receptores almacenan el valor en `std::string` o en buffers de al
menos 4096 bytes, de modo que paths largos no se truncan (el componente
`Model` era el unico con buffer de 100 y fue ampliado a 4096).

**Como usar:**

1. Copia tus assets a las carpetas del proyecto (o crealas con "New Folder" /
   "New File"); el FileSystemWatcher rescanea al detectar cambios externos.
2. Selecciona el objeto y arrastra el asset sobre el control correspondiente
   del panel del componente ("Arrastra modelo", campo Fuente del script,
   dropdown de Sonido, dropdown de Interfaz).
3. La escena guarda el path o nombre; al recargar se resuelve de nuevo.

---

## 7. Física

- Colliders de esfera, cubo o malla; con `RigidBody` participan de la
  simulacion Bullet. Solo simula en modo **Play**: en edicion el gizmo mueve
  el objeto y el motor sincroniza collider/cuerpo/objeto con la matriz global
  compuesta del dueño, para que mover un collider no desincronice el cuerpo.
- Mientras se manipula el gizmo, `stepSimulation` se pausa (la gravedad podria
  "eyectar" el objeto); al soltar, la simulacion sigue.
- Gizmo dedicado de fisica para el collider activo.

---

## 8. Audio

- Crea la carpeta `Sonidos/` (si no existe) y coloca los clips en `Sonidos/` (wav/mp3/etc.). `AudioClipsManager` los
  descubre y los registra **por nombre** en el `AudioEngine` al escanear.
- Agrega `AudioSource` a un objeto; en su panel elige el clip del dropdown
  (o arrastralo desde `Sonidos/` tras crearla), ajusta volumen, loop y "reproduccion
  automatica". En Play, el AudioEngine reproduce en su hilo de audio.
- Cambiar de proyecto re-escanea y limpia el registro de clips.

---

## 9. Interfaces de juego (HUD)

- Crea el asset de interfaz con el **CreadorDeInterfaces** (genera un JSON en
  `Memory/Interfaces/<nombre>.json`).
- Agrega `InterfaceComponent` a un GameObject; su inspector muestra el nombre
  del asset (dropdown + drop desde `Memory/Interfaces/`).
- Al entrar en **Play**, la escena muestra esa interfaz a pantalla completa
  delante de la camara principal (HUD del juego).

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
modal para configurar la exportacion:

| Campo | Descripcion |
|---|---|
| **Nombre del ejecutable** | Nombre del binario final (sin extension). |
| **Nombre del proyecto exportado** | Nombre de la carpeta bajo `MotorGrafico/Exportaciones/`. |
| **Plataforma objetivo** | Linux (nativo) o Windows (cross-compile MinGW). |

Al pulsar **Exportar**, el motor:

1. Genera un proyecto CMake temporal que compila el **engine runtime-only**
   (`funshi_runtime`: sin ImGui, editor, Assimp; solo GLFW, OpenGL, Bullet,
   miniaudio, nlohmann/json, GLM).
2. Recompila los scripts de usuario (BackendCpp) en el build de exportacion.
3. Compila el ejecutable del juego linkando contra `funshi_runtime`.
4. Empaqueta en `MotorGrafico/Exportaciones/<nombre>/`:
   - Ejecutable (`<nombre>.exe` en Windows, `<nombre>` en Linux).
   - Carpeta `Data/` con `Memory/`, `Sonidos/` (si hay audio), `ConfiguracionProyecto.json`.
   - Carpeta `lib/` con dependencias bundleadas (`.dll` / `.so`).

El dialogo muestra un **spinner indeterminado** (barra de progreso falsa) mientras
se ejecuta la compilacion; el proceso no bloquea el editor.

**Requisitos para cross-compile Windows:** toolchain MinGW instalado
(`x86_64-w64-mingw32-g++`, `x86_64-w64-mingw32-gcc`, `windres`).

**Lanzar el juego exportado:**
```bash
# Linux
./MotorGrafico/Exportaciones/MiJuego/MiJuego

# Windows
MotorGrafico\Exportaciones\MiJuego\MiJuego.exe
```

> El flag `--proyecto` del binario del editor sigue disponible para desarrollo
> (salta el menu y abre el proyecto en modo editor). El exportado standalone
> no requiere el editor ni dependencias de desarrollo.

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
  (`MotorGrafico/Configuraciones/Configuracion.json` —junto al binario si el
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
ejecuta en modo Play.

- Se crean desde el explorador: clic derecho sobre la carpeta actual > "New
  Script" y, en el dialogo que se abre, elegir **C++ (`.cpp`)** o **Java
  (`.java`)** (este ultimo disponible si el motor se compilo con soporte JNI).
- **El nombre del archivo debe ser `<ClassName>.cpp`** (la clase == nombre del
  archivo). El backend compila la clase como `FUNSHI_<ClassName>` mediante
  `-DFUNSHI_NOMBRE_CLASE=<ClassName>`.
- Ciclo de vida en C++ (`IScriptBehaviour`): `onStart(owner)` al entrar en
  Play, `onUpdate(owner, deltaTime)` cada frame en Play, y `onStop(owner)`
  opcional al salir de Play.
- **SerializeField:** los campos declarados con macros `REFLECT_*` aparecen
  editables en el inspector, se guardan con la escena y sobreviven al hot
  reload (se reinyectan por nombre de campo).
- Hot reload por mtime del fuente: en Play, guardar el `.cpp`/`.java`
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
| `REFLECT_CAMPO(objetivo)` | `GameObject*` | referencia por nombre del objeto |
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
- Las referencias `GameObject*` se serializan por el **nombre** del objeto (no
  por puntero), y sobreviven al hot reload.
- El editor muestra cada campo con su widget (InputFloat, Checkbox, InputText,
  ColorEdit para `vec3` cuando aplica, dropdown de objetos para referencias).
- Si el inspector muestra "Sin campos SerializeField", verificá que el bloque
  `REFLECT_*` este dentro de la clase y que `camposReflejados()` devuelva
  `reflexion()`.

### 13.3 Acceso al GameObject: tabla `api`

El motor inyecta en cada instancia la tabla `MotorScript::ApiScriptGameObject`
(via `conectarApi`, que el backend llama al crear el comportamiento). Los
scripts la usan como `this->api->...` y **deben comprobar `if (api)`** antes
de usarla.

**Versionado APPEND-ONLY:** los miembros nuevos se agregan siempre al final de
la struct, sin reordenar ni cambiar tipos, de modo que un `.so` compilado
contra una version anterior siga leyendo los miembros viejos en la misma
direccion. El campo `version` (al final) permite guardas:
`if (api->version >= 2) { float y = api->rotacionEjeY(owner); }`.

| Funcion | Version | Firma | Descripcion |
|---|---|---|---|
| `api->nombre(owner)` | 1 | `const char* (const void*)` | nombre del objeto |
| `api->posicionX/Y/Z(owner)` | 1 | `float (const void*)` | posicion mundo por eje |
| `api->fijarPosicion(owner,x,y,z)` | 1 | `void (void*, float, float, float)` | fija la posicion |
| `api->fijarEscala(owner,x,y,z)` | 1 | `void (void*, float, float, float)` | fija la escala |
| `api->fijarRotacionEjes(owner,ang,x,y,z)` | 1 | `void (void*, float, float, float, float)` | rota `ang` rad sobre el eje `(x,y,z)` |
| `api->imprimirConsola("texto")` | 1 | `void (const char*)` | log a la consola del editor |
| `api->rotacionAngulo(owner)` | 2 | `float (const void*)` | angulo de rotacion (radianes) |
| `api->rotacionEjeX/Y/Z(owner)` | 2 | `float (const void*)` | eje de rotacion por componente |
| `api->escalaX/Y/Z(owner)` | 2 | `float (const void*)` | escala por eje |

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

### 13.4 Servicios de escena: tabla `servicios` (v1)

Ademas de `api`, `IScriptBehaviour` expone `this->servicios`: acceso a los
servicios del motor que **no son del objeto** sino de la escena (audio,
busqueda de objetos y teclado). GameScene la inyecta al entrar en Play, antes
del primer `onStart`, y la desconecta al salir (fuera de Play las funciones
son no-ops tolerantes: devuelven `false`/`nullptr`/`-1`, sin bloquear).
Misma convencion APPEND-ONLY con `servicios->version` al final.

| Funcion | Firma | Descripcion |
|---|---|---|
| `servicios->reproducirSonido(clip, vol, loop)` | `int (const char*, float, bool)` | reproduce un clip de `Sonidos/` (la carpeta debe existir) por **nombre**; devuelve handle >= 0, o -1 si el clip no existe |
| `servicios->detenerSonido(handle)` | `void (int)` | detiene la reproduccion del handle |
| `servicios->objetoPorNombre("Enemigo")` | `void* (const char*)` | busca un GameObject por nombre en la escena; `nullptr` si no existe. El puntero vale mientras el objeto viva (todavia no se crean/destruyen objetos desde scripts) |
| `servicios->teclaSostiene("W")` | `bool (const char*)` | tecla mantenida apretada |
| `servicios->teclaPresionada("SPACE")` | `bool (const char*)` | tecla apretada este frame (edge press) |
| `servicios->teclaSoltada("F")` | `bool (const char*)` | tecla soltada este frame (edge release) |

Las teclas usan nombres de tecla GLFW sin el prefijo `GLFW_KEY_`:
`"A"`..`"Z"`, `"0"`..`"9"`, `"F1"`..`"F12"`, `"SPACE"`, `"ENTER"`, `"TAB"`,
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
        servicios->reproducirSonido("disparo.wav", 0.8f, false);

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
> cabeceras. En Java, la tabla `servicios` todavia no esta expuesta por el
> bridge JNI (solo C++); la version Java recibe `api` con `version >= 2`.

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
  `FUNSHI_CXX_RUNTIME_FLAG`; el cache de artefactos incluye ese contrato, asi
  que cambiar los flags recompila solo (no hay que borrar
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

### 14.1 Plantilla generada por el editor

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

### 14.2 Diferencias con C++

- El ciclo es `iniciar` / `actualizar` / `detener` y reciben el objeto como
  `long` (handle nativo), no como `GameObject*`.
- Los campos `public` del comportamiento son SerializeField automaticos; no se
  usan macros.
- Cada carga usa un classloader nuevo (child-first: delega al padre solo
  `Nativo`, `Comportamiento` y clases del JDK), lo que permite hot reload sin
  reiniciar la JVM.
- Utiles para prototipado rapido; para rendimiento, preferi C++.

---

## 16. Hot reload y depuración

- **C++:** guardar el `.cpp` en Play recompila; el editor compara el mtime del
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
- **Errores de carga/compilacion C++** aparecen en la ventana Estado y en el log
  del motor (`logs/FunshiEngineGL_*.log` en la carpeta de datos, seccion 2), no
  en el panel del componente; corregi el fuente y guardalo de nuevo (no hace
  falta salir de Play).
- Al cerrar la aplicacion los comportamientos se descargan sin disparar
  `onStop`; los backends (incluida la JVM) se apagan despues.

---
