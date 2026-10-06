---

## Lote B — auditoría 2026-09-30 (abierto)

Este lote toma los hallazgos de `.agentes/auditorias/*.md`. Los ítems van por
orden de dependencia: las bases que desbloquean otras primero, luego
independientes y al final los de mayor riesgo.

Regla: cuando aparezca un hallazgo nuevo se inserta en su posición según sus
dependencias, con su cadena `síntoma → evidencia → causa → fix → test`. Cada
paso se marca al completar y verificar.

## Índice

| # | Área | Prioridad | Análisis | Estado |
|---|---|---|---|---|
| B2 | Model (cache de fallos) | **ALTA** | `analisis/2026-09-30-b1-b2-modelo.md` | **implementado** (`e4ecb7b`); test rojo→verde 88/88; suite 20/20; pendiente validación del usuario |
| B1 | Model (render directo) | **ALTA** | `analisis/2026-09-30-b1-b2-modelo.md` | **implementado** (`3d4849d`); test rojo→verde (A/B) `escena-serializacion-tests` 119/119; suite 20/20; falta validación visual del usuario |
| B3 | Configuración (apariencia y proyecto fantasma) | **ALTA** | `analisis/2026-09-30-b3-b4-configuracion-inspector.md` | **implementado** (`698f877`); test rojo→verde (A/B) `configuracion-tests` 154/154 y `menu-tests` 40/40; suite 20/20; pendiente validación del usuario |
| B4 | Inspector/GUI (reconstrucción de Settings) | **ALTA** | `analisis/2026-09-30-b3-b4-configuracion-inspector.md` | **implementado** (`b3d9bcb`); test rojo→verde (A/B) `escena-serializacion-tests` 130/130; suite 20/20; pendiente validación visual |
| B5 | Inspector/GUI (progreso de StatusBar) | **ALTA** | `analisis/2026-09-30-b5-b6-inspector.md` | **implementado** (`b884216`); test rojo→verde (sonda A/B) `userinterface-tests` 42/42; suite 20/20; pendiente validación visual |
| B6 | Inspector/GUI (desanudar a raíz) | **MEDIA-ALTA** | `analisis/2026-09-30-b5-b6-inspector.md` | **implementado** (`2e3140e`); test rojo→verde (A/B) `escena-serializacion-tests` 151/151; suite 20/20; pendiente validación visual |
| B7 | FileManager (modal de nueva carpeta) | **MEDIA** | `analisis/2026-09-30-b7-b8-filemanager.md` | **implementado** (`a961be2`); rojo→verde (A/B) `filemanager-tests` 173/173; helper `CrearCarpeta` compartido por árbol y grid; pendiente validación visual |
| B8 | FileManager (Skybox en manifiesto) | **BAJA** | `analisis/2026-09-30-b7-b8-filemanager.md` | **cerrado**: las 6 caras se guardan, leen, relativizan y cuentan para `entradaVacia` |
| B9 | Documentación (conteos y textos) | **BAJA** | `analisis/2026-09-30-b9-b13-tests.md` | **cerrado** `228361c`; los conteos de cada fix posterior se fueron actualizando en el commit del código que los motivó |
| B10 | Windows (JNI: JDK único, excepción, `NewStringUTF`) | **ALTA** | `analisis/2026-09-30-b10-b11-b12-windows.md` | **cerrado**: B10.2-B10.5 implementados y verificados, falta validación en Windows |
| B11 | Windows (includes C++ scripts) | **ALTA** | `analisis/2026-09-30-b10-b11-b12-windows.md` | **cerrado**: B11.1+B11.2 `91c9a2d`, B11.3 `855918d` (el horneado solo cuenta si existe; si no se busca el toolset de la maquina). Sin hardware Windows para e2e |
| B12 | Windows (empaquetado y diagnóstico) | **MEDIA** | `analisis/2026-09-30-b10-b11-b12-windows.md` | **cerrado** B12.1 (`8953174`), B12.2 (`5d87029`), B12.3 (`9f7375d`), B12.4 (`c753a49`, instalación por usuario sin UAC) y B12.5 (`dbb0607`, una sola ruta de salida del instalador); falta validación en Windows |
| B13 | Deuda menor (FileManager, rutas UNC) | **BAJA** | `analisis/2026-09-30-b9-b13-tests.md` | **cerrado**: Dm1 `eb13e26`, Dm2 `eeee7fa`, Dm3 `5d87059`, Dm4 `b38fdc4`, Dm5 `8a742d2`, Dm6 `23cf77e` |

**Estado de la suite (verificado por ejecución el 2026-09-30):** build limpio y
`ctest` en verde, 20/20 en Linux con `Release`, `ENABLE_ASAN=OFF`, `FUNSHI_JAVA=ON`.
Los conteos de comprobaciones que aparecen en la documentación se contrastaron
ejecutando cada binario (ver §Conteo real en el análisis de B9/B13): **18 de 20
coinciden con lo documentado**; los dos que no, erran a favor de la documentación.

### Avance del lote

| Ítem | Commit | Verificación técnica | Validación del usuario |
|---|---|---|---|
| B2 | `e4ecb7b` | rojo→verde `assetmanager-tests` (88/88) + `ctest` 20/20 | pendiente |
| B1 | `3d4849d` | rojo→verde (A/B) `escena-serializacion-tests` (119/119) + `ctest` 20/20 | pendiente (visual: posición/jerarquía) |
| B3 | `698f877` | rojo→verde (A/B) `configuracion-tests` (154/154) + `menu-tests` (40/40) + `ctest` 20/20 | pendiente (visual: tema en vivo; arranque sin proyecto fantasma) |
| B5 | `b884216` | rojo→verde (sonda A/B) `userinterface-tests` (42/42) + `ctest` 20/20 | pendiente (visual: barra con progreso saturado/vacío) |
| B4 | `b3d9bcb` | rojo→verde (A/B) `escena-serializacion-tests` (130/130) + `ctest` 20/20 | pendiente (visual: estado de paneles tras editar/añadir componente) |
| B6 | `2e3140e` | rojo→verde (A/B) `escena-serializacion-tests` (151/151) + `ctest` 20/20; rojo: 5 fallos (pose del hijo, local, descendiente, remove y add del cuerpo) | pendiente (visual: reparentar en el árbol no debe saltar ni frenar sin motivo) |
| B7 | `a961be2` | rojo→verde (A/B) `filemanager-tests` (173/173); rojo: `crearCarpeta` reportaba éxito con la carpeta ya existente | pendiente (visual: árbol y grid avisan el fallo sin cerrar el modal) |
| B11.1+B11.2+B11.3 | `91c9a2d` / `855918d` | rojo→verde (A/B) `scripts-tests` (137/137) y `scripts-runtime-tests` (22/22) + `ctest` 20/20; A/B: sin la comprobacion de existencia del compilador, 2 fallos; con el barrido de vcvars a un nivel, 3 fallos | pendiente (Windows: scripts C++ en la app instalada con y sin Visual Studio) |
| B12.1+B12.2+B12.3 | `8953174`/`5d87059`/`9f7375d` | A/B con el trace del configure (`-DFUNSHI_VERSION` ignorado antes, propagado despues) + `ctest` 20/20 | pendiente (Windows: VERSIONINFO del .exe y del instalador con el tag) |
| B12.1 | *(este commit)* | rojo→verde (A/B) `configuracion-tests` (163/163) + `ctest` 20/20; rojo: sin la reserva de `logs` la migración se la llevaba a `Proyects/`. A/B del arranque real con el ejecutable en una carpeta no escribible: sin candidatas no hay log y 4770 bytes salen a la consola | pendiente (Windows: log bajo `Program Files`, y que no aparezca una consola) |

---

## Correcciones a las hipótesis del plan (2026-09-30)

Después del análisis de implementación hay que corregir lo que estaba mal:

