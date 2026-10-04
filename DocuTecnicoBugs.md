# DocuTecnicoBugs — Registro Técnico de Bugs por Limitaciones de Herramientas

> **Propósito**: Documentar bugs cuyo origen no es un error lógico del código, sino una **limitación, comportamiento no obvio o latencia de una herramienta externa** (filesystem, compilador, API gráfica, dependencia, SO, etc.). Sirve para que agentes (y humanos) reconozcan el patrón, apliquen la mitigación conocida y eviten re-investigar lo ya resuelto.

---

## 1. Primer concepto: **Invalidación Diferida de Cache por Latencia de `mtime` en Filesystem**

### 1.1 Descripción del Problema
Al borrar un archivo o carpeta mediante `std::filesystem::remove/remove_all`, el **`mtime` (modification time) del directorio padre no se actualiza de forma inmediata** en ciertos filesystems / SO / configuraciones. Esto rompe la invalidación de caches que dependen de comparar `mtime` para decidir si re-leer el directorio.

### 1.2 Síntomas
- La UI **no se refresca tras la primera eliminación** (el elemento "fantasma" sigue visible).
- El elemento desaparece **al cambiar de ventana**, **al crear otro archivo**, o **al forzar un rescaneo manual** (acciones que actualizan el `mtime` o invalidan el cache por otra vía).
- No hay error en consola; la operación de borrado **sí tuvo éxito en disco**.

### 1.3 Causa Raíz
```cpp
// Lógica de cache típica (vulnerable):
const auto mtime = FileManager::mtimeDirectorio(path);
if (path != cacheCarpeta || mtime != cacheMtime) {
    // Re-lee directorio
} else {
    // Usa cache ANTIGUO → muestra entrada borrada
}
```
`std::filesystem::last_write_time(dir)` puede devolver el valor **previo a la eliminación** durante un window de tiempo indeterminado (ms a segundos según FS/SO).

### 1.4 Solución Canónica (Patrón R7 — Invalidación Explícita)
**Forzar la invalidación del cache inmediatamente tras la operación mutante**, antes de volver a leer:

```cpp
// En contentGUI() / equivalente, ANTES de recorrer()/re-leer:
if (!archivoAEliminarConfirmado.empty()) {
    fileManager->eliminarArchivo(archivoAEliminarConfirmado);
    // >>> INVALIDACIÓN EXPLÍCITA <<<
    cacheCarpeta.clear();
    cacheMtime = std::filesystem::file_time_type{}; // epoch = "nunca visto"
    archivoAEliminarConfirmado.clear();
}
if (!carpetaAEliminarGridConfirmada.empty()) {
    if (fileManager->eliminarCarpeta(carpetaAEliminarGridConfirmada)) {
        sel->contadorCambios++; // rescanea árbol
    }
    // >>> INVALIDACIÓN EXPLÍCITA <<<
    cacheCarpeta.clear();
    cacheMtime = std::filesystem::file_time_type{};
    carpetaAEliminarGridConfirmada.clear();
}

recorrer(destFolder); // Ahora SÍ re-lee porque cache inválido
```

**Clave**: la invalidación es **síncrona, determinista y no depende del FS**.

---

## 2. Registro de Instancias Conocidas del primer concepto

| # | Ubicación | Operación | Herramienta/FS | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `ContentFolderInterface::contentGUI()` | Eliminar archivo (grid) | `std::filesystem` / NTFS / MSYS2 | Invalidación manual `cacheCarpeta.clear()` + `cacheMtime = {}` | `feat(gui): eliminar archivos y carpetas desde el grid del explorador` |
| 2 | `ContentFolderInterface::contentGUI()` | Eliminar carpeta (grid) | Idem | Idem + `contadorCambios++` para árbol | Idem |
| 3 | `TreeFilesInterface::contentGUI()` | Eliminar carpeta (árbol) | Idem | Ya usaba patrón R7: `carpetaAEliminar` diferida + rescaneo vía `contadorCambios` | Preexistente |
| 4 | `SoltarEnCarpeta.h` + `ContentFolderInterface` | **Drop entre paneles** (grid → árbol / grid → grid carpeta distinta) | `std::filesystem` / NTFS / MSYS2 | `contadorCambios++` siempre (archivo o carpeta) + invalidación explícita cache grid origen **y** destino (`cacheCarpeta.clear(); cacheMtime = {}`) | `fix(explorador): invalidar cache grid al mover entre paneles` |

> **Nota**: `TreeFilesInterface` **no tenía este bug** porque su patrón R7 ya forzaba `contadorCambios++` → `refrescarArbol()` → reconstrucción completa del árbol (que no usa `mtime` de directorio). El bug apareció al replicar la lógica en `ContentFolderInterface` **sin portar la invalidación explícita del cache de grid**.

---

## 3. Guía para Agentes: Cómo Documentar un Nuevo Bug de Esta Clase

### 3.1 Cuándo Aplica Esta Plantilla
Úsala cuando el bug cumple **TODAS** estas condiciones:
- [ ] La operación **sí funciona** (archivo borrado, compile OK, draw call enviado, etc.).
- [ ] El fallo es **visible/latente**: la UI, test o log muestra estado viejo.
- [ ] El estado se corrige **solo** al forzar un refresh externo (cambio de foco, resize, nueva operación, timer).
- [ ] La causa es **latencia o eventualidad de una API externa** (FS, GPU driver, compilador, JNI, etc.), no lógica propia.

