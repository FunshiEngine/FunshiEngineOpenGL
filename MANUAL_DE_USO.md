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
   `E` oculta la UI; `Escape` vuelve al menu.

---

## 2. Proyectos y estructura de carpetas

Al crear un proyecto, el motor genera la estructura bajo
`{app}/MotorGrafico/Proyects/<proyecto>/`:

```
MotorGrafico/
├── Proyects/
│   └── <proyecto>/
│       ├── Memory/
│       │   ├── Binarios/Scene        ← escenas binarias
│       │   ├── Interfaces/           ← assets JSON del CreadorDeInterfaces
│       │   ├── ConfiguracionProyecto.json
│       │   └── imgui.ini
│       └── src<proyecto>/            ← assets del proyecto (raiz del explorador)
│           ├── modelos/              ← .obj/.fbx que arrastra el editor
│           ├── Sonidos/              ← clips de audio (.wav/.mp3/...)
│           └── Scripts/              ← scripts del usuario (.cpp/.java)
├── Configuraciones/
│   └── Configuracion.json            ← configuracion global (tema, idioma, sensibilidad)
└── Exportaciones/
    └── <nombreExportacion>/          ← juegos exportados (ver seccion 10.1)
        ├── <Juego>.exe / <Juego>     ← ejecutable standalone
        ├── Data/
        │   ├── Memory/
        │   ├── Sonidos/
        │   └── ConfiguracionProyecto.json
        └── lib/                      ← dependencias runtime (Bullet, miniaudio, GLFW, etc.)
```

La convencion de assets por nombre usa carpetas `Sonidos/` e `Interfaces/` con
mayuscula inicial. El arbol de archivos del editor lista la **raiz del
proyecto** (`src<nombre>`).

> **Nota de packaging:** en el instalador Windows el explorador apunta a la
> raiz del proyecto; en builds de desarrollo antiguas (Linux) podia apuntar a
> la de scripts. Si un dropdown o drag & drop aparece vacio, verifica que raiz
> lista tu arbol.

**Rutas de la escena.** Las mallas, texturas y fuentes de script que usa la
escena se persisten **relativas** a la carpeta `src<proyecto>/`. Por eso, al
renombrar un proyecto (menu de inicio) o mover su carpeta completa, las
escenas siguen cargando sin tocar nada: la raiz `src<nombre>` se desplaza
entera con el proyecto. Dentro del explorador, al **renombrar** un archivo o
carpeta el motor reescribe al instante las referencias de la escena que
apuntaban a esa ruta y guarda la escena modificada. Limitaciones: mover un
asset *por copia* (arrastre con copia) no se rastrea, y los sonidos de
`Sonidos/` e interfaces de `Interfaces/` se referencian por **nombre**: un
move con el mismo nombre conserva la referencia y un rename la rompe (volve a
seleccionar el clip/interfaz en su dropdown).

**Manifiesto de assets (`SceneAssets.json`).** Junto a los binarios de la
escena se mantiene `Memory/Binarios/SceneAssets.json`, un add-on legible que
centraliza las rutas de asset de cada objeto (malla, las 4 texturas del
material y la dll de script). No reemplaza al `.db`: se regenera en **cada**
guardado y, al cargar, sus rutas tienen **precedencia** sobre las del `.db`
(las escenas viejas, sin manifiesto, se cargan igual contra el `.db`). Al
renombrar/mover assets el guardado automatico lo mantiene al dia.

**Guardado sin salir (Ctrl+S).** El editor guarda el proyecto completo
(escena + manifiesto + configuracion de ventanas/gizmo/camara) con
`Ctrl+S`, ademas del guardado automatico al salir. En un campo de texto de
ImGui la combinacion la consume el editor de texto y no guarda.

---

## 3. Recorrido del editor

| Tecla / accion | Funcion |
|---|---|
| `W` `A` `S` `D` | Mover la camara activa (diagonales normalizadas). Solo con las interfaces del editor ocultas (`E`) o con el clic derecho sostenido sobre el viewport |
| `Espacio` / `Shift izq.` | Subir / bajar la camara (misma condicion que `WASD`) |
| Mouse / clic der. | Nav FPS; el clic derecho sostenido sobre el viewport navega **sin** esconder las interfaces (sensibilidad en Opciones) |
| `E` | Mostrar/ocultar interfaces del editor (solo funciona dentro del editor, no en el menu de inicio) |
| `F5` | Simular (Play): arranca la simulacion de la escena (fisica, scripts y audio) desde el editor |
| `F6` | Pausar/reanudar la simulacion (solo durante el play; congela fisica y scripts sin salir) |
| `F7` | Detener la simulacion y volver al modo edicion |
| `Ctrl+S` | Guardar el proyecto en caliente (escena + manifiesto + config) |
| `Ctrl+Z` | Deshacer ultima accion del editor (undo) |
| `Ctrl+Y` | Rehacer accion deshecha (redo) |
| `Escape` | Volver al menu de inicio |
| `1` / `T` | Gizmo: traslacion (apaga la guia de eje) |
| `2` / `R` | Gizmo: rotacion (apaga la guia de eje) |
| `3` / `U` | Gizmo: escala (la `Y` suelta la tomo la guia de eje; apaga la guia) |
| `X` / `Y` / `Z` | Guia de eje del objeto seleccionado (ver abajo) |
| `G` | Gizmo local / mundo (gizmo y guia de eje) |
| Clic en objeto | Seleccionar en viewport |