| Ítem | Hipótesis original | Correcto |
|---|---|---|
| B1 | basta con evitar el `Transform` duplicado | además hay que **dibujar el mesh con la matriz mundial del objeto**; si no, la jerarquía sigue rota. La ruta de `Modelos3D` se conserva intacta |
| B2 | basta con devolver `false` al cachear | hay que añadir **invalidación del negativo** y conservar el contrato de lanzar excepción |
| B3 | la apariencia se rompe por el orden de suscripción de eventos | la causa es que `dded0d4` dejó de llamar `TemaEditor::aplicarEstilo` en el manejador vivo. El orden de suscripción es higiene, no la causa |
| B3 | un solo defecto de proyecto fantasma | son **cuatro encadenados**: nombre por defecto, persistencia inmediata, fallback de `main.cpp` y creación automática |
| B4 | reconstruir cuando el puntero pierde foco | distinguir **cambio estructural** de **cambio de propiedad**, con evento propio, y reconciliar los `Settings*` |
| B5 | un solo defecto de progreso | **dos**: `append` con tamaño negativo y división diferida por `total_ == 0` |
| B6 | al desanudar, dejar el padre `nullptr` | **no**: la escena exige raíz única. Hay que preservar la pose mundial, conservar la raíz y refrescar el cuerpo físico |
| B7 | el fallo está en cerrar el modal | hay **dos capas**: `crearCarpeta` no expresa "se creó" sino "existe" (falso éxito), y el modal cierra igual. Afecta a **los dos paneles** (árbol y grid), no solo al árbol |
| B8 | omitir las seis caras | además hay que hacer que `entradaVacia` las considere, para que un objeto con solo Skybox genere entrada |
| B9 | dos conteos mal (98 y 145) | **uno solo**: `PROJECT_STRUCTURE.md:752` (98 en vez de 107). `README.md:152` (107) y `scripts-tests` (99) **acierten**. La auditoría además arrastraba dos derivaciones por lectura que resultaron erróneas al ejecutar |
| B10 | convertir la classpath a UTF-8 antes de `optionString` | **REFUTADO**: `optionString` se decodifica en la codificación nativa, no en UTF-8. Convertir sería una regresión. El defecto real es `NewStringUTF` (`BackendJava.cpp:666`), que solo afecta a rutas con acentos y **después** de `FindClass` |
| B11 | arreglar los headers | son **dos** bloqueos: las cabeceras no se empaquetan **y** `FUNSHI_CXX_COMPILER` hornea una ruta absoluta del checkout. Arreglar solo las cabeceras no basta |
| B12 | mover los logs a `%LOCALAPPDATA%` | el motor ya tiene `ProjectPaths::directorioBase()` con prueba de escritura real; los logs son el único consumidor que no lo consulta. El plan de candidatos debe reusarlo |
| B13 | seis ítems de deuda menor | **cerrado**: los seis ítems implementados y verificados (Dm1 `eb13e26`, Dm2 `eeee7fa`, Dm4 `b38fdc4`, Dm5 `8a742d2`, Dm6 `23cf77e`; Dm3 con B12.2) |

---

## Orden de aplicación corregido

El orden por dependencias, ya ajustado con lo que Revelaron los análisis:

1. **B2** antes que B1: B2 toca `AssetManager` y no depende de nada; B1 necesita la
   firma de `MeshRenderer` resuelta y su test depende de poder cargar un mesh con un
   `LoaderStub` fiable.
2. **B3** (apariencia) y **B5** (StatusBar): independientes y de un solo archivo cada
   uno. Arranque barato, buen momento para validar el flujo de commit.
3. **B4** antes que **B6**: B4 fija la semántica del bus de eventos que B6 reutiliza.
4. **B1**: render directo. Cierra el trabajo de las bases.
5. **B6**: jerarquía y física, apoyándose en el bus de B4.
6. **B7** completo en tres pasos: núcleo (`crearCarpeta`) -> helper compartido -> los
   dos paneles -> tests. El paso 1 es una base: sin él los tests del helper no
   significan nada.
7. **B11.1 + B11.2** juntos (cabeceras), luego **B11.3** (compilador) por separado si se
   decide abordarlo. **B12.1** (logs) antes que nada de Windows: sin log no hay
   diagnóstico del resto.
8. **B10.4 + B10.3** (raíz única de JDK y excepción preservada), luego **B10.2**
   (`NewStringUTF`), luego **B10.5** (paridad del instalador). **Los cuatro están
   hechos**; solo queda la validación en Windows, que depende de hardware.
9. **B8**, luego **B9** (documentación, con los `.md` de B8 y B11 actualizados en sus
   propios commits).
10. **B13**: Dm1 y Dm5 tienen test real; Dm3 depende de B12.2; Dm6 es un commit de texto
    propio, sin mezclar con fixes de comportamiento.

**Criterio de cierre:** cada ítem requiere `síntoma → evidencia → causa → fix → test`,
build limpio + `ctest` en verde, y evidencia técnica lista para los flujos visuales.
**No confirmar fixes hasta validación del usuario.**

---

## B1 — Model: renderizar directamente sin `Modelos3D` temporal

**Tipo:** regresión/deuda de arquitectura. **Prioridad:** ALTA. **Depende de:** —.

**Síntoma**:
Un objeto `GameObject` con componente `Model` se dibuja en `(0,0,0)` a pesar de tener un `Transform` no identidad (incluyendo sus hijos). El subárbol padre/hijo se pierde.

**Evidencia**:
- `SceneRenderer.cpp:639-678` usa un objeto temporal `Modelos3D tempModel` para cada componente `Model`.
- `Modelos3D` (constructor en su impl.) crea un `Transform` por defecto y añade el transform pasado (o el del objeto) **como un segundo componente Transform** que nunca se usa.
- `getComponent<Transform>()` devuelve el **primer** componente; por eso el transform copiado queda muerto.
- `MeshRenderer::dibujar` consume el `Transform` del propio `GameObject` asociado, pero la ruta de `Model` obliga a pasar por ese wrapper temporal.
- Tras copiar `t` en `tempModel` (`:652`), el `GameObject` original mantiene su jerarquía y transform correctos; el dibujo pinta el mesh con el transform de `tempModel` (muerto/incorrecto).

**Causa**:
Diseño heredado que crea un `GameObject` efímero por mesh, en lugar de renderizar el `Mesh` compartido directamente con el `Transform` y materiales del `GameObject` original.

**Fix propuesto**:
Renderizar directamente a partir del `GameObject* obj` y del `Mesh* mesh` del `Model`. Ampliar `MeshRenderer` (API) para dibujar un mesh concreto sin crear un objeto temporal. Alternativa: obtener el transform global efectivo y los materiales del `obj` y llamar a `MeshRenderer` pasando el mesh; eliminar la creación de `Modelos3D tempModel` por completo y preservar la jerarquía (no se crea ningún hijo efímero).

**Requisitos**:
- No romper el render de `Modelos3D` existentes (usados en otras rutas).
- Mantener compatibilidad de materiales/texturas (usar los del objeto y/o del componente `Model`).
- Reducir allocations por frame (eliminar el temporal).

**Tests**:
- Test headless que carga una escena con un `GameObject` (con `Transform(2,0,0)`) + `Model` que referencia un mesh y verifica que la matriz mundial calculada para ese draw sea `(2,0,0)`.
- Test con modelo hijo del objeto: el transform relativo debe respetarse.
- `ctest` suite 20/20. No introducir cambios en tests existentes que asuman el comportamiento anterior, solo cubrir la regresión.

---

## B2 — Model: caché de rutas fallidas para evitar excepciones por frame

**Prioridad:** ALTA. **Depende de:** —.

**Síntoma**:
Si un archivo `.obj` no existe o Assimp no puede leerlo, el `try/catch` de `SceneRenderer` evita el cierre, pero **la excepción se lanza/reintenta por objeto y por frame**. Se genera spam de logs y carga innecesaria.

**Evidencia**:
- `SceneRenderer.cpp:639-678` rodea la carga de cada `Model` con `try { ... } catch(...) {}` pero no recuerda que esa ruta falló.
- `AssetManager::getMesh(path)` puede lanzar y no cachea fallos.
- En el loop de render (principal y vistas previas) se vuelve a intentar cada frame.

**Causa**:
Falta una caché de rutas con fallo (blacklist) con TTL o permanente hasta que cambie el path (mtime o edición).

**Fix propuesto**:
Añadir en `AssetManager` (o en una capa de cache dedicada) un mapa `fallidos_[ruta] = timestamp` o solo `set<bool>` con la clave normalizada. Al pedir un mesh:
1. Si está en caché de fallos → devolver nullptr o lanzar una vez, pero no intentar I/O.
2. Al tener éxito → eliminar de fallidos.
3. Al cambiar `Model::setPath` → limpiar entrada para esa ruta.
Limitar a 256-1024 entradas (LRU) para evitar crecimiento infinito.

