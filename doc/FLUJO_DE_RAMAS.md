# Flujo de ramas del repositorio

Convención de ramas de FunshiEngineOpenGL. Define qué ramas viven en el remoto,
para qué sirve cada una, dónde se crean las ramas de trabajo y el camino que
sigue cada cambio desde que se escribe hasta que llega a producción.

> Nota: la descripción de ramas en GitHub es una función solo de la interfaz web
> (y por eso no se puede configurar por API). Este documento es la fuente de
> verdad del propósito de cada rama; la misma información se debe mantener aquí
> al agregar o cambiar ramas.

---

## 1. Ramas permanentes

El flujo tiene **tres** ramas permanentes. No se borran nunca y todas están
protegidas en GitHub.

| Rama | Propósito | Protección aplicada |
|---|---|---|
| `master` | **PRODUCCIÓN**. Es la rama **por defecto** del repositorio. Solo recibe merges desde `test` (promoción de QA aprobada) y hotfixes urgentes. Siempre debe compilar y pasar los tests. Los tags de versión (`v*`) se crean acá. | PR obligatorio + 1 aprobación, descartar aprobaciones obsoletas al hacer push, conversación resuelta obligatoria, force push prohibido, borrado prohibido. |
| `test` | **QA / pruebas de regresión**. Recibe merges desde `develop` cuando el conjunto está listo para verificación. | PR obligatorio + 1 aprobación, descartar aprobaciones obsoletas al hacer push, conversación resuelta obligatoria, force push prohibido, borrado prohibido. |
| `develop` | **DEV**. Integración diaria: es donde los desarrolladores envían su código vía pull request y se prueba el conjunto antes de promoverlo. | PR obligatorio + 1 aprobación, descartar aprobaciones obsoletas al hacer push, conversación resuelta obligatoria, force push prohibido, borrado prohibido. |

Los administradores pueden saltarse la protección (`enforce_admins = false`)
para resolver una emergencia real. Ese bypass es la excepción, no el camino
normal: un push directo a una rama permanente queda registrado como tal y debe
justificarse.

---

## 2. Ramas de trabajo

Son ramas temporales: se crean para un tema concreto y **se borran al mergear**
(el repositorio tiene `delete_branch_on_merge` activado, así que GitHub las
elimina solo al aceptar el PR).

| Rama | Se crea desde | Para qué |
|---|---|---|
| `feature/<tema>` | `develop` | Agrupa un tema o feature compartido entre varios desarrolladores. Es la rama de integración del tema. |
| `fix/<tema>` | `develop` | Igual que `feature/<tema>`, pero para correcciones. Para un hotfix urgente se crea desde `master` (ver sección 4). |
| `docs/<tema>` | `develop` | Cambios de documentación (guías, manuales, este mismo flujo). Igual que las demás ramas de tema, se borra al mergear. |
| `personal/<usuario>/<tema>` | La `feature/<tema>`, `fix/<tema>` o `docs/<tema>` correspondiente | Es donde cada desarrollador hace sus commits. Nunca se abre un PR de una rama personal directamente a `develop`. |

Convención de nombres:

- `kebab-case` en `<tema>`: `feature/dirty-tracking`, `fix/rendering-1282`,
  `docs/flujo-de-ramas`.
- `<usuario>` siempre en **minúsculas**, tal como figura en GitHub:
  `personal/xggmasterx/dirty-tracking`.
- Sin mayúsculas, sin espacios y sin acentos en el nombre de la rama.

Ciclo de vida típico de un tema compartido:

1. Alguien crea la rama del tema (`feature/<tema>`, `fix/<tema>` o
   `docs/<tema>`) desde `develop`.
2. Cada desarrollador crea su `personal/<usuario>/<tema>` desde esa rama de tema
   y commitea ahí.
3. Cada rama personal abre un PR hacia la rama del tema.
4. Cuando el tema está completo, la rama del tema abre un PR hacia `develop`.
5. Al mergear, las ramas de trabajo (incluida `docs/<tema>`) se eliminan.

---

## 3. Flujo de un cambio

```
develop -> feature/<tema> (o fix/<tema>, docs/<tema>) -> personal/<usuario>/<tema>
                                                         |
                                                         v  PR
                                                         feature/<tema> (o fix/<tema>, docs/<tema>)
                                                         |  PR
                                                         v
                                                         develop
                                                         |  PR (candidato estable)
                                                         v
                                                         test  (QA)
                                                         |  PR (aprobado QA)
                                                         v
                                                         master (PRODUCCIÓN)
```

