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
   `instalador\FunshiEngineGL-<version>-<canal>-setup.exe`.

## Que instala el setup.exe

- Programas y archivos en `C:\Program Files\FunshiEngineGL\`.
- Iconos de menu inicio y escritorio (inicia en la carpeta de la app, para que
  `IconosGUI` y el FileManager resuelvan sus rutas relativas).
- Crea la raiz vacia `{app}\MotorGrafico\`: ahi solo viven los proyectos que
  el usuario crea (`<nombre>\Memory\Binarios\Scene`, `<nombre>\src<nombre>`)
  y la configuracion (`Configuracion.json`, `imgui.ini`). No se instalan
  carpetas fijas de `Modelos\Texturas\Imagenes`.

> Requiere permisos de administrador (necesario para crear
> `{app}\MotorGrafico` en `Program Files`).

## Versionar para demo/alpha/beta

Edita arriba del `.iss`:

```iss
#define MiVersion "0.5.0"
#define MiCanal "alpha"   ; demo | alpha | beta | rc
```

Cada combinacion genera su propio archivo de salida, así no se mezclan builds
(por ejemplo `FunshiEngineGL-0.5.0-beta-setup.exe`).

## Primer arranque y escena demo

- La primera vez la escena arranca vacia (el editor crea
  `SceneBBDDObjetos.txt` al cargar).
- Al crear un proyecto, las carpetas se generan solas en
  `{app}\MotorGrafico\<nombre>\` (`Memory\Binarios\Scene` + `src<nombre>`).
  Arrastra meshes desde el árbol de archivos (raiz `src<nombre>`) al
  viewport para armar tu escena demo.
- Las rutas de assets se guardan **relativas al cwd** en Windows
  (`.\MotorGrafico\...`) porque el FileManager usa la carpeta de trabajo; por
  eso todos los accesos directos inician con `Carpeta de trabajo = {app}`.

## GitHub Actions (instalador automatico)

El workflow `.github/workflows/windows-release.yml` construye el instalador en
un runner de Windows y lo publica como artefacto, sin tocar tu PC:

- Manualmente: `Actions` -> `Windows (instalador)` -> `Run workflow`.
- Automaticamente: creando un tag `v*` (por ejemplo `git tag v0.5.0-alpha`).

La primera corrida tarda ~10-15 min (compila Assimp y Bullet desde vcpkg);
después queda cacheada y es mucho mas rapida.

## Limitaciones conocidas (a corregir rumbo a 1.0)

1. **Rutas del proyecto**: `EditorConfig` usa el directorio del ejecutable
   (`{app}\MotorGrafico`, exe-relativo) y el FileManager resuelve `.\MotorGrafico`
   relativo al cwd. Coinciden porque los accesos directos fuerzan `WorkingDir={app}`,
   pero siguen siendo dos mecanismos que convendría unificar en 1.0 (ver
   `EditorConfig::directorioProyectoPorDefecto` + `GUIManager::GUIManager`).
2. **Escenas con rutas Linux**: los `.db` guardan las rutas absolutas que se
   cargaron. Una escena hecha en Linux (`/home/.../MotorGrafico/...`) no
   resolverá sus assets en Windows tal cual; hay que reimportar los modelos
   desde el árbol o regenerar la escena en Windows.