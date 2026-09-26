# Cámaras y vistas previas en vivo (Fase 2)

Documentación de la Fase 2: la cámara pasó de ser un objeto global (`Camera`) a
ser un **componente** (`CameraComponent`) asociado a un `GameObject`, con
**vistas previas en vivo**: cada cámara que tenga activo el checkbox *Vista
previa* renderiza la escena a una textura (FBO) y la muestra en su propia
ventana ImGui ("Vista previa: \<nombre\>").

Incluye el flujo de edición (ventana "Cámaras"), la solución de los crashes
encontrados y el diagnóstico del bug de la ventana de previsualización en negro.

---

## 1. Resumen de lo que se construyó

| Pieza | Qué resuelve |
|---|---|
| `CameraComponent` | Cámara como componente adjuntable a cualquier `GameObject`; la vista se deriva del `Transform` global del dueño. Navegación FPS propia, matrices de vista/proyección, flag `pintar` para la vista previa en vivo. |
| `RenderTarget` | Render-to-texture con FBO (framebuffer + textura de color + renderbuffer de profundidad), sin GLAD/glew: las funciones se cargan por puntero con `glfwGetProcAddress`. |
| Ventana "Cámaras" | Crear cámaras desde la vista activa, elegir cuál se usa para navegar, prender/apagar su vista previa y eliminarlas. |
| Vistas previas | Ventana ImGui pasiva por cámara que muestra en vivo lo que ve esa cámara. |
| Fixes de crash | Corrupción de la serialización con `CameraComponent` y stack-buffer-overflow en el marcador de frustum (≥2 cámaras). |
| Fix de render | Ventana de previsualización en negro por estado del buffer destino del FBO (`glDrawBuffer`). |

---

## 2. `CameraComponent` — la cámara como componente

Archivo: `src/Objetos/Componentes/CameraComponent.h` (+ `.cpp`).

Mismo patrón que `Light`: se adjunta a un `GameObject`, normalmente un objeto
vacío con `Transform`. La posición/orientación de la vista se deriva del
`Transform` **global** del dueño, de modo que el gizmo y el inspector pueden
editar la cámara como a cualquier otro objeto.

Estado interno:

- `m_pos[3]`, `m_dir[3]`, `m_left[3]`, `m_up[3]` — base de la vista.
- `yawX` / `yawY` — acumuladores de rotación del mouse (navegación FPS).
- `speed` (5.0), `fov` (45°), `nearPlane` (0.1), `farPlane` (1000).
  - Nota: la pasada principal usaba antes `gluPerspective(far=500)`; ahora se
    usa siempre `farPlane = 1000`.
- `pintar` (bool, default `false`) — flag de la vista previa en vivo. Se
  serializa junto con el resto del componente.

API relevante:

- `setUp(GameObject*)` — vincula la cámara a su dueño.
- `getViewMatrix(float*)` / `getProjectionMatrix(float*, float aspect)` — vistas
  no cacheadas; la vista se recalcula desde el `Transform` actual (gizmo, picking
  y render comparten la misma fuente de verdad).
- `forward/back/left/right/up/down/diagonales(float dt)` y `updateYaw(dx, dy)` —
  navegación FPS (misma API que la vieja `Camera`).
- `getPintar()/setPintar(bool)` — control de la vista previa.

---

## 3. `RenderTarget` — render a textura (FBO)

Nuevo archivo: `src/Rendering/RenderTarget.h` (+ `.cpp`).

Objetivo: pintar la escena desde cualquier cámara a una textura, sin depender
de GLAD/glew. Los FBO son una extensión que el `gl.h` del sistema no declara en
todas las plataformas; se cargan por puntero con el mismo mecanismo que usa
GLFW:

