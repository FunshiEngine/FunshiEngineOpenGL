# MEMORIA.md

Estado inicial: auditoría arquitectónica solicitada (área por área). 
- No reverter trabajo en proceso. Solo leer, contrastar docs vs código, registrar hallazgos arquitectónicos, proponer actualizaciones de docs si hay desactualizaciones. No tocar código funcional todavía (solo documentar).
- Seguir AGENTS.md: contrastar TODAS las fuentes, orden por dependencias si surgen tareas, etc.

Archivos clave ya leídos: AGENTS.md (guía). Faltan resto de docs y revisión de código.
- Creado REORGANIZACION.md
- Recortado README.md a versión concisa (quitado detalle abrumador).
- Consolidada sección Rendering en PROJECT_STRUCTURE.md (doc-only) eliminando redundancia.
## Hallazgo B14: cámara hija — deriva por conversión mundo→local sin compensar padre (2026-10-05)

**Síntoma:** al poner la cámara como hija de otro objeto y mover ese objeto padre, las coordenadas de la cámara "se disparan de forma errática". Al mover la cámara con WASD/mouse/orbita mientras es hija, también se observa deriva.

**Evidencia (factual):**
- `CameraComponent::leerDesdeTransform()` lee el transform GLOBAL (`owner->getGlobalTransform()`) y deja `m_pos`/dirección/yaw en espacio mundo (`CameraComponent.cpp:113,123-131,137-142`).
- `CameraComponent::escribirATransform()` construye una matriz `mat` con `m_pos` (mundo) y `yawX/yawY` (mundo) y la descompone **directamente** en el Transform LOCAL (`owner->getComponent<Transform>()`) sin hacer `inv(parentGlobal)` (`CameraComponent.cpp:157,165-178`). No existe esa inversión en `CameraComponent.cpp`.
- Navegación (WASD/mouse/orbita, ajustes de radio) hace `leerDesdeTransform()` → suma Δ en mundo → `escribirATransform()` (write-back local crudo). (`EditorInput.cpp:244`, `CameraComponent.cpp:260-271,273-281,283-313`; `EditorInput.cpp:422,452,659`)
- Correcto: Gizmo usa `newLocal = inv(parentGlobal) * mManip` (`GizmoController.cpp:308-321,354`); reparent conserva pose con `inverse(mundoPadre)*mundoHijo` (`SceneRegistry.cpp:225-234`).

**Causa (realimentación frame a frame):** frame n: `g_n = P * l_n`. Navegación suma Δ en mundo → `m_pos = g_n_pos + Δ` (en práctica). `escribirATransform()` escribe `l_{n+1} = (g_n en forma de matriz mundo)` como local (sin compensar P). Frame n+1: `g_{n+1} = P * l_{n+1} = P * (matrizMundoConstruidaDesde_gn_mas_Delta)`. Con `P != identidad`, cada escritura vuelve a componer el padre sobre una pose ya mundial. La acumulación no está en `Transform::actualizarMatrizMundo()` (que asigna `mundo = padre*local`, idempotente), sino en este write-back.

**Fix propuesto:** en `escribirATransform()`, construir `matMundo = T(m_pos)*R(-yawY)*R(-yawX)*S(scaleLocal)`, obtener `parentGlobal` del padre de `owner` (si existe), hacer `matLocal = inv(parentGlobal)*matMundo` cuando hay padre; en caso contrario `matLocal = matMundo`. Descomponer `matLocal` al Transform local. Cambiar solo `CameraComponent.cpp`.

**Validación:** test headless comparando local esperado vs escrito (rojo antes, verde después); A/B visual con cámara hija (sin fix → deriva, con fix → coherente); `ctest 20/20` sin romper nada.

**Riesgo:** circunscrito a CameraComponent (único sitio con este patrón de write-back). No toca Transform/GameObject/comandos.

## B14: fix aplicado (2026-10-05)

Se corrigió CameraComponent::escribirATransform() para convertir el pose mundial (m_pos + yawX/yawY, construido con escala local) al Transform local compensando el padre: si owner tiene padre GameObject se usa parentObj->getGlobalTransform(); en otro caso se consulta parentEntity->getOriginTransform(). matLocal = inv(parentGlobal)*matMundo cuando existe padre. Sin padre, matLocal = matMundo. ctest 20/20. Validación visual pendiente: cámara hija + WASD/mouse/orbita sin deriva.


