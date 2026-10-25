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
#ifndef IMPORTACIONESPE_H
#define IMPORTACIONESPE_H

#include <cstdint>
#include <string>
#include <vector>

// Lectura de la tabla de importaciones de un binario PE (Windows).
//
// El juego exportado tiene que llevar consigo las DLL de las que depende: si
// no, al arrancar Windows las pide ("no se encuentra la biblioteca ..."). Esta
// clase lee los nombres de las DLL que el ejecutable importa, para que el
// exportador las copie junto al binario.
//
// Es solo lectura de archivo: no depende de Windows ni de la plataforma.
class ImportacionesPE {
public:
    // Nombres de las DLL importadas por el ejecutable, en el orden del
    // archivo y sin repetir. Vacio si el archivo no es un PE valido, si no
    // tiene tabla de importaciones o si no se puede leer.
    static std::vector<std::string> leer(const std::string& rutaEjecutable);

    // Lo mismo, pero sobre los bytes ya leidos (el contrato real es el de
    // archivo; esta variante existe para poder ejercitarlo sin tocar disco).
    static std::vector<std::string> leer(const std::vector<unsigned char>& datos);
};

#endif // IMPORTACIONESPE_H
