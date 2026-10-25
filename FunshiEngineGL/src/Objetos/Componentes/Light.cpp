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
#include "Light.h"

#include <iostream>

Light::Light() = default;

void Light::serializeComponent(std::ofstream* file) {
    if (!file || !file->is_open()) return;

    const int typeInt = static_cast<int>(type);
    file->write(reinterpret_cast<const char*>(&typeInt), sizeof(int));
    file->write(reinterpret_cast<const char*>(ambient), sizeof(ambient));
    file->write(reinterpret_cast<const char*>(diffuse), sizeof(diffuse));
    file->write(reinterpret_cast<const char*>(specular), sizeof(specular));
    file->write(reinterpret_cast<const char*>(&constant), sizeof(float));
    file->write(reinterpret_cast<const char*>(&linear), sizeof(float));
    file->write(reinterpret_cast<const char*>(&quadratic), sizeof(float));
    file->write(reinterpret_cast<const char*>(&spotCutOff), sizeof(float));
}

void Light::deserializeComponent(std::ifstream* file) {
    if (!file || !file->is_open()) {
        std::cerr << "Error: archivo invalido o no abierto para lectura (Light).\n";
        return;
    }
    int typeInt = 0;
    file->read(reinterpret_cast<char*>(&typeInt), sizeof(int));
    type = static_cast<LightType>(typeInt);
    file->read(reinterpret_cast<char*>(ambient), sizeof(ambient));
    file->read(reinterpret_cast<char*>(diffuse), sizeof(diffuse));
    file->read(reinterpret_cast<char*>(specular), sizeof(specular));
    file->read(reinterpret_cast<char*>(&constant), sizeof(float));
    file->read(reinterpret_cast<char*>(&linear), sizeof(float));
    file->read(reinterpret_cast<char*>(&quadratic), sizeof(float));
    file->read(reinterpret_cast<char*>(&spotCutOff), sizeof(float));
}

void Light::setAmbient(float r, float g, float b) {
    ambient[0] = r; ambient[1] = g; ambient[2] = b;
}
void Light::setDiffuse(float r, float g, float b) {
    diffuse[0] = r; diffuse[1] = g; diffuse[2] = b;
}
void Light::setSpecular(float r, float g, float b) {
    specular[0] = r; specular[1] = g; specular[2] = b;
}
void Light::setAttenuation(float c, float l, float q) {
    constant = c; linear = l; quadratic = q;
}
void Light::setSpotCutOff(float angle) { spotCutOff = angle; }