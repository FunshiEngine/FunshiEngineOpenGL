## Convenciones del código

- **Idioma**: C++17. Identificadores, comentarios y logs en español
  (`fijarPosicion`, `AudioEngine::reproducir`). Nombres de tipo en PascalCase,
  métodos en camelCase.
- **Copyright**: todo archivo fuente nuevo del motor debe llevar el header
  Apache 2.0 (ver cabecera de cualquier `.h`/`.cpp` existente como modelo) o
  al menos `SPDX-License-Identifier: Apache-2.0`.
- **Estilo**: 4 espacios, llaves en la línea siguiente para clases/funciones.
- **Sin referencias de gestión en los textos del proyecto.** Los comentarios
  del código, la documentación técnica (`README.md`, `doc/MANUAL_DE_USO.md`,
  `doc/PROJECT_STRUCTURE.md`, `DOCUMENTACION.md`, `doc/DocuTecnicoBugs.md` y los
  documentos de diseño), los mensajes de los tests (`CHECK(...)`) y los
  mensajes de log de la consola describen **piezas, comportamiento, contratos y
  causas técnicas, nada más**. No llevan identificadores de tareas o de bugs,
  planes, fases, hitos, fechas, estados ("pendiente", "por hacer") ni alusiones
  a documentos o secciones ajenas al texto (p. ej. "ver §22 del plan",
  "parte de H-19"). Ese contexto pertenece a los documentos de gestión — plan,
  issues, historial de cambios y mensajes de commit/PR —, donde sí debe quedar;
  dentro de los textos del código envejece, porque más adelante nadie sabe qué
  era esa tarea y solo distrae de la lectura. Si una decisión o un texto solo
  se entiende con ese contexto, se explica por sí mismo a partir de lo que hace
  el código.
- **Documentación**: los cambios de API visibles para usuarios se reflejan en
  `doc/MANUAL_DE_USO.md` (especialmente la sección 13, scripting). Cambios de
  arquitectura en `doc/PROJECT_STRUCTURE.md`.
- **Bugs técnicos por limitaciones de herramientas**: documentar en
  `doc/DocuTecnicoBugs.md` (patrón, síntomas, fix canónico, checklist de mitigación).
- **Enums y switch-case**: evaluar de forma recurrente el uso de enumeraciones
  (`enum class`) y `switch-case` para comparaciones y condicionales en lugar
  de cadenas de `if-else` sobre valores enteros, strings o magic numbers.
  Los `enum class` con `switch` exhaustivo (con `default` que lance o loguee)
  permiten al compilador avisar de casos no cubiertos al agregar valores,
  evitan errores de typo en strings, y el código resultante es más legible
  y mantenible. Aplicar al agregar nuevos tipos de componentes, eventos,
  estados de máquinas, tags, o cualquier conjunto cerrado de valores.

## Flujo de trabajo

- **Lista de tareas antes de escribir código**: si la tarea requiere más de un
  paso (analizar, tocar varios archivos, testear, documentar), armar primero una
  lista de pendientes explícita y trabajar contra ella, no de memoria. La lista
  se mantiene al día de forma incremental: cada tarea se marca como completada
  en cuanto está hecha y verificada, y no todas al final. Si aparece un paso
  nuevo o cambia el alcance, la lista se ajusta en el momento. Esto sirve para
  que el usuario vea el avance real del trabajo y para que el agente no cierre
  una tarea dando por hecho algo que quedó sin hacer.

## Fragmentación de tareas y validación de arquitectura

Antes de escribir código nuevo, la tarea general se reduce en la mínima
cantidad de subtareas que, en conjunto, generan la implementación buscada. Cada
subtarea debe ser lo bastante pequeña para que compila por sí sola y tenga su
propia validación (build, test headless o validación manual del usuario).

El ciclo por subtarea es:

1. **Validar la arquitectura preexistente**: leer los archivos, contratos,
   patrones de diseño y convenciones del área que se va a tocar. No se asume
   cómo funciona algo; se verifica archivo:línea (clases, métodos, señales,
   registros en catálogos, inyecciones de dependencia, serialización).
2. **Analizar cambios necesarios**: determinar qué archivos se tocan, qué se
   agrega, qué se modifica y cómo se integra de forma homogénea con lo existente
   (clean code: nombres en español, PascalCase para tipos, camelCase para
   métodos, 4 espacios, llaves en la línea siguiente, sin referencias de
   gestión en textos del proyecto).