**Tests**:
- Cargar ruta inexistente: número de llamadas a I/O (mockeable) == 1 en N frames.
- Ruta corregida (archivo aparece) → se quita de fallidos y carga.
- `ctest` 20/20.

---

## B3 — Configuración: apariencia no se aplica en vivo y proyecto fantasma

**Prioridad:** ALTA. **Depende de:** —.

**Síntoma**:
1. Tras `dded0d4`, tema, acento y blanco/negro **no se aplican en vivo** al cambiar en Opciones.
2. El **primer arranque** persiste `"Nuevo Proyecto"`. El **segundo arranque** crea un proyecto fantasma.

**Evidencia**:
- `MenuModel::setApariencia` (`MenuModel.cpp:121-123`) es síncrono: emite `AparienciaCambio` inmediatamente. El problema no es ahí.
- `main.cpp:299-311` recibe el evento y aplica los colores, pero el registro inicial del evento puede ocurrir **antes de que el contexto de ImGui/ventana esté listo** o antes de que `Apariencia` tenga su valor definitivo.
- Persistencia del proyecto por defecto: al primer inicio, `GestorDeProyectos` guarda un estado con `"Nuevo Proyecto"` (sin carpeta física válida) y en el siguiente ciclo lo carga como proyecto existente → proyecto fantasma.

**Causa**:
Orden de inicialización entre `MenuView/MenuModel`, la suscripción a `AparienciaCambio` y la aplicación de la apariencia cargada desde disco. En el caso del proyecto fantasma, la condición para crear vs. cargar no distingue "proyecto por defecto no inicializado" de "proyecto guardado".

**Fix propuesto**:
- **Apariencia en vivo**: asegurar que al cargar la configuración desde `ConfigPersistence` se aplica **después** de que todos los suscriptores estén registrados. Emitir `AparienciaCambio` **una sola vez** tras la inicialización completa (post-load), o forzar `apply()` explícito en `main` después de conectar señales.
- **Proyecto fantasma**: no persistir `"Nuevo Proyecto"` hasta que el usuario cree/guarde explícitamente. Al primer arranque, si no existe ningún proyecto válido, iniciar en estado "sin proyecto" o con un proyecto temporal no escrito a disco hasta el primer `Ctrl+S`/guardar. Añadir una marca `proyectoInicializado` o comprobar existencia física de la carpeta del proyecto antes de considerarlo válido.

**Tests**:
- Cambio de tema/acento en tiempo real: los colores de ImGui cambian sin reiniciar.
- Primer arranque → no crea archivo de proyecto fantasma en disco; segundo arranque no lo materializa.
- `ctest` 20/20.

---

## B4 — Inspector/GUI: reconstrucción de `Settings*` al emitir `ComponentChanged`

**Prioridad:** ALTA. **Depende de:** B1-B3 (no bloqueantes, pero toca GUI).

**Síntoma**:
`SceneObjectTree` emite `ComponentChanged` al renombrar, cambiar ID o confirmar modal (106, 219, 260). `SettingsObjectInterface` reconstruye **todos** los `Settings*` (`cpp:103-119`) → se pierde:
- `SettingsColliderEsfera::newRadio` (valor tipeado no confirmado)
- `SettingsMaterial::presetAplicado` vuelve a "Personalizado"
- Estado abierto/cerrado de `CollapsingHeader` (los `ImGui::PushID(comp)` cambian al recrear wrappers)

**Evidencia**:
- `SettingsObjectInterface.cpp:103-119`: `limpiarSettings()` + `crearSettings()` por cada cambio de componente.
- `SettingsColliderEsfera.cpp:31` inicializa `newRadio`.
- `SettingsMaterial.h:29` tiene `presetAplicado = -1`.

**Causa**:
Invalidación global por cualquier cambio menor. No distingue "estructura de componentes cambió" (add/remove/reparent) de "propiedad cambió" (rename/ID).

**Fix propuesto**:
- Emitir eventos distintos: `ComponentStructureChanged` (add/remove/move/reparent) vs `ComponentPropertyChanged` (rename, ID editado, campos puntuales).
- `SettingsObjectInterface` solo reconstruye la lista cuando cambia la **estructura** (número/tipos de componentes). Para cambios de propiedad, actualiza los valores existentes o no reconstruye.
- Preservar estado UI: guardar qué headers están abiertos (mapa por `componentId`/tipo+índice) antes de limpiar y restaurarlo después.
- Para campos editados en curso (no confirmados), no sobrescribirlos con el valor del objeto mientras el widget tenga foco (`ImGui::IsItemActive()`/`ImGui::IsItemFocused()`).

**Tests**:
- Editar radio de esfera sin confirmar Enter → no se pierde al renombrar objeto.
- Seleccionar preset de material → persiste tras renombrar.
- Expandir/collapse headers → se conserva tras `ComponentChanged` de propiedad.
- `ctest` 20/20.

---

## B5 — Inspector/GUI: cálculo inseguro de progreso en `StatusBarInterface`

**Prioridad:** ALTA. **Depende de:** —.

**Síntoma**:
`StatusBarInterface.cpp:49-54` puede convertir un progreso `> 100%` en una cantidad **negativa** de `size_t` (`bar.append(10 - bars, ' ')`) → `std::length_error`/`std::bad_alloc`. El guard solo protege `total == 0`, no `hecha > total`.

**Evidencia**:
```cpp
if (enCurso && total > 0) {
    int pct  = int((hecha/total)*100.0f);  // hecha>total → pct>100
    int bars = pct / 10;                     // bars>10
    std::string bar(bars, '#');
    bar.append(10 - bars, ' ');              // 10-bars < 0 → size_t enorme
}
```
También `compilando_` se asigna fuera del `if` (línea 58). División por `total_` en 123 sin guard.

**Causa**:
Clamp unilateral (solo divisor cero). Datos pueden venir desincronizados (cargaHecha_ vs cargaTotal_).

**Fix propuesto**:
Clampar `hecha` entre `0` y `total`:
```cpp
const auto h = std::clamp(hecha, 0ULL, total);
const int pct = (total==0?0 : int((h*100.0)/double(total)));
const int bars = std::clamp(pct/10, 0, 10);
```
Usar `size_t` o `int` con clamp para `10-bars`. Proteger también `total_ == 0` en 123. Añadir asserts en debug o logs si `hecha>total` (para detectar origen).

**Tests**:
- `hecha=150, total=100` → barra llena (10 '#') sin excepción.
- `hecha=-1` (imposible por tipos) no ocurre; con valores corruptos → clamp.
- `total=0` → overlay no dibujado.
- `ctest` 20/20 (unit test para el formateador si se extrae).

---

## B6 — Inspector/GUI: "Desanudar a raíz" pierde pose mundial y desincroniza física

**Prioridad:** MEDIA-ALTA. **Depende de:** B4 (estructura).

**Síntoma**:
Al desanidar un hijo, el subárbol **se teletransporta** (salta). El colisionador/física no se re-registra (`ObjectReparented` sin suscriptores relevantes). El ítem puede reaparecer indefinidamente si no se limpia correctamente.

**Evidencia**:
- `SceneRegistry::reparent` (190-211) llama solo `setParentEntity(parent)`. `GameObject::getGlobalTransform()` recalcula `mundo = padre * local`. Al pasar de tener padre P a raíz (sin padre), el local **no se actualiza** para conservar la pose mundial → salta.
- `ObjectReparented` (`EditorController.cpp:109`) no tiene suscriptores que actualicen cuerpos Bullet/Physics.
- `SceneObjectTree.cpp:309-313`: si `editor == nullptr`, la intención `objetoADesanidar` nunca se limpia (retry por frame).
- `SceneObjectTree.cpp:158`: condición `getParentEntity() != nullptr` sigue cierta después de desanudar si se deja como hijo de raíz en lugar de `nullptr`.

**Causa**:
Falta preservación de pose mundial al reparentar a raíz (`parent == nullptr`). El cambio de jerarquía no propaga a los backends de física.

