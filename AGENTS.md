# AGENTS.md — Convenciones para agentes de código

Guía rápida para agentes (y humanos) que trabajen en este repositorio.

## Comandos esenciales

```bash
# Configurar y compilar (CMake es el único build soportado; la solución de
# Visual Studio la genera CMake dentro del directorio de build, no se versiona)
cmake -S FunshiEngineGL -B FunshiEngineGL/build
cmake --build FunshiEngineGL/build -j$(nproc)

# Ejecutar la suite de tests (18 targets headless + scripts-java si hay JDK)
cd FunshiEngineGL/build && ctest --output-on-failure
```

- **Build y tests como parte del cambio**: al tocar código fuente (o `CMakeLists.txt`
  y tests), revisar SIEMPRE si hay que actualizar el build y la suite: el engine
  usa `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`, pero los targets de test listan
  sus fuentes explícitamente; un `.cpp`/`.h` nuevo, un include, una dependencia,
  un `add_test` o el conteo de comprobaciones de un test existente pueden quedar
  fuera de sincronía. Ajustarlos en el mismo commit que el código que los motiva.

## Convenciones del código

- **Idioma**: C++17. Identificadores, comentarios y logs en español
  (`fijarPosicion`, `AudioEngine::reproducir`). Nombres de tipo en PascalCase,
  métodos en camelCase.
- **Copyright**: todo archivo fuente nuevo del motor debe llevar el header
  Apache 2.0 (ver cabecera de cualquier `.h`/`.cpp` existente como modelo) o
  al menos `SPDX-License-Identifier: Apache-2.0`.
- **Estilo**: 4 espacios, llaves en la línea siguiente para clases/funciones.
- **Documentación**: los cambios de API visibles para usuarios se reflejan en
  `MANUAL_DE_USO.md` (especialmente la sección 13, scripting). Cambios de
  arquitectura en `PROJECT_STRUCTURE.md`.

## Reglas de arquitectura

- **Análisis previo obligatorio**: antes de crear o modificar código, analizar
  las excepciones, estructuras de datos, patrones de diseño y arquitecturas ya
  presentes en el proyecto y reutilizarlas; si el cambio requiere algo nuevo
  o distinto, recomendarlo y justificarlo antes de implementarlo.
- **Tablas de API para scripts (`ApiScriptGameObject`, `ScriptServices`)**:
  estrictamente APPEND-ONLY. Los campos nuevos van SIEMPRE al final, antes del
  campo `int version`; nunca reordenar ni borrar entradas. El campo `version`
  se incrementa al agregar funcionalidad.
- **Modularidad**: los módulos se comunican por tablas de punteros a función
  (patrón de `ScriptGameObject.cpp`, implementado en una TU del ejecutable) o
  por inyección explícita (`GameScene::inyectarServiciosScript`). No acoplar
  el módulo de scripts con Audio/Scenes/Input por includes directos.
- **Backends de scripts**: C++ (`BackendCpp`) y Java JNI (`BackendJava`);
  cualquier servicio nuevo debe documentarse si queda C++-only.
- **Serialización binaria**: sin versionado aún; al ampliar un componente,
  mantener compatibilidad de prefijos y acotar lecturas con `std::min`.
- **Física** detrás de la fachada `PhysicsEngine` → `IPhysicsBackend`; no usar
  Bullet fuera de `Fisicas/`.
- **Compatibilidad multiplataforma**: al usar APIs nativas del sistema operativo
  (Windows API, POSIX, filesystem, threading, dynamic loading, etc.), SIEMPRE
  verificar y mantener compatibilidad con **Linux y Windows**. Usar guardas
  `#ifdef _WIN32` / `#else` (Linux/macOS) y aislar el código dependiente de
  plataforma en capas de abstracción o backends dedicados (p. ej.
  `IRenderBackend`, `FileSystemWatcher`, `BackendCpp`/`BackendJava`). No asumir
  que headers o comportamientos de un SO existen en el otro; probar o validar
  compilación cruzada en CI antes de confirmar cambios.

## Flujo de trabajo

- **Lista de tareas antes de escribir código**: si la tarea requiere más de un
  paso (analizar, tocar varios archivos, testear, documentar), armar primero una
  lista de pendientes explícita y trabajar contra ella, no de memoria. La lista
  se mantiene al día de forma incremental: cada tarea se marca como completada
  en cuanto está hecha y verificada, y no todas al final. Si aparece un paso
  nuevo o cambia el alcance, la lista se ajusta en el momento. Esto sirve para
  que el usuario vea el avance real del trabajo y para que el agente no cierre
  una tarea dando por hecho algo que quedó sin hacer.
- **Una tarea, una rama**: al comenzar una tarea de un tema distinto al actual,
  primero commitear los cambios pendientes de la rama actual y luego cambiar de
  rama. Nunca mezclar temas distintos en una misma rama.
- **Ramas remotas primero**: si la tarea requiere una rama nueva, verificar
  antes si en el remoto ya existe una que cumpla ese rol; en ese caso traerla
  al local (`git fetch` + `git checkout -b <rama> origin/<rama>` o
  `git switch`), actualizarla con su base y trabajar sobre ella. Solo crearla
  localmente si no existe.
