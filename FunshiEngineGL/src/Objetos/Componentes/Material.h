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
#ifndef MATERIAL_H
#define MATERIAL_H
#include <string>

#include "Component.h"
#include "IMaterial.h"

// Componente de material: datos del modelo de iluminacion de un objeto
// (AMBIENT/DIFFUSE/SPECULAR/EMISSION/SHININESS) mas los paths de las texturas
// (descriptores, todos opcionales): difusa, especular, normal y emision. Solo
// data; el renderer (moderno via TextureManager, o legacy via la capa de
// Rendering) lee estos valores al dibujar.
//
// Implementa IMaterial: la capa de render dibuja contra ese contrato, sin
// saber si el material es el unico del objeto o el de una region de la malla.
class Material : public Component, public IMaterial {
private:
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float emission[4];
    float shininess;

    std::string diffuseMapPath_;
    std::string specularMapPath_;
    std::string normalMapPath_;
    std::string emissionMapPath_;

    // Lee un path de textura del stream con tope de longitud (formato v2).
    void leerPathTextura(std::ifstream* file, std::string& out);

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Material();

    void setAmbient(float r, float g, float b);
    void setDiffuse(float r, float g, float b);
    void setSpecular(float r, float g, float b);
    void setEmission(float r, float g, float b);
    void setShininess(float value);

    const float* getAmbient() const override { return ambient; }
    const float* getDiffuse() const override { return diffuse; }
    const float* getSpecular() const override { return specular; }
    const float* getEmission() const override { return emission; }
    float getShininess() const override { return shininess; }

    // Slots de textura (descriptores): path a la imagen compartida por el
    // TextureManager. Vacio = slot sin textura.
    void setDiffuseMapPath(const std::string& path) { diffuseMapPath_ = path; }
    void setSpecularMapPath(const std::string& path) { specularMapPath_ = path; }
    void setNormalMapPath(const std::string& path) { normalMapPath_ = path; }
    void setEmissionMapPath(const std::string& path) { emissionMapPath_ = path; }

    const std::string& getDiffuseMapPath() const override { return diffuseMapPath_; }
    const std::string& getSpecularMapPath() const override { return specularMapPath_; }
    const std::string& getNormalMapPath() const override { return normalMapPath_; }
    const std::string& getEmissionMapPath() const override { return emissionMapPath_; }

    bool hasDiffuseMap() const override { return !diffuseMapPath_.empty(); }
    bool hasSpecularMap() const override { return !specularMapPath_.empty(); }
    bool hasNormalMap() const override { return !normalMapPath_.empty(); }
    bool hasEmissionMap() const override { return !emissionMapPath_.empty(); }
};
#endif