### 3.2 Formato de Entrada (Agregar al final de la tabla en §2)

```markdown
| N | <Archivo::Método> | <Operación> | <Herramienta/FS/SO> | <Fix> | <Commit/PR> |
```

### 3.3 Checklist de Mitigación (Aplicar Antes de Cerrar)
- [ ] **Identificar el cache/estado intermedio** que retiene dato viejo.
- [ ] **Invalidar explícitamente** ese cache **inmediatamente tras la operación mutante**, en el mismo frame / contexto de ejecución.
- [ ] **No confiar en `mtime`, timestamps, callbacks, events o polling** de la herramienta externa para la invalidación crítica.
- [ ] **Testear**: operación → verificación visual/assert en mismo frame → sin acción extra del usuario.
- [ ] **Documentar** en este archivo (§2) con patrón, ubicación y fix.

---

## 4. Patrones Relacionados (Para Referencia Cruzada)

| Patrón | Descripción | Dónde Vive |
|--------|-------------|------------|
| **R7 — Eliminación Diferida con Confirmación** | Encola ruta → modal confirma → borra fuera del recorrido / en siguiente fase segura | `TreeFilesInterface`, `ContentFolderInterface` |
| **Cache por `mtime` de Directorio (R5)** | Re-lee solo si ruta o `mtime` cambiaron; vulnerable a latencia FS | `ContentFolderInterface::recorrer()` |
| **Invalidación Explícita Post-Mutación** | `cache.clear(); timestamp = {};` tras `remove/remove_all/write` | **Este documento** |
| **Contador de Cambios (`contadorCambios`)** | Señal simple para forzar rescaneo de estructuras complejas (árboles) | `FileSelection`, `TreeFilesInterface` |
| **R8 — Resolver la Ruta y Probar la Escritura** | No asumir que el destino es escribible: probar creando y borrando un archivo; si falla, usar la carpeta de datos del usuario y avisar | `ProjectPaths::directorioBase()` (ver §6.4) |
| **R9 — Texto estructurado con seek en binario** | Si el lector hace `seekg(tellg())` para releer (look-ahead/peek), abrir en **binario** y normalizar las líneas (recortar `\r`): en modo texto el CRT de Windows traduce CRLF y los offsets dejan de ser bytes exactos | `SceneSerializer::load()` + `getlineLimpio()` (ver §7.4) |
| **R10 — La bandera de encoding viaja junto al dato** | Si una API acepta un bloque/cadena cuyo encoding depende de una bandera (`CREATE_UNICODE_ENVIRONMENT`), la bandera se decide **dentro** de la función que recibe el dato, nunca en el llamador | `Proceso::lanzar()` (ver §8.4) |
| **R11 — La frescura del artefacto manda, no el estado del que pide** | Antes de reescribir una salida, decidir por los tiempos de **archivo** (existe y no es más viejo que su fuente); una `.dll` cargada en el proceso no se puede reescribir en Windows | `BackendCpp::compilarYCargar()` (ver §9.4) |
| **R12 — El que dibuja posee su VAO** | En un contexto core todo `glDraw*` necesita un VAO ligado, incluso si la pasada no lee atributos (`gl_VertexID`); el VAO lo crea y lo liga el backend, no el llamador | `OpenGL3Backend::drawFullscreenTriangle()` (ver §10.4) |
| **R13 — Toda carpeta del motor en la raíz de datos es un nombre reservado** | Una migración de compatibilidad que mueve "lo que haya en la raíz" se lleva también las carpetas del motor; hay que declararlas reservadas al crear la carpeta, no después | `ProjectPaths::esReservado()` (ver §11.4) |

---

## 5. Principio Rector

> **"No esperes a que la herramienta te avise; invalida tú el estado que controlas."**

Cuando tu código **escribe** en un recurso externo (FS, GPU, proceso hijo, red), **tú** eres la fuente de verdad de cuándo ese recurso cambió. Invalida tus caches **en el mismo punto de escritura**, no en el siguiente frame, no en un callback, no en un timer.

El segundo concepto (§6) obedece al mismo espíritu desde el otro lado: **no le
asumas nada al recurso externo**. Si la escritura puede fallar sin avisar,
comprobala; si la ruta puede no ser escribible, pruébala antes de construir
todo el trabajo sobre ella.

---

## 6. Segundo concepto: **Escritura rechazada en el directorio de instalación**

Distinto del concepto de §1: aquí la operación **no** funciona, y el motivo es
que el proceso no tiene permiso de escritura sobre la ruta de destino. Se
documenta aparte porque la causa no es latencia sino el modelo de permisos de
Windows combinado con un instalador que corre elevado y un ejecutable que no.

### 6.1 Descripción del problema

El motor guardaba **todos** sus datos de usuario (proyectos, escenas,
configuraciones, `imgui.ini`) en `<directorioEjecutable>/MotorGrafico`. Con el
instalador de Windows eso resuelve a `C:\Program Files\FunshiEngineGL\MotorGrafico`,
donde el proceso **no puede escribir**.