```cpp
typedef void (GLAPIENTRY* FN_BindFramebuffer)(GLenum, GLuint);
// ... 10 punteros: Gen/Delete/Bind Framebuffer y Renderbuffer,
//     FramebufferTexture2D, FramebufferRenderbuffer, RenderbufferStorage,
//     CheckFramebufferStatus ...
template <typename T>
void cargar(const char* nombre, T& destino) {
    if (!destino) destino = reinterpret_cast<T>(glfwGetProcAddress(nombre));
}
bool funcionesCargadas() {
    cargar("glGenFramebuffers", pfnGenFramebuffers);
    cargar("glBindFramebuffer", pfnBindFramebuffer);
    // ...
    return /* todos no nulos */;
}
```

Ciclo de vida / operaciones:

- `resize(w, h)` — crea FBO + textura RGBA8 (filtro `GL_LINEAR`,
  `GL_CLAMP_TO_EDGE`) + renderbuffer de profundidad `GL_DEPTH_COMPONENT24`,
  atacha color en `GL_COLOR_ATTACHMENT0` y profundidad, verifica
  `glCheckFramebufferStatus`. No-op si el tamaño no cambia (así se reutiliza
  la textura entre frames).
- `bind()` — `glBindFramebuffer(GL_FRAMEBUFFER, fbo)` **y además fuerza el
  buffer destino/lectura del FBO** (ver §6.3, es el fix de la ventana negra):

```cpp
void RenderTarget::bind() {
    if (!fbo || !pfnBindFramebuffer) return;
    pfnBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
}
```

- `unbind()` — devuelve a framebuffer 0 (la ventana).
- Los FBO se **reutilizan por objeto entre frames**: no se recrean a menos que
  cambie el tamaño. Evita churn de texturas en el GPU.
- Si `funcionesCargadas()` falla (GPU sin FBO), imprime `[RenderTarget] FBO no
  disponible en este GPU.` y las vistas previas simplemente no se dibujan.

---

## 4. Pipeline de vistas previas en `GameScene`

### 4.1 `dibujarViewportsPrevios()` — pasada al FBO

Corre **antes** de la pasada principal, todos los frames:

1. Itera los `GameObject` con `CameraComponent` y `getPintar() == true`.
2. `camara->setUp(objeto)` (vincula al dueño).
3. Toma o crea el `RenderTarget` reutilizado para ese objeto.
4. `resize(400, 250)` → `bind()` → `glViewport(0,0,400,250)` →
   `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)` (ñálogo del fix de
   buffer limpio; evita basura en el primer frame).
5. Calcula view/projection de esa cámara con `aspect = 400/250`.
6. `dibujarEscena(view, projection, objeto)` — el parámetro extra es el
   **"ojo"**: la cámara que está viendo. El marcador de frustum de esa cámara
   no se dibuja (no se dibuja a sí misma).
7. `RenderTarget::unbind()`.

```cpp
camara->setUp(objeto);
// reutilizar el FBO del frame anterior del mismo objeto...
target->resize(kPreviewW, kPreviewH);
target->bind();
glViewport(0, 0, kPreviewW, kPreviewH);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

float view[16], projection[16];
camara->getViewMatrix(view);
camara->getProjectionMatrix(projection,
                            kPreviewW / (float)kPreviewH);
dibujarEscena(view, projection, objeto);
RenderTarget::unbind();
```

Constantes: `kPreviewW = 400`, `kPreviewH = 250` (`GameScene.h`).

### 4.2 Refactor del dibujado con "ojo"

`dibujarEscena(view, projection, camaraOjo)` → `dibujarGameObjectsConOjo(...)`
→ `dibujarObjectConOjo(...)`. Antes se dibujaba directo desde la `Camera`.
Hoy el render comparte las matrices de la cámara activa y los marcadores de
frustum solo se pintan para las cámaras que **no** son el ojo.

Pasada principal (`gameScene()`):

```cpp
dibujarViewportsPrevios();
RenderTarget::unbind();          // volver al framebuffer de la ventana
glViewport(0, 0, fbW, fbH);
// view/projection de getActiveCamera()
dibujarEscena(view, projection, activeCameraObject);
```

### 4.3 `pintarViewportsGUI()` — mostrar las texturas

Dentro de `GUI()` (solo cuando el editor está activo, ver §7):

