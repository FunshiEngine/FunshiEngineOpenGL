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
// Pruebas headless del nucleo del manifiesto de assets de la escena
// (ManifiestoAssetsCore): lectura/escritura del JSON (SceneAssets.json),
// relativizacion/absolutizacion de rutas contra la raiz de assets y la regla
// de precedencia (path vacio no pisa; path distinto aplica). El adaptador
// que recorre GameObjects (ManifiestoAssets) vive en el engine y verifica su
// logica de componente por build/integracion, como RutasReescritura.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Scenes/ManifiestoAssetsCore.h"

namespace fs = std::filesystem;

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

EntradaAssets crearEntrada(const std::string& malla, const std::string& t0,
                           const std::string& t1, const std::string& t2,
                           const std::string& t3, const std::string& script) {
    EntradaAssets e;
    e.malla = malla;
    e.texturas[0] = t0;
    e.texturas[1] = t1;
    e.texturas[2] = t2;
    e.texturas[3] = t3;
    e.script = script;
    return e;
}
} // namespace

int main() {
    // Carpeta temporal unica por proceso: crea y se limpia al salir (RAII).
    TempPruebas::CarpetaPrueba carpetaBase("funshi_manifiesto_tests");
    const fs::path base = carpetaBase.ruta();

    // 1. Round-trip completo de escribir/leer (paths con acentos y vacios).
    {
        const fs::path archivo = base / "completo.json";
        std::map<int, EntradaAssets> entradas;
        entradas[3] = crearEntrada("Modelos/Nave/nave.fbx",
                                   "Texturas/nave_de_la_calista.png",
                                   "Texturas/spec.png", "", "Texturas/e.png",
                                   "Scripts/mi_script.dll");
        entradas[7] = crearEntrada("", "", "", "Texturas/solo_normal.png", "",
                                   "");
        CHECK(ManifiestoAssetsCore::escribirArchivo(archivo.string(), entradas),
              "escribirArchivo escribe el manifiesto completo");

        std::map<int, EntradaAssets> leidas;
        CHECK(ManifiestoAssetsCore::leerArchivo(archivo.string(), leidas),
              "leerArchivo lee el manifiesto escrito");
        CHECK(leidas.size() == 2,
              "se leen las dos entradas del manifiesto");
        CHECK(leidas[3].malla == "Modelos/Nave/nave.fbx",
              "round-trip de la malla");
        CHECK(leidas[3].texturas[0] == "Texturas/nave_de_la_calista.png",
              "round-trip de la textura con acentos");
        CHECK(leidas[3].texturas[1] == "Texturas/spec.png",
              "round-trip de la textura specular");
        CHECK(leidas[3].texturas[2].empty(),
              "slot de textura vacio sigue vacio");
        CHECK(leidas[3].script == "Scripts/mi_script.dll",
              "round-trip del script");
        // Entrada sin malla ni script: solo la textura normal.
        CHECK(leidas[7].malla.empty() && leidas[7].script.empty(),
              "solo aporta los campos que tiene");
        CHECK(leidas[7].texturas[2] == "Texturas/solo_normal.png",
              "la textura del slot 2 se conserva");
    }

    // 2. Archivo inexistente: leer devuelve false sin tocar el mapa.
    {
        const fs::path archivo = base / "no_existe.json";
        std::map<int, EntradaAssets> entradas;
        entradas[1] = crearEntrada("vieja", "", "", "", "", "viejo");
        CHECK(!ManifiestoAssetsCore::leerArchivo(archivo.string(), entradas),
              "leerArchivo devuelve false con archivo inexistente");
        CHECK(entradas.size() == 1 && entradas[1].malla == "vieja" &&
                  entradas[1].script == "viejo",
              "el mapa queda intacto con archivo inexistente");
    }

    // 3. JSON corrupto: no lanza y devuelve false.
    {
        const fs::path archivo = base / "corrupto.json";
        {
            std::ofstream out(archivo);
            out << "{ esto no es json valido [[[";
        }
        std::map<int, EntradaAssets> entradas;
        CHECK(!ManifiestoAssetsCore::leerArchivo(archivo.string(), entradas),
              "leerArchivo tolera un JSON corrupto sin lanzar");
        CHECK(entradas.empty(),
              "mapa vacio tras manifiesto corrupto");
    }

    // 4. Entrada con id no numerico: se descarta sin romper la lectura.
    {
        const fs::path archivo = base / "id_raro.json";
        {
            std::ofstream out(archivo);
            out << "{\"version\":1,\"assets\":{\"abc\":{\"malla\":\"x.fbx\"},"
                   "\"9\":{\"malla\":\"y.fbx\"}}}";
        }
        std::map<int, EntradaAssets> entradas;
        CHECK(ManifiestoAssetsCore::leerArchivo(archivo.string(), entradas),
              "leerArchivo procesa el manifiesto con ids invalidos");
        CHECK(entradas.size() == 1 && entradas[9].malla == "y.fbx",
              "el id no numerico se descarta y el valido se conserva");
    }

    // 5. Relativizar/absolutizar contra la raiz de assets fijada.
    {
        const std::string raiz = "/motor/MotorGrafico/MiJuego/srcMiJuego";
        EditorConfig::fijarRaizAssets(raiz);

        EntradaAssets entrada =
            crearEntrada(raiz + "/Modelos/Nave/nave.fbx",
                         raiz + "/Texturas/dif.png", "", "", "",
                         raiz + "/Scripts/mi_script.dll");
        ManifiestoAssetsCore::relativizarEntrada(entrada);
        CHECK(entrada.malla == "Modelos/Nave/nave.fbx",
              "relativizarEntrada deja la malla relativa");
        CHECK(entrada.texturas[0] == "Texturas/dif.png",
              "relativizarEntrada deja la textura relativa");
        CHECK(entrada.script == "Scripts/mi_script.dll",
              "relativizarEntrada deja el script relativo");

        ManifiestoAssetsCore::absolutizarEntrada(entrada);
        CHECK(entrada.malla == raiz + "/Modelos/Nave/nave.fbx",
              "absolutizarEntrada resuelve la malla a absoluta");
        CHECK(entrada.texturas[0] == raiz + "/Texturas/dif.png",
              "absolutizarEntrada resuelve la textura a absoluta");
        CHECK(entrada.script == raiz + "/Scripts/mi_script.dll",
              "absolutizarEntrada resuelve el script a absoluto");

        EditorConfig::limpiarRaizAssets();
    }

    // 6. Path absoluto legacy (formato viejo): absolutizar lo conserva (la
    //    heuristica no lo reconvierte contra una raiz distinta).
    {
        const std::string raiz = "/motor/MotorGrafico/Otro/srcOtro";
        EditorConfig::fijarRaizAssets(raiz);
        EntradaAssets entrada = crearEntrada("/legacy/Absoluto/cubo.obj", "",
                                             "", "", "", "");
        ManifiestoAssetsCore::absolutizarEntrada(entrada);
        CHECK(entrada.malla == "/legacy/Absoluto/cubo.obj",
              "absolutizarEntrada conserva un path absoluto legacy");
        ManifiestoAssetsCore::relativizarEntrada(entrada);
        CHECK(entrada.malla == "/legacy/Absoluto/cubo.obj",
              "relativizarEntrada no toca un path fuera de la raiz");
        EditorConfig::limpiarRaizAssets();
    }

    // 7. Resolucion de precedencia: vacio no pisa; igual no aplica; distinto
    //    aplica.
    {
        const std::string vigente = "/ruta/Vigente.fbx";
        const std::string igual =
            ManifiestoAssetsCore::resolverRuta(vigente, vigente);
        CHECK(igual == vigente,
              "path persistido igual al vigente no obliga a recargar");

        const std::string vacio =
            ManifiestoAssetsCore::resolverRuta("", vigente);
        CHECK(vacio == vigente,
              "path persistido vacio no pisa al vigente (.db)");

        const std::string distinto =
            ManifiestoAssetsCore::resolverRuta("/ruta/Nuevo.fbx", vigente);
        CHECK(distinto == "/ruta/Nuevo.fbx",
              "path persistido distinto y no vacio aplica (precedencia)");
    }

    // 8. entradaVacia.
    {
        CHECK(ManifiestoAssetsCore::entradaVacia(EntradaAssets()),
              "entrada con todos los campos vacios se considera vacia");
        CHECK(!ManifiestoAssetsCore::entradaVacia(
                  crearEntrada("m.fbx", "", "", "", "", "")),
              "una malla sola ya no es entrada vacia");
    }

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << (total - fallos)
              << '/' << total << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}