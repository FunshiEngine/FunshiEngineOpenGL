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
// Pruebas headless de la importacion de archivos que vienen de fuera del
// proyecto: lo que el usuario arrastra desde el explorador del sistema sobre
// la ventana del editor y lo que pega con Ctrl+V desde el portapapeles.
//
// Sin pila grafica: solo std C++17. Se ejecutan contra carpetas temporales
// unicas por proceso (TempPruebas::CarpetaPrueba) y se corren con ctest.
//
// Casos:
//   - Pegar un archivo copia su contenido intacto en la carpeta destino.
//   - Pegar un archivo con nombre repetido NO pisa el existente: se genera un
//     nombre unico ("a.txt" -> "a (1).txt") y el original conserva su
//     contenido.
//   - Pegar una carpeta copia su contenido recursivamente.
//   - Pegar una ruta inexistente devuelve el error y no aborta el resto.
//   - Nombre unico de la fachada: los sufijos "(1)", "(2)" se suceden.
//   - Parser de text/uri-list: acepta rutas con espacios y no-ASCII, y
//     descarta las lineas que no son URIs de archivo.
//   - Parser de DROPFILES (formato de Windows): devuelve las rutas en el
//     orden del HDROP, en ANSI y en UTF-16, y convierte el UTF-16 a UTF-8.
//   - Copia de un archivo binario: el contenido llega igual (byte a byte).

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/FileManager/FileManager.h"
#include "../FunshiEngineGL/src/FileManager/PortapapelesParsers.h"
#include "../FunshiEngineGL/src/GUI/FileManagerGUI/ImportarArchivos.h"

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

