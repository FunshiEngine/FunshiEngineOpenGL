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
#ifndef LIMITESSERIALIZACION_H
#define LIMITESSERIALIZACION_H

#include <cstddef>
#include <istream>

// Cotas y validacion de los datos que el motor lee de un binario.
//
// El formato de escena y de componentes guarda, antes de cada bloque de
// datos, la longitud de ese bloque. Esa longitud la controla el archivo: si
// esta corrupto o desalineado, el valor puede ser enorme. Usarla tal cual
// como tamano de reserva provoke que `std::string::resize` lance
// `std::length_error` o que `bad_alloc` agote la memoria, y las dos cosas
// terminan en `std::terminate`: el motor aborta en vez de rechazar el
// archivo.
//
// La regla del motor es una sola, y esta en todas partes: toda longitud o
// cantidad leida del disco se acota con un maximo por tipo Y se valida contra
// el contenido que queda en el stream. Un dato invalido corta la lectura del
// objeto con un error claro; nunca aborta el proceso.
namespace LimitesSerializacion {

// Maximos por tipo de dato. Son deliberadamente generosos para datos
// legitimos y pequenos frente a los valores que provocarian el aborto.
constexpr std::size_t kRuta = 4096;        // ruta de archivo
constexpr std::size_t kNombre = 256;       // nombre de clase, objeto o tipo
constexpr std::size_t kTexto = 1u << 20;   // texto de hasta 1 MiB
constexpr std::size_t kCantidad = 100000;  // elementos de una coleccion
constexpr std::size_t kProfundidad = 32;   // anidamiento de estructuras

// Indica si en el stream quedan al menos `bytes` por leer. Si el stream no
// permite saberlo (no es buscable), se asume que si: la cota por tipo sigue
// protegiendo contra la reserva absurda.
inline bool hayContenidoSuficiente(std::istream& in, std::size_t bytes) {
    const std::streampos actual = in.tellg();
    if (actual == std::streampos(-1)) return true;

    in.seekg(0, std::ios::end);
    const std::streampos fin = in.tellg();
    in.seekg(actual);

    if (fin == std::streampos(-1) || fin < actual) return true;
    return static_cast<std::size_t>(fin - actual) >= bytes;
}

// Lee una longitud del binario y la valida: tiene que estar entre el minimo
// y la cota, y el stream tiene que tener suficiente contenido para el bloque
// que esa longitud anuncia. Devuelve false (y deja la longitud en 0) cuando el
// dato no es valido, para que el llamador corte la lectura con un error.
template <class T>
bool leerLongitud(std::istream& in, T& longitud, std::size_t cota,
                  std::size_t bytesPorElemento, std::size_t minimo = 1) {
    longitud = 0;
    in.read(reinterpret_cast<char*>(&longitud), sizeof(longitud));
    if (!in) return false;

    const std::size_t valor = static_cast<std::size_t>(longitud);
    if (valor < minimo || valor > cota) return false;
    if (valor == 0) return true;
    return hayContenidoSuficiente(in, valor * bytesPorElemento);
}

// Igual que la anterior, pero para longitudes que ya fueron leidas y que se
// validan contra la cota y el contenido restante.
inline bool longitudValida(std::istream& in, std::size_t longitud,
                           std::size_t cota, std::size_t bytesPorElemento) {
    if (longitud == 0 || longitud > cota) return false;
    return hayContenidoSuficiente(in, longitud * bytesPorElemento);
}

}  // namespace LimitesSerializacion

#endif  // LIMITESSERIALIZACION_H