### 6.2 Síntomas

- Abrir, navegar y leer el motor funciona con normalidad (lecturas no fallan).
- Crear un proyecto, guardar la configuración o guardar la escena **no producen
  ningún error visible**: el estado en memoria cambia y al reabrir no quedó nada.
- El síntoma visible llega más tarde y por otra vía: al abrir el inspector de un
  script, el motor terminaba con `std::bad_variant_access` (`std::get: wrong
  index for variant`). La causa era la divergencia entre lo que el motor **creía
  haber escrito** y lo que realmente había en disco, que descuadraba el árbol de
  `SerializeField`.
- El build de desarrollo, en una carpeta de usuario, **no reproducía nada**,
  porque ahí sí se puede escribir.

### 6.3 Causa raíz

Son tres hechos que por separado parecen inocuos y juntos cierran la puerta:

| # | Hecho | Consecuencia |
|---|-------|--------------|
| 1 | `PrivilegesRequired=admin` en el `.iss` | El instalador corre como administrador. |
| 2 | El `.iss` crea `{app}\MotorGrafico` **sin directiva `Permissions:`** | La carpeta hereda los ACL de `Program Files`: `Users` tiene lectura y ejecución, **no** escritura. |
| 3 | El `.exe` no lleva manifiesto `requestedExecutionLevel` | El motor corre como usuario normal, sin elevar, y por tanto sin esos permisos. |

El agravante: **ningún llamador comprobaba el retorno de las escrituras**
(`crearArchivo`, `guardarGeneral`, `saveScene`, … devolvían `bool` y se
ignoraban). Por eso el fallo fue silencioso en vez de un error reportado.

### 6.4 Solución canónica (Patrón R8 — Resolver la ruta, probar la escritura)

**No asumir que el directorio de destino es escribible: probarlo, y tener un
plan B.** La comprobación tiene que ser de escritura real, no de existencia ni
de legibilidad, porque en `Program Files` la carpeta existe y es legible y aun
así no se puede crear nada dentro.

```cpp
// NO alcanza: la carpeta existe y es legible, pero escribir falla.
if (fs::exists(dir)) return dir;

// Sí alcanza: se abre y se cierra un archivo; se borra acto seguido.
std::error_code ec;
fs::create_directories(fs::path(dir), ec);
if (ec) return fallback;
const fs::path prueba = fs::path(dir) / ".funshi_prueba_escritura";
{
    std::ofstream salida(prueba, std::ios::binary | std::ios::trunc);
    if (!salida.is_open()) return fallback;
}
fs::remove(prueba, ec);
return dir;
```

Reglas complementarias:

- **Decidir una vez por proceso**, no por llamada: la prueba toca el disco y la
  respuesta es la misma durante toda la vida del proceso.
- **Cachear la decisión** con un `static` de ámbito de función.
- **Exponer si se activó el plan B** (`datosEnRutaDeUsuario()`) para poder
  informarlo por consola o por la barra de estado, en vez de dejar que el
  usuario descubra el cambio de ubicación a ciegas.
- **Migrar sin pisar**: si hay datos en la ruta anterior, copiarlos **solo si el
  destino está vacío**, nunca encima de lo que ya hay.
- **Comprobar los retornos de las escrituras** cuando la ruta destino no es
  fiable. La regla R7 sigue valiendo, pero no cubre el caso de "escribí y no
  pasó nada".

### 6.5 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `ProjectPaths::directorioBase()` | Guardar proyecto, escena, config e `imgui.ini` | Windows / `Program Files` / ACL del instalador | Patrón R8: prueba de escritura + caída a `%APPDATA%` / `$XDG_DATA_HOME`, decisión cacheada, migración sin sobrescribir | `fix(configuracion): resolver la raiz de datos cuando no se puede escribir` |
| 2 | `SettingsScript.cpp` (lectura de `SerializeField`) | Editar un campo del inspector | `std::variant` (efecto, no causa) | Índice acotado al menor de los dos cardinales | Ídem |
| 3 | `Script::cargarSiNecesario()` | Cargar los valores guardados de un script | `std::variant` (efecto, no causa) | `ReflejoScripts::alinearValores()` empareja por nombre al compilar | Ídem |
| 4 | `redirigirSalidaALog()` en `main.cpp` | Escribir el log de arranque | Windows / `Program Files` / ACL del instalador | Patrón R8 aplicado al log: lista ordenada de candidatas (`RutasLog::candidatas()`) —raíz de datos, carpeta del ejecutable y temporal del sistema— y se usa la **primera que acepte escribir**; con la carpeta del ejecutable sola no había log y toda la salida se iba a la consola | `fix(ventana): escribir el log de arranque en una carpeta escribible` |

> **Nota**: las instancias #2 y #3 no son la causa del crash sino el punto donde
> se manifiesta. La causa es la instancia #1: sin ella no habría divergencia
> entre el árbol de valores guardado y los campos que expone el script. Se
> arreglaron las tres porque el acceso fuera de rango es un defecto real por sí
> mismo: la escena guardada y la reflexión actual pueden discrepar siempre, no
> solo cuando falla la escritura.
>
> La instancia #4 tiene además un remate que no es de permisos: al escribir el log
> en la raíz de datos, la carpeta `logs/`$resultó ser una carpeta más de la raíz
> y `migrarProyectosAntiguos()` la arrastró dentro de `Proyects/`. Se documenta
> aparte en §11 (Patrón R13).

