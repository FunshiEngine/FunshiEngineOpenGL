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
| 5 | `ContentFolderInterface::initGUI()` (menu "Copy Exist File") | Copiar un archivo del sistema a la carpeta visible | `std::filesystem` / NTFS / MSYS2 | Invalidación explícita del cache del grid (`invalidarCache()`) tras la copia | `feat(explorador): arrastrar y pegar archivos del sistema` |
| 6 | `ContentFolderInterface::contentGUI()` | Importar archivos del sistema (arrastrados sobre la ventana o pegados con Ctrl+V) | Idem | `ImportarArchivos::pegar` + `invalidarCache()` explícito; `contadorCambios++` solo si se copiaron carpetas (los archivos no aparecen en el árbol) | Idem |

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
| **R14 — Un valor horneado es una pista, no una verdad** | Lo que CMake hornea (rutas de compilador, de cabeceras, de toolchain) es de la máquina que compiló: comprobar que existe antes de usarlo, dejar un override por entorno que gane, buscar la herramienta en la máquina y degradar a un nombre que resuelva el PATH | `BackendCpp::compilador()`, `vcvars64EnRaices()` (ver §12.4) |
| **R15 — El separador nativo manda al comparar rutas** | Quien arma una ruta con `std::filesystem` (o con `PATH_SEP`) produce el separador de su plataforma: en Windows, `base / "logs"` es `\logs`, no `/logs`. Un test que compare contra un literal con `/` solo pasa en Linux; comparar contra `(fs::path(base) / ...).string()` o rehacer el cálculo con la misma herramienta que el código bajo prueba | `tests/EditorConfigTests.cpp` (bloque `RutasLog::candidatas` y `relativizarRuta`) |
| **R16 — La resolución del `mtime` depende del toolchain, no del FS** | NTFS registra el mtime con 100ns, pero la lectura de `std::filesystem::last_write_time` en el CRT de MinGW viene de `stat` (1 segundo) y su setter no aplica sobre directorios: una creación dentro del mismo segundo que la marca guardada no es observable ahi, mientras que con MSVC (FILETIME) si lo es. Quien compare mtimes para decidir (polling, invalidación de cache, hot reload) no debe depender de subsegundos, y un test que ejercite esa comparación debe hacer el cambio visible en ambas granularidades o constatar la limitación | `FileSystemWatcher` (fallback de polling), `tests/FileManagerTests.cpp` (bloque polling) |
| **R17 — Tu binario también es el subproceso** | Un motor multiproceso (CEF) relanza el **mismo ejecutable** con `--type=...`: `CefExecuteProcess` debe correr al comienzo de `main`, antes de log/ventana/GL; devuelve -1 cuando el proceso no es subproceso y el motor sigue. Si no se corta ahí, el subproceso arranca la inicialización completa del motor en paralelo al proceso principal | `WebUIRuntime::ejecutarSubproceso` en `main.cpp` y `main_juego.cpp` (ver §13.4) |
| **R18 — Generar por configuración lo que depende de la configuración** | Un generador multi-config (Visual Studio, Ninja Multi-Config) evalúa lo generado una vez por configuración: lo que dependa de la configuración (la ruta de un binario, `$<TARGET_FILE:...>`) no puede vivir en un único archivo compartido, o CMake aborta con `Evaluation file to be written multiple times with different content`. El archivo va a `.../$<CONFIG>/...` y el include del consumidor a la carpeta de su configuración | `FunshiEngineGL/CMakeLists.txt` (cabecera generada de los plugins de prueba; ver §14.4) |
| **R19 — Declarar cada tipo con el mismo class-key en la definición y en la declaración adelantada** | MSVC mete el class-key (`class`/`struct`) en el nombre del símbolo: si un header adelanta el tipo con un key y otro lo define con el otro, la definición y la referencia se llaman distinto y el enlazado falla con LNK2019 aunque el código sea idéntico (GCC y Clang lo ignoran). La declaración adelantada copia el key de la definición y el aviso que lo anticipa (C4099 en MSVC, `-Wmismatched-tags` en GCC y Clang) se convierte en error | `FunshiEngineGL/src/Rendering/MeshRenderer.h`, `FunshiEngineGL/src/Rendering/DibujoModelo.h` (ver §15.4) |
| **R21 — Identificar por el tipo propio de la biblioteca, no por RTTI** | Una biblioteca compilada sin información de tipos (`/GR-` en MSVC) emite sus tablas de métodos virtuales sin puntero a typeinfo: el `dynamic_cast` lee ese puntero y el proceso aborta con un *access violation* dentro de `__RTDynamicCast`. El paquete de Bullet de vcpkg se compila así a propósito (la feature `rtti` está invertida). Para reconocer una clase hay que usar la API propia de la biblioteca (en Bullet, el "shape type" que devuelve `getShapeType()`) y un `static_cast` tras la comprobación | `FunshiEngineGL/src/Objetos/Componentes/Colliders/TiposShapeBullet.h` (ver §17.4) |
| **R22 — Acotar toda longitud leída del disco** | La longitud de un bloque la controla el archivo: usarla tal cual como tamaño de reserva lanza `std::length_error` o agota la memoria, y las dos terminan en `std::terminate`. Toda longitud se acota con un máximo por tipo **y** se contrasta con el contenido que queda en el stream; un dato inválido corta la lectura con un error, nunca aborta | `FunshiEngineGL/src/Herramientas/LimitesSerializacion.h` (ver §18.4) |
| **R23 — La etiqueta manda sobre el dato guardado** | Un `std::variant` solo admite una alternativa activa: si el tipo del campo cambió desde que se guardó, el valor guardado ya no se puede reinterpretar y leerlo lanza `std::bad_variant_access`. Al emparejar por nombre, si la etiqueta no coincide con la del campo, se usa el valor por defecto del campo | `FunshiEngineGL/src/Behaviour/Reflection/BehaviourReflection.cpp` (ver §19.4) |
| **R24 — El avance del iterador va en la cláusula de incremento** | En un bucle sobre una lista enlazada, con el avance al final del cuerpo, un `continue` lo salta y el bucle gira sobre el mismo elemento para siempre. Con el avance en la cláusula de incremento del `for`, ninguna sentencia del cuerpo puede saltárselo | `FunshiEngineGL/src/Rendering/SceneRenderer.cpp` (ver §20.4) |
| **R25 — La consulta de disponibilidad es el resultado de la carga** | Si la carga de punteros a funciones comprueba treinta funciones y la consulta de disponibilidad mira una sola, existe un estado en el que la carga falló y la consulta dice que todo está bien: el motor llama a un puntero nulo. La consulta devuelve el resultado de la carga | `FunshiEngineGL/src/Rendering/Backend/GLFuncs.h` (ver §21.4) |
| **R26 — El frame arranca limpio** | Dibujar sobre el framebuffer sin limpiarlo color y profundidad supone que el driver dejó el buffer de profundidad en 1.0: no todos lo hacen. El test de profundidad descarta entonces todo lo que se dibuja y la ventana queda en negro, sin error ni aviso. Cada frame limpia color y profundidad con el color del perfil de apariencia, y el editor y el juego exportado usan el mismo helper | `FunshiEngineGL/src/Rendering/FondoEscena.h` (ver §22.4) |
| **R27 — Lo que solo compila otra herramienta, compílalo también en el motor** | Una unidad de traducción que el build del motor excluye porque otra herramienta la compila (el exportador genera su propio proyecto) queda sin compilar en la suite y en la CI: sus errores solo aparecen cuando el usuario exporta, y solo en la plataforma que exporta. El motor la compila en un target propio, y la herramienta generada declara los flags que necesita en vez de heredar los del build de trabajo | `FunshiEngineGL/CMakeLists.txt` (target `runtime-compile-check`) y `FunshiEngineGL/src/Exportador/GameExporter.cpp` (ver §23.4) |
| **R28 — El resultado de una operación se conserva y se muestra** | Si la interfaz decide qué dibujar con la bandera «en curso», el mensaje final (éxito o error) se dibuja solo mientras la operación corre: al terminar, la bandera se apaga, el formulario vuelve a aparecer y el usuario no ve ni el fallo ni la confirmation. El estado de la operación tiene fases explícitas (configuración → en curso → resultado) y la interfaz dibuja lo que la fase dice; el motivo de un rechazo previo queda en el estado, y los datos que la operación necesita llegan por **todos** los caminos que la pueden abrir | `FunshiEngineGL/src/Exportador/EstadoExportacion.h` (ver §24.4) |
| **R29 — El cierre de dependencias es transitivo** | Si el ejecutable importa una biblioteca, y esa biblioteca importa otra, las dos tienen que estar junto al ejecutable: empaquetar solo las dependencias directas deja al proceso pidiendo al sistema librerías que no están | `FunshiEngineGL/src/Exportador/ImportacionesPE.h` (ver §25.4) |
| **R30 — Cada operación que compila, con su directorio** | Un directorio de trabajo con nombre fijo lo vacía la segunda operación que compila mientras la primera sigue, y la primera falla con errores que no son de su código. El directorio se reserva de forma atómica y es de usar y tirar | `FunshiEngineGL/src/Exportador/GameExporter.cpp` (ver §26.4) |
| **R31 — Un lector por plataforma detrás de un contrato** | El portapapeles de archivos del sistema no es texto: Windows publica un bloque `CF_HDROP` binario y Linux un `text/uri-list`. El dominio pide las rutas a un contrato y cada plataforma traduce su formato, con la traducción en funciones puras que se prueban sin portapapeles | `FunshiEngineGL/src/FileManager/PortapapelesParsers.h` (ver §27.4) |
| **R32 — El sistema encola, la vista drena** | El drop de archivos lo entrega la biblioteca de ventanas en su bucle de eventos, fuera del frame: ahí la vista todavía no sabe qué carpeta está viendo. El servicio encola las rutas y la vista las drena en su frame, que es cuando conoce el destino | `FunshiEngineGL/src/FileManager/ReceptorDropGlfw.h` (ver §28.4) |

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

Eran tres hechos que por separado parecían inocuos y juntos cerraban la puerta:

| # | Hecho | Consecuencia |
|---|-------|--------------|
| 1 | `PrivilegesRequired=admin` en el `.iss` | El instalador corre como administrador e instala en `Program Files`. |
| 2 | El `.iss` crea `{app}\MotorGrafico` **sin directiva `Permissions:`** | La carpeta hereda los ACL de `Program Files`: `Users` tiene lectura y ejecución, **no** escritura. |
| 3 | El `.exe` no lleva manifiesto `requestedExecutionLevel` | El motor corre como usuario normal, sin elevar, y por tanto sin esos permisos. |

El agravante: **ningún llamador comprobaba el retorno de las escrituras**
(`crearArchivo`, `guardarGeneral`, `saveScene`, … devolvían `bool` y se
ignoraban). Por eso el fallo fue silencioso en vez de un error reportado.

