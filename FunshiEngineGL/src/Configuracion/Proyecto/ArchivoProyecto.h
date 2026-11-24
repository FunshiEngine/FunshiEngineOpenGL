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
#ifndef ARCHIVOPROYECTO_H
#define ARCHIVOPROYECTO_H

#include <optional>
#include <string>

// Archivo .fnep: identifica un proyecto de FunshiEngine. Vive SIEMPRE dentro de
// la carpeta del proyecto (<Proyects>/<Nombre>/<Nombre>.fnep) y abrirlo (doble
// clic o como argumento) abre ese proyecto en el editor. Es JSON:
//
//   { "formato": "fnep", "version": 1, "nombre": "MiJuego", "motor": "0.4.0" }
//
// Las rutas del proyecto se siguen calculando desde su nombre (ProjectPaths):
// el .fnep no guarda rutas, asi copiar la carpeta de datos a otra maquina no lo
// invalida.
namespace ArchivoProyecto {

constexpr int kVersionFormato = 1;

struct Datos {
    std::string nombre;
    int version = kVersionFormato;
    std::string motor;  // version del motor que lo escribio
};

// <Proyects>/<nombre>/<nombre>.fnep
std::string rutaDe(const std::string& nombreProyecto);

// Escribe el archivo (escritura atomica: temporal + renombrado).
bool escribir(const std::string& ruta, const Datos& datos, std::string& error);

// Lee y valida el archivo (formato, version soportada, nombre presente).
std::optional<Datos> leer(const std::string& ruta, std::string& error);

// Crea el .fnep del proyecto si falta. No pisa uno existente.
bool asegurar(const std::string& nombreProyecto, std::string& error);

// Resuelve que proyecto abre un .fnep: lo lee y comprueba que este en la
// carpeta de su proyecto dentro de Proyects/. Devuelve el nombre del proyecto.
std::optional<std::string> resolverApertura(const std::string& rutaFnep,
                                            std::string& error);

}  // namespace ArchivoProyecto

#endif
