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
// Pruebas headless de los iconos de componente:
//
// El catalogo de componentes es la fuente unica del icono de cada tipo (el
// nombre del tipo ya es el dato persistido de la escena, asi que el icono se
// deriva de el y no se serializa). Se verifica que:
//   - todo componente del catalogo declare icono,
//   - la imagen de cada icono exista de verdad en el disco,
//   - la regla resuelva por nombre de tipo, por alias y con el prefijo que
//     MSVC antepone ("class Transform"),
//   - un tipo desconocido no devuelva icono ni reviente.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <iostream>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Herramientas/IconosGUI/ReglaIconoCatalogo.h"
#include "../FunshiEngineGL/src/Herramientas/IconosGUI/RutasIconos.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/CatalogoComponentes.h"

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Todo componente del catalogo declara icono y la imagen existe en disco.
void catalogoConIconoExistente() {
    const auto& descriptores = CatalogoComponentes::instancia().descriptores();
    CHECK(!descriptores.empty(), "el catalogo tiene componentes registrados");

    bool todosConIcono = true;
    bool todosConArchivo = true;
    bool todasRutasValidas = true;
    for (const DescriptorComponente& d : descriptores) {
        if (d.icono.empty()) {
            todosConIcono = false;
            std::cout << "    componente sin icono: " << d.nombre << std::endl;
        }
        if (!d.icono.empty() &&
            d.icono.rfind("GUI-Image/Component-Image/", 0) != 0) {
            todasRutasValidas = false;
            std::cout << "    icono fuera de la carpeta de componentes: "
                      << d.nombre << " -> " << d.icono << std::endl;
        }
        if (!d.icono.empty() && !RutasIconos::existe(d.icono)) {
            todosConArchivo = false;
            std::cout << "    no se encuentra el archivo: " << d.nombre << " -> "
                      << d.icono << std::endl;
        }
    }
    CHECK(todosConIcono, "todo componente del catalogo declara icono");
    CHECK(todasRutasValidas,
          "los iconos apuntan a la carpeta de imagenes de componente");
    CHECK(todosConArchivo, "el archivo de cada icono existe en el disco");
}

// La regla resuelve por nombre de tipo y tolera el alias y el prefijo de MSVC.
void reglaResuelvePorTipo() {
    const ReglaIconoCatalogo regla;

    const std::string transform =
        regla.rutaIconoPara("Transform");
    CHECK(!transform.empty(), "la regla resuelve el icono de Transform");
    CHECK(transform ==
              "GUI-Image/Component-Image/Transform/comp-transform-white.png",
          "el icono de Transform es el declarado por el catalogo");

    // Alias retrocompatible: el componente de camara se llamaba "Camera".
    CHECK(regla.rutaIconoPara("Camera") == regla.rutaIconoPara("CameraComponent"),
          "el alias Camera devuelve el mismo icono que CameraComponent");

    // Prefijo de MSVC: el nombre persistido puede venir como "class Transform".
    CHECK(regla.rutaIconoPara("class Transform") == transform,
          "el prefijo de MSVC no cambia el icono");

    CHECK(regla.rutaIconoPara("ComponenteInexistente").empty(),
          "un tipo desconocido no devuelve icono");
    CHECK(regla.rutaIconoPara("").empty(), "un tipo vacio no devuelve icono");
}

// El icono se deriva del tipo: el catalogo y la regla no se desincronizan.
void reglaYcatalogoCoinciden() {
    const ReglaIconoCatalogo regla;
    const auto& descriptores = CatalogoComponentes::instancia().descriptores();
    bool todosCoinciden = true;
    for (const DescriptorComponente& d : descriptores) {
        if (regla.rutaIconoPara(d.nombre) != d.icono) {
            todosCoinciden = false;
            std::cout << "    la regla y el catalogo discrepan en: " << d.nombre
                      << std::endl;
        }
    }
    CHECK(todosCoinciden, "la regla devuelve el icono que declara el catalogo");
}

// Un icono compartido por dos componentes es valido (hay componentes que
// comparten imagen, como el audio y la musica), pero dos componentes con
// nombre distinto no pueden acabar con el mismo icono generico vacio.
void iconosPropiosDeCadaComponente() {
    const auto& descriptores = CatalogoComponentes::instancia().descriptores();
    int genericos = 0;
    for (const DescriptorComponente& d : descriptores) {
        if (d.icono == "GUI-Image/Component-Image/comp-empty-white.png") {
            ++genericos;
        }
    }
    // El icono generico es el de los componentes que no tienen imagen propia
    // (Color, Grid y Sombras): no puede ser el de la mayoria.
    CHECK(genericos <= 4,
          "el icono generico no se usa para la mayoria de los componentes");
}

int main() {
    catalogoConIconoExistente();
    reglaResuelvePorTipo();
    reglaYcatalogoCoinciden();
    iconosPropiosDeCadaComponente();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
