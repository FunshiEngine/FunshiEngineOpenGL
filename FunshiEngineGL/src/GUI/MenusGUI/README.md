# Paquete MenuGUI — Interfaz de inicio (menú) del motor

Paquete que modulariza el menú de inicio del motor con una arquitectura sólida
siguiendo buenas prácticas de diseño de software: **separación de
responsabilidades** (SRP), **bajo acoplamiento** entre capas, **alta cohesión**
dentro de cada clase, **inversión de dependencias** (DIP) y **abierto/cerrado**
(OCP) para extender pantallas sin reescribir lógica.

> Originalmente solo reorganizaba el menú previo; la **Fase 3** sumó
> funcionalidad real al paquete: traducción en vivo de etiquetas, observer de
> cambios (pub al bus) y reset de configuración. El modelo sigue siendo puro y
> testeable (sin ImGui/GLFW).

## Archivos del paquete

| Archivo | Capa | Responsabilidad (SRP) |
|---|---|---|
| `MenuModel.h` | **Modelo / lógica pura** | Estado y navegación entre vistas + datos de configuración (nombre del proyecto, idioma). **Sin ImGui ni GLFW** → testeable y reutilizable. |
| `MenuView.h/.cpp` | **Vista / presentación** | Dibujo ImGui. Solo lee el modelo y delega acciones de vuelta (MVP). No decide lógica ni posee datos de dominio. |
| `StartMenuPresenter.h` | **Presentador / puente** | Traduce el modelo a "menú abierto/cerrado" para el resto del motor y sincroniza cambios externos de vuelta. No dibuja. |
| `MenuGUI.h/.cpp` | **Fachada** | Ensambla el paquete y expone la interfaz pública estable. El resto del motor solo conversa con ella. |
| `README.md` | Documentación | Este archivo: arquitectura, reglas y guía de extensión. |

## Arquitectura y dirección de dependencias (una sola)

```
                 main.cpp (solo ve la fachada)
                        │  ConsultarMenu / SetMenuActivo
                        ▼
   ┌────────────── MenuGUI (fachada) ──────────────┐
   │  StartMenuPresenter ──► MenuModel ◄── MenuView│
   │  (puente motor)         (lógica)   (ImGui)    │
   └───────────────────────────────────────────────┘
```

- `MenuGUI` → `StartMenuPresenter` → `MenuModel` ← `MenuView`.
- El modelo **no sabe** que existe la vista (inversión de dependencias: la
  lógica no depende de la UI).
- La vista **no sabe** que existe main (solo conoce el modelo por puntero).
- main **no conoce** las clases internas del paquete (la fachada encapsula).

## Contrato de `MenuModel`

```cpp
enum class Vista { Principal, Opciones, ConfigProyecto, Ninguna };

void mostrarMenu();        // abre el menú (Vista::Principal)
void iniciarEstudio();     // cierra el menú (Vista::Ninguna)
void abrirOpciones();
void abrirConfigProyecto();
void volver();             // de Opciones/ConfigProyecto → Principal

bool estaVisible() const noexcept;
Vista getVista() const noexcept;
// datos: nombreProyecto (get/set), idioma (get/set), idiomas (get),
// sensibilidadCamara (get/set, multiplicador del mouse look en Opciones),
// traducir(clave) (diccionario es/en segun el idioma activo),
// setOnCampoCambio(observer) y restablecerConfiguracion() (reset de opciones)
```

Todas las transiciones son asignaciones triviales del enum `vista`.

## Pantallas (comportamiento conservado)

| Vista | Contenido |
|---|---|
| `Principal` | Botones centrados: "Iniciar Estudio", "Config Proyect" (etiqueta con el nombre actual), "Opciones" (etiqueta con el idioma actual), "Exit". |
| `Opciones` | Título "Opciones", selector de idioma (`BeginCombo` sobre las opciones del modelo) y slider de sensibilidad de cámara, "Volver". |
| `ConfigProyecto` | Título "Config Proyect", `InputText` del nombre del proyecto (buffer de 128 en la vista), "Volver". |
| `Ninguna` | Menú oculto → corre el editor (equivale a "Iniciar Estudio"). |