**Fix propuesto**:
Al reparentar a `nullptr` (raíz):
1. Guardar `T_mundo = obj->getGlobalTransform()` antes del cambio.
2. Llamar `obj->setParentEntity(nullptr)`.
3. Establecer `T_local = T_mundo` (conserva pose mundial).
4. Propagar evento `ObjectReparented` y asegurar que `PhysicsEngine`/backends actualicen cuerpos asociados (re-registrar o actualizar transforms).
También limpiar `objetoADesanidar` **siempre**, con o sin `editor`. Corregir la condición para detectar que ya está en raíz.

**Tests**:
- Hijo en (10,0,10) con padre en (5,0,5) → desanudar a raíz: queda en (10,0,10) (mundo preservado). Antes saltaba a (5,0,5).
- Jerarquía anidada (3 niveles) → conserva posiciones relativas al mundo.
- Física: cuerpo asociado mantiene su pose tras desanudar (sin salto visual).
- `ctest` 20/20.

**Implementado (`2e3140e`)**:
- `SceneRegistry::reparent` (`SceneRegistry.cpp:211-234`): captura el mundo del
  hijo, tras `setParentEntity` reescribe el local como
  `inverse(mundoPadre) × mundoHijo` (con guarda `Transform* != nullptr`). No se
  usa `parent = nullptr`: la raíz sigue siendo el padre. Los descendientes no se
  tocan.
- `EditorController::reparentGameObject` llama a `refreshRigidBody(object)`
  (invalida shape → quita del mundo → recrea → reinserta). Costo anotado:
  reinicia la velocidad del cuerpo; reparentar en Play frena el objeto.
- Predicado puro `esCandidatoADesanidar` en `GUI/SceneGUI/JerarquiaArbol.h`
  (sobre `Entity`); `SceneObjectTree` lo usa y limpia `objetoADesanidar` en ambos
  caminos.
- Tests `reparentarPreservaLaPoseYRefrescaElCuerpo` (backend físico falso) y
  `reparentarSobreviveElGuardado` en `tests/SceneSerializationTests.cpp`. Rojo
  A/B: 5 fallos (pose del hijo, local, descendiente, `remove` y `add` del
  cuerpo). `escena-serializacion-tests` 151/151; `ctest` 20/20.
- Queda fuera: el undo real del reparentado (la GUI no usa `ReparentarComando`)
  y la corrección de `MANUAL_DE_USO.md:347`; van en su propio cambio.

---

## B7 — FileManager: modal "Nueva Carpeta" cierra aunque falle la creación

**Prioridad:** MEDIA. **Depende de:** —.

**Síntoma**:
Modal se cierra incondicionalmente aunque `crearCarpeta` falle (nombre inválido, caracteres prohibidos, ya existe). No muestra mensaje al usuario.

**Evidencia**:
- `TreeFilesInterface.cpp:287-300`: `CloseCurrentPopup()` (299) y `creandoCarpeta = false` están **fuera** del `if (fileManager->crearCarpeta(...))`.
- `GestorDeArchivos.cpp:171-176`: devuelve `true` si el directorio **ya existe** → trata duplicado como éxito silencioso.
- Validación solo `nombreNuevo[0] != '\0'` (290). No replica la validación de separadores de `renombrar` (`GestorDeArchivos.cpp:218`).
- Enter no confirma sobre `InputText` (usa foco del botón).

**Causa**:
Lógica de cierre desacoplada del resultado. Semántica de retorno inconsistente ("ya existe" = éxito).

**Fix propuesto**:
- **Validación previa** en UI: rechazar caracteres inválidos (`/ \ : * ? " < > |` en Windows, `/` en Unix) y nombres reservados; mostrar tooltip/error inline.
- **Semántica correcta**: `crearCarpeta` devuelve `false` si ya existe (o devuelve código de error). Alternativa: distinguir `bool creado` vs `yaExistia`.
- **Cerrar solo en éxito**: mover `CloseCurrentPopup()` y reset dentro del `if (ok)`. Si falla, **no cerrar** el modal y mostrar mensaje de error (`ImGui::TextColored` o barra de estado).
- **UX**: usar `ImGuiInputTextFlags_EnterReturnsTrue` en el `InputText` para confirmar con Enter.
- Limpiar estado `creandoCarpeta` (actualmente nunca leído).

**Tests**:
- Crear carpeta con nombre existente → modal **no se cierra**, aparece error. `crearCarpeta` no devuelve true.
- Crear con `/` → rechazado, sin cerrar.
- Crear válida → cierra y aparece.
- `filemanager-tests`: añadir casos para duplicado/caracteres inválidos.
- Suite 20/20.

---

## B8 — FileManager: manifiesto de assets omite las 6 caras del Skybox

**Prioridad:** BAJA. **Depende de:** —.

**Síntoma**:
Los 6 campos de cubemap (`caraPosX/Y/Z`, `caraNegX/Y/Z`) no se serializan/relativizan/absolutizan en `ManifiestoAssetsCore`, solo en el `.db`. El manifiesto promete precedencia sobre `.db` pero no la cumple para Skybox.

**Evidencia**:
- `ManifiestoAssetsCore.h:45-51`: campos declarados.
- `ManifiestoAssetsCore.cpp`: `entradaAJson` (33-44), `jsonAEntrada` (46-62), `relativizarEntrada`/`absolutizarEntrada` (114-126), `entradaVacia` (133-137) **omiten las 6 caras**.
- Sí se manejan en memoria (`ManifiestoAssets.cpp:59-65, 119-132`) y en reescritura de rutas (`RutasReescritura.cpp:178-193`).
- Skybox sí las guarda en `.db` (`Skybox.cpp:31-60`).

> **Implementado**: `CARAS_CUBEMAP` agrupa las seis caras (con su clave JSON) y las cuatro
> funciones del nucleo las recorren: `entradaAJson`, `jsonAEntrada`,
> `relativizarEntrada`/`absolutizarEntrada` y `entradaVacia`. Se agrupan en un array de
> punteros justamente para que no puedan volver a quedar por debajo de las texturas.
> `manifiesto-assets-tests` 37/37, con A/B: 3 fallos al volver al codigo viejo (round-trip
> de las caras, relativizar y `entradaVacia`).

**Impacto**:
Hoy no visible (`.db` tiene la verdad), pero contrato incompleto y propenso a inconsistencias si algún código confía en el manifiesto.

**Fix propuesto**:
Añadir las 6 claves JSON (`caraPosX`, `caraNegX`, ..., `caraNegZ`) en `entradaAJson` y leerlas en `jsonAEntrada` (con defaults cadena vacía). Aplicar `relativizarRuta`/`absolutizarRuta` a cada una en los respectivos métodos. Incluirlas en `entradaVacia` (no cambian el criterio).

**Tests**:
- Guardar/cargar manifiesto con Skybox completo → round-trip preserva las 6 rutas.
- Mover proyecto → rutas se relativizan/absolutizan correctamente.
- Añadir caso en `tests/ManifiestoAssetsTests.cpp` (cubrir Skybox).
- Suite 20/20.

---

## B9 — Documentación: corregir conteos y descripciones desactualizadas

**Prioridad:** BAJA. **Depende de:** — (no toca código).

**Hallazgos (con evidencia en `2026-09-29-documentacion.md`):**
1. `PROJECT_STRUCTURE.md:752` — `escena-serializacion-tests` dice (98) → **(107)**
2. `PROJECT_STRUCTURE.md:688` y `README.md:136` — `configuracion-tests` (145) → **(144)** (Linux; Windows 146)
3. `PROJECT_STRUCTURE.md:989` — "diecinueve targets" → **"veinte targets"**
4. `README.md:19` — lista de componentes EC omite **`Skybox`**
5. `PROJECT_STRUCTURE.md:300` — árbol omite **`Skybox.h`**
6. `PROJECT_STRUCTURE.md` (árbol src/) — añadir `src/Exportador/ (GameExporter.h/.cpp)` y `src/GUI/Export/ (ExportDialog.cpp)`
7. `MANUAL_DE_USO.md:214` — menú contextual del árbol: describir **4 opciones** (Cambiar ID, Renombrar, Desanudar a raíz, Eliminar). Explicar "Desanudar a raíz" solo aparece cuando está anidado.
8. `DOCUMENTACION.md:162-165` — corregir descripción del degradado: interpola por **dirección de vista** (des-proyección NDC), no por posición de pantalla.
9. `DOCUMENTACION.md:124-125`, `README.md:36-37,46` — decir "colores del cielo `fondoSuperior`/`fondoInferior` con `radioDifuminado`", no "color de fondo único".
10. `MANUAL_DE_USO.md:100-121, 460-461` — aclarar marcador `#` (creado por el usuario) y ruta correcta `Memory/Interfaces/<nombre>.json`.

