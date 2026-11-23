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
#ifndef PORTAPAPELESPARSERS_H
#define PORTAPAPELESPARSERS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Lectores de los formatos en que el sistema operativo publica archivos en el
// portapapeles. Son funciones PURAS (entra una cadena o un bloque de bytes y
// sale una lista de rutas): por eso se pueden probar en headless y en las dos
// plataformas, sin portapapeles y sin escritorio.
//
// Ningun parser decide que hacer con las rutas: solo las traduce del formato
// del sistema a una lista de rutas absolutas. La politica (no pisar, avisar,
// invalidar la vista) vive en el explorador.

// Formatos:
//   - text/uri-list (Linux y convencion XDG): una URI por linea.
//   - Rutas planas separadas por saltos de linea (algunos exploradores de
//     Linux copian la ruta cruda en vez de la URI).
//   - DROPFILES (Windows, CF_HDROP): una cabecera con el desplazamiento de la
//     lista y la lista de rutas en ANSI o UTF-16 terminada en doble null.
namespace PortapapelesParsers {

// Decodifica las secuencias %XX de una URI. Un % que no va seguido de dos
// digitos hexadecimales se deja tal cual: una ruta con un % literal es
// valida en un archivo.
inline std::string decodificarPorcentajes(const std::string& texto) {
    auto digito = [](char c, int& valor) {
        if (c >= '0' && c <= '9') { valor = c - '0'; return true; }
        if (c >= 'a' && c <= 'f') { valor = c - 'a' + 10; return true; }
        if (c >= 'A' && c <= 'F') { valor = c - 'A' + 10; return true; }
        return false;
    };

    std::string salida;
    salida.reserve(texto.size());
    for (std::size_t i = 0; i < texto.size(); ++i) {
        int alto = 0;
        int bajo = 0;
        if (texto[i] == '%' && i + 2 < texto.size() &&
            digito(texto[i + 1], alto) && digito(texto[i + 2], bajo)) {
            salida.push_back(static_cast<char>((alto << 4) | bajo));
            i += 2;
            continue;
        }
        salida.push_back(texto[i]);
    }
    return salida;
}

// Convierte una cadena UTF-16 (la que usa el portapapeles de Windows) a
// UTF-8, que es la codificacion con la que el motor trabaja las rutas.
inline std::string utf16aUtf8(const std::u16string& entrada) {
    std::string salida;
    salida.reserve(entrada.size());
    for (std::size_t i = 0; i < entrada.size(); ++i) {
        std::uint32_t cp = static_cast<std::uint32_t>(entrada[i]);
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            const std::uint32_t bajo =
                (i + 1 < entrada.size())
                    ? static_cast<std::uint32_t>(entrada[i + 1])
                    : 0u;
            if (bajo >= 0xDC00 && bajo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (bajo - 0xDC00);
                ++i;
            } else {
                cp = 0xFFFD;  // sustituto: surrogote suelto
            }
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            cp = 0xFFFD;  // surrogote bajo sin su alto
        }

        if (cp < 0x80) {
            salida.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            salida.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            salida.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            salida.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            salida.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            salida.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            salida.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            salida.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            salida.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            salida.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return salida;
}

// Quita los espacios del principio y del final.
inline std::string recortar(const std::string& texto) {
    std::size_t inicio = 0;
    while (inicio < texto.size() &&
           (texto[inicio] == ' ' || texto[inicio] == '\t')) {
        ++inicio;
    }
    std::size_t fin = texto.size();
    while (fin > inicio &&
           (texto[fin - 1] == ' ' || texto[fin - 1] == '\t')) {
        --fin;
    }
    return texto.substr(inicio, fin - inicio);
}

// Convierte una URI file:// en una ruta del sistema.
//
// "file:///home/x/a.txt"      -> "/home/x/a.txt"
// "file:///C:/x/a.txt"        -> "C:/x/a.txt"   (la barra inicial del UNC
//                                                 local no es parte de la ruta)
// "file://localhost/x/a.txt"  -> "/x/a.txt"
// "file://otrohost/x/a.txt"   -> ""  (es un recurso remoto: no se puede copiar)
inline std::string rutaDesdeUri(const std::string& uri) {
    if (uri.rfind("file://", 0) != 0) return std::string();

    std::string resto = uri.substr(7);  // quita "file://"
    if (resto.rfind("/", 0) != 0) {
        // Hay una autoridad ("file://host/..."): solo se admite el host local.
        const std::size_t barra = resto.find('/');
        if (barra == std::string::npos) return std::string();
        const std::string host = resto.substr(0, barra);
        if (!host.empty() && host != "localhost") return std::string();
        resto = resto.substr(barra);
    }

    std::string ruta = decodificarPorcentajes(resto);
    // "file:///C:/x" deja una barra delante de la letra de unidad: en Windows
    // esa barra no pertenece a la ruta.
    if (ruta.size() >= 3 && ruta[0] == '/' && ruta[2] == ':' &&
        ((ruta[1] >= 'A' && ruta[1] <= 'Z') || (ruta[1] >= 'a' && ruta[1] <= 'z'))) {
        ruta = ruta.substr(1);
    }
    return ruta;
}

// Lee un text/uri-list: una URI por linea. Las lineas que no son URIs de
// archivo (comentarios, http, texto suelto) se descartan.
inline std::vector<std::string> parsearUriList(const std::string& texto) {
    std::vector<std::string> rutas;
    std::size_t inicio = 0;
    while (inicio <= texto.size()) {
        std::size_t fin = texto.find('\n', inicio);
        std::string linea =
            texto.substr(inicio, fin == std::string::npos ? std::string::npos
                                                          : fin - inicio);
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        linea = recortar(linea);
        if (!linea.empty() && linea.rfind("file://", 0) == 0) {
            const std::string ruta = rutaDesdeUri(linea);
            if (!ruta.empty()) rutas.push_back(ruta);
        }
        if (fin == std::string::npos) break;
        inicio = fin + 1;
    }
    return rutas;
}

// Dice si una ruta es absoluta en esta plataforma: "/" al principio (POSIX),
// "X:\" o "X:/" (Windows) o "\\" (UNC de Windows).
inline bool esRutaAbsoluta(const std::string& ruta) {
    if (ruta.empty()) return false;
    if (ruta[0] == '/' || ruta[0] == '\\') return true;
    if (ruta.size() >= 3 && ruta[1] == ':' &&
        (ruta[2] == '\\' || ruta[2] == '/')) {
        return true;
    }
    return false;
}

// Lee una lista de rutas separadas por saltos de linea (una por linea). Solo
// se aceptan rutas absolutas: una relativa no se puede copiar sin saber el
// directorio de trabajo del otro proceso.
inline std::vector<std::string> parsearRutasSeparadas(const std::string& texto) {
    std::vector<std::string> rutas;
    std::size_t inicio = 0;
    while (inicio <= texto.size()) {
        std::size_t fin = texto.find('\n', inicio);
        std::string linea =
            texto.substr(inicio, fin == std::string::npos ? std::string::npos
                                                          : fin - inicio);
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        linea = recortar(linea);
        if (!linea.empty() && esRutaAbsoluta(linea)) rutas.push_back(linea);
        if (fin == std::string::npos) break;
        inicio = fin + 1;
    }
    return rutas;
}

// Lee el bloque DROPFILES de Windows (el contenido de un CF_HDROP).
//
// La cabecera es un DROPFILES: el desplazamiento de la lista (pFiles), un
// POINT, dos BOOL (fNC y fWide) y despues la lista de rutas, terminada en
// doble null. La lista es ANSI si fWide es falso y UTF-16 si es cierto.
inline std::vector<std::string> parsearDropFiles(const unsigned char* datos,
                                                 std::size_t tam) {
    std::vector<std::string> rutas;
    // Cabecera DROPFILES: 4 + 8 + 4 + 4 = 20 bytes.
    const std::size_t cabecera = 20;
    if (datos == nullptr || tam < cabecera) return rutas;

    const std::uint32_t pFiles = static_cast<std::uint32_t>(datos[0]) |
                                 (static_cast<std::uint32_t>(datos[1]) << 8) |
                                 (static_cast<std::uint32_t>(datos[2]) << 16) |
                                 (static_cast<std::uint32_t>(datos[3]) << 24);
    const std::uint32_t fWide = static_cast<std::uint32_t>(datos[16]) |
                                (static_cast<std::uint32_t>(datos[17]) << 8) |
                                (static_cast<std::uint32_t>(datos[18]) << 16) |
                                (static_cast<std::uint32_t>(datos[19]) << 24);
    if (pFiles >= tam) return rutas;

    const unsigned char* lista = datos + pFiles;
    const std::size_t restante = tam - pFiles;

    if (fWide != 0) {
        std::size_t i = 0;
        while (i + 1 < restante) {
            std::u16string actual;
            bool terminada = false;
            while (i + 1 < restante) {
                const unsigned char lo = lista[i];
                const unsigned char hi = lista[i + 1];
                i += 2;
                if (lo == 0 && hi == 0) {
                    terminada = true;
                    break;
                }
                actual.push_back(static_cast<char16_t>(
                    static_cast<std::uint16_t>(lo) |
                    (static_cast<std::uint16_t>(hi) << 8)));
            }
            // Un doble null cierra la lista: lo que viene detras no son rutas.
            if (terminada && actual.empty()) break;
            if (!actual.empty()) rutas.push_back(utf16aUtf8(actual));
            if (!terminada) break;
        }
    } else {
        std::size_t i = 0;
        while (i < restante) {
            std::string actual;
            bool terminada = false;
            while (i < restante) {
                const char c = static_cast<char>(lista[i]);
                ++i;
                if (c == '\0') {
                    terminada = true;
                    break;
                }
                actual.push_back(c);
            }
            if (terminada && actual.empty()) break;
            if (!actual.empty()) rutas.push_back(actual);
            if (!terminada) break;
        }
    }
    return rutas;
}

}  // namespace PortapapelesParsers

#endif
