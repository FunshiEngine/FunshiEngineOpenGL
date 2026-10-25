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
// Pruebas headless de los materiales por region:
//
// El componente Materiales guarda una region por parte de la malla (la cara
// de un cubo, la parte de un personaje) con su material. El resolvedor decide
// que material se dibuja en cada region y cual es el material unico del
// objeto. Se verifica que:
//   - las regiones se anaden, no se duplican y se pueden quitar,
//   - el componente hace round-trip binario (el material de cada region
//    includedo),
//   - el resolvedor devuelve el material de la region, el material unico del
//     objeto o nada, segun lo que tenga el objeto.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Objetos/Componentes/IMaterial.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Material.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Materiales.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/ResolvedorMateriales.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
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

// Alta, busqueda y baja de regiones.
void gestionDeRegiones() {
    Materiales materiales;
    CHECK(materiales.cantidadRegiones() == 0, "un componente nuevo no tiene regiones");

    CHECK(materiales.agregarRegion("Rojo"), "se anade una region");
    CHECK(materiales.agregarRegion("Azul"), "se anade otra region");
    CHECK(materiales.cantidadRegiones() == 2, "hay dos regiones");
    CHECK(!materiales.agregarRegion("Rojo"), "no se duplica una region");
    CHECK(!materiales.agregarRegion(""), "no se anade una region sin nombre");
    CHECK(materiales.cantidadRegiones() == 2, "siguen habiendo dos regiones");

    CHECK(materiales.region("Rojo") != nullptr, "se encuentra la region por nombre");
    CHECK(materiales.region("Verde") == nullptr,
          "una region que no existe no se encuentra");
    CHECK(materiales.materialDe("Azul") != nullptr,
          "la region tiene su propio material");

    CHECK(materiales.quitarRegion("Rojo"), "se quita una region");
    CHECK(materiales.cantidadRegiones() == 1, "queda una region");
    CHECK(!materiales.quitarRegion("Rojo"), "no se quita dos veces la misma");
}

// Sincronizar con las regiones que declara el modelo.
void sincronizarConElModelo() {
    Materiales materiales;
    materiales.agregarRegion("Rojo");
    const std::vector<std::string> nombres = {"Rojo", "Azul", "Verde"};
    const int anadidas = materiales.sincronizarCon(nombres);
    CHECK(anadidas == 2, "se anaden solo las regiones que faltan");
    CHECK(materiales.cantidadRegiones() == 3, "quedan las tres regiones");
    CHECK(materiales.materialDe("Verde") != nullptr,
          "la region nueva trae material por defecto");
    // Sincronizar otra vez no cambia nada.
    CHECK(materiales.sincronizarCon(nombres) == 0,
          "sincronizar dos veces no anade regiones nuevas");
}

// Round-trip binario del componente (con el material de cada region).
void roundTripBinario() {
    TempPruebas::CarpetaPrueba carpeta("funshi_materiales_tests");
    const std::string ruta = (carpeta.ruta() / "materiales.db").string();

    Materiales originales;
    originales.agregarRegion("Rojo");
    originales.agregarRegion("Azul");
    Material* rojo = originales.materialDe("Rojo");
    rojo->setDiffuse(0.8f, 0.1f, 0.1f);
    rojo->setShininess(7.f);
    rojo->setDiffuseMapPath("Assets/Modelos/rojo.png");
    Material* azul = originales.materialDe("Azul");
    azul->setDiffuse(0.1f, 0.1f, 0.8f);

    {
        std::ofstream archivo(ruta.c_str(), std::ios::binary);
        CHECK(archivo.is_open(), "se abre el archivo para escribir");
        originales.saveComponent(&archivo);
    }

    Materiales leidos;
    {
        std::ifstream archivo(ruta.c_str(), std::ios::binary);
        CHECK(archivo.is_open(), "se abre el archivo para leer");
        leidos.loadComponent(&archivo);
    }

    CHECK(leidos.cantidadRegiones() == 2, "se recuperan las dos regiones");
    if (leidos.cantidadRegiones() != 2) return;
    CHECK(leidos.regiones()[0].nombre == "Rojo", "la primera region conserva el nombre");
    CHECK(leidos.regiones()[1].nombre == "Azul", "la segunda region conserva el nombre");

    const Material* rojoLeido = leidos.materialDe("Rojo");
    CHECK(rojoLeido != nullptr, "la region recuperada trae material");
    if (!rojoLeido) return;
    CHECK(rojoLeido->getDiffuse()[0] > 0.7f &&
              rojoLeido->getDiffuse()[1] < 0.2f,
          "el color difuso de la region se conserva");
    CHECK(rojoLeido->getShininess() > 6.5f && rojoLeido->getShininess() < 7.5f,
          "el brillo de la region se conserva");
    CHECK(rojoLeido->getDiffuseMapPath().find("rojo.png") != std::string::npos,
          "la textura de la region se conserva");
}

// El resolvedor decide que material se dibuja en cada region.
void resolvedorDeMateriales() {
    const ResolvedorMateriales resolvedor;

    // Objeto con materiales por region: cada region devuelve su material.
    auto objeto = GameObjectFactory::createSimpleObject(nullptr);
    auto* materiales = new Materiales();
    objeto->addComponent(std::unique_ptr<Component>(materiales));
    materiales->agregarRegion("Rojo");
    materiales->materialDe("Rojo")->setDiffuse(0.8f, 0.1f, 0.1f);

    const IMaterial* deRegion = resolvedor.materialDeRegion(objeto.get(), "Rojo");
    CHECK(deRegion != nullptr, "el resolvedor devuelve el material de la region");
    if (deRegion) {
        CHECK(deRegion->getDiffuse()[0] > 0.7f,
              "el material devuelto es el de la region");
    }
    // Region que no esta en la lista: cae al material unico del objeto (si lo
    // hay) y, si no, a nullptr.
    CHECK(resolvedor.materialDeRegion(objeto.get(), "Verde") == nullptr,
          "una region sin material propio no devuelve material");

    // Objeto con material unico: se usa para cualquier region.
    auto conMaterial = GameObjectFactory::createSimpleObject(nullptr);
    auto* material = new Material();
    material->setDiffuse(0.2f, 0.4f, 0.6f);
    conMaterial->addComponent(std::unique_ptr<Component>(material));
    const IMaterial* unico = resolvedor.materialDeRegion(conMaterial.get(), "Rojo");
    CHECK(unico != nullptr, "el material unico del objeto sirve para cualquier region");
    if (unico) {
        CHECK(unico->getDiffuse()[1] > 0.3f,
              "el material devuelto es el unico del objeto");
    }

    // Objeto sin material: nullptr (el renderer usa el color o los defaults).
    auto sinMaterial = GameObjectFactory::createSimpleObject(nullptr);
    CHECK(resolvedor.materialDeRegion(sinMaterial.get(), "Rojo") == nullptr,
          "un objeto sin material no devuelve material");
    CHECK(resolvedor.materialDeRegion(nullptr, "Rojo") == nullptr,
          "un objeto nulo no devuelve material");
}

int main() {
    gestionDeRegiones();
    sincronizarConElModelo();
    roundTripBinario();
    resolvedorDeMateriales();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