**Fix propuesto**:
Aplicar los 10 ajustes únicamente en `.md` versionados, en el mismo commit que justifique cada cambio (o en un commit `docs:` dedicado si no hay código asociado). No modificar comportamiento.

**Validación**:
Revisar ortografía (sin CJK), coherencia entre README/MANUAL/PROJECT_STRUCTURE.

---

## B10 — Windows: JDK único, excepción preservada y `NewStringUTF` (JNI)

**Prioridad:** ALTA. **Depende de:** —.
**Análisis completo:** `analisis/2026-09-30-b10-b11-b12-windows.md`.

> **Implementado completo (B10.2, B10.3, B10.4 y B10.5)**, todo en el backend Java y el
> instalador. Resumen de lo que quedó y cómo se verificó:
>
> - **B10.4 (raíz única)**: `Behaviour/Backends/ResolucionJdk.h` (header puro, sin JNI,
>   por eso se puede probar en `scripts-tests`) resuelve `libjvm` y `javac` **juntas**, con
>   primer acierto por raíz. Se eliminó el segundo recorrido independiente de
>   `javacExe()`, que era el que mezclaba JDK. `FUNSHI_LIBJVM` deduce su raíz con
>   `raizDesdeLibjvm()` para los cuatro layouts (`lib/server/libjvm.so`,
>   `bin/server/jvm.dll`, `lib/jvm.lib` de FindJNI y la ruta que ya es raíz de JDK, que
>   se toma tal cual y no su carpeta padre).
>   *Correción durante el trabajo*: la primera implementación de `raizDesdeLibjvm`
>   devolvía la carpeta padre cuando la ruta ya era una raíz de JDK, y el propio
>   mensaje del test decía "se toma tal cual". Se detectó al revisar el diff, se
>   bajó a test rojo (1 fallo) y se corrigió la implementación, no el test.
>   `JAVAC` sigue mandando sobre lo deducido (override deliberado y documentado).
> - **B10.2 (`NewStringUTF`)**: conversión a UTF-8 **solo** en la frontera JNI, con
>   `std::filesystem::path::u8string` (en Windows convierte la codificación ANSI nativa a
>   UTF-8): nombre del objeto, ruta de clases y valores de campo `Texto`.
>   `optionString` y `Proceso::ejecutar` **no** se tocan, por el motivo ya refutado de la
>   nota de abajo.
> - **B10.3 (excepción preservada)**: `detalleExcepcion()` lee el mensaje (y si no hay, el
>   nombre de la clase de la excepción) antes de `ExceptionClear()`, y `contextoHerramientas()`
>   añade cache, clases, `javac`, `libjvm` y raíz. Los cuatro puntos que borraban la
>   evidencia (clase `Nativo`, clase `Cargador`, creación del classloader hijo y carga de la
>   clase del script) ahora dicen **por qué** falló. Además, un `javac` con ruta que no
>   existe se detecta antes de invocar el proceso, con `compiladorEjecutable()` en el mismo
>   header puro, y el mensaje dice que hace falta un JDK y no un JRE.
> - **B10.5 (instalador)**: el `.iss` ya no tiene comprobaciones sueltas de `jvm.dll`;
>   todo pasa por `JdkCompletoEn()`, que exige `bin\server\jvm.dll` **y** `bin\javac.exe`,
>   en los cuatro sitios de detección (carpetas comunes, `JAVA_HOME` del proceso,
>   `JAVA_HOME` de la máquina y `jre` embebido).
>
> **Verificación**: `scripts-tests` 125/125, `scripts-java-tests` 22/22, `ctest` 20/20.
> A/B con rojo de verdad (2, 1, 1 y 2 fallos respectivamente al volver al código viejo, y
> verde al restaurar). End-to-end con `FUNSHI_LIBJVM` apuntando al `libjvm.so` real del
> sistema: resuelve `javac` del mismo JDK; con una ruta inexistente el fallo nombra la
> ruta. Suite completa con `TMPDIR` acentuado (`/tmp/opencache/José-test`): 22/22 y la
> carpeta de clases conserva el acento.
> **Hueco declarado**: la conversión ANSI a UTF-8 solo se ejercita de verdad en Windows
> con `%TEMP%` acentuado; en Linux la conversión es identidad, así que el único rojo
> posible para B10.2 ahí es de Windows. Lo mismo con el predicado Pascal del `.iss`,
> que no se puede ejecutar fuera de Windows: su contrato se comprueba sobre el propio
> `.iss` (toda detección pasa por el predicado y solo queda una comprobación de `jvm.dll`,
> la que exige también `javac.exe`).

> **La hipótesis de este ítem cambió después del análisis.** La conversión a UTF-8 de
> la classpath **queda refutada**: `optionString` se decodifica en la codificación
> nativa de plataforma (`CP_ACP` en Windows), no en UTF-8, así que el código actual pasa
> la classpath correctamente y convertirla sería una regresión. Confirmado con la
> especificación JNI y con el código de HotSpot; el experimento previo en Linux no
> podía distinguirlo porque allí la codificación nativa ya es UTF-8.

**Síntoma**:
`FindClass("Nativo")` devuelve NULL con un mensaje genérico que no permite saber por
qué, cuando `%TEMP%` tiene acentos o cuando `javac` y `libjvm` vienen de JDK distintos.

**Evidencia verificada**:
- `BackendJava.cpp:509-514`: la excepción se borra con `ExceptionClear()` y se devuelve
  un mensaje fijo, sin el tipo ni el texto real.
- `BackendJava.cpp:342-348` y `:415-425`: `rutaLibjvm()` y `javacExe()` recorren la lista
  de raíces **por separado** y se quedan con el primer acierto de cada una. Una raíz JRE
  sin `javac` es candidata para una y no para la otra, así que pueden caer en JDK
  distintos; `javac` de Java 17 produce bytecode major 61 y la JVM de Java 8 lo rechaza
  con `UnsupportedClassVersionError`. El comentario de `javacExe()` (416-418) afirma
  que salen del mismo JDK: es la intención declarada y no se cumple.
- `BackendJava.cpp:666`: `NewStringUTF(classesDir)` recibe bytes ACP, y esa función exige
  UTF-8 modificado. Ocurre **después** de `FindClass("Nativo")`, así que **no explica**
  el primer fallo de carga. Con `%TEMP%` ASCII es inocuo.
- `FunshiEngineGL_setup.iss:107-132` y `:148-178`: el instalador busca solo
  `jvm.dll` y no `javac.exe`, así que puede anunciar "JDK detectado" donde el motor no
  puede compilar ningún `.java`.

**Causa**: no es pérdida de codificación en la classpath, sino tres independientes:
una resolución de herramientas que no garantiza coherencia entre `javac` y la JVM, un
diagnóstico que destruye la evidencia, y una conversión solo en la frontera JNI.

**Fix propuesto** (código concreto en el análisis):
1. **Preservar la evidencia** (P0): helper `detalleExcepcion(JNIEnv*)` en el `namespace`
   anónimo de `BackendJava.cpp` que lee el mensaje de la excepción pendiente antes de
   limpiarla, y usarlo en el fallo de `FindClass` junto con cache, clases, javac y libjvm
   efectivos. Añadir el par efectivo a `ScriptRuntime::estadoHerramientas()`.
2. **Una sola raíz para las dos herramientas**: `struct HerramientasJdk { libjvm, javac }`
   resuelta una única vez, para que el `.class` lo produzca el mismo JDK que lo ejecuta.
3. **`NewStringUTF` sí necesita UTF-8, y solo ahí**: helper `utf8DesdeAnsi()` inline en
   `ProjectPaths.h` con guarda `#ifdef _WIN32`, y convertir solo en esa llamada.
   **`optionString` no se toca**: sigue recibiendo la ruta estrecha.
