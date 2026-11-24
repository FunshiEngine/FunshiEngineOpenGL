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
#ifndef STRUCTVEC3_H
#define STRUCTVEC3_H


#include<cmath>
#include<iostream>

struct vec3 {
    float x, y, z;
    vec3();
    vec3(float x_, float y_, float z_);
    vec3 operator+(const vec3& v)const;
    vec3 operator-(const vec3& v)const;
    vec3 operator*(float k)const;
    float mag();
    void normaliza();
    float magnitude() const;
    float distance(const vec3& v) const;
    vec3 prodVetorial(vec3 v);
    float dot(const vec3& v) const;
    void print();
};


struct vec2 {
    float x, y;
    vec2() :x(0.0), y(0.0) {}
    vec2(float x_, float y_) :x(x_), y(y_) {}
};


#endif