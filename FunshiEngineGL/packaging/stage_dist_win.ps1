# ============================================================================
#  FunshiEngineGL - Monta la carpeta packaging/dist para el instalador.
#  Llamado por HacerInstalador.bat (y por el CI en GitHub Actions).
# ============================================================================
param(
    [Parameter(Mandatory = $true)][string]$ProjectDir,   # raiz del proyecto (con CMakeLists.txt)
    [Parameter(Mandatory = $true)][string]$BuildDir,     # salida config Release (contiene FunshiEngineGL.exe)
    [string]$VcpkgInstalled = ""                         # vcpkg installed\x64-windows (para copiar *.dll)
)

$ErrorActionPreference = "Stop"

$Dist = Join-Path $PSScriptRoot "dist"
if (Test-Path $Dist) { Remove-Item $Dist -Recurse -Force }
New-Item -ItemType Directory -Path $Dist | Out-Null

# --- 1. Ejecutable ----------------------------------------------------------
$exe = Join-Path $BuildDir "FunshiEngineGL.exe"
if (-not (Test-Path $exe)) {
    Write-Error "No existe $exe. Compilá primero (HacerInstalador.bat paso 3)."
}
Copy-Item $exe $Dist
Write-Host "  [ok] FunshiEngineGL.exe"

# --- 2. DLLs de vcpkg (glfw3, assimp, bullet, zlib, ...) ---------------------
$dllsCopiadas = 0
if ($VcpkgInstalled) {
    $dllDir = Join-Path $VcpkgInstalled "bin"
    if (Test-Path $dllDir) {
        Copy-Item (Join-Path $dllDir "*.dll") $Dist
        $dllsCopiadas = (Get-ChildItem (Join-Path $dllDir "*.dll")).Count
        Write-Host "  [ok] $dllsCopiadas dlls de vcpkg"
    } else {
        Write-Host "  [aviso] No se encontro el bin de vcpkg en $dllDir"
    }
}

# --- 3. Runtime VC++ (msvcp140/vcruntime140) si el exe usa /MD --------------
#     Si esta la runtime redist instalada, copiamos junto al exe para que el
#     target no dependa del VC Redist. Si no existe (Compilacion /MT), se omite.
$vcr = Get-ChildItem "C:\Program Files\Microsoft Visual Studio\2022\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT\*.dll" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
if ($vcr) {
    $vcr | ForEach-Object { Copy-Item $_ $Dist }
    Write-Host "  [ok] runtime VC143 junto al exe ($(($vcr | Measure-Object).Count) dlls)"
} else {
    Write-Host "  [aviso] No se copio runtime VC++: se asume build /MT o VC Redist instalado en el target."
}

# --- 4. Imagenes del editor (IconosGUI: folder/cpp/hpp/file/cubo .png) -------
$imagenes = Join-Path $ProjectDir "Imagenes"
if (-not (Test-Path $imagenes)) { Write-Error "Falta la carpeta Imagenes en $ProjectDir" }
Copy-Item $imagenes "$Dist\Imagenes" -Recurse
Write-Host "  [ok] Imagenes\ ($((Get-ChildItem (Join-Path $Dist 'Imagenes')).Count) archivos)"

Write-Host ""
Write-Host "  dist montado en: $Dist"
if ((Get-ChildItem $Dist).Count -eq 0) { Write-Error "dist quedo vacio" }