## Sistema de tiempos y UI (requerimiento usuario 2026-10-06)

El usuario solicitó:
- Tres tiempos: **Edición** (inactivo), **Depuración** (activo/debug), **Juego** (tercer tiempo: quita elementos de depuración, funciona como exportado, controles solo si scripts lo programaron).
- 5 botones: Depuración, Pausa (no reinicia), Reset, Play (tiempo de juego), Terminar. Lógica de visibilidad: al tocar Play o Depuración, ambos se ocultan y aparecen los otros 3 (Pausa, Reset, Terminar). No se pueden mostrar todos simultáneamente según estados.
- Cámara principal y malla solo utilizables en Edición y Depuración; deben desaparecer en tiempo de Juego.
- Árboles/filtrado por tiempo: evaluar recorrer árboles distintos o comparar objetos por "tiempo/visibilidad" según arquitectura actual (GameObject/Entity, jerarquía).

Estado actual del código (búsqueda):
- No existen enums/modos `ModoDebug`, `ModoJuego`, `TiempoEdicion/Depuracion/Juego` ni botones nuevos en SceneGUI. Solo aparece `modoPlay` en CanvasInterface.h (`setModoPlay`) y lógica de editor activo en GameScene (`toggleEditorInterfaces`, `isEditorActivo`). 
- Imágenes añadidas (untracked): `debug-button-white.png`, `pause-button-white.png`, `play-button-triangle-white.png`, `restart-button-white.png`, `stop-button-white.png`.
- No hay implementación UI para esos 5 botones ni lógica de estados de tiempo.

Recomendaciones de diseño (basadas en arquitectura existente, sin modificar contratos innecesarios):
1. **Modelo de estado**: añadir enum `enum class EstadoTiempo { Edicion, Depuracion, Juego }` (o similar). Mantener separación clara (no mezclar con `modoPlay` del Canvas). Estado global en GameScene/EditorController (o contexto UI).
2. **Visibilidad de objetos por tiempo**: preferir **flag por objeto** (`visibleEnEdicion`, `visibleEnDepuracion`, `visibleEnJuego` o máscara). GameObject ya tiene `state` (bool) y jerarquía; filtrar en render (`SceneRenderer`) y en jerarquía (`SceneObjectTree`) según `EstadoTiempo` actual. Evitar duplicar árboles (complejidad de sincronización). Filtrado por comparación es simple y consistente con `state`.
3. **Cámara principal/malla**: marcar esos GameObjects con visibilidad restringida (sólo Edición+Depuración). Componentes (Model/CameraComponent) no se destruyen, solo no se dibujan/renderizan ni aparecen en selección según filtro de tiempo.
4. **Lógica de botones**: máquina de estados mínima. Transiciones: Edición→Depuración (activa debug), Edición→Juego (play tiempo juego), Depuración/Juego→Pausa/Reset/Terminar según flujo. Al entrar a Juego: ocultar UI de depuración (gizmos, jerarquía/settings/folders si procede), desactivar captura/input asociado a editor, activar lógica de scripts (Canvas `modoPlay` puede coexistir o integrarse). Al salir (Terminar/Reset) vuelve a estado anterior/Edición.
5. **Arquitectura**: añadir estado en `GameScene` (única fuente de verdad de escena+modo), exponer getters/setters, propagar a `SceneMenuBarInterface` (barra superior) para dibujar botones con imágenes nuevas. Iconos cargados vía `IconosGUI` (ya tiene patrón de carga PNG desde rutas relativas al exe/cwd).
6. **No romper contratos**: respetar separación Rendering/UI (SceneRenderer filtra por visibilidad/tiempo), no tocar serialización salvo flags opcionales si se quieren persistir (probablemente no necesario).

Pendiente: implementar enum+estado, filtros por tiempo, UI de 5 botones con lógica de visibilidad por estado, y ocultar cámara/malla en Juego. Todo documentado aquí para no perderlo.