4. **Paridad instalador/motor**: exigir `jvm.dll` **y** `javac.exe` en el `.iss`.

**Tests (los que finalmente se hicieron, todos permanentes)**:
- `scripts-tests` (headless, sin JDK): `ResolucionJdk::desdeRaices` con resolutores
  inyectados y dos raíces falsas en ambos órdenes (la que solo tiene `libjvm` gana con
  `javac` vacío y **no** se buscan las dos por separado), deducción de raíz en los cuatro
  layouts, `compiladorEjecutable` (nombre suelto, ruta existente, ruta inexistente y
  carpeta) y el contrato del `.iss` leído como texto.
- `scripts-java-tests`: el motivo real de la excepción aparece en `error` cuando la clase
  no existe, y el contexto del toolchain (`javac`, `libjvm`, `jdk`, `cache`, `clases`).
  Demasiado mock en el plan original: el par de raíces se prueba en el test puro, que es
  donde el emparejamiento se decide de verdad.
- Sin test permanente para el `TMPDIR` acentuado: en Linux la conversión es identidad, así
  que un test ahí no distingue el código viejo del nuevo. Se comprobó a mano con
  `TMPDIR=/tmp/opencache/José-test` (22/22, ruta acentuada en el log) y el caso real queda
  para Windows.
- `scripts-java-tests`: con classpath inválida, `error` contiene el motivo y el tipo de la
  excepción.
- `scripts-java-tests` con `TMPDIR`/`TEMP` acentuado: el flujo pasa sin `U+FFFD`.
  **Skip por entorno** si la variable no contiene bytes no ASCII, nunca por sistema
  operativo.
- Paridad instalador/motor: test headless que reimplemente el predicado del `.iss` sobre
  un árbol falso y lo compare con `javacEnRaiz`.

---

## B11 — Windows: includes C++ de scripts horneados con ruta absoluta de build

**Prioridad:** ALTA. **Depende de:** —.

**B11.3 cerrado (`855918d`).** Sin hardware Windows se optó por el patrón ya
usado dos veces en el repo (cabeceras y JDK): el valor horneado es una pista.
`FUNSHI_CXX` del entorno manda siempre; el horneado (`CMAKE_CXX_COMPILER`) solo
si **existe en disco**; si no, `vcvars64EnRaices` busca el toolset MSVC de la
máquina a dos niveles bajo las raíces de Visual Studio (cubre las Build Tools,
que cuelgan como las ediciones completas) y se invoca `cl` a secas, que el
propio entorno de vcvars pone en el PATH; sin toolset, `g++`. El override que no
existe se avisa por log. Las dos decisiones son funciones puras con los datos
por parámetro (`elegirCompilador`, `vcvars64EnRaices`) para verificarlas sin
Windows: "no existe" se simula con una ruta debajo de un archivo regular, que
falla en cualquier SO. A/B: sin la comprobación de existencia, 2 de 137
comprobaciones fallan; con el barrido de vcvars reducido a un nivel, 3 de 137.
Riesgo residual: en Windows el nombre de las carpetas de Visual Studio y el
comportamiento de `directory_iterator` con rutas inválidas no se ejercitan (el
CI corre el test sí, pero sin VS instalado el caso real no queda cubierto).

**Implementado B11.1+B11.2 (`91c9a2d`):** CMake hornea `FUNSHI_SRC_DIR="include"` (relativo) y un target `funshi_cabeceras_script` copia las cuatro cabeceras a `build/include`. `RutaCabecerasScript::resolver` (header puro) respeta absoluta existente, acepta relativa al cwd y busca junto al ejecutable; `BackendCpp` avisa si queda vacío, antes de invocar al compilador. `stage_dist_win.ps1`, `stage_dist_linux.sh` y el `.iss` empaquetan `include/`. Rojo A/B: sin la guarda, el aviso no habla de "cabeceras". **B11.3 (`FUNSHI_CXX_COMPILER` con ruta absoluta del checkout + `vcvars64.bat` derivado de esa ruta) queda pendiente de decisión explícita**: es de mayor calado y el hardware Windows no está disponible para validarlo.

**Síntoma**:
Scripts C++ fallan con error de include `IScriptBehaviour.h` en la instalación de Windows (no en desarrollo). El `-I` apunta a `${CMAKE_SOURCE_DIR}/src` de la máquina que compiló (inexistente en el equipo del usuario).

**Evidencia**:
- `BackendCpp.cpp:90-94`: `directorioSrcMotor()` devuelve `FUNSHI_SRC_DIR` (macro horneada por CMake) si `FUNSHI_SRC_DIR` no está en env. `CMakeLists.txt:118-128` define `FUNSHI_SRC_DIR="${CMAKE_SOURCE_DIR}/src"`.
- `ComandoCompilacionCpp.h:190-192`: único `-I`/`/I`.
- Headers del motor **no se empaquetan**: `install()` no copia headers; `stage_dist_win.ps1` y `.iss` solo incluyen exe/DLLs/runtime/imágenes. En `C:\Program Files\FunshiEngineGL\` no existe `Behaviour/` ni `src/`.
- Mismo patrón para `FUNSHI_CXX_COMPILER` (ruta horneada).

**Causa**:
Contrato frágil: depende de árbol de fuentes en runtime. Empaquetado incompleto para el backend C++ de scripts.

**Fix propuesto (doble capa: código + empaquetado):**

**Código (robusto):**
1. `BackendCpp::directorioSrcMotor()` buscar en orden:
   - `FUNSHI_SRC_DIR` del entorno (si existe y `fs::exists(.../Behaviour/IScriptBehaviour.h)`)
   - exe-relativo: `<exeDir>/include`, `<exeDir>/src`, `<exeDir>/../FunshiEngineGL/src` (comprobar existencia del header crítico)
   - fallback al valor horneado (compatibilidad desarrollo)
2. `BackendCpp` (compiler): mismo fallback exe-relativo para `FUNSHI_CXX_COMPILER` si no existe.
3. Mensajes de error: cuando el `-I` no exista, loggear **qué** se buscó (lista de candidatos) y **qué header** faltó (diagnóstico útil).

**Empaquetado (reproducible):**
1. `CMakeLists.txt`: copiar subconjunto mínimo de headers a `${CMAKE_BINARY_DIR}/include/` en build (Behaviour/{IScriptBehaviour.h,ScriptGameObject.h,Reflection/BehaviourReflection.h}, Matematicas/StructVec3.h). No es necesario copiar todo el árbol.
2. `stage_dist_win.ps1:49-53`: copiar `include\` al dist.
3. `FunshiEngineGL_setup.iss:55-61`: incluir `include\*` recursivo.
4. Replicar en `stage_dist_linux.sh` + IFW (para `.run`).

**Tests**:
- Caso: `FUNSHI_SRC_DIR` apunta a ruta inexistente → fallback exe-relativo encuentra `include/` y compila script de prueba.
- Verificar que el `-I` emitido apunta a un directorio que **existe**.
- `scripts-runtime-tests`: añadir caso de layout de instalación simulado.
- `ctest` 20/20.

---

## B12 — Windows: empaquetado, logs y diagnóstico

**Prioridad:** MEDIA. **Depende de:** B10-B11.

**Síntomas transversales:**
- Logs no se generan en `Program Files` (falla creación de `logs/` por permisos). Sin logs → diagnóstico opaco para los 3 frentes.
- `AssetPath::normalize` rompe rutas UNC (`\\server\share\a.obj` → `/server/share/a.obj`) → inválida en Windows.

**Evidencia**:
- `main.cpp:104-140`: escribe logs en `<exeDir>/logs/`. En `{autopf}` (Program Files) no tiene permisos de escritura → excepción capturada, logs perdidos.
- `AssetPath.h:37-53`: reemplaza `\`→`/` y colapsa `//`; pierde prefijo UNC `\\` y rutas extendidas `\\?\`.

