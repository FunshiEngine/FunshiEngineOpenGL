#!/usr/bin/env bash
# ============================================================================
#  FunshiEngineGL - Monta los datos del paquete IFW de Linux con el binario
#  compilado en CI (build recien hecho) + sus librerias dinamicas locales.
#
#  Replica el flujo Linux que ya fue probado (instalador v0.5.1):
#   - binario con RPATH $ORIGIN/lib (DT_RPATH, NO DT_RUNPATH: runpath no se
#     usa para deps transitivas, y libdraco.so.8/libminizip/libpugixml son
#     transitivas libassimp -> fallaban con "libdraco.so.8: cannot open")
#   - deps de terceros (glfw/assimp/bullet/draco/...) en data/lib/
#   - NO toca packaging/ifw/config (config.xml/controller.qs intactos)
#
#  Uso: stage_dist_linux.sh <build/FunshiEngineGL> [<dir data del paquete>]
# ============================================================================
set -euo pipefail

BIN_PATH="${1:?falta el binario compilado (build/FunshiEngineGL)}"
DATA_DIR="${2:-$(cd "$(dirname "${BASH_SOURCE[0]}")/ifw/packages/com.funshi.engine/data" && pwd)}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ ! -x "$BIN_PATH" ]; then
    echo "[ERROR] No existe el binario $BIN_PATH" >&2
    exit 1
fi

echo "== [1/5] Binario =="
install -m 755 "$BIN_PATH" "$DATA_DIR/FunshiEngineGL"
patchelf --force-rpath --set-rpath '$ORIGIN/lib' "$DATA_DIR/FunshiEngineGL"
echo "  [ok] FunshiEngineGL (RPATH \$ORIGIN/lib)"

echo "== [2/5] Librerias dinamicas (ldd) =="
mkdir -p "$DATA_DIR/lib"
find "$DATA_DIR/lib" -mindepth 1 -maxdepth 1 -type l -o -type f | xargs -r rm -f
mapfile -t LIBS < <(ldd "$BIN_PATH" | sed -n 's/.*=> \([^ ]*\.so[^ ]*\) .*/\1/p')
for lib in "${LIBS[@]}"; do
    [ -n "$lib" ] && cp -L "$lib" "$DATA_DIR/lib/"
done

echo "== [3/5] Excluir libs del sistema (quedan solo deps de terceros) =="
for skip in libc.so libm.so libstdc++ libgcc_s libdl.so libpthread.so librt.so \
            libGL.so libGLX.so libGLdispatch.so libOpenGL.so libGLU.so \
            libX11.so libXext.so libXi.so libXrandr.so \
            libncurses.so libtinfo.so libz.so libzstd.so liblzma.so; do
    find "$DATA_DIR/lib" -maxdepth 1 -name "${skip}*" -delete 2>/dev/null || true
done

echo "== [4/5] Cabeceras para scripts C++ =="
# El script incluye Behaviour/IScriptBehaviour.h, que arrastra ScriptGameObject.h,
# Reflection/BehaviourReflection.h y Matematicas/StructVec3.h. El backend resuelve
# la carpeta "include" junto al ejecutable, asi que el paquete tiene que llevarla.
CABECERAS=(
    "Behaviour/IScriptBehaviour.h:Behaviour"
    "Behaviour/ScriptGameObject.h:Behaviour"
    "Behaviour/Reflection/BehaviourReflection.h:Behaviour/Reflection"
    "Matematicas/StructVec3.h:Matematicas"
)
for par in "${CABECERAS[@]}"; do
    rel="${par%%:*}"; sub="${par##*:}"
    origen="$PROJECT_DIR/src/$rel"
    [ -f "$origen" ] || { echo "[ERROR] Falta la cabecera $origen" >&2; exit 1; }
    mkdir -p "$DATA_DIR/include/$sub"
    install -m 644 "$origen" "$DATA_DIR/include/$sub/"
done
echo "  [ok] include/ (${#CABECERAS[@]} cabeceras para scripts)"

echo "== [5/5] Resultado =="
echo "  binario : $DATA_DIR/FunshiEngineGL"
echo "  libs    :"
ls -1 "$DATA_DIR/lib"