3. **Implementar la subtarea**: escribir solo lo necesario para esa subtarea,
   respetando las convenciones del código y las reglas de `Skills/`.
4. **Validar**: build completo + `ctest` en verde. Si la subtarea toca
   `ShaderSources.h`, ejecutar `.github/scripts/validar-glsl.py`. Si es un fix
   de memoria, build con ASan+UBSan. Si es un bug, test rojo→verde.
5. **Pausa por bug o hallazgo**: si al analizar el código se encuentra un bug
   o un problema sin implementar que bloquea la subtarea en curso, se pausa
   esa subtarea y se abre una subtarea nueva que resuelva el problema
   siguiendo los mismos pasos (validar arquitectura → implementar → validar).
   Una vez cerrada la subtarea de resolución, se retiene la subtarea pausada.
6. **Commitear**: al terminar cada subtarea, hacer un commit atómico con solo
   los archivos propios de esa subtarea (`git add <archivos>`, nunca
   `git add -A` ni `git add .`).

- **Una tarea, una rama**: al comenzar una tarea de un tema distinto al actual,
  primero commitear los cambios pendientes de la rama actual y luego cambiar de
  rama. Nunca mezclar temas distintos en una misma rama.
- **Ramas remotas primero**: si la tarea requiere una rama nueva, verificar
  antes si en el remoto ya existe una que cumpla ese rol; en ese caso traerla
  al local (`git fetch` + `git checkout -b <rama> origin/<rama>` o
  `git switch`), actualizarla con su base y trabajar sobre ella. Solo crearla
  localmente si no existe.
- **Base correcta de la rama**: al crear una rama, verificar contra la
  documentación del flujo (ver [doc/FLUJO_DE_RAMAS.md](../../doc/FLUJO_DE_RAMAS.md)) desde
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
  la tarea. Antes de empezar, revisar dos cosas: (1) los `.md` que describen el
  área afectada — `README.md`, `doc/MANUAL_DE_USO.md`, `doc/PROJECT_STRUCTURE.md`,
  `doc/FLUJO_DE_RAMAS.md`, `doc/DocuTecnicoBugs.md` y los de diseño —; (2) **el
  directorio `.github/`, que también es documentación del proyecto** (aunque
  viva fuera de la raíz y ya se lo cite en reglas puntuales, existen estas
  fuentes):

  - `.github/COMMIT_TEMPLATE.md`: tipos admitidos, ámbitos frecuentes y reglas
    de redacción de los mensajes de commit.
  - `.github/PULL_REQUEST_TEMPLATE.md`: estructura y casillas obligatorias de
    la descripción de todo PR.
  - `.github/workflows/*.yml` (`ci.yml`, `release.yml`, `windows-release.yml`):
    qué plataformas, flags y targets de test compila y ejecuta la CI. Es la
    fuente para saber qué debe seguir en verde: si el cambio agrega un `.cpp`,
    un target de test o una dependencia, ahí hay que mantenerlo al día (los
    targets se listan explícitamente, ver `ci.yml`).

  Durante el trabajo, ir comparando en paralelo lo que la documentación afirma
  contra lo que el código realmente hace y corregir todo lo que quedó
  desactualizado — APIs, arquitectura, comandos y conteo de tests, atajos,
  limitaciones, ejemplos —, además de reflejar lo nuevo que se introduce. Los
  ajustes de documentación entran en el mismo commit que el código que los
  motiva: nunca "la documentación al final", ni entregar avances sin sus
  documentos al día.
- **Exploraciones con subagentes: delegarlas cuando el análisis lo pida.**
  Para recorrer el código hay disponibles subagentes exploradores, y conviene
  lanzarlos cuando la tarea es mayor que un vistazo puntual. Habitualmente es
  necesario en dos casos: (1) **análisis técnico de varias funcionalidades** —
  varios módulos implicados, rastrear un flujo de punta a punta o comparar
  vías alternativas de una misma operación (p. ej. todas las formas de
  renombrar algo y qué actualiza cada una)—; (2) **comparación contra la
  documentación** — contrastar lo que afirman `README.md`, `doc/MANUAL_DE_USO.md`
  u otros `.md` con lo que el código realmente hace, idealmente con los
  recorridos separados por tema y lanzados en paralelo. Si la razón de
  lanzarlos es otra, mencionar antes el porqué (p. ej. un recorrido largo de
  `git log`, una búsqueda que cruza todo el árbol) y proseguir. Para tareas
  acotadas y lineales se sigue con las herramientas directas. Lo que los
  subagentes devuelven es **insumo** de la regla siguiente (fuentes 2 y 3),
  no una conclusión: contrastarlo antes de declarar una causa.