- Por cada vista previa dibuja `ImGui::Begin("Vista previa: <inputName>")`.
- `ImGui::Image` de 400×250 con **UVs verticales invertidas**
  `(0,1) → (1,0)`: la textura del FBO tiene el origen abajo-izquierda.
- El cast del id de textura a `ImTextureID` usa `(intptr_t)` para no truncar
  en punteros de 64 bits.
- Tamaño inicial fijo con `ImGui::SetNextWindowSize(..., ImGuiCond_Once)`
  (ventanas flotantes/colapsables, con la extensión docking nativa de ImGui).

---

## 5. Ventana "Cámaras" — flujo de edición

Dentro de `GUI()`: `pintarVentanaCamaras()` (una sola ventana única
`ventanaCamarasAbierta`, abierta por defecto).

### 5.1 `agregarCamaraEnVistaActiva()` — crear cámara

1. Asegura cámara activa (`getActiveCamera()`); toma su `Transform` global.
2. Crea un `GameObject` vacío (`Modelos3D(nullptr)`) vía
   `editorController->createGameObject`.
3. Copia **traslación y rotación** del `Transform` global de la cámara activa
   (la nueva cámara nace apuntando a lo mismo).
4. Nombre único tipo `"Camara N"` (`N` viene de `contadorCamaras`, con
   verificación de colisión) — necesario porque ImGui identifica ventanas por
   título y las vistas previas usan el nombre del objeto.
5. `pintar = true` (su vista previa aparece sola), `setActiveCamera(creada)` y
   `selectObject(creada)` para poder ubicarla con el gizmo.

### 5.2 Elementos de la ventana

- Botón **“Agregar camara”** → crea la cámara en la vista activa (ver arriba).
- Lista de objetos con cámara; cada fila tiene:
  - `Selectable "Usar"` → `setActiveCamera(objeto)` (la navegación y la vista
    principal pasan a esa cámara). Tooltip: *"Usar esta camara (navegacion +
    vista)"*.
  - `Checkbox "Vista previa"` → `camara->setPintar(...)` (enciende/apaga su
    ventana de preview).
  - `SmallButton "Eliminar"` → borra la cámara. Si era la `requestedActiveCamera`
    se limpia (para que `getActiveCamera` recalcule). Tras mutar la lista se hace
    `break` (no seguir iterando sobre nodos que ya no existen).

### 5.3 Resolución de la cámara activa

`getActiveCamera()` (sin recursión infinita; el caso de siembra solo ocurre una
vez):

1. Si `requestedActiveCamera` sigue viva en `sceneRegistry` y conserva su
   `CameraComponent` → esa es la activa. Si ya no existe, se resetea a `nullptr`.
2. Si no, la **primera** cámara de la escena (scan lineal).
3. Si la escena no tiene ninguna → siembra un objeto vacío
   `"CamaraPrincipal"` con `Transform` (posición `(0, 8, 40)`, mirando `-Z`
   hacia el origen donde vive la grilla; la posición histórica `(1, 1, -50)`
   quedaba de espaldas a ella) + `CameraComponent`, y reintenta.

`setActiveCamera(object)` valida que el objeto exista en `sceneRegistry` y que
tenga `CameraComponent`; solo entonces lo asigna a `requestedActiveCamera`.

---

## 6. Bugs encontrados y corregidos durante la Fase 2

### 6.1 Corrupción al guardar/cargar con cámaras (serialización)

**Síntoma:** la app se cerraba al agregar cámaras y guardar/recargar la escena.

**Causa:** la escena se autoguarda al salir y se autocarga al arrancar
(`main.cpp`). `ComponentFactory` solo reconocía el tipo `"Camera"`, pero el
nombre RTTI que deja la serialización es `"CameraComponent"` (resultado de
`demangle(typeid(...).name())`). Al no reconocer el nombre, `loadComponent` se
salteaba el componente y el stream binario quedaba **desincronizado** → datos
corruptos y crash al cargar escenas que contenían cámaras.

**Fix** en `src/Objetos/Componentes/ComponentFactory.cpp`:

```cpp
if (typeName == "CameraComponent" || typeName == "Camera")
    return std::make_unique<CameraComponent>();
```

