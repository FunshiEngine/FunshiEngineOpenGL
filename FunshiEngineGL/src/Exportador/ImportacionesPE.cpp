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
#include "ImportacionesPE.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace {

// Seccion del PE: donde se mapean las direcciones virtuales al archivo.
struct Seccion {
    std::uint32_t virtualAddress = 0;
    std::uint32_t virtualSize = 0;
    std::uint32_t rawPointer = 0;
    std::uint32_t rawSize = 0;
};

std::uint16_t leerU16(const std::vector<unsigned char>& datos,
                       std::size_t offset, bool& ok) {
    if (offset + 2 > datos.size()) {
        ok = false;
        return 0;
    }
    return static_cast<std::uint16_t>(datos[offset] |
                                      (datos[offset + 1] << 8));
}

std::uint32_t leerU32(const std::vector<unsigned char>& datos,
                       std::size_t offset, bool& ok) {
    if (offset + 4 > datos.size()) {
        ok = false;
        return 0;
    }
    return static_cast<std::uint32_t>(datos[offset]) |
           (static_cast<std::uint32_t>(datos[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(datos[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(datos[offset + 3]) << 24);
}

// Convierte una direccion virtual (RVA) en el offset del archivo, usando las
// secciones. Devuelve false si la RVA no cae en ninguna.
bool rvaAOficio(const std::vector<Seccion>& secciones, std::uint32_t rva,
                std::size_t& offset) {
    for (const Seccion& seccion : secciones) {
        const std::uint32_t tamano =
            std::max(seccion.virtualSize, seccion.rawSize);
        if (tamano == 0) continue;
        if (rva < seccion.virtualAddress) continue;
        if (rva >= seccion.virtualAddress + tamano) continue;
        const std::size_t dentro = rva - seccion.virtualAddress;
        if (dentro >= seccion.rawSize) return false;
        offset = seccion.rawPointer + dentro;
        return offset < static_cast<std::size_t>(-1);
    }
    return false;
}

// Cadena ASCII terminada en nulo que empieza en el offset dado.
std::string leerCadena(const std::vector<unsigned char>& datos,
                        std::size_t offset) {
    std::string texto;
    while (offset < datos.size() && datos[offset] != 0) {
        texto.push_back(static_cast<char>(datos[offset]));
        ++offset;
    }
    return texto;
}

} // namespace

std::vector<std::string> ImportacionesPE::leer(
    const std::vector<unsigned char>& datos) {
    std::vector<std::string> dlls;
    bool ok = true;

    // Cabecera DOS: la firma "MZ" y el offset de la cabecera PE.
    if (datos.size() < 0x40) return dlls;
    if (datos[0] != 'M' || datos[1] != 'Z') return dlls;
    const std::size_t peOffset = leerU32(datos, 0x3C, ok);
    if (!ok) return dlls;

    // Firma "PE\0\0" y cabecera COFF.
    if (peOffset + 24 > datos.size()) return dlls;
    if (datos[peOffset] != 'P' || datos[peOffset + 1] != 'E') return dlls;
    const std::size_t coff = peOffset + 4;
    const std::uint16_t numeroSecciones = leerU16(datos, coff + 2, ok);
    if (!ok) return dlls;
    const std::uint16_t tamanoCabeceraOpcional =
        leerU16(datos, coff + 16, ok);
    if (!ok) return dlls;

    // Cabecera opcional: el magic decide donde empiezan los directorios de
    // datos (PE32: 96; PE32+: 112).
    const std::size_t opcional = coff + 20;
    const std::uint16_t magic = leerU16(datos, opcional, ok);
    if (!ok) return dlls;
    std::size_t directorios;
    if (magic == 0x10B) {
        directorios = opcional + 96;
    } else if (magic == 0x20B) {
        directorios = opcional + 112;
    } else {
        return dlls;
    }

    // Directorio de datos #1: la tabla de importaciones.
    const std::uint32_t rvaImportaciones = leerU32(datos, directorios + 8, ok);
    if (!ok || rvaImportaciones == 0) return dlls;

    // Secciones: donde se resuelve la RVA.
    std::vector<Seccion> secciones;
    const std::size_t inicioSecciones = opcional + tamanoCabeceraOpcional;
    for (std::uint16_t i = 0; i < numeroSecciones; ++i) {
        const std::size_t base = inicioSecciones + static_cast<std::size_t>(i) * 40;
        Seccion seccion;
        seccion.virtualSize = leerU32(datos, base + 8, ok);
        seccion.virtualAddress = leerU32(datos, base + 12, ok);
        seccion.rawSize = leerU32(datos, base + 16, ok);
        seccion.rawPointer = leerU32(datos, base + 20, ok);
        if (!ok) return dlls;
        secciones.push_back(seccion);
    }

    // Descriptores de importacion: cada uno tiene el RVA del nombre de la DLL.
    std::size_t offset = 0;
    if (!rvaAOficio(secciones, rvaImportaciones, offset)) return dlls;
    while (offset + 20 <= datos.size()) {
        const std::uint32_t rvaNombre = leerU32(datos, offset + 12, ok);
        if (!ok) break;
        if (rvaNombre == 0) break; // fin de la tabla
        std::size_t offsetNombre = 0;
        if (rvaAOficio(secciones, rvaNombre, offsetNombre)) {
            const std::string nombre = leerCadena(datos, offsetNombre);
            if (!nombre.empty() &&
                std::find(dlls.begin(), dlls.end(), nombre) == dlls.end()) {
                dlls.push_back(nombre);
            }
        }
        offset += 20;
    }
    return dlls;
}

std::vector<std::string> ImportacionesPE::leer(
    const std::string& rutaEjecutable) {
    std::ifstream archivo(rutaEjecutable, std::ios::binary);
    if (!archivo) return {};
    const std::vector<unsigned char> datos(
        (std::istreambuf_iterator<char>(archivo)),
        std::istreambuf_iterator<char>());
    return leer(datos);
}