std::string contenidoDe(const fs::path& archivo) {
    std::ifstream f(archivo, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

std::string unir(const fs::path& base, const std::string& resto) {
    return (base / resto).string();
}

// Construye un bloque DROPFILES de Windows como el que entrega el portapapeles
// (CF_HDROP): cabecera con el desplazamiento de la lista y la lista de rutas
// terminada en doble null. La lista es ANSI (fWide = 0).
std::vector<unsigned char> construirDropFilesAnsi(
    const std::vector<std::string>& rutas) {
    std::vector<unsigned char> cabecera(20, 0);
    const std::uint32_t pFiles = 20;
    cabecera[0] = static_cast<unsigned char>(pFiles & 0xFF);
    cabecera[1] = static_cast<unsigned char>((pFiles >> 8) & 0xFF);
    cabecera[2] = static_cast<unsigned char>((pFiles >> 16) & 0xFF);
    cabecera[3] = static_cast<unsigned char>((pFiles >> 24) & 0xFF);

    std::vector<unsigned char> lista;
    for (const std::string& ruta : rutas) {
        for (char c : ruta) lista.push_back(static_cast<unsigned char>(c));
        lista.push_back(0);
    }
    lista.push_back(0);  // terminador doble: cierra la lista de rutas

    std::vector<unsigned char> bloque = cabecera;
    bloque.insert(bloque.end(), lista.begin(), lista.end());
    return bloque;
}

// El mismo bloque, pero con la lista en UTF-16 (fWide = 1): es lo que
// entrega Windows cuando el portapapeles tiene rutas con caracteres fuera
// del ASCII.
std::vector<unsigned char> construirDropFilesUtf16(
    const std::vector<std::u16string>& rutas) {
    std::vector<unsigned char> cabecera(20, 0);
    const std::uint32_t pFiles = 20;
    cabecera[0] = static_cast<unsigned char>(pFiles & 0xFF);
    cabecera[1] = static_cast<unsigned char>((pFiles >> 8) & 0xFF);
    cabecera[2] = static_cast<unsigned char>((pFiles >> 16) & 0xFF);
    cabecera[3] = static_cast<unsigned char>((pFiles >> 24) & 0xFF);
    cabecera[16] = 1;  // fWide

    std::vector<unsigned char> lista;
    for (const std::u16string& ruta : rutas) {
        for (char16_t c : ruta) {
            lista.push_back(static_cast<unsigned char>(c & 0xFF));
            lista.push_back(static_cast<unsigned char>((c >> 8) & 0xFF));
        }
        lista.push_back(0);
        lista.push_back(0);
    }
    lista.push_back(0);
    lista.push_back(0);

    std::vector<unsigned char> bloque = cabecera;
    bloque.insert(bloque.end(), lista.begin(), lista.end());
    return bloque;
}

}  // namespace

int main() {
    TempPruebas::CarpetaPrueba carpetaBase("funshi_importar_archivos_tests");
    const fs::path base = carpetaBase.ruta();

    // --- Proyecto sintetico ------------------------------------------------
    const fs::path proy = base / "proyecto";
    fs::create_directories(proy / "Assets" / "Entradas");
    FileManager fm(proy.string());

    const std::string destino = unir(proy, "Assets/Entradas");

    // --- Origenes fuera del proyecto ---------------------------------------
    const fs::path fuera = base / "fuera";
    fs::create_directories(fuera / "carpetaOrigen" / "anidada");
    { std::ofstream f(fuera / "notas.txt"); f << "contenido original"; }
    { std::ofstream f(fuera / "carpetaOrigen" / "dentro.txt"); f << "dentro"; }
    { std::ofstream f(fuera / "carpetaOrigen" / "anidada" / "profundo.txt");
      f << "profundo"; }
    { std::ofstream f(fuera / "binario.dat", std::ios::binary);
      const unsigned char datos[] = {0x00, 0x01, 0xFF, 0x0A, 0x7F};
      f.write(reinterpret_cast<const char*>(datos), sizeof(datos)); }

    // --- Pegar un archivo: copia el contenido intacto -----------------------
    {
        const ImportarArchivos::Resultado resultado = ImportarArchivos::pegar(
            &fm, destino, {unir(fuera, "notas.txt")});
        CHECK(resultado.copiados == 1, "se copia un archivo");
        CHECK(resultado.errores.empty(), "sin errores al copiar un archivo");
        CHECK(fs::exists(unir(destino, "notas.txt")),
              "el archivo aparece en la carpeta destino");
        CHECK(contenidoDe(unir(destino, "notas.txt")) == "contenido original",
              "el contenido del archivo copiado es identico al original");
    }

    // --- Pegar un archivo binario: llega igual, byte a byte ----------------
    {
        const ImportarArchivos::Resultado resultado = ImportarArchivos::pegar(
            &fm, destino, {unir(fuera, "binario.dat")});
        CHECK(resultado.copiados == 1, "se copia un archivo binario");
        CHECK(contenidoDe(unir(destino, "binario.dat")).size() == 5,
              "el binario conserva su tamano");
    }

    // --- Pegar un archivo con nombre repetido: NO pisa ----------------------
    {
        const ImportarArchivos::Resultado resultado = ImportarArchivos::pegar(
            &fm, destino, {unir(fuera, "notas.txt")});
        CHECK(resultado.copiados == 1, "el segundo pegado tambien copia");
        CHECK(resultado.errores.empty(), "el segundo pegado no da error");
        CHECK(contenidoDe(unir(destino, "notas.txt")) == "contenido original",
              "el archivo existente NO se pisa");
        CHECK(fs::exists(unir(destino, "notas (1).txt")),
              "el repetido recibe un nombre unico");
        CHECK(contenidoDe(unir(destino, "notas (1).txt")) == "contenido original",
              "la copia con nombre unico tiene el contenido del original");
    }

    // --- Nombre unico de la fachada: los sufijos se suceden ----------------
    {
        CHECK(FileManager::nombreUnico(destino, "notas.txt") == "notas (2).txt",
              "el siguiente nombre unico continua la serie");
        CHECK(FileManager::nombreUnico(destino, "nuevo.txt") == "nuevo.txt",
              "un nombre libre se queda como esta");
        CHECK(FileManager::nombreUnico(destino, "") == "",
              "un nombre vacio se devuelve tal cual");
    }

    // --- Pegar una carpeta: copia su contenido recursivamente ---------------
    {
        const ImportarArchivos::Resultado resultado = ImportarArchivos::pegar(
            &fm, destino, {unir(fuera, "carpetaOrigen")});
        CHECK(resultado.copiados == 1, "se copia una carpeta");
        CHECK(resultado.carpetasCopiadas == 1,
              "la copia se cuenta como carpeta (aparece en el arbol)");
        CHECK(fs::is_directory(unir(destino, "carpetaOrigen")),
              "la carpeta aparece en la destino");
        CHECK(contenidoDe(unir(destino, "carpetaOrigen/dentro.txt")) == "dentro",
              "el contenido directo de la carpeta se copia");
        CHECK(contenidoDe(unir(destino, "carpetaOrigen/anidada/profundo.txt")) ==
                  "profundo",
              "el contenido anidado se copia tambien");
    }

    // --- Una ruta inexistente: error y el resto sigue -----------------------
    {
        const ImportarArchivos::Resultado resultado = ImportarArchivos::pegar(
            &fm, destino,
            {unir(fuera, "noExiste.txt"), unir(fuera, "notas.txt")});
        CHECK(resultado.copiados == 1,
              "la ruta inexistente no aborta la copia del resto");
        CHECK(resultado.errores.size() == 1,
              "la ruta inexistente genera un error");
        CHECK(resultado.errores.front().find("noExiste.txt") != std::string::npos,
              "el error nombra la ruta que fallo");
    }

    // --- Destino vacio o gestor ausente: no se copia nada -------------------
    {
        const ImportarArchivos::Resultado sinDestino =
            ImportarArchivos::pegar(&fm, "", {unir(fuera, "notas.txt")});
        CHECK(sinDestino.copiados == 0, "sin destino no se copia nada");
        CHECK(!sinDestino.errores.empty(), "sin destino hay error");

        const ImportarArchivos::Resultado sinGestor =
            ImportarArchivos::pegar(nullptr, destino, {unir(fuera, "notas.txt")});
        CHECK(sinGestor.copiados == 0, "sin gestor no se copia nada");
        CHECK(!sinGestor.errores.empty(), "sin gestor hay error");
    }

    // --- Parser de text/uri-list (Linux) ------------------------------------
    {
        const std::string texto =
            "# comentario\n"
            "file:///home/usuario/documento%20con%20espacios.txt\n"
            "file:///home/usuario/acentuado.txt\n"
            "http://ejemplo.com/no/es/un/archivo\n"
            "texto suelto que no es una URI\n";
        const std::vector<std::string> rutas =
            PortapapelesParsers::parsearUriList(texto);
        CHECK(rutas.size() == 2, "el uri-list devuelve solo las URIs de archivo");
        CHECK(!rutas.empty() && rutas[0] == "/home/usuario/documento con espacios.txt",
              "los %20 de la URI se decodifican a espacios");
        CHECK(rutas.size() == 2 && rutas[1] == "/home/usuario/acentuado.txt",
              "la segunda URI se lee tal cual");

        // Una URI con la letra de unidad (Windows) no lleva la barra inicial.
        const std::vector<std::string> unidad =
            PortapapelesParsers::parsearUriList("file:///C:/Users/x/a.txt");
        CHECK(unidad.size() == 1 && unidad[0] == "C:/Users/x/a.txt",
              "la URI de Windows se convierte en ruta con letra de unidad");

        // Un host remoto no es un archivo local: no se puede copiar.
        const std::vector<std::string> remoto =
            PortapapelesParsers::parsearUriList("file://servidor/compartido/a.txt");
        CHECK(remoto.empty(), "la URI de un host remoto se descarta");

        CHECK(PortapapelesParsers::parsearUriList("").empty(),
              "un uri-list vacio no devuelve rutas");
    }

    // --- Parser de rutas planas (Linux, exploradores que copian la ruta) ----
    {
        const std::string texto =
            "/home/usuario/a.txt\n"
            "relativa/no/es/absoluta.txt\n"
            "\n"
            "/home/usuario/b.txt\r\n";
        const std::vector<std::string> rutas =
            PortapapelesParsers::parsearRutasSeparadas(texto);
        CHECK(rutas.size() == 2, "solo se aceptan las rutas absolutas");
        CHECK(!rutas.empty() && rutas[0] == "/home/usuario/a.txt",
              "la primera ruta absoluta se lee");
        CHECK(rutas.size() == 2 && rutas[1] == "/home/usuario/b.txt",
              "el CRLF no deja el retorno de carro pegado a la ruta");
    }

    // --- Parser de DROPFILES (Windows) --------------------------------------
    {
        const std::vector<std::string> esperadas = {
            "C:\\Users\\x\\primero.txt", "C:\\Users\\x\\segundo.txt"};
        const std::vector<unsigned char> bloque =
            construirDropFilesAnsi(esperadas);
        const std::vector<std::string> rutas =
            PortapapelesParsers::parsearDropFiles(bloque.data(), bloque.size());
        CHECK(rutas.size() == 2, "el bloque DROPFILES devuelve las dos rutas");
        CHECK(rutas.size() == 2 && rutas[0] == esperadas[0],
              "las rutas conservan el orden del HDROP");
        CHECK(rutas.size() == 2 && rutas[1] == esperadas[1],
              "la ultima ruta tambien se lee");

        // UTF-16: las rutas vienen en UTF-16LE y el motor trabaja en UTF-8.
        // Los bytes esperados se escriben a mano: la codificacion de
        // ejecucion de un literal estrecho no es UTF-8 en todos los
        // compiladores.
        const std::string acentoUTF8 = "\xC3\xA1";  // 'a' con tilde
        const std::vector<std::u16string> conAcento = {
            u"C:\\Users\\x\\acentu\u00e1.txt"};
        const std::vector<unsigned char> bloqueUtf16 =
            construirDropFilesUtf16(conAcento);
        const std::vector<std::string> rutasUtf16 =
            PortapapelesParsers::parsearDropFiles(bloqueUtf16.data(),
                                                  bloqueUtf16.size());
        CHECK(rutasUtf16.size() == 1,
              "el bloque DROPFILES en UTF-16 devuelve la ruta");
        CHECK(rutasUtf16.size() == 1 &&
                  rutasUtf16[0] == "C:\\Users\\x\\acentu" + acentoUTF8 + ".txt",
              "la ruta UTF-16 se convierte a UTF-8 con el acento intacto");

        // Un bloque truncado o vacio no revienta: devuelve la lista vacia.
        CHECK(PortapapelesParsers::parsearDropFiles(nullptr, 0).empty(),
              "un bloque nulo no devuelve rutas");
        const std::vector<unsigned char> corto = {1, 2, 3};
        CHECK(PortapapelesParsers::parsearDropFiles(corto.data(), corto.size())
                  .empty(),
              "un bloque mas corto que la cabecera no devuelve rutas");
    }

    // --- Conversion de UTF-16 a UTF-8 ---------------------------------------
    {
        const std::string acentoUTF8 = "\xC3\xA1";  // 'a' con tilde
        CHECK(PortapapelesParsers::utf16aUtf8(u"ascii") == "ascii",
              "el ASCII se mantiene");
        CHECK(PortapapelesParsers::utf16aUtf8(u"acentu\u00e1") ==
                  "acentu" + acentoUTF8,
              "un caracter del plano BMP se codifica en UTF-8");
        // Emoji: un par surrogate (U+1F600).
        CHECK(PortapapelesParsers::utf16aUtf8(u"\U0001F600").size() == 4,
              "un par surrogate se codifica en cuatro bytes UTF-8");
    }

    // --- Resultado ----------------------------------------------------------
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "IMPORTAR ARCHIVOS TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