**Fix propuesto**:
1. **Logs**: escribir en `%LOCALAPPDATA%\FunshiEngineGL\logs\` o `%APPDATA%\FunshiEngineGL\logs\` cuando `<exeDir>/logs` no es escribible. Intentar en orden: exeDir (dev), `%LOCALAPPDATA%`, `%TEMP%`, cwd. Registrar en log de arranque qué ruta se usó.
2. **UNC/long paths**: en Windows, preservar prefijos `\\?\` y `\\` (UNC). No normalizar barras hacia adelante si el path comienza con `\\` o `\\?\`. Detectar plataforma (`_WIN32`) y tratar UNC como caso especial. Añadir tests para UNC y rutas extendidas.
3. **Versionado del exe**: `CMakeLists.txt:2,6` fijan `FileVersion` a `0.5.4` manualmente (no sigue tag). Añadir variable `PROJECT_VERSION` desde git/tag o actualizar en release (para evitar confusión entre nombre de setup y versión PE). No bloquear este lote, pero documentar.

**B12.3 cerrado (`9f7375d`):** `FUNSHI_VERSION` pasó a variable de caché y
`project()` la usa; los workflows pasan `-DFUNSHI_VERSION=<tag>`. Con el `set`
normal anterior, `--trace-source` mostró que `-DFUNSHI_VERSION=9.9.9` se
ignoraba: el `.exe` y el instalador salían con números distintos en un tag.

**B12.4 cerrado (`c753a49`):** la pregunta del ítem ("¿el motor escribe en
`{app}`?") se respondió midiendo: con el ejecutable real en una carpeta de solo
lectura no escribe nada allí y todo el árbol cae en la carpeta de datos del
usuario. Con eso `PrivilegesRequired=lowest` es seguro y desaparece la UAC (con
`lowest`, Inno mapea `{autopf}` a `%LOCALAPPDATA%\Programs`). Se aisló la
elección de raíz en `ProjectPathsDetalle::elegirRaizDeDatos` para poder probarla;
A/B con la rama de caída retirada: 1 fallo de 174. Lo único que puede pedir
elevación es el MSI del JDK.

**B12.5 cerrado (`dbb0607`):** confirmado en la doc de Inno que `OutputDir` es
relativo a `SourceDir`, así que la CI subía bien. La discrepancia estaba en el
flujo manual: `/O` sobreescribe `OutputDir`, y por eso el instalador acababa en
`packaging\instalador\` en vez de `packaging\dist\instalador\`. Se quitó el
`/O`: los tres consumidores apuntan a la misma ruta.

**Tests**:
- Simular `logs/` no escribible → crea logs en `%TEMP%`/fallback.
- `AssetPath::normalize` con `\\\\server\\share\\a.obj` conserva UNC (o equivalente válido).
- `ctest` 20/20.

**Implementado B12.1:** `RutasLog::candidatas()` (header puro, `src/Configuracion/RutasLog.h`) da la lista ordenada y deduplicada de candidatas — raíz de datos (`ProjectPaths::directorioBase()`, que ya cae a la carpeta del usuario), carpeta del ejecutable (`argv[0]`) y temporal del sistema (`<temp>/FunshiEngineGL`) — y `redirigirSalidaALog()` usa la **primera que acepte escribir**, en vez de asumir la del ejecutable. `stderr` que no se puede redirigir ya no invalida un `stdout` que sí lo está.

El plan inicial del ítem era "intentar exeDir, `%LOCALAPPDATA%`, `%TEMP%`, cwd". Se cambió por el orden *raíz de datos → ejecutable → temporal* porque la raíz de datos ya resuelve el caso de `Program Files` con la prueba de escritura real (y avisa al usuario por log y barra de estado), mientras que `%LOCALAPPDATA%` duplicaría esa decisión; el ejecutable se conserva como segundo para que el build de desarrollo siga escribiendo junto al binario cuando la raíz de datos sea la de usuario, y la temporal queda como último recurso.

**Hallazgo nuevo dentro de B12.1 (arreglado en el mismo ítem):** al escribir el log en la raíz de datos, `logs/` era una carpeta más de la raíz y `migrarProyectosAntiguos()` la movía a `Proyects/logs/` en el mismo arranque —el log quedaba anunciando una ruta que ya no era la real y la carpeta original vacía—. No es un problema de permisos sino de conocimiento: la única barrera es `esReservado()`, así que `logs` pasa a la lista de nombres reservados (Patrón R13 en `DocuTecnicoBugs.md`, concepto nuevo §11).

**Evidencia del A/B (motor real, dos circunstancias):** con el binario lanzado por un symlink en una carpeta `chmod 555`, la versión de una sola candidata (la del ejecutable) **no deja ningún log** y vuelca 4770 bytes a la consola; con la lista de candidatas el log aparece en `<raizDatos>/logs/`, nada va a la consola y tras la inicialización completa la carpeta sigue ahí (no aparece `Proyects/logs`). Con el ejecutable en una carpeta escribible el log va igualmente a la raíz de datos, por orden de candidatos.

---

## B13 — Deuda menor (FileManager, rutas)

**Prioridad:** BAJA. **Depende de:** —.

**Hallazgos:**

| Ítem | Ubicación | Descripción | Solución |
|---|---|---|---|
| Dm1 | `GestorDeArchivos.cpp` (`copiarCarpeta`) | **CERRADO** `eb13e26`. Guard `destinoDentroDelOrigen()` (namespace anónimo) con `PathUtils::rutaBajo` sobre rutas `lexically_normal()`, reutilizado por `mover` (sustituye el cotejo `rfind` a mano). Test: destino anidado, origen sobre sí mismo, hermano con prefijo común (183 comprobaciones). Evidencia: sin guard, `fs::copy` recursivo se reproduce (`.../dentro/dentro/...`) hasta ENAMETOOLONG y deja el destino creado; con guard, 183/183. | — |
| Dm2 | `SoltarEnCarpeta.h:78` | **CERRADO** `eeee7fa`. `PathUtils::indiceSeparadorFinal()`; el arrastre y las dos acciones de importar del diálogo del sistema lo usan. Rojo: 4 fallos (renombraba a `barra.txt`, padre inventado, evento truncado). | — |
| Dm3 | `AssetPath::normalize` (UNC) | **CERRADO** con B12.2 (`5d87059`): conserva `//` y no toca `\\?\` ni `\\.\`. | — |
| Dm4 | `GameExporter.cpp` | **CERRADO** `b38fdc4`. Tres rutas mal (`Sonidos`, `ConfiguracionProyecto.json`, `scripts`); ahora salen de `ProjectPaths` vía `Exportador/RutasExportacion.h` (header-only) y hay `ProjectPaths::directorioScripts`. Rojo: 3 fallos. | — |
| Dm5 | Prefijos case-insensitive (Windows) | **CERRADO** `8a742d2`. `bytesRutaEquivalentes()` pliega el caso solo bajo `_WIN32` (en Linux `X` y `x` son archivos distintos). La rama Windows se verifico con la sonda `FunshiEngineGL/sondas/rutaBajoPlataforma.cpp` compilada con y sin `-D_WIN32`: con el plegado retirado la sonda Windows da 2 fallos y la POSIX sigue verde. | — |
| Dm6 | Comentarios con IDs de tarea | **CERRADO** `23cf77e`. 32 referencias (R1-R8, B3-B7) y 4 "FASE n" en 13 archivos. Las teclas `F6` no son identificadores y se quedaron. | — |

**Tests:** `filemanager-tests` 190 (186 en Windows), `configuracion-tests` 169.

**Estado:** cerrado.

---

## Estado general del Lote B

- **Total ítems:** 13 (B1–B13)
- **ALTA:** B1,B2,B3,B4,B5,B10,B11 (7)
- **MEDIA-ALTA:** B6 (1)
- **MEDIA:** B7,B12 (2)
- **BAJA:** B8,B9,B13 (3)

**Orden de ejecución:** ver la sección **"Orden de aplicación corregido"**, arriba, que
sustituye a la lista provisional de este resumen.
---

## Auditoría del patrón de valor horneado (cierre del lote)

La checklist de `DocuTecnicoBugs.md` §12.5 pedía `grep FUNSHI_ CMakeLists.txt`
para listar los valores que hornea CMake y comprobar uno por uno si se consumen
con la comprobación de existencia. Solo se había hecho para los que tocaba cada
ítem; se completó aquí.

Valores horneados y su veredicto:

