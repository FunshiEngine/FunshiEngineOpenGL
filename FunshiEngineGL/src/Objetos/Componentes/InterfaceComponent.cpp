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
#include "InterfaceComponent.h"

#include <cstddef>

namespace {

// Valor centinela del formato nuevo: va como primer size_t del payload. Un
// nombre del formato viejo (Memory/Interfaces/<nombre>.json) nunca alcanza este
// tamano, porque los nombres eran cortos, asi que distingue ambos formatos sin
// ambiguedad. Cabe en un size_t de 32 bits.
constexpr std::size_t MARCA_INTERFAZ = 0x49464931u;

// Cotas de sanidad para acotar las lecturas y no construir un string de un
// tamano disparatado si el stream esta corrupto o desalineado.
constexpr std::size_t COTA_RUTA = 4096;
constexpr std::size_t COTA_NOMBRE_VIEJO = 256;

} // namespace

void InterfaceComponent::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;

    const std::size_t marca = MARCA_INTERFAZ;
    file->write(reinterpret_cast<const char*>(&marca), sizeof(marca));

    const std::size_t len = rutaHtml.size();
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0)
        file->write(rutaHtml.data(), static_cast<std::streamsize>(len));

    const std::uint8_t activoByte = activo ? 1 : 0;
    file->write(reinterpret_cast<const char*>(&activoByte), sizeof(activoByte));

    const std::uint8_t anclaByte = static_cast<std::uint8_t>(ancla);
    file->write(reinterpret_cast<const char*>(&anclaByte), sizeof(anclaByte));

    file->write(reinterpret_cast<const char*>(&desplazamiento.x), sizeof(float));
    file->write(reinterpret_cast<const char*>(&desplazamiento.y), sizeof(float));

    const std::uint8_t autoByte = tamanoAuto ? 1 : 0;
    file->write(reinterpret_cast<const char*>(&autoByte), sizeof(autoByte));

    file->write(reinterpret_cast<const char*>(&tamano.x), sizeof(float));
    file->write(reinterpret_cast<const char*>(&tamano.y), sizeof(float));
    file->write(reinterpret_cast<const char*>(&z), sizeof(float));
}

void InterfaceComponent::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) return;

    // El primer size_t decide el formato. El nuevo lleva la marca; el viejo
    // empezaba directo con el largo del nombre de la interfaz.
    std::size_t cabecera = 0;
    file->read(reinterpret_cast<char*>(&cabecera), sizeof(cabecera));
    if (!file->good() && file->gcount() <= 0) return;

    if (cabecera != MARCA_INTERFAZ) {
        // Formato viejo: solo el nombre del asset. Se carga como ruta (para no
        // perder la referencia) y el resto queda en sus valores por defecto,
        // sin consumir bytes de mas ni desalinear el resto del objeto.
        if (cabecera > 0 && cabecera <= COTA_NOMBRE_VIEJO) {
            rutaHtml.resize(cabecera);
            file->read(rutaHtml.data(),
                       static_cast<std::streamsize>(cabecera));
        } else {
            rutaHtml.clear();
        }
        return;
    }

    std::size_t len = 0;
    file->read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len == 0 || len > COTA_RUTA) {
        // Sin ruta, o cota superada (stream desalineado): no se intenta seguir
        // porque las lecturas siguientes quedarian corridas.
        rutaHtml.clear();
        return;
    }
    rutaHtml.resize(len);
    file->read(rutaHtml.data(), static_cast<std::streamsize>(len));

    std::uint8_t activoByte = 1;
    file->read(reinterpret_cast<char*>(&activoByte), sizeof(activoByte));
    activo = activoByte != 0;

    std::uint8_t anclaByte = static_cast<std::uint8_t>(AnclaInterfaz::Centro);
    file->read(reinterpret_cast<char*>(&anclaByte), sizeof(anclaByte));
    if (anclaByte <= static_cast<std::uint8_t>(AnclaInterfaz::AbajoDerecha))
        ancla = static_cast<AnclaInterfaz>(anclaByte);
    else
        ancla = AnclaInterfaz::Centro;

    file->read(reinterpret_cast<char*>(&desplazamiento.x), sizeof(float));
    file->read(reinterpret_cast<char*>(&desplazamiento.y), sizeof(float));

    std::uint8_t autoByte = 1;
    file->read(reinterpret_cast<char*>(&autoByte), sizeof(autoByte));
    tamanoAuto = autoByte != 0;

    file->read(reinterpret_cast<char*>(&tamano.x), sizeof(float));
    file->read(reinterpret_cast<char*>(&tamano.y), sizeof(float));
    file->read(reinterpret_cast<char*>(&z), sizeof(float));
}