Hoy el instalador es **por usuario** (`PrivilegesRequired=lowest`, con lo que
Inno Setup mapea `{autopf}` a `%LOCALAPPDATA%\Programs`), así que la carpeta
del ejecutable está en el perfil y el problema ya no se reproduce en una
instalación nueva. Los tres hechos siguen siendo posibles por otras vías (una
copia del motor en `Program Files`, una carpeta de datos montada en solo
lectura, un proceso elevada), así que el patrón de la sección siguiente se
mantiene como red de seguridad y no como el camino normal. Además, como el motor
resuelve la raíz probando la escritura de verdad, un cambio en el destino se
detecta solo: si `{app}` deja de admitir escritura, los datos se van a la
carpeta del usuario sin tocar nada.

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
- **Preferir que la ruta sea escribible antes de depender del plan B**: instalar
  por usuario (`PrivilegesRequired=lowest`) hace que el destino sea del propio
  usuario y elimina la UAC de paso. El patrón sigue siendo obligatorio porque
  el destino puede cambiar sin que nadie lo decida (carpeta montada en solo
  lectura, copia a otra ubicación, proceso que corre elevado).
- **Poder probar la decisión sin el ejecutable**: la elección entre las dos
  candidatas se aisló en `ProjectPathsDetalle::elegirRaizDeDatos`, sin caché y
  con las rutas por parámetro, para que las pruebas la ejerciten con rutas
  falsas. "No escribible" se simula con una ruta debajo de un archivo
  regular: falla igual en Linux y en Windows y no depende de permisos.

### 6.5 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `ProjectPaths::directorioBase()` | Guardar proyecto, escena, config e `imgui.ini` | Windows / `Program Files` / ACL del instalador | Patrón R8: prueba de escritura + caída a `%APPDATA%` / `$XDG_DATA_HOME`, decisión cacheada, migración sin sobrescribir | `fix(configuracion): resolver la raiz de datos cuando no se puede escribir` |
| 2 | `SettingsScript.cpp` (lectura de `SerializeField`) | Editar un campo del inspector | `std::variant` (efecto, no causa) | Índice acotado al menor de los dos cardinales | Ídem |
| 3 | `Script::cargarSiNecesario()` | Cargar los valores guardados de un script | `std::variant` (efecto, no causa) | `ReflejoScripts::alinearValores()` empareja por nombre al compilar | Ídem |
| 4 | `redirigirSalidaALog()` en `main.cpp` | Escribir el log de arranque | Windows / `Program Files` / ACL del instalador | Patrón R8 aplicado al log: lista ordenada de candidatas (`RutasLog::candidatas()`) —raíz de datos, carpeta del ejecutable y temporal del sistema— y se usa la **primera que acepte escribir**; con la carpeta del ejecutable sola no había log y toda la salida se iba a la consola | `fix(ventana): escribir el log de arranque en una carpeta escribible` |
| 5 | Instalador de Windows (`.iss`) | Instalar sin UAC | UAC / ACL de `Program Files` | `PrivilegesRequired=lowest`: instalación por usuario, con `{autopf}` mapeado a `%LOCALAPPDATA%\Programs`. Verificado con el ejecutable real en una carpeta de solo lectura: no escribe ahí y todo cae en la carpeta de datos del usuario. La elección de raíz se aisló en `ProjectPathsDetalle::elegirRaizDeDatos` para poder probarla | `fix(paquete): instalar por usuario sin pedir privilegios de administrador` |

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

## 12. Octavo concepto: **Un valor horneado del build vale en su máquina y en ninguna más**

Distinto de §6: aquí la ruta es correcta en el equipo que compiló y no
existe en el del usuario. No hay permisos ni latencia de por medio; el valor
viaja con el binario y apunta a un sitio que solo existe en la máquina
de origen.

### 12.1 Descripción del problema

CMake hornea en el binario valores de su propia máquina: la ruta del
compilador (`FUNSHI_CXX_COMPILER = ${CMAKE_CXX_COMPILER}`), la carpeta de
cabeceras (`FUNSHI_SRC_DIR`), el flag de runtime del CRT. El build de
desarrollo los tiene todos a mano, así que en el editor nunca se nota.

Al publicar, el `.exe`/`setup.exe` que se distribuye lleva dentro rutas como

    C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\…\cl.exe

que en el equipo del usuario no existen. El motor invocaba esa ruta tal cual.

### 12.2 Síntomas

- El script C++ no compila en el paquete instalado. El error es
  "no se encuentra el archivo" con una ruta de Visual Studio que el usuario
  nunca escribió: no dice nada de por qué ni de qué hacer.
- Con Visual Studio instalado y correcto en el equipo, el fallo persiste: la
  ruta horneada no existe, así que tampoco se derivaba su `vcvars64.bat`, y el
  compilador además habría necesitado ese entorno.
- Con MinGW/g++ instalado, tampoco: la ruta horneada (de MSVC) tiene prioridad
  sobre lo que el PATH sí sabe resolver.
- En el build de desarrollo todo funciona, porque ahí las rutas sí existen.

### 12.3 Causa raíz

El valor horneado se consumía sin comprobar nada. Se daba por hecho que la
máquina que ejecuta el binario es la que lo compiló, y ninguna de las tres
piezas tenía un plan B: la ruta se invocaba sin preguntar si existe, la
carpeta de cabeceras sin preguntar, y el entorno de vcvars se derivaba
subiendo desde una ruta ajena (que por eso nunca aparecía).

### 12.4 Solución canónica (Patrón R14 — Resolver en runtime, con el valor horneado como último recurso)

**Un valor horneado es una pista, no una verdad.** Al consumirlo:

1. **Override del entorno primero**, y gana siempre: es una petición explícita.
2. **Comprobar que el valor horneado existe** antes de usarlo. Si no existe, no
   se invoca: se pasa al siguiente paso.
3. **Buscar la herramienta en la máquina**, como ya se hace con el JDK
   (`ResolucionJdk`) y con las cabeceras (`RutaCabecerasScript`).
4. **Degradar a un nombre sin ruta** (`cl`, `g++`, `javac`): lo resuelve el
   PATH, que es lo único que se puede afirmar sin conocer la máquina. Con el
   entorno de vcvars en el PATH, `cl` a secas funciona y además evita atar el
   motor a una versión instalada.
5. **Avisar cuando un override del entorno no existe**, en vez de fallar en
   silencio más abajo con un mensaje que no apunta a la causa.

Y para que la decisión sea verificable donde no hay Windows, dejarla en
**funciones puras con los datos por parámetro** (`elegirCompilador`,
`vcvars64EnRaices`, `RutaCabecerasScript::resolver`, `ResolucionJdk`), que el
test ejercita con rutas falsas. "No existe" se simula con una ruta **debajo de
un archivo regular**: falla igual en Linux y en Windows y no depende de
permisos.

### 12.5 Checklist

- [ ] El valor que hornea CMake, ¿existe en la máquina del usuario? Si no,
      ¿hay un plan B y está escrito?
- [ ] ¿Hay un override por entorno, y gana sobre lo horneado?
- [ ] ¿La degradación final es un nombre que resuelve el PATH?
- [ ] ¿La decisión está aislada en una función testeable, con el caso "no
      existe" simulado sin depender del SO ni de permisos?
- [ ] El error que ve el usuario, ¿dice qué ruta se usó y por qué?
- [ ] ¿Hay un sitio donde la ruta horneada aparece **sin** comprobar?
      `grep FUNSHI_ CMakeLists.txt` para listarlos.
- [ ] Si el horneado es la ruta de un **archivo** y el resolutor espera una
      **carpeta**, ¿se reconoce como lo que es? Pasarlo donde esperan raíces lo
      descarta en silencio, también en la máquina donde se compiló.
- [ ] ¿Qué **tipo** de cosa hornea la variable de CMake (archivo o carpeta) y lo
      sabe quien la consume?

### 12.6 Registro de instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|----------------|--------------|--------|
| 1 | `BackendCpp::compilador()` | Compilar un script C++ en el paquete instalado | CMake `CMAKE_CXX_COMPILER` / Visual Studio | Patrón R14: `FUNSHI_CXX` manda; el horneado solo si existe; si no `cl` del PATH con el toolset de la máquina o `g++`; aviso si el override no existe | `fix(scripts): resolver el compilador en la maquina y no solo en el build` |
| 2 | `vcvars64Ruta()` | Derivar el entorno de MSVC del script | Visual Studio | Patrón R14: `vcvars64EnRaices` busca `vcvars64.bat` a dos niveles en las raíces de VS (cubre las Build Tools), con las raíces por parámetro | Ídem |
| 3 | `RutaCabecerasScript::resolver` | Encontrar `include` en el paquete instalado | `include` relativo horneado | Instancia previa del mismo patrón (valor horneado como pista, se resuelve en runtime) | `fix(scripts): resolver las cabeceras del script en la maquina del usuario` |
| 4 | `ResolucionJdk::conBibliotecaHorneada` | Encontrar la JVM cuando ninguna raíz del sistema trae `libjvm` | `JAVA_JVM_LIBRARY` horneado | El valor horneado es la ruta del **archivo**, no una raíz: en la lista de raíces se descartaba siempre. Se resuelve aparte, y solo si el archivo existe en esta máquina | `fix(scripts): usar la biblioteca horneada cuando existe en la maquina` |

---

## 13. Noveno concepto: **El motor web multiproceso relanza tu binario**

### 13.1 Patrón

CEF es multiproceso por diseño: el proceso principal lanza el **mismo
ejecutable** (o `cef_subprocess.exe` en Windows si existe) con
`--type=renderer|gpu-process|utility|zygote` para cada rol. El punto de
entrada debe ejecutar `CefExecuteProcess` **lo primero en `main`** y cortar el
proceso si ese rol no es el del motor: solo el proceso principal continúa con
ventana, log, GL e inicialización de CEF (`CefInitialize`).

### 13.2 Síntomas

- El proceso "principal" arranca más de una ventana o una sola con el log
  duplicado: el subproceso ejecutó la inicialización completa del motor.
- `ps` muestra procesos `--type=...` que viven más que la ventana o mueren con
  error al cerrar (apagado corrido dos veces o en el proceso equivocado).
- Sin `CefInitialize` con `external_message_pump`, los callbacks de CEF no se
  bombean y las vistas quedan sin pintar aunque `CefInitialize` haya devuelto
  cierto.

### 13.3 Causa

El contrato de CEF exige que la bifurcación de rol ocurra **antes** de
cualquier inicialización del motor (ver documentación CEF: `CefExecuteProcess`
"should be called first in main"). Devuelve el código de salida del rol si el
proceso es subproceso, o -1 si debe seguir. Poner la llamada después de abrir
ventana/log/GL hace que el rol de subproceso arranque todo el motor a la vez
que el proceso principal.

### 13.4 Fix canónico

```cpp
// Al comienzo de main, antes de registrar señales/log/ventana/GL:
const int codigo = WebUIRuntime::ejecutarSubproceso(argc, argv);
if (codigo >= 0) return codigo;   // rol de subproceso: no se arranca el motor
// -1: proceso principal, sigue el arranque normal.
```

