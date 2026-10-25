## Reglas de arquitectura

- **Análisis previo obligatorio**: antes de crear o modificar código, analizar
  las excepciones, estructuras de datos, patrones de diseño y arquitecturas ya
  presentes en el proyecto y reutilizarlas; si el cambio requiere algo nuevo
  o distinto, recomendarlo y justificarlo antes de implementarlo.
- **Tablas de API para scripts (`ApiScriptGameObject`, `ScriptServices`)**:
  estrictamente APPEND-ONLY. Los campos nuevos van SIEMPRE al final, antes del
  campo `int version`; nunca reordenar ni borrar entradas. El campo `version`
  se incrementa al agregar funcionalidad.
- **API de inspección de escena (`src/Inspeccion/`)**: es el contrato público
  del motor para herramientas externas. La **CLI es un proyecto aparte**
  (`FunshiEngineCLI`) que consume este contrato: el motor publica la API y la
  CLI la usa, no hay un ejecutable de línea de comandos en este repositorio.
  - El contrato tiene **versión propia** (`versionApiEscena`), independiente de
    `versionRuntimeScript`: cambiar uno no invalida el otro ni recompila los
    scripts de los proyectos.
  - `src/Comandos/` **no** depende de `src/Inspeccion/` (para no crear un
    ciclo de módulos): `AjustarVariableComando` recibe los accesores por
    inyección. La dependencia va en un solo sentido: `Inspeccion` → `Comandos`.
  - Toda mutación de la API pasa por el `GestorComandos`. Como
    `GestorComandos::ejecutar` no puede fallar, la API valida antes de
    encolar (objeto existente, tipo conocido, prerrequisito cumplido, valor
    interpretable) y devuelve el motivo del rechazo.
  - Las variables de los componentes se acceden por sus getters y setters
    públicos, nunca por el dato interno: `Transform` guarda dos copias de su
    estado y solo los setters las sincronizan.
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
- **Sistema de partículas** (`src/Particulas/`): la simulación es CPU pura y
  no toca ni OpenGL ni glm, para que se pueda probar entera en los targets
  headless. El módulo no depende del motor; el componente `EmisorParticulas`
  (en `Objetos/Componentes/`) es quien usa el motor, y el renderer recibe el
  sistema con datos planos, sin conocer el emisor.
- **Interfaz de juego nativa** (`src/UI/`): el módulo es CPU puro y no toca ni
  OpenGL ni glm ni ImGui, para que se pueda probar entero en los targets
  headless. El componente `UIComponent` (en `Objetos/Componentes/`) y el
  renderer `UIRenderer` (en `Rendering/`) son los que usan el motor, y el
  renderer es el único que sabe de ImGui. El nombre del tipo persistido **es**
  el formato de datos: ampliar un componente obliga a versionar su payload, así
  que un sistema nuevo que cambie un componente existente se vuelve un
  componente nuevo.
