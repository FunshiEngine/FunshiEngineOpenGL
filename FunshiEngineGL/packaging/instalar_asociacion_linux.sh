#!/usr/bin/env sh
# SPDX-License-Identifier: Apache-2.0
# Asocia los proyectos .fnep con FunshiEngineGL para el usuario actual:
# instala el lanzador (.desktop) y el tipo MIME, y lo marca como aplicacion
# por defecto. Uso: instalar_asociacion_linux.sh <carpeta donde esta FunshiEngineGL>
set -eu
INSTALACION="${1:?Uso: $0 <carpeta de instalacion de FunshiEngineGL>}"
INSTALACION="$(cd "$INSTALACION" && pwd)"
AQUI="$(cd "$(dirname "$0")" && pwd)"
DATOS="${XDG_DATA_HOME:-$HOME/.local/share}"

mkdir -p "$DATOS/applications" "$DATOS/mime/packages"
sed "s#<RUTA_INSTALACION>#$INSTALACION#g" "$AQUI/FunshiEngineGL.desktop" \
    > "$DATOS/applications/FunshiEngineGL.desktop"
cp "$AQUI/funshiengine-fnep.xml" "$DATOS/mime/packages/funshiengine-fnep.xml"

command -v update-mime-database >/dev/null && update-mime-database "$DATOS/mime"
command -v update-desktop-database >/dev/null && update-desktop-database "$DATOS/applications"
command -v xdg-mime >/dev/null && xdg-mime default FunshiEngineGL.desktop application/x-funshi-proyecto
echo "Listo: los .fnep se abren con $INSTALACION/FunshiEngineGL"
