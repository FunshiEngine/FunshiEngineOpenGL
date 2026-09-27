# DocuTecnicoBugs — Registro Técnico de Bugs por Limitaciones de Herramientas

> **Propósito**: Documentar bugs cuyo origen no es un error lógico del código, sino una **limitación, comportamiento no obvio o latencia de una herramienta externa** (filesystem, compilador, API gráfica, dependencia, SO, etc.). Sirve para que agentes (y humanos) reconozcan el patrón, apliquen la mitigación conocida y eviten re-investigar lo ya resuelto.

---

## 1. Concepto: **Invalidación Diferida de Cache por Latencia de `mtime` en Filesystem**

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

## 2. Registro de Instancias Conocidas

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

---

## 5. Principio Rector

> **"No esperes a que la herramienta te avise; invalida tú el estado que controlas."**

Cuando tu código **escribe** en un recurso externo (FS, GPU, proceso hijo, red), **tú** eres la fuente de verdad de cuándo ese recurso cambió. Invalida tus caches **en el mismo punto de escritura**, no en el siguiente frame, no en un callback, no en un timer.

---

## 6. Historial de Cambios

| Fecha | Autor | Cambio |
|-------|-------|--------|
| 2026-09-27 | Gianfranco Ivan Enrique | Creación del documento; registro de instancias #1–3; definición de plantilla y checklist para agentes. |

---

*Este documento es vivo: cada nuevo bug de esta clase debe registrarse en §2 y, si revela un patrón nuevo, añadirse a §4.*