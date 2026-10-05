# Pendiente de revisar

Todo lo que quedó hecho en el lote B y **todavía no está confirmado** por quien
usa el editor. Los tests automatizados cubren lógica, rutas y serialización;
no cubren lo visual ni lo que solo pasa en Windows, así que el cierre de cada
punto de esta lista depende de probarlo en la aplicación real.

La rama `fix/hallazgos-auditoria` está 24 commits adelante de `origin/develop` y
**no está pusheada**: nada de esto está en el remoto.

Cómo probarlo:

```bash
cd FunshiEngineGL && ./build/FunshiEngineGL          # Linux
cd FunshiEngineGL && ./build/FunshiEngineGL.exe      # Windows
```

La salida va a un log por arranque en `FunshiEngineGL/logs/`, con timestamp UTC.
Si algo falla, ese archivo es lo primero que hay que mirar.

---

## 1. Lo que hay que mirar en el editor (Linux o Windows)

Cada punto dice qué se cambió y **qué se espera ver**. Si algo se comporta como
antes del cambio, es un fallo sin explicar: la evidencia técnica está en el
commit, pero el criterio de aceptación lo define la prueba manual.

| # | Cambio | Qué esperar | Commit |
|---|--------|-------------|--------|
| 1 | El componente Model se dibuja con la matriz del objeto | Un modelo importado aparece **colocado** en el viewport (con antes se dibujaba en el origen o sin girar). Moverlo o rotarlo lo mueve de verdad | `3d4849d` |
| 2 | Tema aplicado en vivo y sin proyecto fantasma al arrancar | Al cambiar el tema, los colores cambian sin reiniciar. Al arrancar sin proyecto, **no** se crea un proyecto fantasma ni aparece uno en el árbol | `698f877` |
| 3 | Barra de progreso acotada | Con progreso 0 o sin datos, la barra no dibuja valores inventados ni se desborda | `b884216` |
| 4 | El inspector se reconstruye solo con cambios estructurales | Editar un campo no tira el panel entero ni pierde el foco; añadir/quitar componente sí lo reconstruye | `b3d9bcb` |
| 5 | Pose preservada al reparentar | Mover un hijo en el árbol conserva su posición, escala y rotación, y el cuerpo físico se refresca sin saltos | `2e3140e` |
| 6 | El explorador avisa si no puede crear la carpeta | Crear una carpeta con un nombre repetido (o sin permisos) avisa del fallo y **no** cierra el modal como si fuera un éxito | `a961be2` |
| 7 | Las seis caras del Skybox se guardan en el manifiesto | Asignar un Skybox y reabrir la escena: siguen todas sus caras, no solo una | `004a973` |
| 8 | Los fallos de carga de malla se recuerdan | Un modelo que falla al cargar se avisa cada vez que se abre la escena, y no en silencio | `e4ecb7b` |
| 9 | Copiar o mover una carpeta dentro de sí misma se rechaza | Al copiar o mover una carpeta **dentro de su propia subcarpeta**, el explorador avisa y no crea nada. Antes llego a dar `ENAMETOOLONG` y dejaba el árbol hecho un desastre | `eb13e26` |
| 10 | Nombres con barra invertida | En Linux se puede crear `con\barra.txt` (barra invertida legal en el nombre). Al arrastrarlo o importarlo, conserva su nombre y su ruta, y no se convierte en `barra.txt` | `eeee7fa` |
| 11 | Prefijos UNC y rutas extendidas | Una ruta `\\server\share\a.obj` o `\\?\C:\...` se conserva al normalizar, no se convierte en `/server/share/...` | `5d87059` |
| 12 | Exportar un juego incluye sonidos, scripts y configuración | Exportar un proyecto: el juego exportado tiene `src<nombre>/Sonidos`, los scripts y `ConfiguracionProyecto.json`. Antes el motor los pedía en rutas que ya no existían y **salía sin ellos, en silencio** | `b38fdc4` |
| 13 | Scripts Java y C++ | Un script Java se compila y carga. El fallo por toolchain (javac de un JDK, JVM de otro) ahora lo dice el motivo real | `61029d0` |

## 2. Lo que hay que mirar en Windows

Aquí no hay hardware, así que nada de esto está comprobado. Cada punto indica
cómo comprobarlo.