El modo Play/Stop se controla desde la barra de menu de la escena; la fisica y
los scripts solo se ejecutan en Play. Clic en un objeto del arbol o del
viewport lo selecciona; el Inspector muestra sus componentes a la derecha.

---

## 4. Objetos y componentes

- **Crear objetos:** "New GameObject" (crea un objeto simple en la escena que posee únicamente el componente `Transform`).
- **Menú contextual en la jerarquía:** clic derecho sobre un objeto despliega "Renombrar" y "Eliminar"; clic derecho en espacio vacío del panel despliega "New GameObject".
- **Componentes:** `Transform`, `Color`, `Model`, `Material`, `Light`,
  `CameraComponent`, colliders (esfera / cubo / malla), `RigidBody`,
  `AudioSource`, `InterfaceComponent` y `Script`.
- **Inspector:** boton "Agregar componente" abre el popup de componentes; cada
  uno tiene su panel propio (Transform, Luz con tipo/atenuacion/color, etc.).
- **Jerarquia:** arrastra un objeto sobre otro en el arbol para reparentar; el
  motor rechaza ciclos y la raiz. "Childs Freeze" congela la transformacion de
  los hijos durante la edicion del padre.
- **Gizmos** (ImGuizmo): traslacion/rotacion/escala con `1`/`2`/`3` o
  `T`/`R`/`U`, local/mundo con `G`; la fisica tiene su gizmo propio para el
  collider activo.

### Guia de eje (`X` / `Y` / `Z`)

Con un objeto seleccionado, `X`, `Y` o `Z` dibujan la recta sobre la que se
puede mover ese objeto: la tecla pulsada es el eje que varia y **las otras dos
coordenadas quedan fijadas** a las del objeto.

Por ejemplo, un objeto en `(3, 2, -5)` y se pulsa `X`: la recta es `(t, 2, -5)`
para todo `t`. Es decir, el objeto solo se desplaza en X y su altura (Y) y su
profundidad (Z) no se mueven. Con `Y` el efecto es el inverso: quedan fijos X y Z.

- Cada eje tiene su color (X rojo, Y verde, Z azul) y la recta llega hasta el
  **horizonte**, difuminandose con el mismo criterio que la grilla: opaca cerca de
  la camara y desvanciendose en el mismo punto donde el piso se acaba. Asi la
  guia se lee como un eje que atraviesa la escena entera, no como un palo corto
  pegado al objeto.
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

- Coloca los clips en `Sonidos/` (wav/mp3/etc.). `AudioClipsManager` los
  descubre y los registra **por nombre** en el `AudioEngine` al escanear.
- Agrega `AudioSource` a un objeto; en su panel elige el clip del dropdown
  (o arrastralo desde `Sonidos/`), ajusta volumen, loop y "reproduccion
  automatica". En Play, el AudioEngine reproduce en su hilo de audio.
- Cambiar de proyecto re-escanea y limpia el registro de clips.

---

## 9. Interfaces de juego (HUD)

- Crea el asset de interfaz con el **CreadorDeInterfaces** (genera un JSON en
  `Interfaces/<nombre>.json`).
- Agrega `InterfaceComponent` a un GameObject; su inspector muestra el nombre
  del asset (dropdown + drop desde `Interfaces/`).
- Al entrar en **Play**, la escena muestra esa interfaz a pantalla completa
  delante de la camara principal (HUD del juego).

---

## 10. Cámaras

- `CameraComponent` con **vistas previas en vivo** (render a FBO); detalle
  completo en [CAMARAS_VISTAS_PREVIAS.md](CAMARAS_VISTAS_PREVIAS.md).
- "Usar" en el panel de la camara la marca como activa; el id se persiste en
  `Configuracion.json` (default automatica si el id ya no existe al cargar).

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
   - Carpeta `Data/` con `Memory/`, `Sonidos/`, `ConfiguracionProyecto.json`.
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