## Opciones del menú (vista Opciones) — efecto en vivo y persistencia

| Opción | Efecto inmediato | Persistencia | Canal / test |
|---|---|---|---|
| **Idioma** (`setIdioma`) | Las etiquetas del menú se traducen al instante (`MenuModel::traducir`) | Config **general** al cambio (`guardarGeneral`) | `IdiomaCambio`; `menu-tests` (traducción) |
| **Sensibilidad de cámara** (`setSensibilidadCamara`) | Se aplica a la escena al soltar el slider (mouse look) | Config **general** al cambio | `SensibilidadCambio`; `eventbus-tests` |
| **Apariencia** | Estilo ImGui completo (tema, acento RGB en todos los roles, grises azulados de fábrica a gris neutro), fondo y grilla del viewport al instante | Config **general** al cambio | `AparienciaCambio`; `eventbus-tests` y `tema-tests` (cobertura del acento) |
| **Restablecer apariencia** | Vuelve el perfil a los valores de fábrica | Config **general** | Evento `AparienciaCambio` con perfil default; `menu-tests` |
| **Restablecer configuracion** | Vuelve idioma/sensibilidad/apariencia a fábrica + estado del proyecto (ventanas, gizmo, cámara) | General + proyecto al instante | `ReiniciarConfiguracion`; `menu-tests` (reset) y `EditorConfigTests` (`restablecer`) |

Reglas del flujo:
- Toda mutación del modelo (vista o fachada) notifica a `MenuModel::Campo`;
  la fachada traduce el campo a un evento del bus (`MenuGUI::publicarCambio`) y
  **main** es quien aplica a la escena/persiste. La vista no conoce el bus.
- El **nombre del proyecto** se conserva en el reset: define la carpeta/proyecto
  (`src<Nombre>` + `Memory`) y un reset no debe cambiar de proyecto.
- Persistencia: `apariencia`, `idioma`, `sensibilidadCamara` viven en la config
  **general**; `estadoVentanas`, `gizmoOperacion`, `camaraActivaId` en la del
  **proyecto** (ver `EditorConfig`).

## Integración con `main.cpp`

`main` ya no mira estados internos de clases GUI: solo conversa con la fachada
`MenuGUI`.

```cpp
MenuGUI menu(window);          // ensamblado por inyección de la ventana

// en el bucle principal:
const bool menuAbierto = menu.ConsultarMenu();
if (menuAbierto) {
    menu.SetMenuActivo(true);
    menu.ConsultarCierre();    // "Iniciar Estudio" → cierra (consumo único)
} else {
    menu.SetMenuActivo(false); // mantiene el modelo en Ninguna
}
```

La máquina de estados (`src/States/ApplicationStateMachine.h`, `MainMenu`/
`Editing`) queda disponible como la **fuente de verdad** de la transición;
la fachada solo informa el estado del menú.

## Reglas de extensión (guía para el futuro)

1. **Nueva pantalla**: (a) nuevo valor en `MenuModel::Vista`, (b) método de
   transición en el modelo, (c) caso nuevo en el `switch` de `MenuView::contentGUI`
   + método de render. OCP: no se reescribe lógica existente.
2. **Nueva fuente de datos** (p. ej. ruta del proyecto): propiedad en
   `MenuModel` + accessors + sincronización desde `StartMenuPresenter`
   (nunca desde la vista).
3. **`MenuModel` debe seguir libre de ImGui/GLFW** — es lo que mantiene la
   lógica testeable y la UI reemplazable.
4. La vista puede asumir ImGui (es su responsabilidad), pero no debe importar
   cabeceras del motor fuera del paquete.
5. Cambios externos al estado del menú (apertura/cierre, datos) entran por
   `MenuGUI` (fachada), nunca mutando las clases internas directamente.
