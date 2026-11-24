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
#ifndef IMPORTARPROYECTO_H
#define IMPORTARPROYECTO_H

#include <string>

// Importar un proyecto que esta fuera de la carpeta de proyectos (por ejemplo,
// traido de otra computadora): se COPIA a <Proyects>/<Nombre>, que es el unico
// lugar donde viven los proyectos. El origen no se toca.
//
// Un proyecto es una carpeta con Memory/ y src<Nombre>/ (y su <Nombre>.fnep).
// Las rutas internas se calculan desde el nombre, asi que importar con otro
// nombre renombra src<Nombre> y reescribe el .fnep.
//
// La copia se hace primero en una carpeta oculta (".importando-<Nombre>") y
// recien al terminar se renombra al nombre final: un proyecto a medio copiar
// nunca aparece en la lista de proyectos.
//
// Sin interfaz ni textos: devuelve un Problema y quien muestra decide el texto.
namespace ImportarProyecto {

enum class Problema {
    Ninguno,
    NoExiste,           // la ruta elegida no existe
    NoEsProyecto,       // la carpeta no tiene Memory/ y src<Nombre>/
    YaEstaEnProyectos,  // el origen ya esta dentro de la carpeta de proyectos
    NombreInvalido,     // el nombre no sirve como nombre de carpeta
    NombreRepetido,     // ya hay un proyecto con ese nombre
    ErrorCopia          // fallo de disco al copiar o renombrar
};

struct Origen {
    Problema problema = Problema::NoExiste;
    std::string carpeta;  // carpeta del proyecto de origen
    std::string nombre;   // nombre del proyecto en el origen (el de src<Nombre>)
    std::string motor;    // version del motor que escribio su .fnep (si tiene)
    bool valido() const { return problema == Problema::Ninguno; }
};

// Analiza lo que eligio el usuario: la carpeta del proyecto o su archivo .fnep.
// raizProyectos es la carpeta de proyectos (para rechazar lo que ya esta ahi).
Origen analizar(const std::string& ruta, const std::string& raizProyectos);
Origen analizar(const std::string& ruta);

// Problema del nombre con el que entraria el proyecto (Ninguno si sirve).
Problema validarNombre(const std::string& nombre, const std::string& raizProyectos);

// Copia el proyecto a <raizProyectos>/<nombreDestino>. Devuelve Ninguno si
// quedo importado; ante un fallo no deja nada a medias en la carpeta.
Problema importar(const Origen& origen, const std::string& nombreDestino,
                  const std::string& raizProyectos);
Problema importar(const Origen& origen, const std::string& nombreDestino);

}  // namespace ImportarProyecto

#endif
