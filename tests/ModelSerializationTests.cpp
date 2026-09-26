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
// Pruebas headless de la serializacion binaria del componente Model
// (prefijo size_t con la longitud + bytes del path).
//
// Regresion del core al cargar escenas: el escritor guarda el path completo y
// el lector debe consumir del stream exactamente esa cantidad. Si recorta la
// lectura al tamaño de su buffer sin descartar el excedente, los bytes
// sobrantes desalinean los componentes siguientes y la carga termina pidiendo
// std::string con una longitud basura (std::length_error -> abort).
// Caso real: paths de 117 bytes con el buffer de lectura de 100.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Model.h"

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

// Escribe un payload crudo (size_t longitud + bytes): permite simular los
// archivos que dejaria otra version del motor.
void escribirPayloadCrudo(std::ofstream& out, const std::string& texto) {
    const std::size_t len = texto.size();
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(texto.data(), static_cast<std::streamsize>(len));
}

void escribirMarcador(std::ofstream& out, const std::string& texto) {
    escribirPayloadCrudo(out, texto);
}

// Lee un payload crudo; si el stream quedo desalineado devuelve un texto
// centinela para que las comparaciones fallen.
std::string leerPayloadCrudo(std::ifstream& in) {
    std::size_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!in || len > 4096)
        return "<payload ilegible: stream desalineado>";
    std::string texto(len, '\0');
    if (len > 0)
        in.read(&texto[0], static_cast<std::streamsize>(len));
    return texto;
}
} // namespace

int main() {
    // Carpeta temporal unica por proceso: crea y se limpia al salir (RAII).
    TempPruebas::CarpetaPrueba carpetaBase("funshi_model_tests");
    const fs::path base = carpetaBase.ruta();

    const std::string marcador = "SIGUIENTE_COMPONENTE";

    // 1. Round-trip de un path corto.
    {
        const fs::path archivo = base / "corto.bin";
        const std::string path = "/proyecto/modelos/cubo.obj";
        {
            Model modelo;
            modelo.setPath(path);
            std::ofstream out(archivo, std::ios::binary);
            modelo.saveComponent(&out);
            escribirMarcador(out, marcador);
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(modelo.getPath() == path, "path corto round-trip");
        CHECK(leerPayloadCrudo(in) == marcador,
              "stream alineado tras path corto");
    }

    // 2. Regresion: path de 117 bytes (el real de la escena que abortaba).
    {
        const fs::path archivo = base / "117bytes.bin";
        const std::string path =
            "/home/ggmaster/Projects/FunshiEngineGL/FunshiEngineGL/build/"
            "MotorGrafico/NuevoProyecto/srcNuevoProyecto/Personaje.obj";
        CHECK(path.size() == 117, "el path de regresion mide 117 bytes");
        {
            Model modelo;
            modelo.setPath(path);
            std::ofstream out(archivo, std::ios::binary);
            modelo.saveComponent(&out);
            escribirMarcador(out, marcador);
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(modelo.getPath() == path, "path de 117 bytes round-trip");
        CHECK(leerPayloadCrudo(in) == marcador,
              "stream alineado tras path de 117 bytes");
    }

    // 3. Path mas largo que el buffer: se conserva lo que entra (4095) y el
    //    stream queda alineado.
    {
        const fs::path archivo = base / "gigante.bin";
        const std::string path(5000, 'a');
        {
            Model modelo;
            modelo.setPath(path);
            std::ofstream out(archivo, std::ios::binary);
            modelo.saveComponent(&out);
            escribirMarcador(out, marcador);
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(modelo.getPath().size() == 4095,
              "path de 5000 bytes recortado a 4095");
        CHECK(leerPayloadCrudo(in) == marcador,
              "stream alineado tras path de 5000 bytes");
    }

    // 4. Payload crudo con longitud mayor que el buffer (archivo de otra
    //    version): el excedente se descarta sin desalinear.
    {
        const fs::path archivo = base / "crudo.bin";
        const std::string path(5000, 'b');
        {
            std::ofstream out(archivo, std::ios::binary);
            escribirPayloadCrudo(out, path);
            escribirMarcador(out, marcador);
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(modelo.getPath().size() == 4095,
              "longitud > buffer: se conserva lo que entra");
        CHECK(leerPayloadCrudo(in) == marcador,
              "longitud > buffer: stream alineado");
    }

    // 5. Path vacio.
    {
        const fs::path archivo = base / "vacio.bin";
        {
            Model modelo;
            modelo.setPath("");
            std::ofstream out(archivo, std::ios::binary);
            modelo.saveComponent(&out);
            escribirMarcador(out, marcador);
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(modelo.getPath().empty(), "path vacio round-trip");
        CHECK(leerPayloadCrudo(in) == marcador,
              "stream alineado tras path vacio");
    }

    // 6. Archivo truncado a la mitad del path: no debe lanzar excepciones.
    {
        const fs::path archivo = base / "truncado.bin";
        {
            std::ofstream out(archivo, std::ios::binary);
            const std::size_t len = 200;
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            const std::string mitad(40, 'c');
            out.write(mitad.data(), static_cast<std::streamsize>(mitad.size()));
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(true, "archivo truncado no lanza excepciones");
    }

    // 7. Serializacion portable con raiz de assets fijada (src<nombre>): el
    //    path bajo esa raiz se PERSISTE relativo (la escena sigue valida al
    //    mover/renombrar el proyecto) y al cargar se resuelve a absoluto.
    {
        const std::string raiz = "/motor/MotorGrafico/MiJuego/srcMiJuego";
        EditorConfig::fijarRaizAssets(raiz);

        const fs::path archivo = base / "relativa.bin";
        const std::string abs = raiz + "/Modelos/Nave/nave.fbx";
        {
            Model modelo;
            modelo.setPath(abs);
            std::ofstream out(archivo, std::ios::binary);
            modelo.saveComponent(&out);
            escribirMarcador(out, marcador);
        }
        // Lo que quedo en disco es la ruta RELATIVA (no la absoluta original).
        {
            std::ifstream formatoOut(archivo, std::ios::binary);
            CHECK(leerPayloadCrudo(formatoOut) == "Modelos/Nave/nave.fbx",
                  "se persiste la ruta relativa a la raiz de assets");
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(modelo.getPath() == abs,
              "la relativa se resuelve a absoluta al cargar");
        CHECK(leerPayloadCrudo(in) == marcador,
              "stream alineado tras path relativo");

        EditorConfig::limpiarRaizAssets();
    }

    // 8. Escena legacy: un path ABSOLUTO persistido (formato viejo) se carga
    //    intacto aunque haya raiz de assets fijada.
    {
        const std::string raiz = "/motor/MotorGrafico/Otro/srcOtro";
        EditorConfig::fijarRaizAssets(raiz);
        const fs::path archivo = base / "legacy_abs.bin";
        const std::string absoluto = "/legacy/Absoluto/cubo.obj";
        {
            std::ofstream out(archivo, std::ios::binary);
            escribirPayloadCrudo(out, absoluto);
        }
        Model modelo;
        std::ifstream in(archivo, std::ios::binary);
        modelo.loadComponent(&in);
        CHECK(modelo.getPath() == absoluto,
              "path absoluto legacy se conserva tal cual");
        EditorConfig::limpiarRaizAssets();
    }

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << (total - fallos)
              << '/' << total << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}

