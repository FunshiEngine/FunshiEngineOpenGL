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
#ifndef PREPROCESADORSHADERS_H
#define PREPROCESADORSHADERS_H

#include <string>
#include <unordered_map>

namespace Rendering {

// Preprocesador de GLSL: resuelve los #include "nombre" en CPU.
//
// OpenGL 3.3 no tiene ARB_shading_language_include, asi que el include se
// resuelve antes de mandar el fuente al driver. Los fragmentos que se pueden
// incluir van embebidos en el binario (no en archivos): el ejecutable
// exportado no puede depender de assets extra.
//
// Reglas del preprocesado:
//  - solo se resuelve #include "nombre" (con comillas). Un #include <nombre>
//    se deja intacto: es del sistema y lo resuelve el driver si puede,
//  - el fragmento incluido se inserta en el punto del include, con su #line
//    para que un error del driver senale al fragmento y no a la linea del
//    fuente compuesto,
//  - hay un tope de anidamiento y se detectan los ciclos: un fragmento que se
//    incluye a si mismo no puede colgar el motor.
class PreprocesadorShaders {
public:
    // Fragmentos disponibles para incluir, por nombre (sin extension).
    using Fragmentos = std::unordered_map<std::string, std::string>;

    // Procesa el fuente y devuelve el fuente compuesto. Lanza
    // ShaderIncludeException si un include no existe, si hay un ciclo o si
    // se supera el tope de anidamiento.
    static std::string preprocesar(const std::string& fuente,
                                   const Fragmentos& fragmentos);
};

}  // namespace Rendering

#endif  // PREPROCESADORSHADERS_H
