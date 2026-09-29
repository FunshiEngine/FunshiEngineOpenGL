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
#ifndef PROJECT_PATHS_H
#define PROJECT_PATHS_H

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#endif

#include <string>

namespace ProjectPaths {

// Directorio del ejecutable (ancla portable: config/proyectos junto al binario)
std::string directorioEjecutable();

// Raiz MotorGrafico, donde viven los proyectos, las escenas y la configuracion.
//
// Antes vivia SIEMPRE en <exeDir>/MotorGrafico, lo que solo funciona si el motor
// esta en una carpeta escribible. Instalado en "C:\Program Files" no lo esta:
// el ejecutable corre sin elevar y la carpeta la creo el instalador (que si es
// admin) con los ACL de Program Files, asi que todas las escrituras fallaban en
// silencio. Por eso la raiz se RESUELVE: se prueba de verdad si se puede
// escribir y, si no, se usa una carpeta de datos por usuario (%APPDATA% en
// Windows, ~/.local/share en Linux). La decision se cachea por proceso.
std::string directorioBase();

// La ubicacion historica (<exeDir>/MotorGrafico) este o no sea la elegida. Se
// expone para poder informar al usuario y para la migracion de la linea de abajo.
std::string directorioBaseOriginal();

// true cuando directorioBase() cayo a la carpeta de datos por usuario porque la
// ruta junto al ejecutable no es escribible.
bool datosEnRutaDeUsuario();

// Migra una sola vez los datos que hubieran quedado en directorioBaseOriginal()
// hacia la carpeta de datos por usuario. Solo actsua si se esta usando el
// fallback y el destino esta vacio (nunca pisa datos existentes). Devuelve
// true si copio algo y deja en `mensaje` el texto para el usuario; devuelve
// false con `mensaje` vacio si no habia nada que migrar.
bool migrarDatosDesdeRutaOriginal(std::string& mensaje);

// Copia el arbol `origen` dentro de `destino`, que debe estar VACIO: si ya tiene
// algo, no copia nada y no es un error (deja `error` vacio y devuelve false).
// Es la operacion que hace la migracion de arriba, expuesta aparte porque en un
// runner de CI la carpeta del ejecutable SIEMPRE es escribible, asi que la
// migracion completa no se puede ejecutar ahi y esta parte queda sin cubrir.
bool copiarArbolSiDestinoVacio(const std::string& origen,
                               const std::string& destino,
                               std::string& error);

// Carpetas de primer nivel bajo MotorGrafico/
std::string directorioProyects();
std::string directorioConfiguraciones();
std::string directorioExportaciones();

// Proyecto especifico: <Proyects>/<nombre>
std::string directorioProyecto(const std::string& nombreProyecto);

// Subcarpetas del proyecto
std::string directorioMemory(const std::string& nombreProyecto);
std::string directorioBinarios(const std::string& nombreProyecto);
std::string directorioInterfaces(const std::string& nombreProyecto);
std::string nombreRaizSrc(const std::string& nombreProyecto);
std::string directorioSrc(const std::string& nombreProyecto);
std::string directorioSonidos(const std::string& nombreProyecto);

// Archivos de configuracion
std::string rutaConfiguracionGeneral();        // <Configuraciones>/Configuracion.json
std::string rutaConfiguracionProyecto(const std::string& nombreProyecto); // <Memory>/ConfiguracionProyecto.json

// Archivos de escena (binarios)
std::string rutaScenePrefijo(const std::string& nombreProyecto);
std::string rutaSceneBBDD(const std::string& nombreProyecto);
std::string rutaSceneDir(const std::string& nombreProyecto);
std::string rutaImguiIni(const std::string& nombreProyecto);

// Exportacion: <Exportaciones>/<nombreExportacion>/
std::string directorioExportacion(const std::string& nombreExportacion);

// Validacion de nombre de proyecto (sin separadores, vacio, reservados)
bool esNombreValido(const std::string& nombre);

} // namespace ProjectPaths

#endif // PROJECT_PATHS_H