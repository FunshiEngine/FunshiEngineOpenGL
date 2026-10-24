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
// Pruebas headless de las regiones con material propio de una malla.
//
// Un archivo con varios materiales (un .obj con varios "usemtl") produce una
// region por material: un rango contiguo de indices con el nombre del
// material. El importador antes concatenaba todas las sub-mallas y perdia esa
// informacion, asi que un modelo con varios materiales llegaba al motor como si
// tuviera uno solo.
//
// La suite escribe un .obj con dos materiales en una carpeta temporal, lo carga
// con el importador real (Assimp) y comprueba que las regiones salen bien.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/Assets/AssimpMeshLoader.h"
#include "../FunshiEngineGL/src/Assets/Mesh.h"
#include "../tests/TempPruebas.h"

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

// Escribe un .obj con dos materiales (dos "usemtl") y su .mtl.
bool escribirModeloConDosMateriales(const std::string& rutaObj,
                                     const std::string& rutaMtl) {
    {
        std::ofstream mtl(rutaMtl.c_str());
        if (!mtl) return false;
        mtl << "newmtl Rojo\n";
        mtl << "Kd 0.8 0.1 0.1\n";
        mtl << "newmtl Azul\n";
        mtl << "Kd 0.1 0.1 0.8\n";
    }
    std::ofstream obj(rutaObj.c_str());
    if (!obj) return false;
    obj << "# cubo con dos materiales, para la prueba de regiones\n";
    obj << "mtllib dosMateriales.mtl\n";
    obj << "v -1.0 -1.0  1.0\n";
    obj << "v  1.0 -1.0  1.0\n";
    obj << "v  1.0  1.0  1.0\n";
    obj << "v -1.0  1.0  1.0\n";
    obj << "v -1.0 -1.0 -1.0\n";
    obj << "v  1.0 -1.0 -1.0\n";
    obj << "v  1.0  1.0 -1.0\n";
    obj << "v -1.0  1.0 -1.0\n";
    obj << "vt 0.0 0.0\n";
    obj << "vt 1.0 0.0\n";
    obj << "vt 1.0 1.0\n";
    obj << "vt 0.0 1.0\n";
    obj << "usemtl Rojo\n";
    obj << "f 1/1 2/2 3/3\n";
    obj << "f 1/1 3/3 4/4\n";
    obj << "usemtl Azul\n";
    obj << "f 5/1 6/2 7/3\n";
    obj << "f 5/1 7/3 8/4\n";
    return true;
}

// El importador real: un archivo con dos materiales produce dos regiones.
void cargaConDosMateriales() {
    TempPruebas::CarpetaPrueba carpeta("funshi_regiones_tests");
    const std::string rutaObj = (carpeta.ruta() / "dosMateriales.obj").string();
    const std::string rutaMtl = (carpeta.ruta() / "dosMateriales.mtl").string();
    CHECK(escribirModeloConDosMateriales(rutaObj, rutaMtl),
          "se escribe el modelo de prueba");

    AssimpMeshLoader loader;
    std::shared_ptr<Mesh> malla;
    try {
        malla = loader.load(rutaObj);
    } catch (const std::exception& ex) {
        CHECK(false, "el modelo de prueba se carga sin error");
        std::cout << "    " << ex.what() << std::endl;
        return;
    }
    CHECK(malla != nullptr, "el modelo de prueba se carga");
    if (!malla) return;

    CHECK(malla->subMallas.size() == 2,
          "un archivo con dos materiales produce dos regiones");
    if (malla->subMallas.size() != 2) return;

    CHECK(malla->subMallas[0].nombreMaterial == "Rojo",
          "la primera region conserva el nombre del material");
    CHECK(malla->subMallas[1].nombreMaterial == "Azul",
          "la segunda region conserva el nombre del material");

    // Los rangos son contiguos, empiezan en cero y cubren todos los indices.
    unsigned int esperadoInicio = 0;
    bool rangosContiguos = true;
    bool cubrenTodo = true;
    for (const SubMalla& region : malla->subMallas) {
        if (region.indiceInicio != esperadoInicio) rangosContiguos = false;
        if (region.indiceCantidad == 0) cubrenTodo = false;
        esperadoInicio = region.indiceInicio + region.indiceCantidad;
    }
    CHECK(rangosContiguos, "los rangos de las regiones son contiguos");
    CHECK(cubrenTodo, "ninguna region se queda sin indices");
    CHECK(esperadoInicio == malla->indices.size(),
          "las regiones cubren todos los indices de la malla");

    // La busqueda de region por indice acierta en cada rango.
    const SubMalla* primera = malla->regionDeIndice(0);
    const SubMalla* segunda =
        malla->regionDeIndice(malla->subMallas[1].indiceInicio);
    CHECK(primera != nullptr && primera->nombreMaterial == "Rojo",
          "el indice 0 pertenece a la primera region");
    CHECK(segunda != nullptr && segunda->nombreMaterial == "Azul",
          "el primer indice de la segunda region devuelve esa region");
    CHECK(malla->regionDeIndice(malla->indices.size() + 10) == nullptr,
          "un indice fuera de la malla no devuelve region");
}

// Una malla sin materiales no tiene regiones: se dibuja entera de una vez.
void mallaSinMateriales() {
    Mesh malla;
    malla.vertices.push_back(vec3(0.f, 0.f, 0.f));
    malla.indices.push_back(0);
    CHECK(malla.subMallas.empty(), "una malla sin regiones no declara ninguna");
    CHECK(malla.regionDeIndice(0) == nullptr,
          "sin regiones no hay region a la que pertenece un indice");
}

int main() {
    cargaConDosMateriales();
    mallaSinMateriales();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
