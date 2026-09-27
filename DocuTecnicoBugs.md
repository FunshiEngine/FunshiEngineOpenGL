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

> **Nota**: las instancias #2 y #3 no son la causa del crash sino el punto donde
> se manifiesta. La causa es la instancia #1: sin ella no habría divergencia
> entre el árbol de valores guardado y los campos que expone el script. Se
> arreglaron las tres porque el acceso fuera de rango es un defecto real por sí
> mismo: la escena guardada y la reflexión actual pueden discrepar siempre, no
> solo cuando falla la escritura.

---

## 7. Historial de Cambios

| Fecha | Autor | Cambio |
|-------|-------|--------|
| 2026-09-27 | Gianfranco Ivan Enrique | Creación del documento; registro de instancias #1–3; definición de plantilla y checklist para agentes. |
| 2026-09-27 | Gianfranco Ivan Enrique | Añadido el segundo concepto (Patrón R8, escritura rechazada en el directorio de instalación) con su registro de instancias, a raíz del crash al asignar un script en el binario instalado. |

---

*Este documento es vivo: cada nuevo bug de esta clase debe registrarse en la tabla de su concepto (§2 para el primero, §6.5 para el segundo) y, si revela un patrón nuevo, añadirse a §4.*