- **Base correcta de la rama**: al crear una rama, verificar contra la
  documentación del flujo (ver [FLUJO_DE_RAMAS.md](FLUJO_DE_RAMAS.md)) desde
  cuál rama debe partir — `develop`, `release`, `staging` o `test` — para
  mantener el orden de desarrollo; solo usar `master` (o la rama que
  corresponda según ese documento) como base cuando el flujo lo indique.
- **Rama desactualizada: ponerla al día antes de escribir código**: nada más
  crear o traer la rama —y también cada vez que se retome el trabajo en ella—
  medir la distancia con
  `git rev-list --left-right --count origin/<rama>...HEAD` y `git fetch`. Si le
  faltan commits —porque el equipo estuvo mergeando a otra rama, o porque la
  base indicada en el flujo quedó rezagada (pasó con `develop`, que quedó
  muchas releases atrás de `master` mientras los PR seguían entrando a
  `master`)—, actualizarla de inmediato con `git merge --ff-only origin/<rama>`
  cuando la base sea ancestro directo, o `git rebase origin/<rama>` si la rama
  ya tiene commits propios, y recién entonces arrancar el trabajo. Nunca dejar
  que la tarea se siga sobre un árbol viejo ni mezclar el atraso al final: el
  costo (conflictos, archivos borrados que reaparecen, código que ya no compila)
  es siempre mayor al hacerlo al comienzo. El objetivo es siempre trabajar
  contra el estado actual del remoto y no contra una referencia vieja: hacer
  `git fetch` antes de medir, y si la base indicada en el flujo se movió o la
  rama quedó atrás por merges del equipo, actualizarla de inmediato. Si la
  actualización produce
  conflictos por commits ajenos, reportarlos y esperar instrucciones en lugar de
  resolverlos por cuenta propia.
- **Documentación sincronizada**: la documentación no es un extra, es parte de
  la tarea. Antes de empezar, revisar los `.md` que describen el área afectada
  (`README.md`, `MANUAL_DE_USO.md`, `PROJECT_STRUCTURE.md`,
  `FLUJO_DE_RAMAS.md` y los de diseño); durante el trabajo, ir comparando en
  paralelo lo que la documentación afirma contra lo que el código realmente
  hace y corregir todo lo que quedó desactualizado — APIs, arquitectura,
  comandos y conteo de tests, atajos, limitaciones, ejemplos —, además de
  reflejar lo nuevo que se introduce. Los ajustes de documentación entran en
  el mismo commit que el código que los motiva: nunca "la documentación al
  final", ni entregar avances sin sus documentos al día.
- **Commits atómicos por tarea**: cada tarea terminada cierra con su commit
  (o los que sean necesarios si la tarea es grande), con mensaje descriptivo
  y convencionales (`feat:`, `fix:`, `docs:`, `refactor:`, `chore:`).
- **Sin atribuciones ajenas al cambio**: los mensajes de commit (y cualquier
  metadato asociado) deben describir únicamente la implementación o los
  cambios realizados. No se permite adjudicar coautoría, autoría, firmas ni
  menciones a herramientas, asistentes o terceros que no correspondan al
  trabajo concreto sobre el código.
- **Commits solo con archivos propios**: al commitear, agregar únicamente los
  archivos que modificó el agente en la tarea actual (`git add <archivos>`),
  NO usar `git add -A` ni `git add .` que incluyen cambios ajenos sin
  commitear de otros colaboradores. Cada commit debe reflejar solo el trabajo
  concreto realizado.
- **Comandos destructivos requieren permiso y justificación**: antes de
  ejecutar `git checkout`, `git reset`, `git restore`, `git clean` o cualquier
  comando que descarte cambios (staged o unstaged), el agente debe:
  1. Pedir permiso explícito al usuario.
  2. Argumentar detalladamente en español por qué es necesario ese comando
     destructivo y qué cambios se perderán.
  3. Esperar confirmación antes de ejecutarlo.
  Esto evita pérdida accidental de trabajo del usuario o cambios ajenos sin
  commitear.
- Validar antes de commitear: build completo + `ctest` en verde.
- **Build con cambios ajenos**: si el build falla y hay archivos modificados
  por otros colaboradores (no tocados por el agente), reportar el fallo,
  indicar que hay cambios ajenos pendientes, y esperar instrucciones;
  NO modificar archivos ajenos para "arreglar" el build.
- El CI (`.github/workflows/ci.yml`) compila el engine en Ubuntu y corre la
  suite en Linux/Windows/macOS; no pushear sin pasar los tests localmente.
- **NO confirmar fixes hasta validación del usuario**: nunca declarar un bug
  como "resuelto" o "fix real" hasta que el usuario lo pruebe y lo confirme
  explícitamente. Los tests automatizados (ctest) no cubren flujos visuales
  de UI (dock, layout, ventanas); el criterio de aceptación lo define el
  usuario probando la aplicación real.
