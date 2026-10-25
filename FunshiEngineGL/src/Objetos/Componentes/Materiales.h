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
#ifndef MATERIALES_H
#define MATERIALES_H

#include <memory>
#include <string>
#include <vector>

#include "Component.h"
#include "Material.h"

// Componente de materiales por region: cada region de la malla (la cara de un
// cubo, la parte de un personaje) puede llevar su propio material y sus
// propias texturas.
//
// El nombre de la region es el que declara el archivo del modelo (el material
// de un .obj o de un .fbx). El motor dibuja la region con su material y, si la
// region no tiene material, con el material unico del objeto.
class Materiales : public Component {
public:
    // Region con su material. El material vive dentro de la region (no es un
    // componente mas del objeto): es parte del dato de la region.
    struct Region {
        std::string nombre;
        std::unique_ptr<Material> material;
    };

private:
    std::vector<Region> regiones_;
    // Indice de la region editada (para el panel); -1 si ninguna.
    int regionSeleccionada_ = -1;

    void leerRegion(std::ifstream* file, Region& region);
    void escribirRegion(std::ofstream* file, const Region& region);

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Materiales();

    // Regiones del componente (en el orden en que se anadieron).
    const std::vector<Region>& regiones() const { return regiones_; }
    std::size_t cantidadRegiones() const { return regiones_.size(); }

    // Anade una region con el material por defecto. Si ya existe una region
    // con ese nombre, no se anade otra: devuelve false.
    bool agregarRegion(const std::string& nombre);
    // Quita la region por su nombre. Devuelve false si no existe.
    bool quitarRegion(const std::string& nombre);
    // Region por nombre (nullptr si no existe).
    const Region* region(const std::string& nombre) const;
    Region* region(const std::string& nombre);

    // Material de la region (nullptr si la region no existe).
    const Material* materialDe(const std::string& nombre) const;
    Material* materialDe(const std::string& nombre);

    // Empareja las regiones con las del modelo: las que el archivo declara y
    // el componente no tiene se anaden (con material por defecto) y las que el
    // componente tiene y el archivo no, se quedan. Devuelve cuantas se
    // anadieron.
    int sincronizarCon(const std::vector<std::string>& nombres);

    void setRegionSeleccionada(int indice) { regionSeleccionada_ = indice; }
    int regionSeleccionada() const { return regionSeleccionada_; }
};

#endif // MATERIALES_H