| Variable | Cómo la consume el código | Veredicto |
|---|---|---|
| `FUNSHI_CXX_COMPILER` | `BackendCpp::compilador()`, que ya comprueba existencia | correcto (B11.3) |
| `FUNSHI_SRC_DIR` | `RutaCabecerasScript`, que lo trata como nombre relativo y busca junto al ejecutable | correcto (B11.1+B11.2) |
| `FUNSHI_JAVAC_DEFAULT` | `javacExe()`, y el llamador pasa el resultado por `ResolucionJdk::compiladorEjecutable`, que **sí** comprueba existencia | correcto: degrada al nombre que resuelve el PATH |
| `FUNSHI_LIBJVM_DEFAULT` | `raicesJdk()` lo metía en la lista de **raíces**, y `libjvmEnRaiz()` le colgaba `lib/server/libjvm.so` | **roto**: se descartaba siempre (instancia #4, corregida) |
| `FUNSHI_CXX_RUNTIME_FLAG` | `argumentosCompilacion`, sin comprobar | correcto: un flag del CRT, no una ruta de máquina |
| `FUNSHI_VERSION` | `project()` y los `.rc` de Windows | correcto: dato de versión, no ruta |

El fallo de `FUNSHI_LIBJVM_DEFAULT` tiene una característica que la checklist no
contemplaba: no era "se usa sin comprobar", sino "se usa donde no sirve". El
valor es la ruta del **archivo** de biblioteca (`JAVA_JVM_LIBRARY`), y el
consumidor esperaba una **carpeta** raíz de JDK; al pasarlo por la lista de
raíces, la comprobación de existencia sí ocurría, pero sobre
`<archivo>/lib/server/libjvm.so`, que no existe jamás. El síntoma era invisible:
un `continue` en el bucle. Peor: tampoco funcionaba en la máquina donde se
compiló, porque ahí el archivo **sí** existe pero la ruta buscada tampoco.

Fix: `ResolucionJdk::conBibliotecaHorneada`, que trata el valor como lo que es
(comprobación de existencia del archivo + `raizDesdeLibjvm` para buscarle el
`javac` al lado) y solo entra si las raíces del sistema no dieron nada. La
función es pura y recibe `existe` por parámetro, para poder ejercitar el "no
existe" sin disco. A/B: con el valor neutralizado, 1 fallo de 142; restaurado,
142/142. `scripts-java-tests` 22/22 y `ctest` 20/20.

La checklist de §12.5 se amplía con dos preguntas nuevas: si el horneado es un
archivo y el resolutor espera una carpeta, y si el resolutor sabe qué tipo de
cosa recibe.
---

## B14 — Cámara hija: navegación escribe pose mundial en local sin compensar padre

**Prioridad:** ALTA. **Depende de:** —.

**Síntoma:** Al poner la cámara como HIJA de otro objeto y mover ese objeto padre, las coordenadas de la cámara "se disparan de forma errática" (deriva/explosión). Al mover la cámara con WASD/mouse/orbita mientras es hija, también se observa deriva.

**Evidencia (archivo:línea):**

- Lectura global: `CameraComponent::leerDesdeTransform()` llama `owner->getGlobalTransform()` (`CameraComponent.cpp:113`) y guarda `m_pos`/dirección/yaw derivados del transform GLOBAL (`CameraComponent.cpp:123-131, 137-142`).
- Escritura local sin compensar: `CameraComponent::escribirATransform()` obtiene `owner->getComponent<Transform>()` (local, `CameraComponent.cpp:109,157`), construye una matriz con `m_pos` (mundo) y `yawX/yawY` (derivados de mundo) y hace `decomposeMatrixToTransform` directamente al local (`CameraComponent.cpp:165-178`). No existe `inverse(parentGlobal)` en `CameraComponent.cpp`.
- Navegación: `EditorInput::aplicarMovimiento()` → `CameraComponent::moverDireccion()` lee global y escribe local (`EditorInput.cpp:244`, `CameraComponent.cpp:260-271`). `updateYaw()` (`CameraComponent.cpp:273-281`), `orbitAround()` (`CameraComponent.cpp:283-313`) siguen el mismo patrón. EditorInput también escribe tras ajustar radio de órbita (`EditorInput.cpp:422,452,659`).
- Correcto: Gizmo usa `newLocal = inv(parentGlobal) * mManip` (`GizmoController.cpp:308-321`, `354`); reparent conserva pose con `inverse(mundoPadre)*mundoHijo` (`SceneRegistry.cpp:225-234`).

**Causa:** Realimentación frame a frame: frame n → `g_n = P * l_n`. Navegación suma Δ en mundo → `m_pos += ...` (mundo). `escribirATransform()` asigna `l_{n+1} = g_n + Δ` como local. Frame n+1 → `g_{n+1} = P * l_{n+1} = P*(g_n + Δ)`. Con `P != identidad` (cámara hija de un objeto con transform no nulo), el término `P*Δ` se añade cada escritura (y si el padre es trasladado, la acumulación es `t` por escritura). Los yaw también se reconducen desde el global y se escriben como rotación local sin rotar inversamente por el padre. No hay doble acumulación en Transform (asigna `mundo = padre*local`, idempotente), la acumulación ocurre porque el mundo ya compuesto se vierte sin compensación al local.

**Hipótesis competidora descartada:** "doble multiplicación en Transform" — `Transform::actualizarMatrizMundo()` asigna `matrizMundo = padre->getMatrizMundo() * matrizLocal` (Transform.cpp:86), no multiplica acumulando. Tampoco `actualizarMatrizMundoRecursivo()` (Transform.cpp:98). Por tanto, el cálculo de mundo no acumula; quien acumula es el write-back de la cámara.

**Fix propuesto (una sola variable):** En `CameraComponent::escribirATransform()`, construir `matMundo = T(m_pos) * R(-yawY)*R(-yawX)*S(scaleLocal)`, obtener la matriz mundo del padre del owner (`owner->getParentEntity()` + `getGlobalTransform()`), si existe padre → `matLocal = inv(padreGlobal) * matMundo`, descomponer `matLocal` en el Transform local; si no hay padre → `matLocal = matMundo`. Reutilizar el mismo enfoque para cualquier camino que escriba pose mundial en local del owner.

**Validación:** Test headless que simule owner con Transform local y Transform padre no identidad; tras `leerDesdeTransform()` (lee global), simular navegación que fije `m_pos` a un valor mundo y llamar `escribirATransform()` → el local escrito debe ser `inv(padre)*mundo`. Rojo antes, verde después. A/B con escena visual (cámara hija, mover padre y luego mover cámara con WASD): sin fix deriva; con fix se mueve coherentemente.

**Riesgo:** Cambio circunscrito a `CameraComponent`. Efectos colaterales: (a) sin protección ante matriz singular (congelación silenciosa), (b) deriva de escala si el padre tiene escala ≠ 1, (c) rama `transformOrigin` poco coherente (poco probable). Ver hallazgos colaterales abajo.
**Hallazgos colaterales tras fix B14 (con subagentes):**

- Protecciones: `glm::inverse(parentGlobal)` (`CameraComponent.cpp:189`) no valida matriz singular/no finita. `Transform.cpp:169-172` aborta en silencio si `decomposeMatrixToTransform` recibe no-finitos → cámara deja de responder sin log. `GizmoController.cpp:314-319` sí protege el mismo cálculo.
- Rama Entity+transformOrigin: inalcanzable (`GameObject` es única subclase) y semánticamente incorrecta si se alcanzara (usa `getOriginTransform()` del padre en lugar del global). Poco riesgo en práctica.
- Deriva de escala: se toma `scaleLocal` (`:160`) y se escribe en `matMundo` (`:171`), luego `inv(P)*matMundo` modifica la escala local cada escritura con `P` escalado ≠1 (`leerDesdeTransform` no restaura escala). Con escala de padre unitaria (root por defecto) no ocurre. Requiere seguimiento.
- Casos que aún escriben mundo→local sin compensar: `GameScene.cpp:393-397` (copiar pose de cámara activa a nueva cámara) y `GameScene.cpp:333-335` (sembrar CamaraPrincipal). Ambos con padre root (identidad) → no visible.
- Tests: ningún test ejercita `CameraComponent`; el plan pedía test headless rojo→verde pero no se creó.

**Recomendación:** añadir guardas finitas (igual patrón que Gizmo) antes de `glm::inverse` y considerar test headless mínimo para el caso padre identidad vs traslación. Validación visual requerida por usuario.