- **Compatibilidad multiplataforma**: el motor es compatible con **Linux y Windows**,
  sin excepción. Al usar APIs nativas del sistema operativo (Windows API, POSIX,
  filesystem, threading, dynamic loading, etc.), SIEMPRE verificar y mantener las
  dos plataformas. Usar guardas `#ifdef _WIN32` / `#else` (Linux/macOS) y aislar el
  código dependiente de plataforma en capas de abstracción o backends dedicados
  (p. ej. `IRenderBackend`, `FileSystemWatcher`, `BackendCpp`/`BackendJava`). No asumir
  que headers o comportamientos de un SO existen en el otro; probar o validar
  compilación cruzada en CI antes de confirmar cambios.
  - **Énfasis en filesystem y servicios del SO**: es donde más divergen. Al trabajar
    con rutas, archivos o servicios propios del sistema (separadores `/` frente a `\`,
    mayúsculas y minúsculas, CRLF frente a LF, `mtime` y latencias del FS, permisos
    y ACL, symlinks, variables de entorno, procesos hijo, carga dinámica, watchers
    de archivos, toolchains), diseñar para las dos plataformas desde el primer
    boceto: `std::filesystem` con el separador nativo, ramas `#ifdef _WIN32` con
    fallback POSIX documentado, y decisiones probadas por escritura real (no por
    existencia del directorio). Los patrones conocidos viven en `doc/DocuTecnicoBugs.md`.
  - **Tests de compatibilidad multisistema**: cada funcionalidad que toque filesystem
    o servicios del SO lleva su prueba headless que la ejercite en Windows y en Linux
    (ramas `#ifdef _WIN32` / `#else` en el propio test cuando el contrato difiera por
    plataforma, o comparación contra `(fs::path(base) / ...).string()` en vez de
    literales con `/`). Si el test resulta demasiado artificial (simular el SO sin
    ejercer el contrato real), se deja constancia de por qué no se implementa y se
    cubre con sonda local o CI en la plataforma real; la decisión queda a evaluar
    según la funcionalidad.

## Memoria de trabajo: documentos locales ignorados por git

Todo el estado local que un agente necesita para retomar el trabajo sin
re-investigarlo desde cero vive bajo **una única carpeta ignorada por git**,
`.agentes/`, en la raíz del repo. No es documentación del proyecto ni se
versiona; es la memoria de trabajo del agente.

| Ruta | Para qué sirve |
|---|---|
| `.agentes/MEMORIA.md` | Memoria de contexto entre sesiones: estado del trabajo en curso, sistemas del código ya analizados (con punteros a archivos y líneas), supuestos abiertos, hallazgos pendientes, comandos de build/test que funcionan en el entorno y archivos que no se deben tocar. |
| `.agentes/ARQUITECTURA.md` | Mapa de la estructura local de `.agentes/` y propósito de cada carpeta de trabajo. |
| `.agentes/PLAN GENERAL DE FIX.md` | Lote de correcciones: cada hallazgo con su cadena `síntoma → evidencia → causa → fix → test`, la tabla de avance con los commits que cierran cada ítem y lo que queda por validar. |
| `.agentes/auditorias/` | Informes de auditoría por área, uno por archivo: qué se revisó, qué se encontró, con evidencia `archivo:línea`, y si es regresión de un commit reciente o deuda preexistente. |
| `.agentes/pendiente-revision/PENDIENTE_DE_REVISAR.md` | Lista local de verificaciones manuales todavía pendientes; no forma parte de la documentación versionada del proyecto. |
| `.agentes/plan-*.md` | Planes de trabajo por tema (integración de un sistema, lote de fixes, refactor): qué se va a construir, en qué orden, con qué arquitectura y cómo se valida. |

Reglas:

- **Los planes de integración, de corrección y de refactor van siempre en
  `.agentes/`**: un plan de trabajo (qué sistemas se integran, en qué orden,
  con qué arquitectura, con qué testers) es material del agente, no
  documentación del proyecto, así que **no se versiona**. Si el plan ya está
  escrito y en la raíz del repo, se mueve a `.agentes/` y se retira del
  árbol versionado con un commit propio. Del plan solo se versiona lo que
  describe el proyecto terminado (la arquitectura que queda, el manual de uso
  y el registro de bugs), y eso va en los `.md` del repo, en el commit que lo
  justifica.
- **Crear lo que haga falta, manteniendo la raíz ordenada**: `.agentes/` es
  extensible. Un informe nuevo de una revisión puntual es un archivo nuevo en
  `.agentes/auditorias/`; un plan nuevo es un archivo nuevo
  `.agentes/plan-<tema>.md` (un tema que ya no tiene archivo propio se agrupa
  en un `.md` con nombre claro en `.agentes/`). No crear archivos sueltos en
  la raíz del repo: la raíz es del proyecto, `.agentes/` es del agente.
- **Si falta un archivo, recrearlo**: reconstruir su contenido desde el estado
  real (git log, código fuente, esta guía) y tratarlo desde ese momento como
  la fuente de contexto de la tarea — no empezar de cero sin comprobar antes
  qué ya sabía el documento.
- **Leerlos al arrancar** una tarea que toque temas que cubren, y **escribir
  en ellos** lo nuevo relevante (decisiones, evidencia, estado verificado) de
  forma incremental durante el trabajo: así la siguiente sesión no repaga el
  análisis y no arrastra contexto que ya no sirve.
- **Actualizarlos al cerrar cada paso** (mismo criterio que la lista de
  tareas): lo que quedó hecho, verificado y con qué commit queda registrado
  ahí.
- Lo que se vuelva regla estable del proyecto **se promueve** a los `.md` del
  repo (`doc/MANUAL_DE_USO.md`, `doc/PROJECT_STRUCTURE.md`, esta guía) en el commit
  que lo justifica; mientras tanto vive solo en `.agentes/`.
- Al commitear, **nunca** incluir esta carpeta: está ignorada a propósito y un
  `git add <ruta>` explícito de los archivos propios de la tarea es la única
  forma correcta de agregar cambios.