`WebUIRuntime::ejecutarSubproceso` delega en el backend activo
(`IWebUIBackend::ejecutarSubproceso`; el nulo devuelve -1). La inicialización
va **después** de crear el contexto GL (`WebUIRuntime::inicializar`), el
bombeo una vez por frame (`WebUIRuntime::bombear`, `CefDoMessageLoopWork` con
`external_message_pump`) y el apagado (`WebUIRuntime::apagar` →
`CefShutdown`) después de descargar todas las vistas y antes de romper
ImGui/GL. Los mismos cuatro puntos van en `main.cpp` (editor) y
`main_juego.cpp` (juego exportado), bajo `FUNSHI_CEF`.

### 13.5 Checklist de mitigación

- [ ] `ejecutarSubproceso` es la **primera** llamada del punto de entrada, sin
      log ni ventana por delante.
- [ ] El proceso que no es subproceso recibe -1 y el arranque sigue igual
      (sin CEF compilado, el backend nulo devuelve -1 siempre).
- [ ] `inicializar` se llama una vez, con el contexto GL creado; si falla, el
      runtime cae al backend nulo y el motor sigue (nunca aborta).
- [ ] `bombear` por frame en el hilo principal; `apagar` tras descargar vistas
      y antes de destruir ImGui/GL.
- [ ] El juego exportado repite los mismos cuatro puntos: con CEF compilado,
      su binario **también** es el subproceso.
- [ ] El runtime de CEF (`.so`/`.dll` + `resources.pak` + `icudtl.dat` +
      `locales/`) queda junto al ejecutable; si falta, `inicializar` devuelve
      false y la GUI muestra el motivo en la vista, sin romper el arranque.

### 13.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `main.cpp::EjecutarMotor`, `main_juego.cpp::EjecutarJuego` | Arranque del editor y del juego exportado con backend CEF | CEF 154 multiproceso | Los cuatro puntos de §13.4 bajo `FUNSHI_CEF`, con fallback al backend nulo | `feat(webui): backend CEF off-screen, recarga en caliente y preview de interfaz` |

Evidencia de la sonda previa (`FunshiEngineGL/sondas/cef_probe/`, local): con
`CefExecuteProcess` al inicio de `main`, los roles `--type=gpu-process`,
`--type=renderer`, `--type=utility` y `--type=zygote` se relanzaron sobre el
mismo binario y la app principal siguió; `OnPaint` pintó el DOM con CSS/JS
externos y el bitmap se subió a textura byte a byte.

---

## 14. Décimo concepto: **Un archivo generado en tiempo de configuración vale en una configuración y en ninguna más**

### 14.1 Patrón

Un generador **multi-config** (Visual Studio, Ninja Multi-Config) compila cada
configuración en su propia carpeta (`build/Debug/`, `build/Release/`) y evalúa
el contenido generado **una vez por configuración**. Lo que depende de la
configuración —la ruta de un binario, la de una biblioteca— no puede escribirse
en un único archivo compartido: cada evaluación produciría un contenido
distinto en el mismo destino.

CMake lo dice así: `file(GENERATE)` aborta con

```
CMake Error in CMakeLists.txt:
  Evaluation file to be written multiple times with different content.
```

cuando el contenido varía entre configuraciones y la salida es la misma.

### 14.2 Síntomas

- La **configuración** falla con `Evaluation file to be written multiple times
  with different content`, y el error nombra el archivo generado (por ejemplo
  `generated/RutasPluginsMods.h`). El fallo es en la fase de generación: no
  compila nada.
- El mismo código **funciona** con Ninja o Makefiles (una sola configuración) y
  **falla** con Visual Studio. Una CI en verde no dice nada del build de
  Visual Studio, que es el que genera el instalador de Windows.
- Con `Ninja Multi-Config` el fallo se reproduce en cualquier sistema operativo:
  no es un problema de Windows.

### 14.3 Causa

`file(GENERATE)` se evalúa una vez por configuración. Si el contenido usa algo
que cambia con la configuración —`$<TARGET_FILE:tgt>`, que apunta a
`build/<CONFIG>/tgt`— y el archivo de salida es el mismo para todas, cada
configuración quiere escribir un contenido distinto en el mismo sitio.

### 14.4 Fix canónico (Patrón R18 — generar por configuración lo que depende de la configuración)

1. El **archivo** generado lleva la configuración en su ruta, de modo que CMake
   escriba uno por configuración y deje de quejarse:

   ```cmake
   file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/RutasPluginsMods.h
       CONTENT "... $<TARGET_FILE:mods-test-plugin-a> ...")
   ```

2. El **include** del consumidor apunta a la carpeta de su configuración:

   ```cmake
   target_include_directories(mods-tests PRIVATE
       ${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>)
   ```

   Con un generador de una sola configuración, `$<CONFIG>` es el tipo de build
   (`Release`), así que el include sigue resolviendo igual que antes.

Alternativas cuando el consumidor puede recibir la ruta por otros medios:
`target_compile_definitions` con `$<TARGET_FILE:...>` (el valor también se
evalúa por configuración) o localizar el artefacto en runtime a partir de la
ruta del propio ejecutable.

### 14.5 Checklist de mitigación

- [ ] Ningún `file(GENERATE)` escribe en una ruta compartida si su contenido
      depende de la configuración.
- [ ] Todo include de un archivo generado por configuración apunta a la carpeta
      `$<CONFIG>` correspondiente.
- [ ] La CI compila y prueba **con un generador multi-config** (Visual Studio
      en Windows), no solo con Ninja: es el generador que usa el job que
      publica el instalador.
- [ ] El fallo se puede reproducir sin Visual Studio: `Ninja Multi-Config`
      reproduce la misma semántica en cualquier sistema.

### 14.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/CMakeLists.txt` (`file(GENERATE)` de `RutasPluginsMods.h`) | Generar la ruta de las bibliotecas de prueba del sistema de mods | CMake `file(GENERATE)` con generador multi-config | El header se genera en `generated/$<CONFIG>/` y el include del test apunta a esa carpeta (Patrón R18) | `fix(cmake): generar por configuracion las rutas de los plugins de prueba` |

---

## 15. Undécimo concepto: **Un tipo declarado con un class-key distinto al de su definición produce dos símbolos distintos**

### 15.1 Patrón

En MSVC, el *class-key* (`class` o `struct`) con el que se declara un tipo
forma parte del nombre del símbolo que genera el compilador: `struct Mesh`
mangling como `PEBUMesh` y `class Mesh` como `PEBVMesh`, aunque el tipo sea el
mismo y los miembros idénticos.

Cuando un header **adelanta** el tipo con un key y otro header lo **define**
con el otro, cada unidad de compilación ve una declaración distinta y genera
un símbolo distinto para la misma función. La definición y la referencia ya no
coinciden y el enlazado falla.

GCC y Clang no distinguen el class-key: el nombre del símbolo es el mismo en
las dos unidades y el enlace funciona. Por eso el fallo es exclusivo de
Visual Studio, y una CI verde en Linux no dice nada del build de Windows.

### 15.2 Síntomas

- El **enlazado** falla con `LNK2019`, con el símbolo en forma no descompuesta
  y descompuesta, y la diferencia se ve en el key:

  ```
  funshi_engine.lib(SceneRenderer.obj) : error LNK2019: unresolved external symbol
    "public: bool __cdecl MeshRenderer::intentarRenderMesh(class GameObject *,
     struct Mesh const *, ...)"
    (?intentarRenderMesh@MeshRenderer@@QEAA_NPEAVGameObject@@PEBUMesh@@...@Z)
  ```

  La definición real del mismo método manglea como `PEBVMesh`: el mismo
  método, dos nombres.
- Todo **compila**: no hay error de compilación, el desajuste solo se ve al
  enlazar.