- **Análisis profundo de cada bug: contrastarlo contra TODAS las fuentes
  disponibles antes de arreglarlo.** Un síntoma —aunque lo haya reportado el
  usuario— no es un diagnóstico. Antes de tocar código hay que reconstruir la
  cadena `síntoma → evidencia → causa → fix → test` contrastando el caso con
  cada fuente que pueda contradecirlo:
  1. **Evidencia primaria**: reproducirlo y aislar las variables (paso a paso
     exacto, `logs/` junto al ejecutable, stdout/stderr, exit codes,
     artefactos que quedan en disco). Lo que solo "se ve" sin evidencia
     reproducible es hipótesis, no causa. El "cómo" consigue esa evidencia —
     tests rojo/verde, experimento A/B determinista, sondas y laboratorios en
     carpetas ignoradas por git, sanitizadores, gdb/valgrind — está en
     "Depuración de bugs: instrumentación y validación".
  2. **Documentación del repo**: `README.md`, `doc/MANUAL_DE_USO.md`,
     `doc/PROJECT_STRUCTURE.md`, `DOCUMENTACION.md`, `doc/DocuTecnicoBugs.md`,
     `Skills/`, los documentos de diseño y el plan/issue del lote. Si la doc
     afirma que ese comportamiento es el correcto y el código no lo cumple, es
     bug de código; si el código es el correcto y la doc quedó vieja, se
     arregla la doc; si nadie lo documenta, es un hallazgo nuevo.
  3. **Código fuente**: rastrear el flujo completo —llamadores, estados,
     ciclo de vida, caminos alternativos—, no solo la línea del síntoma, y
     verificar si el comportamiento es intencional (comentarios, diseño) o si
     otra parte del código ya lo compensa.
  4. **Historial**: `git log` / `git blame` para saber si es una regresión
     reciente, un bug conocido que recae o un fix dejado a medias.
  5. **Tests existentes**: qué cubren, qué asumen y por qué no lo detectaron
     (¿falta el test, o el test fija el comportamiento erróneo?).
  6. **Fuentes externas**: documentación oficial de la API o herramienta
     involucrada (`std::filesystem`, `cmd.exe`, GLFW, ImGui, JNI…) e issues
     conocidas, para no "arreglar" algo que en realidad es el contrato de la
     herramienta.
  7. **Alternativa antes que el fix**: si el problema se puede sortear con
     otra técnica (cambiar el enfoque, renombrar, aplazar, otra API), evaluarla
     y compararla contra arreglar la causa; si no se arregla aún, dejarlo
     anotado con su justificación.
  Cierre del análisis: (a) **no se declara causa raíz sin evidencia que la
  aisle** y descarte al menos la hipótesis competidora principal — si dos
  hipótesis explican el síntoma, buscar el caso que las diferencie, no
  quedarse con la primera plausible—, y esa evidencia se consigue con las
  técnicas de "Depuración de bugs: instrumentación y validación"
  (experimento A/B con una sola variable, test rojo→verde, sanitizer cuando
  toque memoria); (b) el análisis queda escrito donde viva
  el lote (plan, issue), con síntoma, evidencia, causa, fix y test, en la
  posición por dependencias; (c) si el contraste no alcanza para decidir qué
  lado está mal, se reporta y se esperan instrucciones en vez de elegir una
  causa por conveniencia. Nunca se declara un bug ni se arregla algo "al
  vuelo" sin ese contraste.
- **Commits atómicos por tarea**: cada tarea terminada cierra con su commit
  (o los que sean necesarios si la tarea es grande), con mensaje descriptivo
  y convencionales (`feat:`, `fix:`, `docs:`, `refactor:`, `chore:`). El
  formato completo, con los tipos admitidos, los ámbitos frecuentes y las
  reglas de redacción, está en `.github/COMMIT_TEMPLATE.md`.
- **Commits de funcionalidades grandes incompletas**: si se crea un commit
  para guardar una funcionalidad grande que todavía no está terminada o
  validada, el cuerpo del mensaje debe detallar explícitamente las
  insuficiencias: qué partes faltan, qué validaciones se realizaron y cuáles
  siguen pendientes, y qué limita su cierre. El asunto y el cuerpo no deben
  presentar la funcionalidad como completa; actualizar el estado cuando se
  integre o valide lo pendiente.