`"Camera"` se conserva como alias hacia atrás.

### 6.2 Stack-buffer-overflow en el marcador de frustum

**Síntoma:** crash (stack-buffer-overflow) con ≥2 cámaras en la escena.

**Causa:** `dibujarMarcadorCamara` dibuja el octaedro del frustum de cada
cámara con una tabla `edges[12][2]` cuyos índices van de 0 a 7, pero los
vértices estaban en dos arrays separados `vN[4][3]` / `vF[4][3]` de a 4
elementos. Acceder a `vN[7]` es lectura fuera de pila. Se reproducía con
gdb + ASan (build con sanitizers, ver §8).

**Fix:** un solo array de 8 vértices:

```cpp
float vFrustum[8][3];   // se llena con los 4 del near + 4 del far
// edges[12][2] indexa 0..7 sobre vFrustum
```

### 6.3 Ventana de previsualización negra / "no hace nada"

**Síntoma (reporte del usuario):** *"la ventana de previsualización no hace
nada, la idea es que la cámara esté en esa ventana"*: la ventana aparecía pero
vacía/inerte.

**Diagnóstico (cómo se encontró):** se instrumentó temporalmente una sonda de
píxeles dentro de la pasada de preview (leer el FBO con `glReadPixels` y contar
píxeles no negros). Resultado revelador:

| Escenario | Píxeles no-negros (de 100 000) |
|---|---|
| `dibujarEscena` sobre el FBO, sin `glDrawBuffer` explícito | **0** (textura vacía) |
| Con `glClear` de control (azul) antes de dibujar | 23 180 (el FBO **sí** dibuja) |
| Con `glDrawBuffer(GL_COLOR_ATTACHMENT0)` + `glClear` | **92 330** |

El FBO escribía bien; el problema era el **estado del buffer destino**: en
contextos GL legacy el `DRAW_BUFFER` del FBO puede quedar en `GL_BACK`, con lo
que todos los draws se descartan y la textura permanece negra — exactamente la
ventana "que no hace nada".

**Fix** (doble):

1. `RenderTarget::bind()` fuerza el destino correcto del FBO:
   ```cpp
   glDrawBuffer(GL_COLOR_ATTACHMENT0);
   glReadBuffer(GL_COLOR_ATTACHMENT0);
   ```
2. La pasada de preview hace `glClear(COLOR | DEPTH)` explícito después de
   `bind()` + `glViewport` (primer frame sin basura).

**Hallazgo menor (no corregido ahora):** `mallaScene` emite `glEndList()` sin
un `glNewList` previo → genera un `glGetError() == 0x0502` (GL_INVALID_OPERATION)
benigno tolerado por el pipeline fijo. También queda un aviso UBSan **benigno**
en `ImGuizmo.cpp:1029-1031` (valor `4294967295` leído como `OPERATION`); no
aborta la ejecución.

---

## 7. Cómo funciona el frame (resumen)

```
gameScene()                                   (cada frame)
  ├─ getActiveCamera()                          → cámara activa (+ "ojo")
  ├─ dibujarViewportsPrevios()                  → pasada FBO por cámara con "pintar"
  │    ├─ bind FBO reutilizado + viewport 400x250 + glClear
  │    ├─ getViewMatrix/getProjectionMatrix(400/250)
  │    └─ dibujarEscena(view, proj, ojo)        → escena + marcadores (menos el ojo)
  ├─ RenderTarget::unbind() + glViewport(0,0,fbW,fbH)
  ├─ dibujarEscena(view, proj, activeCameraObject)  ← pasada principal (ventana)
  ├─ picking + gizmo ImGuizmo (sobremirada)
  └─ si isEditorActivo():
       GUI()
         ├─ pintarViewportsGUI()                → ventanas "Vista previa: <nombre>"
         └─ pintarVentanaCamaras()              → ventana "Cámaras"
```

- Las **vistas previas** se renderizan todos los frames (pasada barata de
  400×250); las **ventanas** solo se dibujan cuando el editor está activo.