Pasos numerados por cambio:

1. **Crear la rama del tema** desde `develop`:
   `feature/<tema>`, `fix/<tema>` o `docs/<tema>`.
2. **Crear la rama personal** desde la rama del tema:
   `personal/<usuario>/<tema>`.
3. **Desarrollar** con commits pequeños y mensajes en español. Compilar y pasar
   los tests antes de pedir revisión.
4. **PR de la rama personal a la rama del tema.** Un PR por desarrollador y por
   tema; se revisa y se aprueba como cualquier otro cambio.
5. **PR de la rama del tema a `develop`** cuando el tema está integrado y
   compila. Este es el único punto por el que el código entra a la integración
   diaria.
6. **Promover `develop` → `test`** con un PR cuando el conjunto está estable y
   listo para QA.
7. **Promover `test` → `master`** con un PR cuando QA aprobó la promoción. En
   ese momento (o antes, si corresponde) se crea el tag de versión sobre
   `master`.

El flujo avanza en una sola dirección: `develop → test → master`; nunca
`master → develop`, salvo el back-merge de hotfix de la sección 4. Las ramas de
trabajo nunca reciben merges desde `master`.

---

## 4. Hotfix urgente y back-merge

Un hotfix es la única vía que no nace en `develop`. Se usa solo cuando
producción está rota y no se puede esperar el ciclo normal.

1. **Crear la rama desde `master`**:
   `git switch -c fix/<tema> origin/master`.
2. **Arreglar y abrir PR directamente a `master`** (revisión + 1 aprobación).
3. **Back-merge obligatorio**: el back-merge se hace **siempre desde `master`
   hacia `develop`**, con un PR `master → develop` una vez mergeado el hotfix, para
   que el arreglo no se pierda en la próxima promoción. Nunca se deja `master`
   con un fix que `develop` no tenga. La rama del hotfix **no** se reutiliza para
   el back-merge: con `delete_branch_on_merge` activado se borra al mergear el PR
   a `master` y GitHub además cierra solo el PR abierto desde ella. Como la rama
   del hotfix ya no existe tras el merge, si hace falta rehacer el arreglo en
   lugar de mergear `master` sobre `develop`, se crea `fix/<tema>-backmerge`
   desde `develop`.
4. Verificar que `develop` quede con el mismo arreglo antes de cerrar el
   incidente. Si el PR `master → develop` presenta conflictos, no se resuelven en
   ese PR (su cabeza es `master`, que está protegida): se crea una rama desde
   `develop` (por ejemplo `fix/<tema>-backmerge`), se mergea `master` dentro de
   esa rama, se resuelven los conflictos ahí y se abre el PR de esa rama hacia
   `develop`. Nunca se reescribe `master` ni `develop`.
5. Si el hotfix necesita una publicación inmediata, el tag de versión se crea
   sobre `master` una vez mergeado el PR.

---

## 5. Reglas duras

- **Nadie commitea directo a `master`, `test` ni `develop`.** Siempre pull
  request con al menos 1 aprobación. La única excepción es el bypass de
  administrador en una emergencia real.
- **Una sola dirección:** `develop → test → master`. Nunca `master → develop`,
  salvo el back-merge de hotfix de la sección 4.
- **Ninguna rama reescribe el historial de otra.** El force push está prohibido
  en las ramas permanentes; tampoco se hace `rebase`/`reset` sobre ramas
  compartidas.
- **Los tags de versión (`v*`) se crean sobre `master`**, nunca sobre `test` ni
  `develop`.
- Las ramas de trabajo (`feature/*`, `fix/*`, `docs/*`, `personal/*`) se borran
  al mergear; no se reutilizan para un tema posterior.

---

## 6. Ramas heredadas (pendiente de decisión)

El modelo anterior tenía cinco ramas. Las ramas `release` y `staging` **siguen
existiendo en el remoto**, pero quedaron congeladas en el commit `ad4b545`
(20-oct-2026), muy por detrás del código actual (el 2026-10-28 `develop` estaba
en `379fe7a`).

- **NO forman parte del flujo nuevo.** No se promueve nada hacia ellas ni desde
  ellas.
- No se debe mergear ni commitear ahí: cualquier trabajo que aparezca en esas
  ramas está fuera de proceso.