---

## 7. Tercer concepto: **Offsets engañosos en streams de texto (`tellg`/`seekg` con CRLF)**

Distinto de §1 (latencia de `mtime`) y de §6 (permisos de escritura): aquí la
lectura **sí funciona**, byte a byte, hasta que el código retrocede en el
archivo para releer una línea y el stream no vuelve al mismo sitio. La causa
es que en modo texto el CRT de Windows traduce `CRLF`↔`LF`, y en esa
traducción `tellg()` deja de devolver un offset físico consistente con el que
`seekg()` después interpreta.

### 7.1 Descripción del problema

`SceneSerializer::load()` abría `SceneBBDDObjetos.txt` en **modo texto**.
El `look-ahead` de `loadPreOrder` hace lo siguiente con cada línea:

```cpp
const std::streampos markerPosition = file.tellg();  // ¿posición exacta?
std::getline(file, marker);
if (marker no es "=>" ni "<=")
    file.seekg(markerPosition);                      // volver a leerla como línea
```

En modo texto, ese `seekg(tellg())` **no es idempotente** en
MinGW/libstdc++ sobre Windows: la releitura arranca en un offset erróneo.

### 7.2 Síntomas

- La línea del **siguiente hermano** en el bloque de hijos se relee
  **truncada** por la izquierda (`"ObjectN2.db"` → `"ectN2.db"`,
  `"basura"` → `"sura"`). Medido con sondas: la deriva va de **+2 a +6
  bytes** y depende del búfer, no es constante ni acumulativa de forma
  predecible.
- **Solo con ≥2 hermanos en el mismo nivel**: con un hijo por nivel el
  look-ahead siempre ve `"=>"` o `"<="` y nunca se ejecuta el `seekg`
  (por eso los round-trip con jerarquía en escalera no lo reproducían).
- Efecto en H-17 (bug de los objetos fantasma "Scene", ver
  `PLAN GENERAL DE FIX.md` §20): con el código previo, la línea truncada no
  contenía `"ObjectN"` → el id quedaba en su default `0` → `loadEntity`
  leía `ObjectN0.db` (el binario de la **raíz**) → nacía un hijo
  `Modelos3D` con el nombre "Scene" que, al guardarse con id propio, **se
  auto-propagaba**. Ningún error en el log: el código ni siquiera avisaba.
- El índice en disco **se ve limpio** (el guardado escribe secuencialmente,
  sin `seekg`), así que inspeccionar el archivo no revela nada.

### 7.3 Causa raíz

La traducción `CRLF`↔`LF` del CRT en modo texto hace que la posición que
devuelve `tellg()` y la que interpreta `seekg()` no midan lo mismo. La
única garantía de round-trip exacto es el modo **binario**, donde los
offsets son bytes literales. (Los demás `tellg`/`seekg` del motor —
`Script`, `Model`, `Material`, `Transform`, `Modelos3D` — leen vía
`Binario`, que ya abre con `std::ios::binary`: el problema era exclusivo
del índice de escena.)

### 7.4 Solución canónica (Patrón R9 — texto estructurado con seek en binario)

**Si el lector retrocede (`seekg(tellg())`, peek, relectura), abrir en
binario y normalizar las líneas al leerlas:**

```cpp
// load(): apertura en binario — offsets de bytes exactos.
std::ifstream file(pathTxt, std::ios::binary);

// helper: getline + recorte del '\r' que en binario ya no traduce nadie.
bool getlineLimpio(std::ifstream& file, std::string& line) {
    if (!std::getline(file, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return true;
}
```

Reglas complementarias:

- El `trim` de `\r` tiene que estar en **todas** las lecturas de ese
  archivo (bucle principal, look-ahead, saltos de bloque): una sola
  lectura sin normalizar rompe `line == "<="`.
- **Testear con ≥2 hermanos consecutivos**: es el único caso que ejercita
  el `seekg` de releertura; un round-trip en escalera (1 hijo por nivel)
  no lo toca.
- Validar además el patrón de cada línea (`ObjectN<entero>.db`) y saltarla
  con aviso si no cumple: es la red de seguridad para índices que **ya**
  quedaron corruptos con el formato viejo.

### 7.5 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `SceneSerializer::load()` / `loadPreOrder` (look-ahead línea ≈518/563) | Cargar la escena del proyecto | MinGW/libstdc++ sobre Windows, CRT en modo texto con CRLF | R9: apertura en binario + `getlineLimpio()`; validación estricta de líneas con aviso | `fix(escena): impedir los objetos fantasma Scene y leer el indice en binario` |

---

## 8. Cuarto concepto: **Bloques de entorno UTF-16 rechazados por falta de `CREATE_UNICODE_ENVIRONMENT`**