- La pasada principal vuelve siempre al framebuffer 0 con el viewport completo,
  para que tocar los FBO no ensucie la ventana principal.

---

## 8. Verificación y reproducción

**Build (Debug con sanitizers, detalle importante):** el CMake habilita
AddressSanitizer + UBSan por defecto — así se atrapó el overflow de §6.2.

```bash
cmake --build build --target FunshiEngineGL -j4
```

> Nota histórica: para reproducir estos bugs se usó temporalmente una
> instrumentación de diagnóstico (un hot-wire de cámara al arrancar y una sonda
> de píxeles controlada por variables de entorno). Ambas ramas fueron revertidas
> y el código final no las contiene; los fixes se verificaron con los sanitizers
> del build Debug.

---

## 9. Archivos creados / modificados

| Archivo | Cambio |
|---|---|
| `src/Rendering/RenderTarget.h` / `.cpp` | **Nuevos.** FBO render-to-texture con punteros `glfwGetProcAddress`. |
| `FunshiEngineGL/FunshiEngineGL.vcxproj` / `.filters` | **Eliminado.** Arrastraba rutas absolutas de otra máquina (`glad.c` en una carpeta de descargas) y CMake ya genera los proyectos de Visual Studio: el `.sln` del build alcanza. |
| `src/Objetos/Componentes/CameraComponent.h` / `.cpp` | Cámara como componente + flag `pintar`. |
| `src/GUI/ObjetosGUI/Camera/SettingsCamera.h` / `.cpp` (y registro en GUI) | Inspecta `CameraComponent`, expone FOV / planos / speed y el checkbox de vista previa. |
| `src/Objetos/Componentes/ComponentFactory.cpp` | Acepta `"CameraComponent"` (fix §6.1). |
| `src/Scenes/GameScene.h` / `.cpp` | `dibujarEscena`/`dibujar*ConOjo` con ojo; `dibujarViewportsPrevios`; `pintarViewportsGUI`; `pintarVentanaCamaras`; `getActiveCamera`/`setActiveCamera`/`agregarCamaraEnVistaActiva`; marcador con `vFrustum[8][3]` (fix §6.2); pasada de preview con `glClear` (fix §6.3). `GUI()` restaurado en el header. |
| `src/Rendering/RenderTarget.cpp` | `glDrawBuffer`/`glReadBuffer` en `bind()` (fix §6.3). |

---

## 10. Uso (flujo del editor)

1. Abrir el editor y presionar `E` (menú/editor activo → aparece la ventana
   "Cámaras" y las vistas previas).
2. Posicionar la vista actual (WASD + mouse FPS).
3. En "Cámaras" → **Agregar camara**: se crea un objeto vacío con cámara en esa
   posición/orientación, queda seleccionado (moverlo con el gizmo `W/E/R`), su
   vista previa "Vista previa: Camara N" aparece activa y pasa a ser la cámara
   activa.
4. **Usar** en otra cámara → la navegación y la vista principal cambian a esa.
5. **Vista previa** (checkbox) → prende/apaga cada ventana de preview.
6. **Eliminar** → borra la cámara (y su ventana).
7. Al cerrar, la escena se autoguarda; al abrir se autocarga (sin corrupción).

---

## 11. Pendientes / posibles siguientes pasos

- [ ] Conectar la ventana de previsualización con la **selección**: clic dentro
      de una preview para elegir esa cámara (hoy es pasiva).
- [ ] Mostrar las ventanas de vista previa durante el gameplay como "camera
      monitor": la pasada FBO ya corre todos los frames, pero las ventanas solo
      se dibujan con el editor activo.
- [ ] Limpiar el `glEndList()` huérfano de `mallaScene`.
- [ ] Versionado/validación de la serialización binaria (evitaría la clase de
      desincronización de §6.1 para futuros cambios de componentes).

> Resuelto después de esta fase: el aviso UBSan de `ImGuizmo` (operación
> inválida 4294967295) se corrigió inicializando `mOperation` al comienzo de
> `Manipulate`; la física ya no simula fuera del modo Play; y el gizmo ya no
> genera dobles visuales con el wireframe del collider.