| # | Cambio | Cómo se comprueba | Commit |
|---|--------|-------------------|--------|
| 14 | El log se escribe aunque el motor no pueda escribir junto al binario | Arrancar el motor instalado y comprobar que hay `FunshiEngineGL_<AAAAMMDD_HHMMSS>.log` en `logs/`, y que **no** aparece una consola detrás | `8953174` |
| 15 | El motor instalado abre sin UAC | Instalar el `setup.exe`: no pide contraseña de administrador. Queda en `%LOCALAPPDATA%\Programs\FunshiEngineGL`, y `MotorGrafico` **dentro de esa carpeta** | `c753a49` |
| 16 | Actualizar desde una versión instalada con permisos de administrador | Si había una versión vieja en `C:\Program Files`, **desinstalarla antes**. Inno no la ve (HKLM contra HKCU) y quedarían dos copias | `c753a49` |
| 17 | Un script C++ compila en la app instalada **sin** Visual Studio en el PATH | Con MinGW/g++ en el PATH, un script C++ compila (se invoca `g++`) | `855918d` |
| 18 | Un script C++ compila en la app instalada **con** Visual Studio instalado | Con Visual Studio presente y sin tocar nada, un script C++ compila: se encuentra `vcvars64.bat` y se invoca `cl` a secas | `855918d` |
| 19 | Forzar un compilador concreto | `set FUNSHI_CXX=C:\ruta\cl.exe` y arrancar: manda sobre lo de arriba. Si la ruta no existe, el log lo avisa | `855918d` |
| 20 | Cabeceras del script en la app instalada | Un script C++ que incluya `ScriptGameObject.h` compila con las cabeceras de `include/` que instala el setup | `91c9a2d` |
| 21 | La versión del `.exe` y la del instalador coinciden | Publicar por un tag `v0.6.0`: en Propiedades del `.exe`, `ProductVersion` debe ser 0.6.0, igual que el nombre del setup | `9f7375d` |
| 22 | El JDK (lo único que puede pedir elevación) | Instalar sin JDK: pregunta si se descarga Temurin 17. Ese paso **sí** pide UAC aunque el resto no. Aceptando, `JAVA_HOME` queda en el equipo y los scripts Java funcionan | `c753a49` |
| 23 | La ruta del log y de los datos con el motor elevado | Arrancar el `.exe` como administrador: los datos van a `{app}\MotorGrafico` (la carpeta del ejecutable), no a la del usuario | `c753a49` |
| 24 | Instalador desde cero y desde CI, misma carpeta de salida | Compilar el setup a mano: queda en `packaging\dist\instalador\`, la misma que sube la CI | `dbb0607` |

## 3. Decisiones que cambiaron comportamiento

No son bugs: son cambios de producto que conviene que alguien mire con
intención, porque no se pueden deshacer solo revertiendo código.

- **El instalador ya no pide administrador** (`c753a49`). Es una mejora de
  seguridad y de experiencia (sin UAC), pero mueve la carpeta de instalación de
  `Program Files` a `%LOCALAPPDATA%\Programs`. Quien tenga datos en la
  instalación vieja tiene que desinstalar y volver a instalar, y sus proyectos
  los copia el motor solo (sin pisar nada).
- **El valor horneado del compilador pasó a ser "una pista"** (`855918d`): si
  la ruta del compilador del build no existe en la máquina, el motor busca el
  toolset de Visual Studio y, si no hay, usa `g++` del PATH. En un equipo sin
  ninguno de los dos, el fallo es "no se encuentra g++" en lugar de una ruta
  de Visual Studio que el usuario nunca escribió: el mensaje es peor en el caso
  de fallo y mejor en el de éxito.
- **La versión del producto es una variable de caché de CMake** (`9f7375d`),
  sobreescribible con `-DFUNSHI_VERSION=`. Compile a mano sin pasarla y el
  número sale del repositorio (0.5.4), no del que se publique.
- **Las rutas de assets se guardan relativas a la raíz del proyecto**
  (`src<nombre>`), no al ejecutable ni al cwd. El manual se actualizó con
  esto; el texto anterior estaba obsoleto.

## 4. Lo que quedó anotado sin arreglar

Cosas reales que encontré y decidí **no** tocar en este lote, con el motivo,
para que no se pierdan:

| Hallazgo | Por qué no se tocó |
|---|---|
| `EditorConfig::directorioProyectoPorDefecto()` no tiene ningún llamador en el repo | Código muerto. Borrarlo es una decisión de API, no un fix de bug |
| `ProjectPaths` usa `"MotorGrafico"` **relativo al cwd** si no consigue la ruta del ejecutable (`ProjectPaths.cpp:133`) | Solo pasa si `argv[0]` no trae directorio (lanzar `FunshiEngineGL` a secas, sin `./`). Es un agujero real, pero de otro tipo |
| `BackendCpp` y `BackendJava` leen `TEMP`/`TMPDIR` del entorno y, si falta, caen a `"."` (Windows) o `/tmp` (Linux). Si se lanzan con el entorno vaciado, la cache de scripts se crea en el cwd | En Windows `TEMP` siempre está definida para un usuario interactivo, y `/tmp` existe en Linux. No se ha visto nunca, y el fallo sería un aviso de cache en una carpeta inesperada |
| `IconosGUI` y el logo de la ventana buscan imágenes en el cwd si no las encuentran junto al binario, y su mensaje de error dice solo "varias rutas relativas al cwd", cuando ya se probaron antes las del ejecutable | Cosmético y de solo desarrollo: el paquete instalado siempre trae las imágenes junto al `.exe`, y el aviso confunde sobre dónde se buscó |
| ~~El patrón "valor horneado" puede quedar en otro sitio~~ **auditado y cerrado** | Se hizo el `grep FUNSHI_ CMakeLists.txt` que pedía la checklist. Salió un caso real más: la biblioteca de la JVM (`FUNSHI_LIBJVM_DEFAULT`) llega como la ruta del **archivo**, pero se pasaba por la lista de **raíces**, así que buscaba `<archivo>/lib/server/libjvm.so` y se descartaba siempre (tampoco en la máquina donde se compiló). Corregido y con test (142 comprobaciones) |
| El `log` de los scripts C++ se borra o se queda en `%TEMP%\funshi_scripts\cpp` | Es intentional (es un artefacto de compilación, no del proyecto), pero no está documentado en el manual |

## 5. Pendiente de infraestructura

- **La rama no está pusheada.** 24 commits por encima de `origin/develop`.
- **Existe una rama de respaldo** `respaldo-rewrite-mensajes`: apunta al árbol
  antes de rehacer dos mensajes de commit (para quitar dos ideogramas que se me
  colaron y un subject mal conjugado). El árbol de ambas es **idéntico**
  (`git diff respaldo-rewrite-mensajes HEAD` sale vacío), así que la rama se
  puede borrar sin miedo. Los hashes de los commits del final del lote
  cambiaron al rehacerlos.
- **No hay CI verde comprobado para Windows**: el workflow compila con
  `-DFUNSHI_VERSION`, cambio que no se ha ejecutado nunca en un runner.

---

*Documento generado al cerrar el lote B (24 commits sobre `origin/develop`).*
