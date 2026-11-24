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
#ifndef IMATERIAL_H
#define IMATERIAL_H

#include <string>

// Vista de solo lectura de un material: los colores del modelo de iluminacion
// (ambiente, difuso, especular, emision y brillo) y las rutas de sus cuatro
// texturas.
//
// La capa de render trabaja con este contrato y no con el componente concreto:
// asi el renderer no depende de como se guardan los materiales (un material
// unico del objeto o el material de una region de la malla) y el motor puede
// resolver la region que toca sin que el codigo de dibujo sepa de componentes.
class IMaterial {
public:
    virtual ~IMaterial() = default;

    virtual const float* getAmbient() const = 0;
    virtual const float* getDiffuse() const = 0;
    virtual const float* getSpecular() const = 0;
    virtual const float* getEmission() const = 0;
    virtual float getShininess() const = 0;

    // Rutas de las texturas (descriptores): vacio = slot sin textura.
    virtual const std::string& getDiffuseMapPath() const = 0;
    virtual const std::string& getSpecularMapPath() const = 0;
    virtual const std::string& getNormalMapPath() const = 0;
    virtual const std::string& getEmissionMapPath() const = 0;

    virtual bool hasDiffuseMap() const = 0;
    virtual bool hasSpecularMap() const = 0;
    virtual bool hasNormalMap() const = 0;
    virtual bool hasEmissionMap() const = 0;
};

#endif // IMATERIAL_H
