#!/usr/bin/env python3
"""Valida el GLSL embebido en el motor.

Los shaders del motor van embebidos en el binario (ShaderSources.h y los
headers que incluye), asi que
un error de sintaxis no lo ve el compilador de C++: solo aparece cuando el
driver intenta compilar el programa, con la app ya abierta. Este script
extrae los fuentes embebidos y los compila con glslangValidator, que es un
validador de GLSL que no necesita GPU.

Codigo de salida:
  0  todos los shaders compilan
  1  algun shader no compila (o el archivo de shaders no se entiende)
 77  no hay validador en la maquina: la comprobacion se salta
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

RUTO_SHADERS = os.path.join(
    "FunshiEngineGL", "src", "Rendering", "Shaders", "ShaderSources.h")

# Cada shader embebido: inline const char* const kNombre = R"( ... )";
PATRON = re.compile(r'inline const char\* const (\w+) = R"\((.*?)\)";', re.S)


def clasificar(nombre):
    """Devuelve la etapa del shader ('vert' o 'frag') segun su nombre."""
    if "Fragment" in nombre:
        return "frag"
    if "Vertex" in nombre:
        return "vert"
    return None


def leer_con_includes(ruta, vistos=None):
    """Contenido de ruta mas el de los headers locales que incluye.

    ShaderSources.h agrupa los shaders por tema en varios headers
    (ShaderMalla.h, ShaderSombras.h...) y solo los incluye: hay que seguir esos
    #include para encontrar los fuentes.
    """
    vistos = set() if vistos is None else vistos
    ruta = os.path.normpath(ruta)
    if ruta in vistos or not os.path.isfile(ruta):
        return ""
    vistos.add(ruta)
    with open(ruta, encoding="utf-8") as archivo:
        contenido = archivo.read()
    carpeta = os.path.dirname(ruta)
    for incluido in re.findall(r'^\s*#include "([^"]+)"', contenido, re.M):
        contenido += "\n" + leer_con_includes(os.path.join(carpeta, incluido), vistos)
    return contenido


def main():
    if not os.path.isfile(RUTO_SHADERS):
        print("No se encuentra el archivo de shaders: " + RUTO_SHADERS)
        return 1

    validador = shutil.which("glslangValidator")
    if validador is None:
        print("glslangValidator no esta disponible: se salta la validacion.")
        return 77

    contenido = leer_con_includes(RUTO_SHADERS)

    shaders = PATRON.findall(contenido)
    if not shaders:
        print("No se encontro ningun shader embebido en " + RUTO_SHADERS)
        return 1

    fallos = 0
    with tempfile.TemporaryDirectory() as carpeta:
        for nombre, fuente in shaders:
            etapa = clasificar(nombre)
            if etapa is None:
                print("[FALLO] " + nombre +
                      ": el nombre no dice si es vertex o fragment")
                fallos += 1
                continue
            if "#version" not in fuente:
                print("[FALLO] " + nombre + ": el fuente no declara #version")
                fallos += 1
                continue

            ruta = os.path.join(carpeta, nombre + "." + etapa)
            with open(ruta, "w", encoding="utf-8", newline="\n") as salida:
                salida.write(fuente)

            resultado = subprocess.run(
                [validador, "-S", etapa, ruta],
                capture_output=True, text=True)
            if resultado.returncode != 0:
                print("[FALLO] " + nombre + " (" + etapa + ")")
                print(resultado.stdout.strip())
                print(resultado.stderr.strip())
                fallos += 1
            else:
                print("OK   " + nombre + " (" + etapa + ")")

    if fallos:
        print(str(fallos) + " shader(s) no compilan.")
        return 1
    print(str(len(shaders)) + " shaders compilan.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