- Quedan **pendientes de decisión**: eliminarlas o redefinirlas. Hasta que se
  decida, se las trata como congeladas y de solo lectura.
- Existen tags locales de respaldo `backup/<rama>-2026-10-28` por si hace falta
  recuperar el estado anterior de cualquiera de las ramas del modelo viejo.

---

## 7. Comandos útiles

Todos los ejemplos asumen que `origin` es
`https://github.com/FunshiEngine/FunshiEngineOpenGL.git`.

### Crear una rama de tema desde `develop`

```bash
git fetch origin
git switch -c feature/mi-tema origin/develop
```

Para un fix común (no hotfix), idéntico cambiando el prefijo:

```bash
git switch -c fix/mi-bug origin/develop
```

### Crear la rama personal desde la rama del tema

```bash
git switch feature/mi-tema          # si aún no estás parado en ella
git switch -c personal/mi-usuario/mi-tema
```

La rama personal se crea desde la rama del tema, **no** desde `develop`.

### Subir la rama y abrir el PR

```bash
git push -u origin HEAD
```

GitHub imprime en la salida el enlace para abrir el PR. Hacia dónde va el PR
según la rama:

- `personal/<usuario>/<tema>` → la rama de tema (`feature/<tema>`,
  `fix/<tema>` o `docs/<tema>`).
- `feature/<tema>`, `fix/<tema>` o `docs/<tema>` → `develop`.
- `develop` → `test` y `test` → `master` solo en las promociones.

### Promover `develop` → `test`

El camino normal es mergear el PR `develop → test`. El push directo **no
reemplaza al PR**. La aprobación de QA habilita el paso siguiente
(`test → master`), no la entrada a `test`.

**Atajo de administrador** (opcional, nunca en lugar del PR):

```bash
git fetch origin
git push origin origin/develop:test
```

Este push actualiza `test` para que refleje `develop` sin tocar el working tree
local.

> ⚠️ **Puede fallar por protección de rama.** `test` exige pull request, así que
> el push directo es rechazado (`protected branch hook declined`) salvo que lo
> haga un administrador. El camino recomendado es abrir un PR
> `develop → test` y mergearlo; usar el push solo como atajo de administrador
> en una promoción ya aprobada.

### Promover `test` → `master`

```bash
git fetch origin
git push origin origin/test:master
```

Mismo requisito y misma advertencia: **PR `test → master` con aprobación de QA
es el camino normal**. El push directo solo funciona con bypass de
administrador y queda registrado.

### Hotfix

```bash
git fetch origin
git switch -c fix/mi-bug origin/master
# ... arreglo, commit y PR a master ...
```

Back-merge obligatorio a `develop` una vez mergeado el hotfix en `master`, siempre
con un PR `master → develop`:

```bash
git fetch origin
git push origin origin/master:develop
```

> ⚠️ Igual que en las promociones, `develop` está protegida: el push directo
> solo pasa con bypass de administrador. Lo recomendado es abrir un PR
> `master → develop` y mergearlo.
>
> La rama del hotfix no se usa para el back-merge: con `delete_branch_on_merge`
> activado se borra al mergear el PR a `master` y GitHub cierra solo cualquier PR
> abierto desde ella. Si ya no existe y hay que rehacer el arreglo, se crea una
> rama nueva desde `develop` (por ejemplo `fix/mi-bug-backmerge`) y se abre el PR
> desde ahí.

### Ver el estado del flujo

```bash
git branch -a
git log --oneline --graph origin/develop origin/test origin/master
```

### Limpiar ramas locales ya mergeadas

```bash
git branch -d mi-rama-mergeada        # -d falla si no está mergeada, a propósito
git fetch --prune                     # borra las referencias remotas que ya no existen
```

`git branch -d` solo borra si la rama ya está integrada; si git se niega, revisar
si realmente se mergeó antes de forzar con `-D`.

---

## 8. Estado de la sincronización inicial (2026-10-28)

Registro de cómo quedó el remoto al migrar al modelo de tres ramas:

- `develop` = `379fe7a`.
- `master` = `test` = `c241f1e` (merge de `develop`), es decir, el mismo
  contenido que `develop`.
- Se eliminó la rama `diag/rendering-1282` (ya mergeada en `develop` vía PR #77).
- La rama `feature/interfaz-juego-nativa` ya no existe en el remoto.
- `release` y `staging` quedaron congeladas en `ad4b545` (ver sección 6).
- `master` es la rama por defecto del repositorio.
