# Paquete Windows: beta / alpha / demo

Este directorio contiene TODO lo necesario para generar el instalador de
Windows de FunshiEngineGL (tambien lo usa el pipeline de GitHub Actions).

## Que genera

```
packaging/
├── FunshiEngineGL_setup.iss      Script Inno Setup 6 (el instalador)
├── HacerInstalador.bat           UN CLIC: dependencias + build + dist + setup.exe
├── stage_dist_win.ps1            Monta packaging\dist\ (exe + dlls + Imagenes)
├── README_PAQUETE_WINDOWS.md     (este archivo)
└── dist\                         Montaje de lo que se instala (autogenerado)
└── instalador\                   Setup final: FunshiEngineGL-<version>-<canal>-setup.exe
```

## Requisitos (una sola vez, en tu PC Windows)

1. Visual Studio 2022 (Desktop C++).
2. CMake en PATH.
3. vcpkg clonado (http://vcpkg.io + `bootstrap-vcpkg.bat`). Se detecta via
   `VCPKG_ROOT`, o `C:\Users\gianf\vcpkg`, o `C:\vcpkg`.
4. Inno Setup 6 (https://jrsoftware.org/isinfo.php) — **opcional**: sin el solo
   se arma `dist\`, no el instalador.

## Hacer el instalador

```bat
cd FunshiEngineGL\packaging
HacerInstalador.bat            REM build + dist + setup.exe
```

Pasos que ejecuta:

1. `vcpkg install glfw3 assimp bullet3 glm --triplet x64-windows`
   (baja dependencias; la primera vez tarda varios minutos).
2. `cmake` configura `build-win` con el toolchain de vcpkg en **Release**.
3. Compila `FunshiEngineGL.exe` y corre `ctest` (si los tests fallan, **no**
   genera instalador: no se publica rojo).
4. `stage_dist_win.ps1` arma `dist\`:
   - `FunshiEngineGL.exe`
   - DLLs de vcpkg (`glfw3.dll`, `assimp-*.dll`, `Bullet*.dll`, `zlib1.dll`, ...)
   - runtime VC++ (`msvcp140.dll`, `vcruntime140*.dll`) si existe el redist
   - `Imagenes\` (iconos del editor, obligatorios)
5. Si Inno Setup existe, compila
   `dist\instalador\FunshiEngineGL-<version>-<canal>-setup.exe` (la carpeta de
   salida la decide el `.iss`: `OutputDir` es relativo a `SourceDir`, que es
   `dist`). Es la misma ruta que publica la CI, así que un instalador hecho a
   mano se sube como artefacto sin mover nada.

## Que instala el setup.exe

- Programa, DLL y carpetas en
  `%LOCALAPPDATA%\Programs\FunshiEngineGL\` (la carpeta de programas del
  usuario).
- Iconos de menú inicio y escritorio, que inician en la carpeta de la app para
  que `IconosGUI` encuentre sus imágenes y los scripts en C++ sus cabeceras.
- Crea la raíz vacía `{app}\MotorGrafico\`: ahí viven los proyectos que el
  usuario crea (`<nombre>\Memory\Binarios\Scene`, `<nombre>\src<nombre>`) y
  la configuración (`Configuracion.json`, `imgui.ini`). No se instalan carpetas
  fijas de `Modelos\Texturas\Imagenes`.

La instalación es **por usuario y sin UAC** (`PrivilegesRequired=lowest`). No
pide contraseña de administrador ni escribe nada en `Program Files`.

> Única excepción: si aceptás la descarga del JDK de la se siguiente, el MSI de
> Temurin se instala a nivel de máquina y ese paso sí puede pedir elevación,
> porque deja `JAVA_HOME` en el entorno del equipo.

### Actualizar desde una versión instalada con permisos de administrador

Las instalaciones antiguas con `PrivilegesRequired=admin` quedaron en
`C:\Program Files\FunshiEngineGL` y registradas en HKLM. La nueva se registra
por usuario (HKCU), así que Inno no las ve como la misma aplicación: **desinstalá
la versión anterior antes de instalar la nueva**, o te van a coexistir dos
copias. Tus proyectos no se pierden: están en la carpeta `MotorGrafico` de la
instalación vieja y el motor copia solo lo que falta cuando encuentra la
carpeta original sin escribir (migración de `ProjectPaths`).

### Dónde guarda el motor los datos del usuario

Toda escritura cuelga de una sola raíz, la que resuelve
`ProjectPaths::directorioBase()`: `{app}\MotorGrafico` (junto al ejecutable) si
ahí se puede escribir, y si no la carpeta de datos del usuario
(`%APPDATA%\FunshiEngineGL\MotorGrafico`). El motor elige la raíz probando de
verdad si puede escribir, no suponiendo dónde está.

Con la instalación por usuario lo normal es el primer caso, porque
`{app}` está en tu perfil. El segundo aparece cuando el motor corre elevado
sobre una instalación en `Program Files`, o si la carpeta del ejecutable está
en solo lectura.

El log va a `logs\` dentro de esa misma raíz, así que siempre se escribe
(`FunshiEngineGL_<AAAAMMDD_HHMMSS>.log`, hora UTC).

Comprobado con el ejecutable real: con su carpeta en solo lectura no escribe
nada allí y todo el árbol (`Configuraciones`, `Proyects`, `Exportaciones`,
`logs`) aparece en la carpeta de datos del usuario. Esto no es hipotético: sin
el fallback las escrituras fallaban en silencio —ningún llamador comprobaba el
retorno— y el motor leía bien pero no podía guardar ni la configuración ni las
escenas.

## Versionar para demo/alpha/beta

La versión del producto tiene una sola fuente: `FUNSHI_VERSION` en
`FunshiEngineGL/CMakeLists.txt`. De ahí salen la versión del proyecto y el
recurso `VERSIONINFO` del `.exe`. Se puede cambiar sin tocar el archivo:

```bash
cmake -B build -S FunshiEngineGL -DFUNSHI_VERSION=0.6.0
```

El `.iss` lleva su propio `MiVersion` para el nombre del instalador y el canal:

```iss
#define MiVersion "0.5.0"
#define MiCanal "alpha"   ; demo | alpha | beta | rc
```

Tiene que coincidir con la del `.exe` que se está empaquetando; si no, el
instalador anuncia una versión y dentro viaja otra. Al publicar desde GitHub
Actions eso no hay que controlarlo: el workflow pasa el número del tag a
`-DFUNSHI_VERSION` y reescribe el `MiVersion` del `.iss` con el mismo valor.
Compilando a mano, cambia los dos sitios.

Cada combinacion genera su propio archivo de salida, así no se mezclan builds
(por ejemplo `FunshiEngineGL-0.5.0-beta-setup.exe`).

## Primer arranque y escena demo

- La primera vez la escena arranca vacia (el editor crea
  `SceneBBDDObjetos.txt` al cargar).
- Al crear un proyecto, las carpetas se generan solas en
  `{app}\MotorGrafico\<nombre>\` (`Memory\Binarios\Scene` + `src<nombre>`).
  Arrastra meshes desde el árbol de archivos (raiz `src<nombre>`) al
  viewport para armar tu escena demo.
- Las rutas de assets se guardan **relativas a la raíz del proyecto**
  (`src<nombre>`), así que sobreviven a que la carpeta del proyecto se mueva de
  sitio.

## GitHub Actions (instalador automatico)

El workflow `.github/workflows/windows-release.yml` construye el instalador en
un runner de Windows y lo publica como artefacto, sin tocar tu PC:

- Manualmente: `Actions` -> `Windows (instalador)` -> `Run workflow`.
- Automaticamente: creando un tag `v*` (por ejemplo `git tag v0.5.0-alpha`).

La primera corrida tarda ~10-15 min (compila Assimp y Bullet desde vcpkg);
después queda cacheada y es mucho mas rapida.

## Limitaciones conocidas (a corregir rumbo a 1.0)

1. **Rutas absolutas del proyecto**: las rutas de assets se guardan relativas a
   la raíz del proyecto abierto (`src<nombre>`), no a la carpeta del ejecutable.
   Se puede mover la carpeta del proyecto y siguen valiendo. Lo que no se
   resuelve todavía es una ruta guardada que venga de otro equipo.
2. **Escenas con rutas Linux**: los `.db` guardan las rutas absolutas que se
   cargaron. Una escena hecha en Linux (`/home/.../MotorGrafico/...`) no
   resolverá sus assets en Windows tal cual; hay que reimportar los modelos
   desde el árbol o regenerar la escena en Windows.