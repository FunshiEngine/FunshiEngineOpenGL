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
#ifndef LIGHT_H
#define LIGHT_H
#include "Component.h"

// Luz como Component: se adjunta a cualquier GameObject (incluso a un objeto
// vacío sin cuerpo, el "Gizmo"). La posición/dirección de la luz se deriva del
// Transform del objeto dueño, así que moviendo el objeto se mueve la luz.
enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT
};

class Light : public Component {
private:
    LightType type = LightType::DIRECTIONAL;
    float ambient[3] = {0.1f, 0.1f, 0.1f};
    float diffuse[3] = {1.f, 1.f, 1.f};
    float specular[3] = {1.f, 1.f, 1.f};
    float constant = 1.f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float spotCutOff = 45.f;

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Light();

    LightType getType() const { return type; }
    void setType(LightType value) { type = value; }

    const float* getAmbient() const { return ambient; }
    const float* getDiffuse() const { return diffuse; }
    const float* getSpecular() const { return specular; }

    void setAmbient(float r, float g, float b);
    void setDiffuse(float r, float g, float b);
    void setSpecular(float r, float g, float b);

    float getConstant() const { return constant; }
    float getLinear() const { return linear; }
    float getQuadratic() const { return quadratic; }
    void setAttenuation(float c, float l, float q);

    float getSpotCutOff() const { return spotCutOff; }
    void setSpotCutOff(float angle);
};
#endif