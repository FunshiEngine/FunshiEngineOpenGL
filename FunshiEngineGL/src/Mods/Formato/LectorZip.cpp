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
#include "LectorZip.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <miniz/miniz.h>

namespace fs = std::filesystem;

namespace {

// Cierra el lector pase lo que pase (incluidos los return tempranos).
class LectorZipRAII {
public:
    explicit LectorZipRAII(mz_zip_archive& zip) : zip_(zip) {}
    ~LectorZipRAII() { mz_zip_reader_end(&zip_); }

    LectorZipRAII(const LectorZipRAII&) = delete;
    LectorZipRAII& operator=(const LectorZipRAII&) = delete;

private:
    mz_zip_archive& zip_;
};

// Nombre de la entrada `indice` tal como lo guarda el ZIP (separador '/'),
// reservando espacio dinamico para nombres largos.
std::string nombreEntrada(mz_zip_archive& zip, mz_uint indice) {
    std::vector<char> buffer(256);
    mz_uint largo = mz_zip_reader_get_filename(&zip, indice, buffer.data(),
                                               static_cast<mz_uint>(buffer.size()));
    if (largo == 0)
        return std::string();
    if (largo >= buffer.size())
        return std::string(buffer.data());
    return std::string(buffer.data(), largo - 1);
}

// Divide una ruta de entrada en componentes, tratando '/' y '\' como
// separadores (un paquete puede venir con separador de Windows aunque el ZIP
// use '/' por formato).
std::vector<std::string> componentes(const std::string& ruta) {
    std::vector<std::string> partes;
    std::string actual;
    for (char c : ruta) {
        if (c == '/' || c == '\\') {
            partes.push_back(actual);
            actual.clear();
        } else {
            actual.push_back(c);
        }
    }
    partes.push_back(actual);
    return partes;
}

} // namespace

bool LectorZip::esRutaSegura(const std::string& nombreEntrada) {
    if (nombreEntrada.empty())
        return false;
    // Una ruta absoluta (POSIX '/' o unidad de Windows 'C:') apunta fuera del
    // destino por definicion.
    if (nombreEntrada.front() == '/' || nombreEntrada.front() == '\\')
        return false;
    if (nombreEntrada.find(':') != std::string::npos)
        return false;
    // Ningun componente puede ser ".." ni el separador final de un directorio
    // disfrazar un padre.
    for (const std::string& parte : componentes(nombreEntrada)) {
        if (parte == "..")
            return false;
    }
    return true;
}

bool LectorZip::leerEntrada(const std::string& rutaZip,
                            const std::string& nombreEntrada,
                            std::string& contenido, std::string& error) {
    error.clear();
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_reader_init_file(&zip, rutaZip.c_str(), 0)) {
        error = "no se pudo abrir el paquete como ZIP: " + rutaZip;
        return false;
    }
    LectorZipRAII cierre(zip);

    // locate_file_v2 devuelve el indice; permite nombres con subdirectorios.
    mz_uint32 indice = 0;
    if (!mz_zip_reader_locate_file_v2(&zip, nombreEntrada.c_str(), nullptr, 0,
                                      &indice)) {
        error = "el paquete no contiene la entrada '" + nombreEntrada + "'";
        return false;
    }

    size_t tamano = 0;
    void* datos = mz_zip_reader_extract_to_heap(&zip, indice, &tamano, 0);
    if (datos == nullptr) {
        error = "no se pudo extraer la entrada '" + nombreEntrada + "'";
        return false;
    }
    contenido.assign(static_cast<const char*>(datos), tamano);
    mz_free(datos);
    return true;
}

bool LectorZip::extraerTodo(const std::string& rutaZip,
                            const std::string& directorioDestino,
                            std::string& error) {
    error.clear();
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_reader_init_file(&zip, rutaZip.c_str(), 0)) {
        error = "no se pudo abrir el paquete como ZIP: " + rutaZip;
        return false;
    }
    LectorZipRAII cierre(zip);

    const mz_uint total = mz_zip_reader_get_num_files(&zip);

    // Primera pasada: validar TODAS las rutas antes de escribir nada. Un
    // paquete con una entrada insegura se rechaza entero y no deja restos.
    std::vector<std::string> nombres;
    nombres.reserve(total);
    for (mz_uint i = 0; i < total; ++i) {
        const std::string nombre = nombreEntrada(zip, i);
        if (nombre.empty()) {
            error = "el paquete tiene una entrada sin nombre";
            return false;
        }
        if (!esRutaSegura(nombre)) {
            error = "el paquete tiene una entrada insegura (fuera del "
                    "destino): '" + nombre + "'";
            return false;
        }
        nombres.push_back(nombre);
    }

    std::error_code ec;
    fs::create_directories(directorioDestino, ec);
    if (ec) {
        error = "no se pudo crear la carpeta de extraccion: " +
                directorioDestino;
        return false;
    }
    const fs::path destinoCanonico = fs::weakly_canonical(directorioDestino, ec);
    if (ec) {
        error = "no se pudo resolver la carpeta de extraccion: " +
                directorioDestino;
        return false;
    }

    for (mz_uint i = 0; i < total; ++i) {
        const std::string& nombre = nombres[i];
        const fs::path relativo = fs::path(nombre);
        const fs::path destino = destinoCanonico / relativo;

        // Segunda guarda: aunque la ruta haya pasado la validacion sintactica,
        // el destino resuelto tiene que quedar dentro de la carpeta base.
        const fs::path destinoResuelto = fs::weakly_canonical(destino, ec);
        if (ec) {
            error = "no se pudo resolver la ruta de la entrada '" + nombre +
                    "'";
            return false;
        }
        auto base = destinoCanonico.begin();
        auto fin = destinoCanonico.end();
        auto hijo = destinoResuelto.begin();
        bool dentro = true;
        for (; base != fin; ++base, ++hijo) {
            if (hijo == destinoResuelto.end() || *hijo != *base) {
                dentro = false;
                break;
            }
        }
        if (!dentro) {
            error = "la entrada '" + nombre + "' resuelve fuera del destino";
            return false;
        }

        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            fs::create_directories(destino, ec);
            if (ec) {
                error = "no se pudo crear el subdirectorio '" + nombre + "'";
                return false;
            }
            continue;
        }

        fs::create_directories(destino.parent_path(), ec);
        if (ec) {
            error = "no se pudo crear el subdirectorio de '" + nombre + "'";
            return false;
        }
        if (!mz_zip_reader_extract_to_file(&zip, i, destino.string().c_str(),
                                           0)) {
            error = "no se pudo extraer la entrada '" + nombre + "'";
            return false;
        }
    }
    return true;
}
