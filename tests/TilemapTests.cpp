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
// Pruebas headless del tileset (JSON estilo Tiled) y del componente Tilemap:
//   - parseo del tileset, geometria de la grilla y flag de solidez por tile,
//   - capas, pintado por celda y revision,
//   - horneado de la malla (una malla por tilemap, cuatriangulos por tile),
//   - round-trip binario del componente.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI/OpenGL, y salida final "OK/FALLOS: N comprobaciones".

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <btBulletDynamicsCommon.h>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Assets/Mesh.h"
#include "../FunshiEngineGL/src/Assets/Tileset.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Colliders/TilemapCollider.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Colliders/TiposShapeBullet.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Tilemap.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Transform.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"

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

namespace {
bool casi(float a, float b) { return std::fabs(a - b) < 1e-4f; }

std::string ruta(const TempPruebas::CarpetaPrueba& carpeta,
                 const std::string& nombre) {
    return (carpeta.ruta() / nombre).string();
}

void escribir(const std::string& ruta, const std::string& contenido) {
    std::ofstream file(ruta, std::ios::binary);
    file << contenido;
}

// Tileset de 4 columnas x 2 filas de tiles de 16x16 en un atlas de 64x32, con
// los tiles 2 y 5 marcados como solidos.
void escribirTileset(const std::string& ruta) {
    std::ofstream f(ruta, std::ios::binary);
    f << "{\n"
         "  \"type\": \"tileset\",\n"
         "  \"name\": \"prueba\",\n"
         "  \"image\": \"atlas.png\",\n"
         "  \"imagewidth\": 64,\n"
         "  \"imageheight\": 32,\n"
         "  \"tilewidth\": 16,\n"
         "  \"tileheight\": 16,\n"
         "  \"margin\": 0,\n"
         "  \"spacing\": 0,\n"
         "  \"columns\": 4,\n"
         "  \"tilecount\": 8,\n"
         "  \"tiles\": [\n"
         "    {\"id\": 2, \"properties\": [{\"name\": \"solid\", \"type\": "
         "\"bool\", \"value\": true}]},\n"
         "    {\"id\": 5, \"properties\": [{\"name\": \"solid\", \"type\": "
         "\"bool\", \"value\": true}]}\n"
         "  ]\n"
         "}\n";
}

void tilesetValidoYMetadatos(const std::string& rutaTileset) {
    Tileset ts;
    std::string error;
    CHECK(cargarTilesetDesdeJson(rutaTileset, ts, error),
          "el tileset valido carga");
    CHECK(ts.esValido(), "el tileset cargado es valido");
    CHECK(ts.columns == 4, "el tileset infiere las columnas");
    CHECK(ts.rows == 2, "el tileset infiere las filas desde la imagen");
    CHECK(ts.tileCount == 8, "el tileset usa el tilecount declarado");
    CHECK(ts.rutaImagen == "atlas.png", "el tileset conserva la ruta de imagen");

    CHECK(ts.solido.size() == 8, "hay un flag de solidez por tile");
    CHECK(ts.solido[2] == 1 && ts.solido[5] == 1,
          "los tiles marcados como solid son solidos");
    CHECK(ts.solido[0] == 0 && ts.solido[7] == 0,
          "los tiles sin propiedad solid no son solidos");

    float uv[4] = {};
    ts.uvDeTile(0, uv);
    const float insetU = 0.5f / 64.0f;
    const float insetV = 0.5f / 32.0f;
    CHECK(casi(uv[0], 0.0f + insetU) && casi(uv[2], 0.25f - insetU),
          "el tile 0 abarca la primera columna del atlas en U");
    CHECK(casi(uv[1], 0.5f + insetV) && casi(uv[3], 1.0f - insetV),
          "el tile 0 abarca la fila superior del atlas en V");

    ts.uvDeTile(5, uv);
    CHECK(casi(uv[0], 0.25f + insetU) && casi(uv[2], 0.5f - insetU),
          "el tile 5 (fila 1, col 1) usa la segunda columna en U");
    CHECK(casi(uv[1], 0.0f + insetV) && casi(uv[3], 0.5f - insetV),
          "el tile 5 usa la fila inferior del atlas en V");

    const std::string rutaImagen = ts.rutaImagenAbsoluta(rutaTileset);
    CHECK(!rutaImagen.empty() && rutaImagen.find("atlas.png") != std::string::npos,
          "la ruta absoluta de la imagen se resuelve junto al tileset");
}

void tilesetErrores(const std::string& carpeta) {
    Tileset ts;
    std::string error;

    const std::string sinImagen = carpeta + "/sin_imagen.json";
    {
        std::ofstream f(sinImagen);
        f << "{\"tilewidth\":16,\"tileheight\":16,\"imagewidth\":32,"
             "\"imageheight\":32}";
    }
    CHECK(!cargarTilesetDesdeJson(sinImagen, ts, error),
          "un tileset sin 'image' se rechaza");
    CHECK(!error.empty(), "el rechazo deja un motivo");

    const std::string corrupto = carpeta + "/corrupto.json";
    {
        std::ofstream f(corrupto);
        f << "esto no es json";
    }
    CHECK(!cargarTilesetDesdeJson(corrupto, ts, error),
          "un JSON invalido se rechaza");

    const std::string sinTamano = carpeta + "/sin_tamano.json";
    {
        std::ofstream f(sinTamano);
        f << "{\"image\":\"a.png\",\"tilewidth\":0,\"tileheight\":16}";
    }
    CHECK(!cargarTilesetDesdeJson(sinTamano, ts, error),
          "un tileset sin tilewidth valido se rechaza");

    CHECK(!cargarTilesetDesdeJson(carpeta + "/no_existe.json", ts, error),
          "un archivo ausente se rechaza");
}

void tilemapCapasYPintado(const std::string& rutaTileset) {
    Tilemap tm;
    CHECK(!tm.tilesetValido(), "un tilemap sin tileset no es valido");

    tm.setRutaTileset(rutaTileset);
    CHECK(tm.tilesetValido(), "asignar un tileset valido lo carga");

    CHECK(tm.cantidadCapas() == 0, "el tilemap arranca sin capas");
    CHECK(tm.agregarCapa("Fondo", 4, 2, false) == 0,
          "se agrega la primera capa");
    CHECK(tm.agregarCapa("Solidos", 4, 2, true) == 1,
          "se agrega la segunda capa");
    CHECK(tm.agregarCapa("Mala", 0, 2, false) == -1,
          "una capa con dimensiones invalidas se rechaza");

    const CapaTilemap* fondo = tm.capa(0);
    CHECK(fondo && fondo->ancho == 4 && fondo->alto == 2 &&
              fondo->tiles.size() == 8,
          "la capa reserva una celda por posicion");

    tm.pintar(0, 0, 0, 1);
    tm.pintar(0, 3, 1, 7);
    tm.pintar(1, 0, 0, 2);
    CHECK(tm.tileEn(0, 0, 0) == 1 && tm.tileEn(0, 3, 1) == 7,
          "pintar asigna el id de tile en la celda");
    CHECK(tm.tileEn(1, 0, 0) == 2, "pintar afecta solo la capa indicada");
    CHECK(tm.tileEn(0, 9, 9) == 0, "consultar fuera de rango devuelve vacio");

    const std::uint64_t antes = tm.revision();
    tm.pintar(0, 1, 0, 1);
    CHECK(tm.revision() > antes, "pintar incrementa la revision");
    const std::uint64_t tras = tm.revision();
    tm.pintar(0, 1, 0, 1);
    CHECK(tm.revision() == tras,
          "repintar el mismo id no cambia la revision");

    tm.setCapaVisible(1, false);
    CHECK(tm.capa(1) && !tm.capa(1)->visible,
          "la visibilidad de la capa se actualiza");
}

void tilemapGeometria(const std::string& rutaTileset) {
    Tilemap tm;
    tm.setRutaTileset(rutaTileset);
    tm.agregarCapa("Fondo", 2, 2, false);

    Mesh vacio;
    CHECK(!tm.construirGeometria(vacio),
          "un tilemap sin celdas pintadas no genera geometria");

    tm.pintar(0, 0, 0, 1);
    tm.pintar(0, 1, 1, 3);

    Mesh malla;
    CHECK(tm.construirGeometria(malla), "pintar celdas genera geometria");
    CHECK(malla.vertices.size() == 8, "cada tile aporta cuatro vertices");
    CHECK(malla.indices.size() == 12, "cada tile aporta dos triangulos");
    CHECK(malla.hasUvs(), "la malla horneada trae una uv por vertice");

    float uv[4] = {};
    tm.getTileset().uvDeTile(1, uv);
    CHECK(casi(uv[0], malla.uvs[0].x) && casi(uv[1], malla.uvs[0].y),
          "el primer tile usa la esquina inferior del uv del tile 1");
    CHECK(casi(uv[2], malla.uvs[2].x) && casi(uv[3], malla.uvs[2].y),
          "el primer tile usa la esquina superior del uv del tile 1");

    tm.setCapaVisible(0, false);
    Mesh oculto;
    CHECK(!tm.construirGeometria(oculto),
          "una capa oculta no aporta geometria");
}

void tilemapSerializacion(const std::string& carpeta,
                          const std::string& rutaTileset) {
    Tilemap original;
    original.setRutaTileset(rutaTileset);
    original.agregarCapa("Fondo", 3, 3, false);
    original.agregarCapa("Solidos", 3, 3, true);
    original.pintar(0, 0, 0, 1);
    original.pintar(0, 2, 2, 4);
    original.pintar(1, 1, 1, 2);
    original.setCapaVisible(1, false);

    const std::string archivo = carpeta + "/tilemap.bin";
    {
        std::ofstream out(archivo, std::ios::binary);
        original.saveComponent(&out);
    }

    Tilemap cargado;
    {
        std::ifstream in(archivo, std::ios::binary);
        cargado.loadComponent(&in);
    }

    CHECK(cargado.getRutaTileset() == original.getRutaTileset(),
          "el round-trip conserva la ruta del tileset");
    CHECK(cargado.cantidadCapas() == 2, "el round-trip conserva las capas");
    CHECK(cargado.capa(1) && cargado.capa(1)->colision,
          "el round-trip conserva el flag de colision");
    CHECK(cargado.capa(1) && !cargado.capa(1)->visible,
          "el round-trip conserva la visibilidad");
    CHECK(cargado.tileEn(0, 2, 2) == 4 && cargado.tileEn(1, 1, 1) == 2,
          "el round-trip conserva los ids pintados");

    cargado.recargarTileset();
    CHECK(cargado.tilesetValido(),
          "el tilemap cargado recarga su tileset desde disco");
}

// El colisionador del tilemap deriva su forma compuesta del propio Tilemap: una
// caja por tramo horizontal de tiles solidos de las capas con colision. Se
// fusionan celdas contiguas, se ignoran los tiles no solidos y las capas sin
// colision, y al invalidar la shape se reconstruye con los tiles actuales.
void tilemapColliderTramos(const std::string& rutaTileset) {
    auto obj = GameObjectFactory::createSimpleObject(nullptr);
    obj->addComponent(std::unique_ptr<Component>(new Tilemap()));
    Tilemap* tm = obj->getComponent<Tilemap>();
    CHECK(tm != nullptr, "el objeto tiene un Tilemap");
    if (!tm) return;
    tm->setRutaTileset(rutaTileset);

    const int capaCol = tm->agregarCapa("Solidos", 4, 2, true);
    tm->agregarCapa("Decorado", 4, 2, false);
    CHECK(capaCol == 0, "se agrega la capa con colision");

    obj->addComponent(std::unique_ptr<Component>(new TilemapCollider(
        5.0f, obj->getComponent<Transform>(), obj.get())));
    TilemapCollider* col = obj->getComponent<TilemapCollider>();
    CHECK(col != nullptr && col->getOwner() == obj.get(),
          "el colisionador conoce su objeto");

    // Sin tiles solidos la shape compuesta queda vacia.
    auto* vacia = TiposShapeBullet::comoCompuesta(col->getCollisionShape());
    CHECK(vacia != nullptr, "el colisionador de tilemap arma una shape compuesta");
    CHECK(vacia && vacia->getNumChildShapes() == 0,
          "sin tiles solidos no hay cajas");

    // Fila 0: un tramo de tres (x=0..2). Fila 1: dos tramos de uno (x=0 y x=2).
    // El tile 1 no es solido y la capa sin colision no aporta aunque use el 5.
    tm->pintar(capaCol, 0, 0, 2);
    tm->pintar(capaCol, 1, 0, 2);
    tm->pintar(capaCol, 2, 0, 2);
    tm->pintar(capaCol, 0, 1, 2);
    tm->pintar(capaCol, 2, 1, 2);
    tm->pintar(capaCol, 3, 1, 1);
    tm->pintar(1, 3, 0, 5);

    col->invalidateCollisionShape();
    auto* compuesta = TiposShapeBullet::comoCompuesta(col->getCollisionShape());
    CHECK(compuesta != nullptr, "la shape del tilemap sigue siendo compuesta");
    CHECK(compuesta && compuesta->getNumChildShapes() == 3,
          "se fusionan los tramos: 3 cajas (un tramo de 3 y dos de 1)");

    if (compuesta && compuesta->getNumChildShapes() >= 1) {
        auto* caja0 = TiposShapeBullet::comoCaja(compuesta->getChildShape(0));
        CHECK(caja0 != nullptr, "cada tramo se modela con una caja");
        if (caja0) {
            const btVector3 mitad = caja0->getHalfExtentsWithMargin();
            CHECK(std::fabs(mitad.x() - 1.5f) < 0.05f,
                  "el tramo de tres tiene media anchura 1.5");
        }
    }

    // El colisionador no cambia la colision al editar tiles si la shape sigue
    // cacheada: al invalidarla se reconstruye con los tiles actuales.
    tm->pintar(capaCol, 1, 1, 2);
    auto* cacheada = TiposShapeBullet::comoCompuesta(col->getCollisionShape());
    CHECK(cacheada && cacheada->getNumChildShapes() == 3,
          "la shape cacheada no cambia hasta invalidarla");

    col->invalidateCollisionShape();
    auto* reconstruida =
        TiposShapeBullet::comoCompuesta(col->getCollisionShape());
    CHECK(reconstruida && reconstruida->getNumChildShapes() == 2,
          "al invalidar la shape se reconstruye con los tiles actuales");
}

// Las shapes de Bullet se identifican por su "shape type" y no con RTTI:
// Bullet se compila sin informacion de tipos (vcpkg lo instala con
// USE_MSVC_DISABLE_RTTI=ON) y un dynamic_cast sobre un tipo de Bullet aborta
// el proceso con un access violation en MSVC. El helper reconoce cada clase
// por su tipo proxy y rechaza las demas.
void identificacionShapesSinRtti() {
    btBoxShape caja(btVector3(1.0f, 1.0f, 1.0f));
    btCompoundShape compuesta;
    btConvexHullShape casco;
    casco.addPoint(btVector3(0.0f, 0.0f, 0.0f));
    casco.addPoint(btVector3(1.0f, 0.0f, 0.0f));
    casco.addPoint(btVector3(0.0f, 1.0f, 0.0f));
    casco.addPoint(btVector3(0.0f, 0.0f, 1.0f));

    CHECK(TiposShapeBullet::comoCaja(&caja) == &caja, "la caja se reconoce");
    CHECK(TiposShapeBullet::comoCompuesta(&caja) == nullptr,
          "la caja no se toma por compuesta");
    CHECK(TiposShapeBullet::comoCascoConvexo(&caja) == nullptr,
          "la caja no se toma por casco convexo");

    CHECK(TiposShapeBullet::comoCompuesta(&compuesta) == &compuesta,
          "la compuesta se reconoce");
    CHECK(TiposShapeBullet::comoCaja(&compuesta) == nullptr,
          "la compuesta no se toma por caja");
    CHECK(TiposShapeBullet::comoCascoConvexo(&compuesta) == nullptr,
          "la compuesta no se toma por casco convexo");

    CHECK(TiposShapeBullet::comoCascoConvexo(&casco) == &casco,
          "el casco convexo se reconoce");
    CHECK(TiposShapeBullet::comoCaja(&casco) == nullptr,
          "el casco convexo no se toma por caja");
    CHECK(TiposShapeBullet::comoCompuesta(&casco) == nullptr,
          "el casco convexo no se toma por compuesta");

    CHECK(TiposShapeBullet::comoCaja(nullptr) == nullptr,
          "una shape nula no se toma por caja");
    CHECK(TiposShapeBullet::comoCompuesta(nullptr) == nullptr,
          "una shape nula no se toma por compuesta");
    CHECK(TiposShapeBullet::comoCascoConvexo(nullptr) == nullptr,
          "una shape nula no se toma por casco convexo");
}
} // namespace

int main() {
    TempPruebas::CarpetaPrueba carpeta("funshi_tilemap_tests");
    const std::string base = carpeta.ruta().string();

    const std::string rutaTileset = base + "/tileset.json";
    escribirTileset(rutaTileset);

    identificacionShapesSinRtti();
    tilesetValidoYMetadatos(rutaTileset);
    tilesetErrores(base);
    tilemapCapasYPintado(rutaTileset);
    tilemapGeometria(rutaTileset);
    tilemapSerializacion(base, rutaTileset);
    tilemapColliderTramos(rutaTileset);

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