Distinto de §1 (cache viejo), §6 (permisos de escritura) y §7 (offsets de
texto): aquí la llamada **falla en el momento**, con un código de error
genérico, y lo engañoso es que *todo* bloque parece inválido — hasta el que
el propio sistema te dio. La duda se va hacia el contenido del bloque
("mi parser corrompe el UTF-16") cuando el problema está en una bandera
que no se pasó.

### 8.1 Descripción del problema

`CreateProcessW(..., lpEnvironment, ...)` interpreta `lpEnvironment` como
**ANSI a menos que se pase `CREATE_UNICODE_ENVIRONMENT` (0x400)** en
`dwCreationFlags`. El contrato está documentado en MSDN, pero es invisible
en la firma de la función: no hay parámetro de encoding, solo la bandera.
Sin ella, un bloque UTF-16 perfectamente formado se lee byte a byte:

```text
bloque UTF-16:   'P' 00 'A' 00 'T' 00 'H' 00 ...
leído como ANSI: "P"      ← termina en el primer NUL
```

queda una "clave" sin `=` (o directamente una cadena vacía) y el sistema
responde `ERROR_INVALID_PARAMETER` (87).

### 8.2 Síntomas

- `CreateProcessW` devuelve **87** con **cualquier** bloque que contenga
  variables, incluida una **copia literal** del bloque del propio proceso
  padre y el bloque **oficial** de `CreateEnvironmentBlock` (userenv): el
  contenido no importa, así que "revisar el parser" es un callejón sin
  salida.
- Un bloque **vacío** (dos NULs) **sí** funciona — se parece a un fallo
  intermitente o dependiente de alguna variable concreta.
- El mismo bloque pasado por `CreateProcessA` (ANSI es su default) **sí**
  funciona, y .NET (`ProcessStartInfo.Environment`, que sí agrega la
  bandera) también: solo cae la combinación *W + UTF-16 + sin bandera*.
- Como `lpEnvironment=NULL` (heredar el entorno) nunca falla, el bug
  permanece latente hasta que aparece un único código que pasa bloque
  (entorno extra, harvest de vcvars).

### 8.3 Causa raíz

La bandera falta en la llamada. Diagnóstico por reducción (sonda
`FunshiEngineGL/sondas/sonda_bloque.cpp`): bloque vacío ✓ / `A` con
bloque ✓ / sin bloque ✓ / `.NET` ✓ / copia literal ✗ / userenv ✗ /
cualquier mapa ✗ → el único diferenciador era `dwCreationFlags=0`.

### 8.4 Solución canónica (Patrón R10 — la bandera de encoding viaja junto al dato)

```cpp
const DWORD flags = bloque ? CREATE_UNICODE_ENVIRONMENT : 0;
CreateProcessW(nullptr, linea, nullptr, nullptr, heredar, flags,
               bloque, cwd, &si, &pi);
```

Reglas complementarias:

- La bandera se decide **dentro** de la función que recibe el bloque: una
  API que acepta bloques no puede delegar al llamador el encoding — si el
  llamador puede equivocarse, ese error no tiene test posible en la capa
  que llama.
- `CreateProcessA` con bloque ANSI no necesita bandera; al portar de `A` a
  `W` hay que acordarse de agregarla (el compilador no avisa).
- **Testear con un hijo que reciba y reporte la variable** (no solo
  `rc==0`): un bloque aceptado pero ignorado daría `rc=0` con el entorno
  heredado y pasaría inadvertido.

### 8.5 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `Proceso::lanzar()` (`src/FileManager/Proceso.cpp`) | Spawn con entorno propio (variable extra y bloque de vcvars) | Win32 `CreateProcessW` | R10: `CREATE_UNICODE_ENVIRONMENT` condicional a que haya bloque | `fix(scripts): invocar el compilador y los sondajes sin cmd.exe` |

---

## 9. Quinto concepto: **Una imagen cargada bloquea su archivo (Windows)**

Distinto de §1 (cache con `mtime` viejo), §6 (permisos de la carpeta) y §8
(encoding del entorno): acá el archivo es escribible, el proceso tiene permiso
y la operación sale bien las demás veces. Lo que cambia es que **otra parte
del proceso ya tiene la imagen cargada** y el sistema mantiene el fichero
mientras viva esa carga. El error —permiso denegado— apunta justo a lo que no
falta, y ahí se pierde una tarde probando permisos, antivirus y borrado de
temporales.

### 9.1 Descripción del problema

`dlopen`/`LoadLibrary` mapea la `.dll` en el proceso y el sistema deja el
archivo con bloqueo de escritura mientras la imagen siga cargada (en POSIX no
hay bloqueo: reescribir un fichero mapeado está permitido). Si el proceso
vuelve a invocar al enlazador con **la misma ruta de salida** —recompilar el
mismo artefacto—, `ld` no puede abrir su archivo de salida y aborta:

```text
ld.exe: cannot open output file C:\...\script_<hash>.dll: Permission denied
collect2.exe: error: ld returned 1 exit status
```

La clave del artefacto se calcula por ruta de fuente + compilador + flags, así
que todos los componentes que apuntan al mismo fuente comparten la misma
salida: recompilar para el segundo componente es, inevitablemente, intentar
reescribir lo que el primero ya cargó. `FreeLibrary`/`dlclose` es lo único que
libera el archivo.

