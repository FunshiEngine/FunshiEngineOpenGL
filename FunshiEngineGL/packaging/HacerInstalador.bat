@echo off
REM ===========================================================================
REM  FunshiEngineGL - Build Release + montaje de dist + instalador (Windows)
REM ===========================================================================
REM  Requisitos:
REM   - Visual Studio 2022 (MSVC C++)
REM   - CMake >= 3.20 en PATH
REM   - vcpkg (define VCPKG_ROOT o se busca en C:\Users\gianf\vcpkg / C:\vcpkg)
REM   - [Opcional] Inno Setup 6 para generar el instalador .exe
REM
REM  Uso:  HacerInstalador.bat
REM ===========================================================================
setlocal EnableExtensions

set "PKG_DIR=%~dp0"
set "PROJ_DIR=%PKG_DIR%.."
set "TRIPLET=x64-windows"

REM --------------------------------------------------------------------------
REM  1. Localizar vcpkg
REM --------------------------------------------------------------------------
if not defined VCPKG_ROOT           set "VCPKG_ROOT=C:\Users\gianf\vcpkg"
if not exist "%VCPKG_ROOT%\vcpkg.exe" if exist "C:\vcpkg\vcpkg.exe" set "VCPKG_ROOT=C:\vcpkg"
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [ERROR] No se encontro vcpkg.
    echo          Instala vcpkg (https://vcpkg.io) o define VCPKG_ROOT.
    exit /b 1
)
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [ERROR] Falta el toolchain de vcpkg: no se hizo bootstrap.
    exit /b 1
)
echo [vcpkg] %VCPKG_ROOT%

REM --------------------------------------------------------------------------
REM  2. Dependencias (glfw3, assimp, bullet3, glm) - modo clasico de vcpkg.
REM     Los .dll se copiaran a dist despues (ver stage_dist_win.ps1).
REM --------------------------------------------------------------------------
echo [1/5] Instalando dependencias con vcpkg (%TRIPLET%)...
"%VCPKG_ROOT%\vcpkg.exe" install glfw3 assimp bullet3 glm --triplet %TRIPLET%
if errorlevel 1 exit /b 1

REM --------------------------------------------------------------------------
REM  3. Configurar CMake (MultiConfig VS; el user ya compila con MSVC)
REM --------------------------------------------------------------------------
echo [2/5] Configurando CMake (Release)...
cmake -S "%PROJ_DIR%" -B "%PROJ_DIR%\build-win" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=%TRIPLET% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DENABLE_ASAN=OFF
if errorlevel 1 exit /b 1

REM --------------------------------------------------------------------------
REM  4. Compilar + probar
REM --------------------------------------------------------------------------
echo [3/5] Compilando el ejecutable (Release)...
cmake --build "%PROJ_DIR%\build-win" --config Release --target FunshiEngineGL
if errorlevel 1 exit /b 1

echo [4/5] Ejecutando pruebas (ctest)...
ctest --test-dir "%PROJ_DIR%\build-win" -C Release --output-on-failure
if errorlevel 1 (
    echo [ERROR] Las pruebas fallan. No se publica un instalador rojo.
    exit /b 1
)

REM --------------------------------------------------------------------------
REM  5. Montar packaging/dist: exe + dlls + Imagenes (sin assets demo: el
REM     MotorGrafico del usuario solo contiene sus proyectos y configs).
REM --------------------------------------------------------------------------
echo [5/5] Montando packaging/dist...
powershell -NoProfile -ExecutionPolicy Bypass -File "%PKG_DIR%stage_dist_win.ps1" ^
    -ProjectDir "%PROJ_DIR%" ^
    -BuildDir "%PROJ_DIR%\build-win\Release" ^
    -VcpkgInstalled "%VCPKG_ROOT%\installed\%TRIPLET%"
if errorlevel 1 exit /b 1

REM --------------------------------------------------------------------------
REM  6. Instalador (Inno Setup) si esta disponible
REM --------------------------------------------------------------------------
set "ISCC="
if not defined ISCC_PATH set "ISCC_PATH=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if exist "%ISCC_PATH%" set "ISCC=%ISCC_PATH%"
if not defined ISCC if exist "C:\Program Files\Inno Setup 6\ISCC.exe" set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"
if defined ISCC (
    echo [OK] Compilando instalador con Inno Setup...
    "%ISCC%" /O"%PKG_DIR%instalador" "%PKG_DIR%FunshiEngineGL_setup.iss"
) else (
    echo [AVISO] Inno Setup no esta instalado: no se genero el instalador.
    echo          Descargalo de https://jrsoftware.org/isinfo.php y reintenta,
    echo          o compila el .iss a mano con ISCC.exe.
)

echo.
echo ===========================================================================
echo  Listo! Revisa:
echo   - packaging\dist\            (contenido que se instalara)
echo   - packaging\instalador\      (setup.exe o aviso de Inno Setup)
echo ===========================================================================
endlocal