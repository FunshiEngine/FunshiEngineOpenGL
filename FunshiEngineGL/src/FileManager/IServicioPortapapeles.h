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
#ifndef ISERVICIOPORTAPAPELES_H
#define ISERVICIOPORTAPAPELES_H

#include <string>
#include <vector>

// Contrato del portapapeles del sistema operativo: que archivos hay copiados
// o cortados FUERA del editor.
//
// El portapapeles de archivos no es texto en ninguna de las dos plataformas:
// en Windows es una lista de estructuras HDROP y en Linux es un
// text/uri-list. Por eso no se puede leer con una llamada generica y por eso
// el motor no lo lee: pide las rutas a este servicio y decide que hacer con
// ellas (la politica de no pisar y los avisos viven en el explorador, no
// aqui).
//
// El servicio es una interfaz porque el modulo de archivos no debe depender
// de la API del sistema ni de la biblioteca de ventanas: la implementacion
// concreta la inyecta el editor.
class IServicioPortapapeles {
public:
    virtual ~IServicioPortapapeles() = default;

    // Rutas absolutas de los archivos y carpetas que el sistema tiene en el
    // portapapeles. Vacio si no hay archivos.
    virtual std::vector<std::string> archivosEnPortapapeles() const = 0;

    // Si el portapapeles del sistema se puede consultar en esta maquina. Un
    // servidor sin escritorio no lo tiene: la GUI avisa en vez de fallar.
    // `motivo` (opcional) recibe la explicacion.
    virtual bool disponible(std::string* motivo = nullptr) const = 0;
};

#endif
