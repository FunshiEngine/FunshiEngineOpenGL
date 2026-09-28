/*
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    SPDX-License-Identifier: Apache-2.0
*/
#ifndef PROCESO_H
#define PROCESO_H

// Ejecucion de procesos SIN shell (H-3 nivel 2).
//
// std::system arma `cmd.exe /c <comando>` en Windows: entre el motor y el
// compilador se colaban el quoting de cmd.exe (que no procesa escapes con
// backslash), el strip de la primera/ultima comilla cuando el comando
// arranca con comilla, y la expansion de %VAR% sobre las rutas de usuario.
// Aca el comando se arma como argv explicito y el proceso se lanza con
// CreateProcessW (Windows) o fork+execvp (POSIX): cero shell, cero quoting
// de cmd. Ver PLAN GENERAL DE FIX.md §21.
//
// El header NO incluye windows.h: eso vive solo en Proceso.cpp (con su
// _HAS_STD_BYTE=0), para que los TUs del motor incluyan este archivo junto
// a la stdlib sin heredar el baile de headers de Windows. Mismo criterio que
// FileSystemWatcher: la abstraccion de SO vive en FileManager.
//
// Paridad multiplataforma (AGENTS.md): los argumentos pasan como bytes en
// las dos ramas; en Windows la linea se convierte CP_ACP -> ancho, la misma
// conversion que hacian std::system/LoadLibraryA.

#include <string>
#include <utility>
#include <vector>

namespace Proceso {

// Cita un argumento para la LINEA de comandos de Windows siguiendo las
// reglas con las que el hijo (la CRT de C/C++ / CommandLineToArgvW) vuelve a
// parsear su argv:
//  - los backslashes internos se dejan tal cual: para el hijo no son
//    escapes;
//  - solo se duplica el tramo FINAL de backslashes, el unico que quedaria
//    pegado a la comilla de cierre (una ruta terminada en \, como la del
//    /Fo de cl.exe, dejaria \" y el hijo se comeria la comilla);
//  - una comilla interior se escribe \" (en rutas de Windows no puede haber
//    comillas; si llegara una, de un archivo tocado a mano, ese es el
//    escape que el hijo entiende).
//
// La misma regla servia para cmd.exe (H-3 nivel 1, commit 1087f50); ahora
// que no hay shell, protege el parseo del hijo. En POSIX no se usa:
// execvp(argv) recibe el argv tal cual.
inline std::string citar(const std::string& ruta) {
    std::string::size_type fin = ruta.size();
    while (fin > 0 && ruta[fin - 1] == '\\') --fin;

    std::string resultado = "\"";
    for (std::string::size_type i = 0; i < fin; ++i) {
        if (ruta[i] == '"')
            resultado += "\\\"";
        else
            resultado += ruta[i];
    }
    for (std::string::size_type i = fin; i < ruta.size(); ++i)
        resultado += "\\\\";
    resultado += "\"";
    return resultado;
}

// La linea completa que recibe el hijo en Windows: cada argv citado y unido
// con un espacio. Se puede assertar en tests en las TRES plataformas aunque
// solo la usa CreateProcessW (asi el quoting se prueba tambien en CI Linux).
inline std::string lineaDeArgumentos(const std::vector<std::string>& argv) {
    std::string linea;
    for (const std::string& arg : argv) {
        if (!linea.empty())
            linea += ' ';
        linea += citar(arg);
    }
    return linea;
}

// Lanza argv[0] con argv[1..] SIN shell y espera a que termine.
//  - log: archivo que recibe stdout+stderr (se trunca); vacio = el hijo
//    hereda los std del motor (igual que std::system).
//  - cwd: directorio de trabajo del hijo; vacio = el del motor.
//  - entornoExtra: variables que se agregan o sobreescriben en el entorno
//    del hijo (Windows: bloque montado sobre el entorno actual y ordenado
//    alfabeticamente; POSIX: se fijan en el hijo antes del exec).
// Devuelve el exit code del hijo. Si no arranco: -1 en Windows y 127 en
// POSIX (el hijo fallo en el exec); en las dos ramas alcanza con != 0 para
// tratarlo como error.
int ejecutar(const std::vector<std::string>& argv,
             const std::string& log = std::string(),
             const std::string& cwd = std::string(),
             const std::vector<std::pair<std::string, std::string>>&
                 entornoExtra = {});

#if defined(_WIN32)
// Igual que ejecutar() pero con un bloque de entorno COMPLETO ya construido
// (multi-sz UTF-16, la forma nativa): es el camino del entorno de vcvars,
// que llega en UTF-16 desde la salida de `set /U` y no tiene que pasar
// jamas por narrow (cero perdida de encoding). entornoExtra no aplica.
int ejecutarConBloque(const std::vector<std::string>& argv,
                      const std::wstring& bloqueEntorno,
                      const std::string& log = std::string(),
                      const std::string& cwd = std::string());

// Lanza `cmd.exe` con la receta cruda tal cual, SIN citarla: cmd.exe no
// entiende el escape \" de la CRT, asi que una receta armada a mano con sus
// comillas balanceadas es lo unico que le llega limpio. SOLO para lineas
// fijas del motor sin datos de usuario (ver
// ComandoCompilacionCpp::comandoEntornoVcvars); los programas reales van
// por ejecutar().
int ejecutarCmdCrudo(const std::string& receta, const std::string& log);
#endif

} // namespace Proceso

#endif // PROCESO_H