### 9.2 Síntomas

- El fallo **no es de compilación sino de enlace**, y aparece donde `ld` abre
  su salida: el código traducido del fuente está bien.
- Ocurre solo cuando **dos componentes comparten el mismo fuente** y el
  primero ya cargó el artefacto; con un único componente por fuente no se
  reproduce.
- El mismo archivo, con los mismos permisos, se escribe sin problemas
  segundos antes y segundos después (fuera de la ventana en que la imagen
  está cargada): parece un problema de permisos o de antivirus y no lo es.
- En POSIX no se manifiesta, lo que sugiere un problema del sistema de
  archivos cuando es del sistema operativo.

### 9.3 Causa raíz

1. La recompilación se decidía por el `mtime` **guardado en el componente**
   (`salida.mtimeFuente`), vacío en un componente recién cargado de la
   escena: recompila siempre, aunque el artefacto se haya escrito hace
   milisegundos.
2. La cola de compilación encola **por componente**, sin agrupar por fuente,
   así que los dos llegan en la misma pasada y en serie.
3. El primero enlaza, hace `dlopen` y deja el archivo bloqueado; el segundo
   vuelve a enlazar sobre esa misma ruta: denegado.

### 9.4 Solución canónica (Patrón R11 — decidir por la frescura del artefacto, no por el estado de quien pide compilar)

- La recompilación se condiciona a que **falte el artefacto o sea más viejo
  que su fuente** (`last_write_time(artefacto) < last_write_time(fuente)`).
  Es una propiedad del **archivo**, no del componente que pide la carga: un
  artefacto al día se usa tal cual y el segundo componente solo lo vuelve a
  abrir (la biblioteca se referencia, no se duplica), sin pasar por el
  enlazador.
- La clave del artefacto sigue llevando el contrato de compilación
  (compilador + flags): cambiar los flags cambia la clave, la salida "nueva"
  no existe y se recompila sola, sin borrar temporales a mano.
- Cuando sí hay que recompilar (el fuente se editó), quien lo tiene cargado
  descarga **antes** de volver a compilar (`Script::recargar` →
  `ScriptRuntime::descargar` → `FreeLibrary`): así el archivo queda libre
  para el enlazador.

Checklist de mitigación:

- [ ] No invocar al enlazador sobre un artefacto que **este proceso** tenga
      cargado: comprobarlo por frescura de archivo antes de compilar.
- [ ] Si hay que reescribirlo, descargar **primero** todas las instancias que
      lo carguen y solo después compilar y volver a cargar.
- [ ] Mirar los dos frentes: quién pide la compilación y quién tiene el
      archivo abierto en este proceso.
- [ ] Test: dos componentes sobre el mismo fuente, el segundo con la
      estructura de carga vacía, y comprobar que carga **sin** reescribir el
      artefacto.

### 9.5 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `BackendCpp::compilarYCargar()` (`src/Behaviour/Backends/BackendCpp.cpp`) | Compilar el script al entrar en Play con dos objetos sobre el mismo `.cpp` | MinGW `ld` sobre Windows (`LoadLibrary`) | R11: `artefactoVigente()` (existe y no es más viejo que el fuente) decide la recompilación; mismo criterio en `BackendJava` sobre la `.class` | `fix(scripts): no recompilar un artefacto que ya esta al dia` |

---

## 10. Sexto concepto: **Un contexto Core Profile rechaza todo dibujo sin VAO ligado**

### 10.1 Descripción del problema

En un perfil **core** de OpenGL el *Vertex Array Object* 0 no es un objeto
usable: la API lo eliminó para forzar el pipeline moderno. Cualquier llamada de
dibujo (`glDrawArrays`, `glDrawElements`, instanciada o no) con **ningún VAO
ligado** se rechaza con `GL_INVALID_OPERATION` y **no dibuja nada**. Como el
error no interrumpe ni imprime nada por defecto (Mesa no loguea `glGetError`),
el síntoma se ve como "la pasada no hace nada" y no como un error de compilación
o de shader.

El caso típico que cae en esto es la pasada **sin atributos**: un shader que
arma sus vértices con `gl_VertexID` (triángulo a pantalla completa, post-proceso,
estencil) y por eso no lleva VBO. Es fácil creer que "no hay geometría, entonces
no hace falta VAO": con ese razonamiento la pasada funciona en un contexto de
compatibilidad y no en el core que pide el motor.

### 10.2 Síntomas

- Una pasada completa (fondo, cielo, post-proceso) **no aparece nunca**, mientras
  el resto de la escena se dibuja bien.
- El área afectada muestra el **color de limpieza** del framebuffer (o lo que
  haya quedado de una pasada anterior), no negro ni basura.
- Sin mensajes: ni error de compilación de shader, ni excepción, ni línea de log.
  El shader linkea, los uniforms se encuentran y se suben con éxito.
- Los valores que la pasada debería usar llegan correctamente (se pueden trazar
  por log hasta el `setUniform`): el problema no está en los datos.
- Reproducible en drivers que imponen core estricto (Mesa con
  `GLFW_OPENGL_CORE_PROFILE`, macOS, GPU modernas); en un contexto de
  compatibilidad el mismo código puede funcionar y esconder el bug.