- El build con **GCC o Clang** enlaza sin problema, con el mismo código.
- Con MSVC, el aviso que lo anticipa es **C4099** ("nombre de tipo visto
  anteriormente usando 'class' ahora se ve usando 'struct'"), y sale en el
  **nivel de aviso 4**: con el nivel por defecto no aparece.

### 15.3 Causa

Una declaración adelantada escrita con un class-key distinto del de la
definición. El C++ estándar exige redeclarar un tipo con el mismo class-key;
GCC y Clang lo aceptan como extensión sin avisar, MSVC lo diagnostica (C4099)
pero solo con `/W4` y, aun así, el símbolo ya salió con el key equivocado.

El efecto depende de **qué unidad ve qué declaración**: la unidad donde la
declaración adelantada se lee primero genera el símbolo con el key de la
adelantada, y la que ve primero la definición real lo genera con el key de la
definición. De ahí que el mismo método aparezca con dos nombres.

### 15.4 Fix canónico (Patrón R19 — declarar cada tipo con el mismo class-key en la definición y en la declaración adelantada)

1. La **declaración adelantada** usa el mismo class-key que la definición:

   ```cpp
   // Assets/Mesh.h
   struct Mesh { ... };

   // Rendering/MeshRenderer.h
   struct Mesh;   // igual que en la definición, no "class Mesh;"
   ```

2. El **compilador avisa y el aviso es error**, para que el desajuste no pueda
   colarse:

   ```cmake
   if(MSVC)
     # C4099: el tipo se vio antes como "class" y ahora como "struct".
     check_cxx_compiler_flag("/we4099" FUNSHI_AVISO_CLASS_KEY)
     if(FUNSHI_AVISO_CLASS_KEY)
       add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/we4099>)
     endif()
   else()
     # Mismo desajuste, visto por GCC y Clang.
     check_cxx_compiler_flag("-Werror=mismatched-tags" FUNSHI_AVISO_CLASS_KEY)
     if(FUNSHI_AVISO_CLASS_KEY)
       add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Werror=mismatched-tags>)
     endif()
   endif()
   ```

   Con el aviso como error, la inconsistencia aparece **al compilar** y no
   como un LNK2019 al publicar el instalador.

3. Cuando el fallo aparece en un símbolo concreto, se compara el nombre
   descompuesto de la referencia con el de la definición: si solo difieren en
   el class-key (`struct` frente a `class`), el desajuste está en una
   declaración adelantada.

### 15.5 Checklist de mitigación

- [ ] Toda declaración adelantada (`class X;` / `struct X;`) usa el mismo
      class-key que la definición del tipo.
- [ ] El build de Windows con MSVC es parte de la CI, o el aviso C4099 está
      activo como error: un fallo de enlace que solo aparece con Visual
      Studio no lo detecta una CI que solo compila con Ninja.
- [ ] La CI compila el **motor completo** (no solo las suites headless) en al
      menos un compilador que enlace: un `LNK2019` solo se ve al enlazar.
- [ ] Al comparar dos nombres de símbolo, se mira el **class-key**: dos
      nombres que difieren solo en `struct`/`class` son el mismo tipo con dos
      declaraciones distintas.

### 15.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Rendering/MeshRenderer.h` (`Mesh`, `Image`) y `FunshiEngineGL/src/Rendering/DibujoModelo.h` (`Mesh`) | Dibujado de mallas con el pipeline moderno | MSVC (Visual Studio) al enlazar `funshi_engine` | Las declaraciones adelantadas usan `struct`, igual que la definición, y C4099 pasa a ser error de compilación (Patrón R19) | `fix(render): declarar Mesh e Image con el mismo class-key que su definicion` |
| 2 | `FunshiEngineGL/src/Objetos/Componentes/RigidBody/RigidBody.h` (`btDefaultMotionState`) | Cuerpo físico de Bullet | Aviso del compilador al activar `-Werror=mismatched-tags` (GCC) y `/we4099` (MSVC) | La declaración adelantada usa `struct`, que es el class-key con el que Bullet define el tipo (Patrón R19) | `fix(fisicas): declarar btDefaultMotionState con el class-key de bullet` |

---

## 16. Duodécimo concepto: **El nombre de un tipo persistido depende del compilador**

### 16.1 Patrón

`typeid(x).name()` no devuelve el nombre del tipo: devuelve una **cadena opaca
específica del compilador**. En MSVC es el nombre legible con el prefijo de
class-key (`"class Transform"`, `"struct Color"`); en GCC y Clang es el
*mangling* (`"9Transform"`, `"N6Objetos10Componentes6ColorE"`), y solo se puede
leer si se des-*manglinga* con `__cxa_demangle`.

Cuando ese nombre se **persiste** (en un prefab, en una escena, en un catálogo),
el artefacto queda atado al compilador que lo escribió. Y cuando se **compara**
contra un literal (`"Transform"`), el resultado depende de la plataforma: en
Linux coincide, en Windows no.

El nombre legible de un tipo, sea cual sea el compilador, es el último
componente de su nombre *fully-qualified*: `Objetos::Componentes::Transform`
→ `Transform`.

### 16.2 Síntomas

- En Windows, un prefab cargado **no conserva la posición** de su `Transform`:
  el `Transform` del objeto no es el que se cargó, se crea **otro** y la
  posición guardada se queda en el original (0, 0, 0).
- Con la misma escena, los tests **pasan en Linux y fallan en Windows**.
- Un componente cuyo nombre contiene una `E` o empieza por `E` desaparece al
  guardarse: el recorte manual del *mangling* de GCC ("hasta la `E` final")
  lo trunca (`"SpriteRenderer"` → `"SpriteRender"`) o lo deja vacío
  (`"EsfereCollider"` → `""`), y con el nombre vacío se pierde **todo** el
  prefab.
- Un artefacto escrito en una plataforma no se puede leer en la otra.

### 16.3 Causa

El código tomaba `typeid(...).name()` como si fuera un nombre de tipo legible y
estable, y lo comparaba con literales o con un nombre **parseado a mano** del
*mangling*. Tres fallos encadenados:

1. **Prefijo de class-key**: MSVC escribe `"class Transform"`; la comparación
   contra `"Transform"` falla y el caso especial del `Transform` nunca se
   alcanza.
2. **Namespace**: el nombre *fully-qualified* no es la clave con la que se
   registra un componente en el catálogo.
3. **Parseo manual del *mangling***: recortar por la letra `E` (el terminador
   del *mangling* de GCC) trunca los nombres que la contienen.

### 16.4 Fix canónico (Patrón R20 — un único helper produce el nombre persistido)

Un helper `nombreSimpleTipo()` en `Herramientas/TypeUtils` centraliza la
normalización:

1. des-*mangling* (en GCC y Clang) o 그대로 (en MSVC);
2. quita el prefijo de class-key (`class `, `struct `, `union `, `enum `),
   también cuando el nombre viene de un archivo escrito por otra plataforma;
3. se queda con el último componente del namespace (`rfind("::")`).

Ese helper se usa **tanto al escribir como al leer** y **tanto al comparar
como al mostrar**. Nunca se persiste un `typeid().name()` crudo, y nunca se
compara contra un literal sin pasar por el helper.

Detalle que hace fallar el helper sin avisar: `find_last_of("::")` busca un
**carácter**, no una subcadena → para el separador de namespace hay que usar
`rfind("::")`.

### 16.5 Checklist de mitigación

- [ ] Ningún `typeid(...).name()` se persiste ni se compara sin pasar por
      `nombreSimpleTipo()`.
- [ ] El helper es el **único** punto donde se decide el nombre: escribir y
      leer usan la misma función.
- [ ] El catálogo reconoce un componente por su nombre simple; si acepta otras
      formas, es por normalización, no por comparación literal.
- [ ] La CI ejecuta las suites headless en **Windows** además de en Linux:
      este tipo de fallo no se ve en una CI solo-Linux.
- [ ] Hay un test que comprueba que el nombre escrito se puede leer en la
      otra plataforma.

### 16.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Objetos/Prefab.cpp` (escritor y lector de prefabs) | Guardar y cargar un prefab con componentes | MSVC (Visual Studio) al ejecutar `prefab-tests` | El nombre del componente se normaliza con `nombreSimpleTipo()` al escribir y al leer (Patrón R20) | `fix(prefabs): persistir el nombre de tipo con un helper portable` |
| 2 | `FunshiEngineGL/src/Objetos/Componentes/CatalogoComponentes.cpp` | Buscar un componente por su nombre | MSVC (Visual Studio) | La búsqueda normaliza el nombre recibido antes de comparar (Patrón R20) | `fix(prefabs): persistir el nombre de tipo con un helper portable` |
| 3 | `FunshiEngineGL/src/Objetos/GameObject.cpp` (`serializeEntityComponents`, `getComponentByName`) | Serializar una escena y buscar un componente por nombre | MSVC (Visual Studio) | Ambos caminos usan el nombre simple del tipo (Patrón R20) | `fix(prefabs): persistir el nombre de tipo con un helper portable` |
| 4 | `FunshiEngineGL/src/Objetos/ClonadorObjetos.cpp` | Clonar un objeto con sus componentes | MSVC (Visual Studio) | El cloner identifica los componentes por su nombre simple (Patrón R20) | `fix(prefabs): persistir el nombre de tipo con un helper portable` |
| 5 | `FunshiEngineGL/src/Comandos/AgregarComponenteComando.cpp` | Nombrar el componente añadido desde el catálogo | MSVC (Visual Studio) | El nombre del componente se obtiene con el helper (Patrón R20) | `fix(prefabs): persistir el nombre de tipo con un helper portable` |

---

## 17. Decimotercer concepto: **Una biblioteca de terceros compilada sin RTTI invalida el `dynamic_cast`**

### 17.1 Patrón

Una biblioteca compilada **sin información de tipos** (`/GR-` en MSVC,
`-fno-rtti` en GCC y Clang) emite sus tablas de métodos virtuales **sin puntero
a typeinfo**. El `dynamic_cast` del runtime lee ese puntero de la tabla: si es
nulo, el proceso aborta con un *access violation* dentro de la rutina que
implementa el cast (`__RTDynamicCast`, en VCRUNTIME140.dll).

El paquete de Bullet que instala vcpkg se compila así a propósito: la feature
`rtti` está **invertida** y, sin ella, el port pasa
`-DUSE_MSVC_DISABLE_RTTI=ON`. En Linux el paquete del sistema y el de vcpkg
usan RTTI, de modo que el mismo código funciona allí y el fallo solo aparece
en Windows.

### 17.2 Síntomas

- Un test que pasa en Linux aborta en Windows con `0xC0000409` (fast-fail),
  provocado por un *access violation* `0xC0000005` previo dentro de
  VCRUNTIME140.dll.
- El runtime imprime "Access violation - no RTTI data!".
- El crash ocurre **exactamente** en el primer `dynamic_cast` sobre un objeto
  de la biblioteca: la comprobación anterior pasa y la siguiente no llega a
  ejecutarse.
- El mismo `dynamic_cast` en el motor (no en el test) revienta la aplicación
  real en Windows.

### 17.3 Causa

El motor identificaba las formas de colisión de Bullet con `dynamic_cast`
(`btCompoundShape`, `btBoxShape`, `btConvexHullShape`). Con Bullet compilado
sin RTTI, ese cast lee un typeinfo nulo.

### 17.4 Fix canónico (Patrón R21 — identificar por el tipo propio de la biblioteca, no por RTTI)

Bullet trae para eso el **shape type**: un entero que cada clase fija en su
constructor y que `getShapeType()` devuelve (`BOX_SHAPE_PROXYTYPE`,
`COMPOUND_SHAPE_PROXYTYPE`, `CONVEX_HULL_SHAPE_PROXYTYPE`; el enumerado lleva
esos nombres sin prefijo desde Bullet 3.09, y las versiones anteriores usaban
macros `BT_SHAPE_*`).

El helper `TiposShapeBullet` (en `Objetos/Componentes/Colliders`) reconoce cada
clase por su tipo proxy y devuelve `nullptr` cuando no encaja, de modo que el
`static_cast` que sigue es seguro. No queda ningún `dynamic_cast` sobre un
tipo de Bullet.

Si aun así se necesita RTTI de Bullet, vcpkg la expone con la feature
invertida `bullet3[rtti]`.

### 17.5 Checklist de mitigación

- [ ] Ningún `dynamic_cast` (ni `typeid`) sobre un tipo de una biblioteca de
      terceros que pueda venir compilada sin RTTI.
- [ ] La identificación del tipo usa la API propia de la biblioteca.
- [ ] La CI compila y **enlaza** el motor con MSVC y corre las suites: este
      fallo no se ve en una CI solo-Linux.
- [ ] Hay un test que comprueba que la identificación reconoce cada clase y
      rechaza las demás.

### 17.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Objetos/Componentes/Colliders/MallaCollider.cpp` | Dibujar el hull de un colisionador de malla | MSVC (Visual Studio) | El hull se identifica con `TiposShapeBullet::comoCascoConvexo()` (Patrón R21) | `fix(fisicas): identificar las shapes de Bullet sin RTTI` |
| 2 | `tests/TilemapTests.cpp` | Comprobar la shape compuesta del colisionador de tilemap | MSVC (Visual Studio) | Las shapes se identifican con `TiposShapeBullet` (Patrón R21) | `fix(fisicas): identificar las shapes de Bullet sin RTTI` |

---

## 18. Decimocuarto concepto: **Una longitud leída del binario usada como tamaño de reserva**

### 18.1 Patrón

El motor serializa cada bloque de datos precededido de su longitud: la ruta de
un script, el nombre de un clip, la cantidad de valores de un campo
reflejado. Esa longitud la **controla el archivo**: si el archivo está
corrupto o desalineado, el valor puede ser enorme.

Usarla tal cual como tamaño de reserva hace que `std::string::resize` lance
`std::length_error` (por encima del máximo de `size_t`) o que `bad_alloc`
agote la memoria (por debajo). Las dos excepciones terminan en
`std::terminate`: el motor **aborta** en vez de rechazar el archivo.

### 18.2 Síntomas

- El motor muere al cargar una escena, un proyecto o un script: `terminate
  called after throwing an instance of 'std::length_error'`, sin traza útil.
- Solo ocurre con archivos dañados; con archivos válidos nunca se ve.
- El fallo parece aleatorio: depende de qué byte del archivo quedó
  desalineado.

### 18.3 Causa

La longitud se leía del archivo y se pasaba directamente al `resize` o al
constructor del `std::vector`, sin cota ni contraste con el contenido que
quedaba en el stream.

### 18.4 Fix canónico (Patrón R22 — acotar la longitud y contrastarla con el contenido restante)

Toda longitud o cantidad leída del disco se valida con **dos** filtros:

1. **Una cota por tipo de dato** (ruta, nombre, texto, cantidad de elementos),
   en un único sitio compartido: `Herramientas/LimitesSerializacion.h`.
2. **El contenido restante**: la longitud no puede anunciar más bytes de los
   que quedan en el archivo.

Un dato inválido **corta la lectura del objeto con un error claro** (el stream
queda marcado como fallido y el llamador abandona) y el proceso sigue vivo.

El helper `leerLongitud` centraliza la operación:

```cpp
LimitesSerializacion::leerLongitud(archivo, longitud,
                                    LimitesSerializacion::kRuta, 1, 0);
```

El último parámetro es el mínimo admisible: `1` para un texto (no puede
estar vacío) y `0` para una cantidad (una colección puede estar vacía).

### 18.5 Checklist de mitigación

- [ ] Toda longitud o cantidad leída del disco pasa por `leerLongitud`.
- [ ] La cota depende del **tipo** del dato, no es un número único.
- [ ] Un dato inválido corta la lectura con un error; nunca aborta.
- [ ] Hay un test headless que escribe un archivo con longitudes absurdas y
      comprueba que la lectura se corta y el proceso sigue vivo.

### 18.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Objetos/Componentes/Script.cpp` | Leer la ruta del fuente y el nombre de la clase de un script serializado | GCC/Clang y MSVC | Las tres longitudes se leen con `leerLongitud` (Patrón R22) | `fix(serializacion): acotar las longitudes leidas del binario` |
| 2 | `FunshiEngineGL/src/Objetos/Componentes/AudioSource.cpp` | Leer el nombre del clip de una fuente de audio | GCC/Clang y MSVC | La longitud se lee con `leerLongitud` (Patrón R22) | `fix(serializacion): acotar las longitudes leidas del binario` |
| 3 | `FunshiEngineGL/src/Behaviour/Reflection/BehaviourReflection.cpp` | Leer el árbol de valores de un comportamiento (nombres, cantidades, grupos) | GCC/Clang y MSVC | Longitudes acotadas, cantidad de elementos acotada y tope de anidamiento (Patrón R22) | `fix(serializacion): acotar las longitudes leidas del binario` |

---

## 19. Decimoquinto concepto: **La etiqueta de un valor guardado deja de corresponder a su campo**

### 19.1 Patrón

Un valor serializado guarda **su tipo** junto al dato. Si el tipo del campo
cambia en el código (un texto que pasa a ser un entero, una lista que pasa a
ser un grupo), el archivo conserva la etiqueta vieja: el valor guardado y la
etiqueta que el código espera ya no coinciden.

El `std::variant` que envuelve el dato solo admite **una** alternativa
activa. Leerlo con el tipo que espera el código lanza `std::bad_variant_access`.

### 19.2 Síntomas

- El motor aborta **dentro del render de la interfaz** (el `std::get` se
  ejecuta al dibujar el panel del inspector): `terminate called after
  throwing an instance of 'std::bad_variant_access'`.
- El fallo aparece al abrir una escena guardada antes de un cambio de tipo en
  un script, no al guardar.
- El proceso muere en vez de descartar el dato que ya no es válido.

### 19.3 Causa

Al emparejar los valores guardados con los campos del script, el motor
conservaba el valor tal cual, incluida su etiqueta. El emparejado es por
nombre (a propósito, para sobrevivir a reordenamientos), pero no comprobaba
que el tipo del valor guardado fuera el que declara el campo.

### 19.4 Fix canónico (Patrón R23 — cuando el tipo guardado no es el del campo, se usa el valor por defecto)

Al emparejar, si la etiqueta del valor guardado **no** coincide con la del
campo actual, el valor guardado se descarta y se usa el valor por defecto del
campo: un dato que ya no se puede reinterpretar no se arrastra.

La regla general: **la etiqueta manda sobre el dato**. El valor guardado solo
se conserva cuando su tipo sigue siendo válido.

Como segunda barrera, el Inspector no debe leer el `variant` con un `std::get`
a ciegas: el acceso se hace sobre el tipo que la etiqueta declara.

### 19.5 Checklist de mitigación

- [ ] El emparejado por nombre comprueba también el tipo.
- [ ] Un valor cuyo tipo ya no es válido se sustituye por el valor por
      defecto del campo.
- [ ] Ningún `std::get` sobre un `variant` cuya alternativa activa no se ha
      comprobado antes.
- [ ] Hay un test headless que empareja un valor guardado con un tipo distinto
      al del campo y comprueba que el acceso no lanza.

### 19.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Behaviour/Reflection/BehaviourReflection.cpp` | Emparejar el árbol de valores de la escena con los campos del script | GCC/Clang y MSVC | Si la etiqueta no coincide, se usa el valor por defecto del campo (Patrón R23) | `fix(reflexion): el valor guardado con otro tipo no se arrastra` |

---

## 20. Decimosexto concepto: **Un `continue` salta el avance del iterador y el bucle no termina**

### 20.1 Patrón

En un bucle que recorre una lista enlazada, el avance al siguiente elemento
suele escribirse **al final del cuerpo**. Un `continue` dentro del cuerpo
salta ese avance: el bucle vuelve a la condición con el **mismo** elemento y
no progresa nunca.

### 20.2 Síntomas

- La aplicación se cuelga (no termina) en una operación concreta.
- El bucle repite el mismo trabajo: se ven crear y destruir el mismo recurso
  una y otra vez.
- El cuelgue depende del estado (si la condición que dispara el `continue` se
  cumple), así que no aparece siempre.

### 20.3 Causa

El avance del iterador estaba al final del cuerpo del `while`, y una guarda
interior usaba `continue` para saltarlo.

### 20.4 Fix canónico (Patrón R24 — el avance va en la cláusula de incremento)

El avance del iterador se escribe en la **cláusula de incremento** del `for`.
Así, ninguna sentencia del cuerpo puede saltárselo: el `continue` pasa a la
siguiente vuelta, que avanza el iterador por construcción.

```cpp
for (auto* pos = lista->first(); pos && pos->getElement();
     pos = (pos != lista->last()) ? lista->next(pos) : nullptr) {
    ...
}
```

Además, la guarda que significa "no hay backend" se evalúa **antes** de crear
o enlazar recursos: salir del bucle a mitad de una operación deja el estado
gráfico a medias.

### 20.5 Checklist de mitigación

- [ ] Los bucles sobre listas del motor avanzan en la cláusula de incremento.
- [ ] Ningún `continue` en un bucle con avance manual al final del cuerpo.
- [ ] Las guardas que abandonan el bucle se evalúan antes de tocar recursos.
- [ ] Cuando el fallo depende del estado, hay un test headless o una sonda
      que lo reproduce de forma determinista.

### 20.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Rendering/SceneRenderer.cpp` | Dibujar las vistas previas de cámara en el editor | GCC/Clang y MSVC | El avance va en la cláusula de incremento del `for` y la guarda se evalúa antes de crear el objetivo de render (Patrón R24) | `fix(render): el avance del iterador no puede quedar detras de un continue` |

---

## 21. Decimoséptimo concepto: **Una comprobación de disponibilidad más débil que la inicialización que la respalda**

### 21.1 Patrón

Un subsistema carga funciones por puntero (OpenGL, Vulkan, un driver) y expone
dos cosas: el resultado de la carga (qué funciones se resolvieron) y una
consulta de disponibilidad. Si la consulta comprueba **menos** que la
carga, entonces hay un estado en el que la carga falló y la consulta dice que
todo está bien.

### 21.2 Síntomas

- Un fallo que solo aparece en máquinas con un driver concreto: el motor
  sigue el camino de dibujo y llama a un puntero de función **nulo**.
- El symptoms no apunta al subsistema: el crash ocurre dentro de la llamada
  nula, lejos de la causa.

### 21.3 Causa

La carga comprobaba los treinta punteros que el motor usa, pero la consulta de
disponibilidad miraba **uno solo**.

### 21.4 Fix canónico (Patrón R25 — la consulta de disponibilidad es el resultado de la carga)

La carga guarda su resultado en una variable y la consulta la devuelve tal
cual. Las dos respuestas no pueden discrepar.

```cpp
inline bool inicializado = false;

inline bool init() {
    // ... carga de las funciones ...
    inicializado = pfnA && pfnB && /* todas las que se usan */;
    return inicializado;
}

inline bool available() { return inicializado; }
```

### 21.5 Checklist de mitigación

- [ ] La consulta de disponibilidad devuelve el resultado de la carga, no una
      comprobación parcial propia.
- [ ] Toda función cargada por puntero se consulta antes de usarla.
- [ ] Un estado "no disponible" se comprueba **antes** de crear o enlazar
      recursos.

### 21.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Rendering/Backend/GLFuncs.h` | Decidir si el backend de OpenGL puede dibujar | GCC/Clang y MSVC | `available()` devuelve el resultado de `init()`, que exige las treinta funciones (Patrón R25) | `fix(render): la disponibilidad del backend comprueba todas las funciones` |

---

## 22. Decimoctavo concepto: **Un framebuffer que no se limpia descarta lo que se dibuja**

### 22.1 Patrón

Un renderizador dibuja sobre el framebuffer de la ventana y confía en su
contenido inicial: no limpia el color ni la profundidad y espera que el driver
deje el buffer de profundidad en 1.0 (el valor que hace que el test de
profundidad deje pasar todo lo que se dibuja).

No todos los drivers lo garantizan. Cuando el valor inicial no es 1.0, el test
de profundidad (`LESS`) rechaza **todos** los fragmentos: no se ve el cielo ni
la geometría, y la ventana queda en negro.

### 22.2 Síntomas

- La ventana del juego exportado abre completamente negra; la misma escena, en
  el editor, se ve bien.
- El log no dice nada: no hay error ni excepción, solo píxeles negros.
- El fallo depende de la máquina: en un driver el buffer sí arranca en 1.0 y la
  escena aparece; en otro no.

### 22.3 Causa

El bucle del juego exportado no limpiaba el framebuffer. El editor sí lo hacía
(`clearScreen` al comienzo del frame), así que el mismo motor, con dos puntos
de entrada, tenía uno que limpiaba el frame y otro que no.

### 22.4 Fix canónico (Patrón R26 — el frame arranca limpio)

Cada frame empieza limpiando color y profundidad, con el color que sale del
perfil de apariencia. La limpieza vive en un helper compartido
(`FondoEscena::limpiar`) para que el editor y el juego exportado arranquen el
frame igual:

```cpp
inline void limpiar(Rendering::Backend::IRenderBackend& backend,
                    const Apariencia& apariencia) {
    float fondo[3];
    color(apariencia, fondo);
    backend.setClearColor(fondo);
    backend.clearScreen(nullptr);
}
```

### 22.5 Checklist de mitigación

- [ ] Cada frame empieza con una limpieza de color **y** de profundidad.
- [ ] El color de la limpieza sale del perfil de apariencia (una sola fuente),
      no de un valor suelto repetido en dos puntos de entrada.
- [ ] Editor y juego exportado usan el mismo helper de limpieza.

### 22.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Runtime/main_juego.cpp` | Limpiar el framebuffer al inicio del frame del juego exportado | Captura de pantalla + lectura de píxeles (`glReadPixels`) | El frame limpia color y profundidad con `FondoEscena::limpiar` antes de `scene->gameScene()` (Patrón R26) | `fix(export): el juego exportado limpia el frame antes de dibujar` |

---

## 23. Decimonoveno concepto: **Lo que solo compila otra herramienta no lo compila nadie**

### 23.1 Patrón

Una unidad de traducción que el build del motor deja fuera (porque es el punto
de entrada de otra herramienta, que genera su propio proyecto) queda sin
compilarse en ninguna parte: ni en la suite, ni en la CI, ni en el build de
trabajo. Sus errores aparecen recién cuando el usuario exporta, con el
toolchain de la plataforma que exporta.

Si además el proyecto generado hereda los flags del build del motor (por
ejemplo los sanitizadores, que el toolchain de exportación no trae), el fallo
pasa de "no compila" a "no enlaza".

### 23.2 Síntomas

- La suite en verde y la CI en verde, y el juego exportado no compila.
- Los errores son del bloque de la plataforma que exporta (`#ifdef _WIN32`),
  así que ni siquiera se ven en Linux.
- El enlazado falla con `cannot find -lasan`: el proyecto generado activó los
  sanitizadores que el motor activa por defecto.

### 23.3 Causa

`src/Runtime/main_juego.cpp` estaba fuera de la librería del motor
(`list(REMOVE_ITEM SOURCES ...)`) y solo lo compilaba el proyecto que genera
el exportador. Nadie compilaba esa unidad en la CI, así que sus errores se
acumulaban en silencio; y el proyecto generado heredaba `ENABLE_ASAN=ON`, que
el toolchain MinGW no soporta.

### 23.4 Fix canónico (Patrón R27 — lo que solo compila otra herramienta, compílalo también en el motor)

- El motor compila la unidad en un target propio (sin enlazar), en cada build:
  `runtime-compile-check` compila `src/Runtime/main_juego.cpp` con las mismas
  cabeceras y definiciones que el binario del juego.
- La herramienta que genera el proyecto declara los flags que necesita: el
  proyecto exportado fuerza `ENABLE_ASAN=OFF`, porque los sanitizadores son del
  build de trabajo del motor, no de la exportación.

```cmake
if(NOT BUILD_RUNTIME)
    add_library(runtime-compile-check OBJECT ${SRC_DIR}/Runtime/main_juego.cpp)
    target_link_libraries(runtime-compile-check PRIVATE funshi_engine glfw)
endif()
```

### 23.5 Checklist de mitigación

- [ ] Toda unidad que un target externo compila tiene un target en el build del
      motor que la compila (compilar es lo que detecta los errores; enlazar
      solo añade los de símbolos).
- [ ] El proyecto generado declara sus flags (sanitizadores, tipo de build) en
      vez de heredar los del build de trabajo por descuido.
- [ ] Ese target entra en la CI de las dos plataformas: el error de la
      plataforma que exporta solo se ve allí.

### 23.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Runtime/main_juego.cpp` | Compilar el punto de entrada del juego exportado en Windows | GCC (MinGW) | El runtime se compila en el target `runtime-compile-check` del motor, que la CI construye en Linux y Windows (Patrón R27) | `ci: el runtime del juego exportado se compila en el build del motor` |
| 2 | `FunshiEngineGL/src/Exportador/GameExporter.cpp` | Enlazar el binario del juego exportado con el toolchain MinGW | CMake + GCC (MinGW) | El proyecto generado fuerza `ENABLE_ASAN=OFF`: el enlace pedía `libasan`, que el toolchain no trae (Patrón R27) | `fix(export): el juego exportado enlaza con el toolchain de la plataforma` |

---

## 24. Vigésimo concepto: **El resultado de una operación se dibuja solo mientras la operación corre**

### 24.1 Patrón

Un panel de la interfaz arranca una operación larga (compilar, exportar,
empacar) y While dura, dibuja su progreso y su bitácora. Al terminar, la
operación escribe su resultado (éxito o error) en un campo y **apaga la
bandera de «en curso»**. El panel dibuja lo que esa bandera decide: con la
bandera apagada vuelve a dibujar el formulario, y el mensaje —la única traza
de lo que pasó— queda en un campo que ya nadie lee.

El resultado es que un fallo se ve exactamente igual que un clic que no hizo
nada. Y si la operación ni siquiera llegó a empezar (porque le faltaba un
dato), el usuario no tiene forma de saberlo: el log tampoco lo dice, porque
la operación no se announcement.

### 24.2 Síntomas

- Al pulsar el botón, la interfaz «no hace nada»: el formulario sigue ahí,
  sin progreso y sin error.
- El log del motor no muestra ninguna traza de la operación (nunca empezó) o
  muestra que empezó pero no dice cómo terminó.
- El mismo fallo se reproduce siempre por el mismo camino (por ejemplo, solo
  cuando el editor arrancó con un parámetro concreto).

### 24.3 Causa

Dos cosas juntas:

1. El panel decide qué dibujar con la bandera «en curso» en lugar de con el
   estado de la operación: el resultado final no tiene fase propia y se
   pierde en cuanto la operación acaba.
2. El dato que la operación necesita (el proyecto del que exportar) solo
   llegaba por el camino interactivo (elegir el proyecto en el menú). En los
   demás caminos —por ejemplo, arrancar el editor con el proyecto ya
   indicado— el panel lo recibía vacío, así que la operación se rechazaba
   antes de empezar, y el motivo se perdía con ella.

### 24.4 Fix canónico (Patrón R28 — el resultado se conserva y se muestra)

El estado de la operación vive en una clase de dominio, sin interfaz, con
fases explícitas; el panel dibuja lo que la fase dice:

```cpp
enum class Fase { Configuracion, Exportando, Resultado };
enum class Resultado { Pendiente, Exito, Error };
```

- `finalizar(exito, mensaje)` deja la fase en `Resultado` y **conserva** el
  mensaje: el panel tiene una vista para ese estado y dibuja el motivo (en
  verde o en rojo) hasta que el usuario lo cierra.
- Las precondiciones se comprueban **antes** de arrancar y su fallo deja el
  motivo en el estado (no en un `return` que se lleva el texto).
- El dato que la operación necesita se propaga por todos los caminos que la
  pueden abrir: el gestor de proyectos lo refleja en la barra de menú también
  cuando el proyecto se resuelve al arrancar, no solo cuando el usuario lo
  elige a mano.
- La operación se anuncia en el log (stdout) al empezar: así, «¿arrancó?» se
  responde desde el log y no desde la impresión de la interfaz.

### 24.5 Checklist de mitigación

- [ ] El estado de la operación tiene una fase de «terminada» que sobrevive al
      final, con el resultado (éxito/error) y el motivo.
- [ ] El mensaje se dibuja en **todas** las fases donde existe, no solo
      mientras la operación corre.
- [ ] Las precondiciones se comprueban antes de arrancar y su motivo queda
      en el estado, a la vista.
- [ ] Los datos que la operación necesita llegan por todos los caminos que
      pueden abrirla (arranque con parámetros, cambio de proyecto, etc.).
- [ ] La operación deja traza en el log al empezar y al terminar.

### 24.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/GUI/Export/ExportDialog.cpp` y `FunshiEngineGL/src/Proyectos/GestorDeProyectos.cpp` | Exportar el juego standalone desde el diálogo de exportación | Log de la sesión del editor (sin traza `[Exporter]`) + sonda local que replica el flujo del diálogo | El estado pasa a `EstadoExportacion` con fase de resultado y el motivo siempre visible; el gestor refleja el proyecto en la barra de menú también al arrancar con `--proyecto` (Patrón R28) | `fix(exportacion): el proyecto llega al dialogo y el resultado no se pierde` |
| 2 | `FunshiEngineGL/src/Exportador/GameExporter.cpp` y `FunshiEngineGL/src/FileManager/Proceso.cpp` | Exportar el juego standalone: la salida de la compilación se perdía (solo se pintaba en la terminal del editor) | Compilación observada en la terminal durante la exportación (el usuario no podía revisarla después) | La exportación deja su traza en un archivo propio (`logs/Exportacion_*.log`) y el mensaje final dice la ruta; las etapas de la compilación se agregan al final del mismo archivo en vez de pisarse (Patrón R28) | `feat(exportacion): la traza de la exportación queda en un archivo` |

---

## 25. Vigesimoprimer concepto: **Las dependencias de una biblioteca son dependencias de quien la usa**

### 25.1 Patrón

Un binario que se lleva consigo sus bibliotecas tiene que llevar consigo, no
solo las que importa él, sino **todas las que importan las que él importa**. El
cierre es transitivo: si A importa B y B importa C, el paquete necesita A, B y
C.

Copiar solo el primer nivel (la tabla de importaciones del ejecutable) deja el
paquete incompleto: el juego arranca y falla al pedir la segunda dependencia.

### 25.2 Síntomas

- El juego exportado arranca y el sistema pide una biblioteca que no está en la
  carpeta (típicamente `zlib1.dll` o `libminizip-1.dll`, que pide la biblioteca
  de modelos).
- Las bibliotecas que el ejecutable importa directamente **sí** están en la
  carpeta: el error no viene de la copia, viene de lo que faltaba.
- El fallo se reproduce en cualquier máquina, no depende del entorno.

### 25.3 Causa

El copiador de dependencias copiaba lo que el ejecutable importaba, sin mirar
qué importan esas DLL. En Windows, las bibliotecas de terceros (assimp) se
enlazan contra bibliotecas del sistema (zlib, minizip): el enlazador deja esas
dependencias en la DLL que las usa, no en el ejecutable, asi que no aparecen en
su tabla de importaciones.

### 25.4 Fix canónico (Patrón R29 — el cierre de dependencias es transitivo)

- El ejecutable se lee como lo que es: un PE con su tabla de importaciones, de
  donde salen los nombres exactos (con su version) de las DLL que pide.
- El cierre se calcula **recursivamente**: por cada DLL copiada se leen sus
  importaciones y se copian tambien, con un conjunto de vistas para no repetir
  ni ciclar.
- Las DLL que trae el sistema no se copian (las tiene el sistema operativo), y
  las que no se encuentran quedan avisadas en el log: el juego las pedira al
  arrancar.

### 25.5 Checklist de mitigación

- [ ] Las dependencias se leen del binario (no de una lista escrita a mano que
      se queda vieja con cada version de las bibliotecas).
- [ ] El cierre es transitivo: se recorren tambien las importaciones de cada
      DLL copiada.
- [ ] Hay un conjunto de vistas (evita ciclos y trabajo repetido).
- [ ] Las DLL del sistema no se copian, ni las que taparian la version del
      sistema (por ejemplo, una `opengl32.dll` de software en lugar de la del
      driver).
- [ ] Lo que no se encuentra queda avisado en el log.

### 25.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Exportador/GameExporter.cpp` | Empaquetar el juego exportado con las DLL que necesita | Ejecucion del `.exe` exportado (pide `zlib1.dll` y `libminizip-1.dll`) + `objdump -p` del ejecutable y de las DLL copiadas | `ImportacionesPE` (lee la tabla de importaciones) + cierre transitivo de dependencias (Patrón R29) | `fix(export): el juego exportado lleva sus dependencias` |

---

## 26. Vigesimosegundo concepto: **Dos operaciones que compilan necesitan su propio directorio de trabajo**

### 26.1 Patrón

Cuando una operacion compila, escribe en un directorio de trabajo. Si ese
directorio tiene un nombre fijo (por ejemplo, `<salida>_build`), dos operaciones
que se solapan comparten el mismo sitio: la segunda vacia el directorio
mientras la primera compila, y la primera falla con errores de compilacion
que no tienen nada que ver con el codigo que esta compilando.

El sintoma caracteristico: `fatal error: opening dependency file ... No such
file or directory` en archivos que si existen, y errores en archivos que
**no** tocaron el cambio que se estaba haciendo.

### 26.2 Síntomas

- Una exportacion falla a mitad de la compilacion, con errores de
  "dependency file" en archivos que existen y que nadie toco.
- La exportacion funciona si se espera a que termine la otra, y falla si se
  lanzan a la vez.
- Los errores apuntan a archivos que no tienen relacion con el cambio: el
  fallo parece del codigo que se compila, y no es.

### 26.3 Causa

El directorio de compilacion se derivaba del nombre de la salida con un
sufijo fijo, y la exportacion lo vaciaba (`remove_all`) al empezar. Con dos
exportaciones simultaneas (la del editor y una sonda, por ejemplo), la
segunda borra el directorio de la primera mientras compila.

### 26.4 Fix canónico (Patrón R30 — cada operación que compila, con su directorio)

- El directorio de compilacion es **propio de cada operacion** y se crea
  de forma **atomica** (`create_directory`, que falla si ya existe), asi que
  dos procesos que exporting a la vez no obtienen el mismo nombre.
- Vive en la **carpeta temporal del sistema**: es de usar y tirar, y ahi no se
  acumulan los restos de las exportaciones fallidas.
- La exportacion lo borra al terminar bien; si falla, el directorio queda en la
  carpeta temporal (que el sistema limpia sola).

### 26.5 Checklist de mitigación

- [ ] Ninguna operacion que compile usa un directorio de trabajo con nombre
      fijo.
- [ ] El directorio se reserva de forma atomica (crear el directorio es el
      que decide quien lo usa).
- [ ] El directorio es de usar y tirar (temporal), y no deja restos en la
      carpeta del usuario.
- [ ] Cuando se borra un directorio de trabajo, se comprueba antes que no haya
      otra operacion usandolo.

### 26.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta | Fix Aplicado | Commit |
|---|-----------|-----------|-------------|--------------|--------|
| 1 | `FunshiEngineGL/src/Exportador/GameExporter.cpp` | Exportar el juego standalone (dos exportaciones a la vez) | Log de la exportación (`logs/Exportacion_*.log`): errores `opening dependency file ... No such file or directory` en varios archivos | `GameExporter::directorioBuildTemporal()`: directorio propio, reservado de forma atómica, en la carpeta temporal del sistema (Patrón R30) | `fix(exportacion): cada exportacion compila en su propio directorio` |

---

## 27. Vigesimotercer concepto: **El portapapeles de archivos del sistema no es texto**

### 27.1 Patrón

El portapapeles del sistema operativo guarda **archivos**, no texto, y cada
plataforma los guarda en un formato distinto:

- **Windows**: un `CF_HDROP`, es decir un bloque `DROPFILES` con una cabecera
  (el desplazamiento de la lista y dos banderas) y la lista de rutas en ANSI o
  en UTF-16 terminada en doble null. Es un **formato binario con su propia
  estructura**, no una cadena.
- **Linux**: un `text/uri-list`, es decir texto con una URI `file://` por
  linea (con porcentajes escapados en el nombre).

La lectura de "el texto del portapapeles" que dan la mayoría de las
bibliotecas de ventanas no llega al portapapeles de archivos: en Windows
devuelve vacío o nulo cuando lo que hay copiado son archivos, y en Linux
devuelve la URI, no la ruta.

### 27.2 Síntomas

- Pegar con `Ctrl+V` no hace nada, aunque en el explorador del sistema los
  archivos estén copiados.
- El portapapeles devuelve texto que no son rutas (`file:///home/...`) y la
  copia falla con "no se encontró el archivo".
- En Windows, una ruta con acentos o con caracteres fuera del plano básico
  llega corrupta: el portapapeles los entrega en UTF-16 y el motor trabaja en
  UTF-8.

### 27.3 Causa

Suponía que el portapapeles de archivos era texto plano, igual que el
portapapeles de un editor. No lo es: cada sistema operativo publica un
formato propio, y el que publica el shell de Windows es binario.

### 27.4 Fix canónico (Patrón R31 — un lector por plataforma detrás de un contrato)

- El dominio **no habla con la API del sistema**: pide las rutas a un
  contrato (`IServicioPortapapeles`) cuya implementación es de la plataforma.
- Cada implementación **traduce el formato del sistema a una lista de
  rutas**: el bloque `DROPFILES` en Windows, el `text/uri-list` (y, como
  respaldo, la lista de rutas planas) en Linux.
- La traducción va en **funciones puras** (entra un bloque de bytes o una
  cadena y sale una lista de rutas), con lo que se prueban en headless y en
  las dos plataformas sin necesitar un portapapeles real.
- Las rutas en UTF-16 se **convierten a UTF-8** al leerlas: el motor trabaja
  en UTF-8 y las comparaciones byte a byte de rutas fallarían si cada una
  Midiera con su propia codificación.

### 27.5 Checklist de mitigación

- [ ] Nadie llama "al texto del portapapeles" esperando archivos.
- [ ] El formato de cada plataforma está en una función pura y probada, no
      repartido por el código que la usa.
- [ ] La conversión de codificación ocurre al **leer**, no al comparar.
- [ ] El caso "no hay portapapeles" (servidor sin escritorio) degrada a un
      aviso, no a un fallo.

### 27.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|---------------|--------------|--------|
| 1 | `FunshiEngineGL/src/FileManager/PortapapelesSistema.cpp` | Pegar con `Ctrl+V` los archivos del sistema | Windows (`CF_HDROP`) / Linux (`text/uri-list`) | Contrato `IServicioPortapapeles` + lector por plataforma + parsers puros en `PortapapelesParsers.h` (Patrón R31) | `feat(explorador): arrastrar y pegar archivos del sistema` |

---

## 28. Vigesimocuarto concepto: **El drop del sistema llega fuera del frame de la GUI**

### 28.1 Patrón

La biblioteca de ventanas entrega el drop de archivos en su **bucle de
eventos**, no en el frame de dibujado. En ese instante la vista todavía no
sabe qué carpeta está viendo, porque la carpeta visible es un dato del frame
(la ha elegido el usuario en la navegación).

### 28.2 Síntomas

- Los archivos soltados caen en una carpeta fija, o en la carpeta que había
  cuando se solvable la ventana, y no en la que el usuario tenía delante.
- La vista se modifica desde el callback del sistema y el recorrido de la
  carpeta se invalida a mitad (elementos fantasma, punteros colgando).
- El destino se decide en el sitio del drop y el usuario no puede corregirlo.

### 28.3 Causa

Trató el callback del drop como si fuera un frame de la GUI: en él se
consultaba el estado de la vista y se escribía en ella. Pero en el callback
ese estado es del frame anterior (o ninguno).

### 28.4 Fix canónico (Patrón R32 — el sistema encola, la vista drena)

- El servicio del sistema **encola** las rutas y no toca la vista.
- La vista **drena la cola en su frame**, y es entonces cuando sabe la
  carpeta visible: el destino se calcula ahí, con el dato fresco.
- La cola es la frontera entre el hilo de eventos del sistema y el frame de
  la GUI, así que está protegida (el sistema puede entregar el drop desde su
  propio hilo).

### 28.5 Checklist de mitigación

- [ ] El callback del sistema no lee ni escribe el estado de la vista.
- [ ] El destino de una operación se calcula en el frame, no en el callback.
- [ ] La cola entre el sistema y la GUI está protegida si los hilos difieren.
- [ ] El sistema que entrega los eventos (ventana) y el que los consume
      (vista) son piezas distintas, con un contrato entre ellas.

### 28.6 Registro de Instancias

| # | Ubicación | Operación | Herramienta/SO | Fix Aplicado | Commit |
|---|-----------|-----------|---------------|--------------|--------|
| 1 | `FunshiEngineGL/src/FileManager/ReceptorDropGlfw.cpp` + `ContentFolderInterface::contentGUI()` | Soltar archivos del explorador del sistema sobre la ventana del editor | GLFW (drop de ventana) | Contrato `IServicioDrop`: el servicio encola y la vista drena en su frame, donde conoce la carpeta visible (Patrón R32) | `feat(explorador): arrastrar y pegar archivos del sistema` |

---

## 29. Historial de Cambios

| Fecha | Autor | Cambio |
|-------|-------|--------|
| 2026-09-27 | Gianfranco Ivan Enrique | Creación del documento; registro de instancias #1–3; definición de plantilla y checklist para agentes. |
| 2026-10-04 | Gianfranco Ivan Enrique | Instancia #4: la biblioteca horneada de la JVM se pasaba donde se esperaban raíces de JDK y se descartaba siempre. Checklist ampliada con el tipo de dato horneado. |
| 2026-09-27 | Gianfranco Ivan Enrique | Añadido el segundo concepto (Patrón R8, escritura rechazada en el directorio de instalación) con su registro de instancias, a raíz del crash al asignar un script en el binario instalado. |
| 2026-09-27 | Gianfranco Ivan Enrique | Añadido el tercer concepto (Patrón R9, offsets engañosos de `tellg`/`seekg` en streams de texto con CRLF) con su instancia #1, a raíz del bug de los objetos fantasma "Scene" (H-17). |
| 2026-09-27 | Gianfranco Ivan Enrique | Añadido el cuarto concepto (Patrón R10, `CREATE_UNICODE_ENVIRONMENT` obligatorio con bloques UTF-16 en `CreateProcessW`) con su instancia #1, a raíz del error 87 al pasar el entorno de vcvars/variable extra (H-3 nivel 2). |
| 2026-09-28 | Gianfranco Ivan Enrique | Añadido el quinto concepto (Patrón R11, una imagen cargada bloquea su archivo en Windows) con su instancia #1, a raíz del `Permission denied` de `ld` al haber dos objetos sobre el mismo script (H-20). |
| 2026-09-28 | Gianfranco Ivan Enrique | Añadida instancia #4 al primer concepto: drop entre paneles (ShowFolder → BrowseFile y ShowFolder → ShowFolder carpeta distinta) con invalidación explícita de cache grid en origen y destino. |
| 2026-09-29 | Gianfranco Ivan Enrique | Añadido el sexto concepto (Patrón R12, un contexto Core Profile rechaza todo dibujo sin VAO ligado) con su instancia #1, a raíz del cielo degradado que no se dibujaba en un contexto 4.6 core. |
| 2026-10-04 | Gianfranco Ivan Enrique | Añadido el octavo concepto (Patrón R14, un valor horneado del build vale en su máquina y en ninguna más) con sus instancias #1–#2, a raíz de los scripts C++ que no compilaban en el paquete instalado; registrada la instancia #5 del segundo concepto (instalación por usuario, sin UAC). |
| 2026-10-03 | Gianfranco Ivan Enrique | Añadido el séptimo concepto (Patrón R13, una migración de compatibilidad se lleva las carpetas del motor) con su instancia #1, y registrada la instancia #4 del segundo concepto (Patrón R8, escritura rechazada en el directorio de instalación) a raíz del log de arranque que no se escribía instalado en `Program Files`. |
| 2026-10-11 | Gianfranco Ivan Enrique | Añadido el Patrón R15 (el separador nativo manda al comparar rutas) a la tabla de patrones: `std::filesystem` arma `\logs` en Windows y los tests comparaban contra literales con `/`, lo que dejaba 6 comprobaciones rojas solo en windows-latest. |
| 2026-10-11 | Gianfranco Ivan Enrique | Añadido el Patrón R16 (la resolución del `mtime` depende del toolchain, no del FS): la lectura del CRT de MinGW viene de `stat` (1 segundo) y su setter no aplica sobre directorios, de modo que el fallback de polling del `FileSystemWatcher` no observa cambios subsegundo ahi. El ctor del watcher ahora siembra el mtime real de la raíz en vez del epoch (evita un rescaneo vano al arrancar) y el intervalo quedó inyectable para poder probar el contrato. |
| 2026-10-16 | Gianfranco Ivan Enrique | Añadido el noveno concepto (Patrón R17, el motor web multiproceso relanza tu binario): `CefExecuteProcess` al comienzo de `main` en editor y juego exportado, con init tras GL, bombeo por frame y apagado antes de romper ImGui/GL. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el décimo concepto (Patrón R18, un archivo generado en tiempo de configuración vale en una configuración y en ninguna más) con su instancia #1, a raíz del fallo de configuración del build de Windows con Visual Studio: el header con las rutas de los plugins de prueba se generaba en un archivo compartido y CMake abortaba al escribirlo una vez por configuración. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el undécimo concepto (Patrón R19, un tipo declarado con un class-key distinto al de su definición produce dos símbolos distintos) con sus instancias #1–#2, a raíz del LNK2019 al enlazar el motor con Visual Studio: `Mesh` e `Image` se adelantaban como `class` y se definen como `struct`, y MSVC distingue los dos nombres de símbolo. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el duodécimo concepto (Patrón R20, el nombre de un tipo persistido depende del compilador) con sus instancias #1–#5, a raíz del fallo de `prefab-tests` en Windows: MSVC escribe `"class Transform"` en `typeid().name()` y el recorte manual del mangling de GCC truncaba los nombres con `E`. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el decimotercer concepto (Patrón R21, una biblioteca de terceros compilada sin RTTI invalida el `dynamic_cast`) con sus instancias #1–#2, a raíz del aborto de `tilemap-tests` en Windows: el paquete de Bullet de vcpkg se compila con `USE_MSVC_DISABLE_RTTI=ON` y el `dynamic_cast` sobre una shape leía un typeinfo nulo. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el decimocuarto concepto (Patrón R22, una longitud leída del binario usada como tamaño de reserva) con sus instancias #1–#3: las longitudes de la ruta del script, del nombre del clip y del árbol de valores se acotaban y se contrastaban con el contenido restante; sin cota, un archivo corrupto abortaba el motor con `std::length_error`. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el decimoquinto concepto (Patrón R23, la etiqueta de un valor guardado deja de corresponder a su campo) con su instancia #1: un script que cambia el tipo de un campo dejaba el `std::variant` del inspector con la alternativa vieja y el render de ImGui abortaba con `std::bad_variant_access`. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el decimosexto concepto (Patrón R24, un `continue` salta el avance del iterador y el bucle no termina) con su instancia #1: la pasada de vistas previas del editor giraba sobre el mismo elemento si el backend no estaba disponible. |
| 2026-10-20 | Gianfranco Ivan Enrique | Añadido el decimoséptimo concepto (Patrón R25, una comprobación de disponibilidad más débil que la inicialización que la respalda) con su instancia #1: `available()` del backend de OpenGL comprobaba una sola de las treinta funciones que la carga exige. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadido el decimoctavo concepto (Patrón R26, el frame arranca limpio) con su instancia #1: el juego exportado no limpiaba el framebuffer, el buffer de profundidad no arrancaba en 1.0 en el driver de la máquina y la ventana se quedaba en negro. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadido el decimonoveno concepto (Patrón R27, lo que solo compila otra herramienta no lo compila nadie) con sus instancias #1–#2: el punto de entrada del juego exportado quedaba fuera de todo build (lo compilaba solo el exportador) y el proyecto generado heredaba los sanitizadores del motor, que el toolchain de exportación no trae. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadido el vigésimo concepto (Patrón R28, el resultado de una operación se conserva y se muestra) con su instancia #1: el diálogo de exportación dibujaba el motivo solo mientras la exportación corría, y con el editor arrancado con `--proyecto` el diálogo recibía el proyecto vacío: la exportación se rechazaba y el fallo se perdía. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadido el vigesimoprimer concepto (Patrón R29, las dependencias de una biblioteca son dependencias de quien la usa) con su instancia #1: el juego exportado se llevaba solo las DLL que importaba él, y las que importan esas (zlib y minizip, de la biblioteca de modelos) se quedaban fuera: al arrancar, el sistema las pedía. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadido el vigesimosegundo concepto (Patrón R30, dos operaciones que compilan necesitan su propio directorio de trabajo) con su instancia #1: la exportación compilaba en `<salida>_build`, un nombre fijo que se vaciaba al empezar; dos exportaciones simultáneas se pisaban y la primera fallaba con errores de «dependency file» que no tenían relación con el código. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadida la segunda instancia del patrón R28 (el resultado de una operación se conserva y se muestra): la salida de la compilación de la exportación solo se pintaba en la terminal del editor, así que no se podía revisar; ahora queda en un archivo de traza propio, cuya ruta anuncia el mensaje final. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadidos el vigesimotercer y el vigesimocuarto conceptos (Patrón R31, el portapapeles de archivos del sistema no es texto; Patrón R32, el drop del sistema llega fuera del frame de la GUI) con su instancia #1 cada uno: el portapapeles de archivos se publica como un bloque `CF_HDROP` en Windows y como un `text/uri-list` en Linux, y el drop lo entrega la biblioteca de ventanas en su bucle de eventos, cuando la vista todavía no sabe qué carpeta está viendo. |
| 2026-10-21 | Gianfranco Ivan Enrique | Añadidas las instancias #5–#6 del patrón R7 (invalidación explícita de caché): el menú "Copy Exist File" copiaba el archivo sin invalidar el caché del grid, y la importación desde el sistema tampoco lo hacía. |

---

*Este documento es vivo: cada nuevo bug de esta clase debe registrarse en la tabla de su concepto (§2 para el primero, §6.5 para el segundo, §7.5 para el tercero, §8.5 para el cuarto, §9.5 para el quinto), §14.6 para el décimo, §15.6 para el undécimo, §16.6 para el duodécimo, §17.6 para el decimotercer, §18.6 para el decimocuarto, §19.6 para el decimoquinto, §20.6 para el decimosexto, §21.6 para el decimoséptimo, §22.6 para el decimoctavo, §23.6 para el decimonoveno, §24.6 para el vigésimo, §25.6 para el vigesimoprimer y §26.6 para el vigesimosegundo; si revela un patrón nuevo, añadirse a §4.*
## CEF: "Invalid file descriptor to ICU data received" en InitializeICU

- **Síntoma**: al arrancar el motor con `FUNSHI_CEF=ON`, `CefInitialize` aborta en
  `base::i18n::InitializeICU()` con `Invalid file descriptor to ICU data received`
  (SIGTRAP). Build y tests pasan; solo falla el arranque real.
- **Evidencia**: `gdb` backtrace muestra el fallo en `CefInitialize` →
  `content::ContentMainRunnerImpl::Initialize` → `base::i18n::InitializeICU`.
  `icudtl.dat` está junto al binario y es válido (magic `90 00 da 27`).
- **Causa probable**: diferencia entre la sonda funcional (`sondas/cef_probe/`) y
  el motor en el arranque de CEF (argv[0]/cwd, archivos auxiliares como
  `v8_context_snapshot.bin`, `chrome_*.pak`, o la configuración de `CefSettings`).
  La sonda sí arranca; el motor no, pese a tener los mismos recursos.
- **Causa real (resuelta 2026-10-17)**: CEF busca `icudtl.dat` (y el resto de
  recursos) **en el directorio de `libcef.so`**, no junto al ejecutable. En el
  build, `libcef.so` vive en `build-cef/cef/.../Release/` mientras que los
  recursos se copiaban a `build-cef/` (junto al binario). Por eso
  `InitializeICU` recibía un fd inválido.
- **Fix**: copiar `icudtl.dat` (y `v8_context_snapshot.bin`, `resources.pak`,
  `chrome_*.pak`, `locales/`) al directorio de `libcef.so`. Después de eso el
  motor arrancó con `[webui] Backend activo: cef`.
- **Estado**: resuelto para el build de desarrollo; el POST_BUILD de CMake debe
  copiar los recursos junto a `libcef.so` (no solo junto al ejecutable) para que
  el export y CI funcionen. El warning de `resources.pak` (no encontrado en el
  cwd) es menor y se resuelve copiando también ese archivo.
- **Checklist**: verificar que todos los archivos de la sonda estén copiados
  (`v8_context_snapshot.bin`, `chrome_100/200_percent.pak`, `resources.pak`,
  `icudtl.dat`, `locales/`, `libcef.so`), que `CefExecuteProcess` se llame
  primero en `main`, y que `argv[0]` apunte al binario real.
