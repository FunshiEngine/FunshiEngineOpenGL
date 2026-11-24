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
#ifndef ESCRITORPERFIL_H
#define ESCRITORPERFIL_H

#include <string>

#include "PerfilParticula.h"

// Escribe un perfil de efecto como JSON, en el mismo formato que lee
// cargarPerfilDesdeJson (LectorPerfil.h). Es la contraparte de escritura: con
// ambos, un perfil viaja de la memoria a un archivo y vuelve sin perder
// campos, de modo que el editor puede "generar" un efecto como archivo y el
// explorador de contenido ofrecerlo para arrastrar sobre un emisor.
//
// El escritor exige las mismas invariantes que el lector impone al cargar
// (vida minima mayor que cero, minimos no mayores que los maximos, rozamiento
// no negativo) para que un perfil guardado nunca sea un perfil que la carga
// rechace.
//
// Devuelve false y describe el motivo en 'error' si los datos no cumplen las
// invariantes, si la carpeta destino no se pudo preparar o si el archivo no
// se pudo escribir.
bool guardarPerfilAJson(const std::string& rutaAbsoluta,
                        const PerfilParticula& perfil, std::string& error);

#endif