### 10.3 Causa raíz

```cpp
// Pasada sin atributos (fallaba): el shader usa gl_VertexID, asi que "no
// necesita VAO"... en un contexto de compatibilidad.
backend.useProgram(skyProgram_);
backend.setUniformVec3(locTop, colorSup);
glDrawArrays(GL_TRIANGLES, 0, 3);   // <-- sin VAO ligado: GL_INVALID_OPERATION
```

El motor desliga el VAO al terminar cada dibujo (`glBindVertexArray(0)`) y el
backend de ImGui restaura el VAO previo al cerrar su frame, que es 0. Por eso el
estado al entrar a la pasada era "ningún VAO": en core, dibujo descartado.

Comprobación directa (`glGetError` después del draw): `0x0502`
= `GL_INVALID_OPERATION` y los píxeles quedan en el color de clear; ligando un
VAO vacío el mismo draw devuelve `GL_NO_ERROR` y pinta el degradado esperado.

### 10.4 Solución canónica (Patrón R12 — el que dibuja posee su VAO)

**Toda pasada sin atributos necesita un VAO ligado, aunque esté vacío**, y ese
VAO lo debe poseer quien ejecuta el dibujo, no el llamador:

```cpp
void OpenGL3Backend::drawFullscreenTriangle() {
    if (vaoPantallaCompleta_ == 0) GLFuncs::pfnGenVertexArrays(1, &vaoPantallaCompleta_);
    GLFuncs::pfnBindVertexArray(vaoPantallaCompleta_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    GLFuncs::pfnBindVertexArray(0);
}
```

Puntos clave:

- El VAO vacío se crea **una sola vez** y se reusa; no hay que subir geometría
  ni atributos.
- El dibujo va por el **backend** (`IRenderBackend`), no por un `glDrawArrays`
  suelto en la capa de escena: así el estado gráfico queda en la única capa que
  lo posee y cualquier pasada futura reusa el mismo camino.
- Fuera del backend, la regla general es: **si hay `glDraw*`, antes hay un
  `glBindVertexArray`** (los meshes y los batches de líneas ya lo hacían).

Checklist de mitigación:

- [ ] Revisar todo `glDrawArrays`/`glDrawElements` del motor y confirmar que
      tiene un VAO ligado (propio, del backend o del objeto que dibuja).
- [ ] En una pasada nueva sin atributos, usar la pasada a pantalla completa del
      backend en vez de llamar a `glDrawArrays` directo.
- [ ] Al depurar "una pasada no pinta nada": `glGetError` inmediatamente después
      del draw (`0x0502` = VAO ausente) y leer píxeles del framebuffer para
      distinguir "no dibuja" de "dibuja con otro color".
- [ ] No confiar en "no hay geometría, no hace falta VAO": en core el VAO es
      obligatorio siempre.
- [ ] Test: sonda que corre la misma pasada con y sin VAO ligado y compara
      píxeles (la sonda vive fuera del repo; el fix se cierra con la validación
      en la app).

### 10.5 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `SceneRenderer::dibujarCielo()` (`src/Rendering/SceneRenderer.cpp`) | Cielo degradado (triángulo a pantalla completa con `gl_VertexID`) sin VAO ligado en contexto core | OpenGL core (`GLFW_OPENGL_CORE_PROFILE`, Mesa 4.6) | R12: `IRenderBackend::drawFullscreenTriangle()` posee un VAO vacío y lo liga por dentro | `fix(rendering): ligar VAO en el pase a pantalla completa del cielo` |

---


## 11. Séptimo concepto: **Una migración de compatibilidad se lleva las carpetas del motor**

Distinto de §6: aquí la escritura **sí funciona**. El daño lo produce, unos
segundos más tarde, código del propio motor que reorganiza la carpeta de datos.

### 11.1 Descripción del problema

La raíz de datos (`MotorGrafico/`) comparte sitio con dos cosas a la vez: los
proyectos del usuario y las carpetas del motor (`Proyects`, `Configuraciones`,
`Exportaciones`, `logs`). Para migrar los proyectos de una estructura antigua,
`migrarProyectosAntiguos()` recorre **todo** lo que hay en la raíz y mueve a
`Proyects/` cada carpeta cuyo nombre sea válido como proyecto:

```cpp
for (const auto& entry : std::filesystem::directory_iterator(base, ec)) {
    if (!entry.is_directory(ec)) continue;
    const std::string nombre = entry.path().filename().string();
    if (ProjectPaths::esNombreValido(nombre) && !existeDir(proyectsDir + "/" + nombre)) {
        std::filesystem::rename(entry.path(), proyectsDir + "/" + nombre, ec);
    }
}
```

`esNombreValido()` es la **única** barrera: una carpeta del motor cuyo nombre no
esté en la lista de reservados se comporta como un proyecto legacy.

### 11.2 Síntomas

- La carpeta se crea y el archivo se escribe en ella **correctamente**.
- En el mismo arranque, la carpeta desaparece de su sitio y aparece anidada
  dentro de `Proyects/`, con su contenido dentro.
- El motor anuncia una ruta que ya no es la real: el log dice
  `Log en: <raiz>/logs/…` y el archivo está en `<raiz>/Proyects/logs/…`.
