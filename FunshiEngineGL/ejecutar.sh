#!/usr/bin/env bash
# Lanza FunshiEngineGL SIN abrir una terminal atras de la ventana:
# compila si hace falta y ejecuta el editor desacoplado (nohup, en segundo
# plano). Toda la salida de consola la captura la propia app en logs/.
set -e
cd "$(dirname "$0")"

echo ">>> Compilando (si hay cambios)..."
cmake --build build --target FunshiEngineGL -j"$(nproc)"

BIN="build/FunshiEngineGL"
if [ ! -x "$BIN" ]; then
    echo "ERROR: no se encontro el ejecutable $BIN"
    exit 1
fi

nohup "$BIN" >/dev/null 2>&1 &
echo ">>> FunshiEngineGL lanzado (pid $!). Logs en: $(dirname "$BIN")/logs/"