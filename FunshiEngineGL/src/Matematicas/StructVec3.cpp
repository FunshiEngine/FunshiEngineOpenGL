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
#include "StructVec3.h"

vec3::vec3() :x(0.0), y(0.0), z(0.0) {
}

vec3::vec3(float x_, float y_, float z_) :x(x_), y(y_), z(z_) {
}

vec3 vec3::operator+(const vec3& v)const {
    return vec3(x + v.x, y + v.y, z + v.z);
}

vec3 vec3::operator-(const vec3& v) const
{
    return vec3(x - v.x, y - v.y, z - v.z);
}

vec3 vec3::operator*(float k)const {
    return vec3(x * k, y * k, z * k);
}

float vec3::mag() {
    return sqrt(x * x + y * y + z * z);
}

void vec3::normaliza() {
    float m = mag();
    x = x / m;
    y = y / m;
    z = z / m;
}

vec3 vec3::prodVetorial(vec3 v) {
    return vec3(
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x);
}

float vec3::magnitude() const {
    return sqrt(x * x + y * y + z * z);
}

float vec3::distance(const vec3& other) const {
    return (*this - other).magnitude();
}

float vec3::dot(const vec3& v) const {
    return x * v.x + y * v.y + z * v.z;
}

void vec3::print()
{
    std::cout << "(" << x << "," << y << "," << z << ")";
}