- En el arranque siguiente se crea una carpeta nueva y vacía, así que el
  síntoma se renueva solo y parece intermitente.
- No hay ningún error: `rename()` dentro del mismo volumen funciona.

### 11.3 Causa raíz

La lista de nombres reservados y la migración se escribieron en momentos
distintos, y la lista no se revisó al añadir una carpeta nueva a la raíz. El
defecto no está en la migración (que hace lo que debe para lo que conoce), sino
en que **el conocimiento de "esto es mío" no tiene una fuente única**: vive en
una lista de cadenas que se olvida actualizar.

### 11.4 Solución canónica (Patrón R13 — declarar la reserva al crear la carpeta)

**Toda carpeta que el motor cree dentro de la raíz de datos se declara reservada
en el mismo cambio que la crea.** Si el nombre está reservado, la migración la
ignora y el usuario tampoco puede elegirlo como nombre de proyecto, que es justo
lo que tiene que pasar.

```cpp
// La lista es la frontera entre "carpeta del motor" y "proyecto del usuario".
// Si añades una carpeta aquí abajo, añádela también a esReservado().
static const char* reservados[] = {
    "Proyects", "Configuraciones", "Exportaciones",
    "Binarios", "Memory", "Interfaces", "Sonidos",
    "Configuracion.json", "imgui.ini", "logs"
};
```

Regla operativa: **la prueba de que una carpeta del motor no se mueve es que su
nombre no es válido como proyecto.** No hace falta una lista paralela en el
lugar donde se crea la carpeta.

Checklist antes de cerrar un cambio que cree una carpeta en la raíz de datos:

- [ ] El nombre está en `esReservado()` (o cumple el patrón `src*`).
- [ ] Hay un test que fija `!esNombreValido(<nombre>)` para ese nombre.
- [ ] El arranque completo del motor deja la carpeta donde se creó: se comprueba
      en disco **después** de la inicialización, no justo después de crearla.
- [ ] Si la carpeta puede convivir con proyectos, el nombre en pantalla y la
      documentación la nombra en el árbol de la raíz.

### 11.5 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `ProjectPaths::esReservado()` | Crear `logs/` en la raíz de datos al escribir el log de arranque | `std::filesystem` / cualquier SO | Patrón R13: `logs` pasa a ser nombre reservado, así `migrarProyectosAntiguos()` no la mueve a `Proyects/` y ningún proyecto puede usar ese nombre | `fix(ventana): escribir el log de arranque en una carpeta escribible` |

---

## 12. Historial de Cambios

| Fecha | Autor | Cambio |
|-------|-------|--------|
| 2026-09-27 | Gianfranco Ivan Enrique | Creación del documento; registro de instancias #1–3; definición de plantilla y checklist para agentes. |
| 2026-09-27 | Gianfranco Ivan Enrique | Añadido el segundo concepto (Patrón R8, escritura rechazada en el directorio de instalación) con su registro de instancias, a raíz del crash al asignar un script en el binario instalado. |
| 2026-09-27 | Gianfranco Ivan Enrique | Añadido el tercer concepto (Patrón R9, offsets engañosos de `tellg`/`seekg` en streams de texto con CRLF) con su instancia #1, a raíz del bug de los objetos fantasma "Scene" (H-17). |
| 2026-09-27 | Gianfranco Ivan Enrique | Añadido el cuarto concepto (Patrón R10, `CREATE_UNICODE_ENVIRONMENT` obligatorio con bloques UTF-16 en `CreateProcessW`) con su instancia #1, a raíz del error 87 al pasar el entorno de vcvars/variable extra (H-3 nivel 2). |
| 2026-09-28 | Gianfranco Ivan Enrique | Añadido el quinto concepto (Patrón R11, una imagen cargada bloquea su archivo en Windows) con su instancia #1, a raíz del `Permission denied` de `ld` al haber dos objetos sobre el mismo script (H-20). |
| 2026-09-28 | Gianfranco Ivan Enrique | Añadida instancia #4 al primer concepto: drop entre paneles (ShowFolder → BrowseFile y ShowFolder → ShowFolder carpeta distinta) con invalidación explícita de cache grid en origen y destino. |
| 2026-09-29 | Gianfranco Ivan Enrique | Añadido el sexto concepto (Patrón R12, un contexto Core Profile rechaza todo dibujo sin VAO ligado) con su instancia #1, a raíz del cielo degradado que no se dibujaba en un contexto 4.6 core. |
| 2026-10-03 | Gianfranco Ivan Enrique | Añadido el séptimo concepto (Patrón R13, una migración de compatibilidad se lleva las carpetas del motor) con su instancia #1, y registrada la instancia #4 del segundo concepto (Patrón R8, escritura rechazada en el directorio de instalación) a raíz del log de arranque que no se escribía instalado en `Program Files`. |

---

*Este documento es vivo: cada nuevo bug de esta clase debe registrarse en la tabla de su concepto (§2 para el primero, §6.5 para el segundo, §7.5 para el tercero, §8.5 para el cuarto, §9.5 para el quinto) y, si revela un patrón nuevo, añadirse a §4.*