- **Sin atribuciones ajenas al cambio**: los mensajes de commit (y cualquier
  metadato asociado) deben describir únicamente la implementación o los
  cambios realizados. No se permite adjudicar coautoría, autoría, firmas ni
  menciones a herramientas, asistentes o terceros que no correspondan al
  trabajo concreto sobre el código.
- **Sin coautorías**: nunca agregar trailers `Co-authored-by` a los mensajes
  de commit, independientemente de la persona o herramienta que se atribuya.
- **Commits solo con archivos propios**: al commitear, agregar únicamente los
  archivos que modificó el agente en la tarea actual (`git add <archivos>`),
  NO usar `git add -A` ni `git add .` que incluyen cambios ajenos sin
  commitear de otros colaboradores. Cada commit debe reflejar solo el trabajo
  concreto realizado.
- **Plantilla de descripción para pull requests**: todo PR debe llevar su
  descripción con la estructura de `.github/PULL_REQUEST_TEMPLATE.md`
  (Descripción / Tipo de cambio / Cambios realizados / Pruebas / Arquitectura /
  Compatibilidad / Documentación / Información adicional). Las secciones que
  traen casillas se completan marcando la casilla que aplique, y las de texto
  libre con información real del cambio: ninguna queda en blanco sin
  justificar. Cuando el agente redacte la descripción de un PR, debe usar
  exactamente esos encabezados, esas casillas y esa estructura.
- **Comandos destructivos requieren permiso y justificación**: antes de
  ejecutar `git checkout`, `git reset`, `git restore`, `git clean` o cualquier
  comando que descarte cambios (staged o unstaged), el agente debe:
  1. Pedir permiso explícito al usuario.
  2. Argumentar detalladamente en español por qué es necesario ese comando
     destructivo y qué cambios se perderán.
  3. Esperar confirmación antes de ejecutarlo.
  Esto evita pérdida accidental de trabajo del usuario o cambios ajenos sin
  commitear.
- **El historial publicado no se reescribe**: no se ejecutan comandos que
  cambien el hash o la firma de los commits que ya están en el historial
  compartido: `git commit --amend` sobre un commit ya pusheado, `git rebase`
  (incluida la variante interactiva), `git rebase --onto`, `git filter-branch`,
  `git filter-repo`, `git commit-tree`, `git replace`, `git notes` sobre commits
  publicados, o un `git push --force` que reescriba una rama o un tag ya
  publicado. Un commit ya publicado se corrige con un commit **nuevo** que
  explique la corrección. La única excepción es `git commit --amend` sobre un
  commit que nunca salió del equipo (no se ha pusheado): su hash cambia solo
  en local y desaparece al pushear. Si aun así hacer falta reescribir el
  historial, se reporta al usuario y se espera su decisión. Mover un tag que
  todavía no se publicó no cambia el hash de ningún commit, pero sigue rigiendo
  la regla de "Comandos destructivos requieren permiso y justificación": se
  pide permiso explícito al usuario.
- Validar antes de commitear: build completo + `ctest` en verde. Si el cambio
  cierra un bug, además la comprobación que corresponda según "Depuración de
  bugs: instrumentación y validación": test rojo→verde, sanitizer cuando toque
  memoria, y la reproducción del caso en más de una circunstancia.
- **Build con cambios ajenos**: si el build falla y hay archivos modificados
  por otros colaboradores (no tocados por el agente), reportar el fallo,
  indicar que hay cambios ajenos pendientes, y esperar instrucciones;
  NO modificar archivos ajenos para "arreglar" el build.
- El CI (`.github/workflows/ci.yml`) compila el engine en Ubuntu, compila y
  **enlaza** el engine en Windows con MSVC (generador por defecto y
  dependencias de vcpkg, como el job que publica el instalador), corre la
  suite headless en Linux/Windows, el backend Java en Ubuntu con JDK y las
  headless en Windows con el generador multi-config (Visual Studio); no
  pushear sin pasar los tests localmente.
- **NO confirmar fixes hasta validación del usuario**: nunca declarar un bug
  como "resuelto" o "fix real" hasta que el usuario lo pruebe y lo confirme
  explícitamente. Los tests automatizados (ctest) no cubren flujos visuales
  de UI (dock, layout, ventanas); el criterio de aceptación lo define el
  usuario probando la aplicación real. El fix llega a esa validación con su
  evidencia técnica ya lista (test rojo→verde, sanitizer, experimento en más
  de una circunstancia, ver "Depuración de bugs: instrumentación y
  validación"), no solo con la impresión de haberlo visto funcionar una vez.