- Tema claro/oscuro, modo blanco y negro (desatura la interfaz completa y
  acompaña fondo y grilla del viewport), color de acento (solo RGB: la
  transparencia de cada elemento la define el tema, no el color elegido) y color
  de fondo de la escena, aplicados en vivo por `TemaEditor` / `AparienciaUtil`.
  El acento alcanza **todos** los roles de la interfaz (botones, solapas del
  dock, campos de entrada, sliders, checkboxes, enlaces, bordes, separadores y
  tablas) y los grises azulados de fábrica quedan en gris neutro, así que al
  cambiar de color no quedan restos del azul clásico ni hace falta reiniciar el
  editor.
- Sensibilidad de camara, ventana de camaras, visibilidad de ventanas y la
  apariencia se guardan junto al binario en
  `<directorioEjecutable>/MotorGrafico/Configuraciones/Configuracion.json`
  (la configuración por proyecto vive en
  `Proyects/<proyecto>/Memory/ConfiguracionProyecto.json`). La escritura es
  **atómica** (archivo temporal + rename: un corte no deja el JSON cortado) y
  la configuración general se guarda de forma **diferida**: mientras cambiás
  opciones en vivo se escribe como máximo una vez cada 250 ms, y siempre al
  salir o con Ctrl+S. Tolera archivos ausentes o corruptos.
- Idioma del editor: Espanol / English desde Opciones.

---

## 13. Scripts: conceptos y ciclo de vida

Los comportamientos del juego se escriben como **scripts dinamicos**: archivos
`.cpp` o `.java` dentro del proyecto que el editor compila en caliente y
ejecuta en modo Play.

- Se crean desde el explorador: "New Script" (C++) o "New Script Java"
  (disponible si el motor se compilo con soporte JNI).
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
  log del motor (`logs/FunshiEngineGL_*.log`); el panel del componente no los
  repite: queda con el fuente asignado y sus SerializeField.

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
extern "C" IScriptBehaviour* FUNSHI_CREAR_COMPORTAMIENTO(
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
| `servicios->reproducirSonido(clip, vol, loop)` | `int (const char*, float, bool)` | reproduce un clip de `Sonidos/` por **nombre**; devuelve handle >= 0, o -1 si el clip no existe |
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

- El fuente se compila a `.so` (Linux), `.dll` (Windows/MSVC) o `.dylib`
  (macOS) con `-std=c++17 -shared -fPIC -O2` y
  `-DFUNSHI_NOMBRE_CLASE=<ClassName>`.
- El `.so` del script **no enlaza contra el motor**: todo el acceso pasa por
  la tabla `api` de punteros a funcion. No incluyas cabeceras del motor mas
  alla de `IScriptBehaviour.h`; en particular evita arrastrar Bullet/Assimp
  (opcional: usa `<cmath>`, `<string>`, `<vector>` y demas STL).
- La clase compilada se llama `FUNSHI_<ClassName>`: el mismo template
  (`FUNSHI_NOMBRE_CLASE`) compila para cualquier `<ClassName>.cpp`.
- El cache de artefactos compilados y la ruta del compilador se muestran en
  la ventana Estado.
- **Windows:** el motor invoca `cl.exe` a traves de `vcvars64.bat` del mismo
  toolset MSVC (se busca subiendo desde la carpeta del compilador), porque
  `cl.exe` resuelve los headers del CRT (incluido `<cstddef>`) y las librerias
  por `INCLUDE`/`LIB`. Con esto el editor funciona igual si se lanza desde el
  Explorador o desde Visual Studio. Si el compilador configurado no es MSVC
  (`FUNSHI_CXX` a MinGW/g++, por ejemplo), hace falta un entorno con `cl.exe`
  disponible. Los scripts Java no tienen este requisito (javac se invoca por
  ruta absoluta).

---

## 15. Scripting Java (JNI)

Requiere que el motor se haya compilado con el JDK disponible
(`FUNSHI_JAVA=ON`); la ventana Estado muestra `javac`, `libjvm` y si el
soporte esta activo.

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
- **Java:** igual, con el classloader child-first; la JVM se reutiliza.
- **Ventana Estado:** para cada script muestra nombre, ok/error y mensaje
  (errores de compilacion incluidos), ademas del toolchain detectado.
- **Errores de carga/compilacion C++** aparecen en la ventana Estado y en el log
  del motor (`logs/FunshiEngineGL_*.log` junto al ejecutable), no en el panel del
  componente; corregi el fuente y guardalo de nuevo (no hace falta salir de
  Play).
- Al cerrar la aplicacion los comportamientos se descargan sin disparar
  `onStop`; los backends (incluida la JVM) se apagan despues.

---
