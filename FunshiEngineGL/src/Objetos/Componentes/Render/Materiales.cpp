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

#include "Materiales.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {

// Marcador binario del payload de Materiales. Distingue este formato de los
// demas componentes (que tienen su propio marcador) y permite detectar un
// archivo viejo o corrupto sin desalinear el stream.
const char kMaterialesMagic[4] = {'F', 'M', 't', 'R'};
// Longitud maxima del nombre de una region.
const std::size_t kMaxNombreRegion = 256;
// Cantidad maxima de regiones que se leen de un archivo (un archivo corrupto
// no puede hacer que se reserven millones de regiones).
const std::size_t kMaxRegiones = 1024;

} // namespace

Materiales::Materiales() {}

bool Materiales::agregarRegion(const std::string& nombre) {
    if (nombre.empty()) return false;
    if (region(nombre)) return false;
    Region region;
    region.nombre = nombre;
    region.material = std::make_unique<Material>();
    regiones_.push_back(std::move(region));
    return true;
}

bool Materiales::quitarRegion(const std::string& nombre) {
    for (std::size_t i = 0; i < regiones_.size(); ++i) {
        if (regiones_[i].nombre != nombre) continue;
        regiones_.erase(regiones_.begin() + static_cast<std::ptrdiff_t>(i));
        if (regionSeleccionada_ >= static_cast<int>(regiones_.size())) {
            regionSeleccionada_ = static_cast<int>(regiones_.size()) - 1;
        }
        return true;
    }
    return false;
}

const Materiales::Region* Materiales::region(const std::string& nombre) const {
    for (const Region& region : regiones_) {
        if (region.nombre == nombre) return &region;
    }
    return nullptr;
}

Materiales::Region* Materiales::region(const std::string& nombre) {
    for (Region& region : regiones_) {
        if (region.nombre == nombre) return &region;
    }
    return nullptr;
}

const Material* Materiales::materialDe(const std::string& nombre) const {
    const Region* region = this->region(nombre);
    return region ? region->material.get() : nullptr;
}

Material* Materiales::materialDe(const std::string& nombre) {
    Region* region = this->region(nombre);
    return region ? region->material.get() : nullptr;
}

int Materiales::sincronizarCon(const std::vector<std::string>& nombres) {
    int anadidas = 0;
    for (const std::string& nombre : nombres) {
        if (agregarRegion(nombre)) ++anadidas;
    }
    return anadidas;
}

void Materiales::escribirRegion(std::ofstream* file, const Region& region) {
    const std::uint32_t largo = static_cast<std::uint32_t>(region.nombre.size());
    file->write(reinterpret_cast<const char*>(&largo), sizeof(largo));
    file->write(region.nombre.data(),
                static_cast<std::streamsize>(region.nombre.size()));
    if (region.material) {
        region.material->saveComponent(file);
    } else {
        // Region sin material: se escribe un material por defecto para que el
        // stream quede alineado al leerlo.
        Material porDefecto;
        porDefecto.saveComponent(file);
    }
}

void Materiales::leerRegion(std::ifstream* file, Region& region) {
    std::uint32_t largo = 0;
    file->read(reinterpret_cast<char*>(&largo), sizeof(largo));
    if (file->gcount() != static_cast<std::streamsize>(sizeof(largo))) return;
    if (largo > kMaxNombreRegion) {
        // Nombre demasiado largo: se saltan los bytes para no desalinear.
        file->seekg(static_cast<std::streamoff>(largo), std::ios::cur);
        return;
    }
    region.nombre.assign(largo, '\0');
    if (largo > 0) {
        file->read(&region.nombre[0], static_cast<std::streamsize>(largo));
    }
    region.material = std::make_unique<Material>();
    region.material->loadComponent(file);
}

void Materiales::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;
    file->write(kMaterialesMagic, sizeof(kMaterialesMagic));
    const std::uint32_t cantidad = static_cast<std::uint32_t>(regiones_.size());
    file->write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));
    for (const Region& region : regiones_) {
        escribirRegion(file, region);
    }
}

void Materiales::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) {
        std::cerr << "Error: archivo invalido o no abierto para lectura "
                     "(Materiales).\n";
        return;
    }
    regiones_.clear();

    char magic[4];
    file->read(magic, sizeof(magic));
    if (file->gcount() != static_cast<std::streamsize>(sizeof(magic))) {
        std::cerr << "[Materiales] payload incompleto: se ignoran las regiones\n";
        return;
    }
    if (std::memcmp(magic, kMaterialesMagic, sizeof(kMaterialesMagic)) != 0) {
        // Formato desconocido: se rebobina para no desalinear el stream (el
        // componente se queda vacio).
        file->seekg(-static_cast<std::streamoff>(sizeof(magic)), std::ios::cur);
        std::cerr << "[Materiales] marcador desconocido: el componente queda "
                     "vacio\n";
        return;
    }

    std::uint32_t cantidad = 0;
    file->read(reinterpret_cast<char*>(&cantidad), sizeof(cantidad));
    if (file->gcount() != static_cast<std::streamsize>(sizeof(cantidad))) return;
    if (cantidad > kMaxRegiones) {
        std::cerr << "[Materiales] el archivo declara " << cantidad
                  << " regiones: se leen hasta el limite\n";
        cantidad = static_cast<std::uint32_t>(kMaxRegiones);
    }
    for (std::uint32_t i = 0; i < cantidad; ++i) {
        Region region;
        leerRegion(file, region);
        if (region.nombre.empty()) continue;
        regiones_.push_back(std::move(region));
    }
}
