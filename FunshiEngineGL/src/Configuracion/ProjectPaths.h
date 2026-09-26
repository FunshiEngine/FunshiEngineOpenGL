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

// Raiz MotorGrafico: <exeDir>/MotorGrafico
std::string directorioBase();

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