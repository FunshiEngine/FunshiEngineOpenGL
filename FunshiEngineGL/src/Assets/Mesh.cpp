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
#include "Mesh.h"

#include <algorithm>
#include <cmath>

bool Mesh::computeBounds(vec3& outMin, vec3& outMax) const {
    if (vertices.empty()) return false;
    outMin = vertices.front();
    outMax = vertices.front();
    for (size_t i = 1; i < vertices.size(); ++i) {
        const vec3& v = vertices[i];
        outMin.x = std::min(outMin.x, v.x);
        outMin.y = std::min(outMin.y, v.y);
        outMin.z = std::min(outMin.z, v.z);
        outMax.x = std::max(outMax.x, v.x);
        outMax.y = std::max(outMax.y, v.y);
        outMax.z = std::max(outMax.z, v.z);
    }
    return true;
}

bool Mesh::computeNormals(bool soloFaltantes) {
    if (vertices.empty() || indices.empty() || indices.size() % 3 != 0)
        return false;

    if (soloFaltantes) {
        // Respeta las normales que ya trae el asset: sin esto habria que
        // arranque con un vector por vertice.
        if (!hasNormals()) normals.assign(vertices.size(), vec3(0.f, 0.f, 0.f));
    } else {
        normals.assign(vertices.size(), vec3(0.f, 0.f, 0.f));
    }

    for (size_t f = 0; f + 2 < indices.size(); f += 3) {
        const unsigned int i0 = indices[f + 0];
        const unsigned int i1 = indices[f + 1];
        const unsigned int i2 = indices[f + 2];
        if (i0 >= vertices.size() || i1 >= vertices.size() ||
            i2 >= vertices.size())
            continue;

        // El producto cruz de los dos lados tiene por longitud el doble del
        // area de la cara: acumularlo pondera cada cara por su area, que es lo
        // que hace que un promediado simple se vea mal en mallas con triangulos
        // de tamanhos muy distintos.
        const vec3& p0 = vertices[i0];
        const vec3& p1 = vertices[i1];
        const vec3& p2 = vertices[i2];
        const vec3 n = (p1 - p0).prodVetorial(p2 - p0);

        for (const unsigned int i : {i0, i1, i2}) {
            // En modo "soloFaltantes" una normal ya informada no se toca: se
            // acumula solo en los vertices que siguen en cero.
            if (soloFaltantes && normals[i].magnitude() > 1e-8f) continue;
            normals[i] = normals[i] + n;
        }
    }

    for (size_t v = 0; v < vertices.size(); ++v) {
        // normaliza() divide por la magnitud: un vertice sin caras (aislado
        // o con caras degeneradas) se queda con la normal nula en vez de un NaN
        // que contaminaria la iluminacion de toda la malla en el shader.
        if (normals[v].magnitude() < 1e-8f) continue;
        normals[v].normaliza();
    }
    return hasNormals();
}

bool Mesh::computeTangents() {
    if (!hasUvs() || !hasNormals() || indices.empty() ||
        indices.size() % 3 != 0)
        return false;

    tangents.assign(vertices.size(), vec3(0.f, 0.f, 0.f));
    bitangents.assign(vertices.size(), vec3(0.f, 0.f, 0.f));

    for (size_t f = 0; f + 2 < indices.size(); f += 3) {
        const unsigned int i0 = indices[f + 0];
        const unsigned int i1 = indices[f + 1];
        const unsigned int i2 = indices[f + 2];
        if (i0 >= vertices.size() || i1 >= vertices.size() ||
            i2 >= vertices.size())
            continue;

        const vec3& p0 = vertices[i0];
        const vec3& p1 = vertices[i1];
        const vec3& p2 = vertices[i2];
        const vec2& uv0 = uvs[i0];
        const vec2& uv1 = uvs[i1];
        const vec2& uv2 = uvs[i2];

        const vec3 e1 = p1 - p0;
        const vec3 e2 = p2 - p0;
        const float duv1u = uv1.x - uv0.x;
        const float duv1v = uv1.y - uv0.y;
        const float duv2u = uv2.x - uv0.x;
        const float duv2v = uv2.y - uv0.y;

        // Area en espacio UV de la cara: degenerada => no aporta tangente.
        const float area = duv1u * duv2v - duv1v * duv2u;
        if (std::fabs(area) < 1e-9f) continue;

        const float r = 1.f / area;
        const vec3 t = (e1 * duv2v - e2 * duv1v) * r;
        const vec3 b = (e2 * duv1u - e1 * duv2u) * r;

        tangents[i0] = tangents[i0] + t;
        tangents[i1] = tangents[i1] + t;
        tangents[i2] = tangents[i2] + t;
        bitangents[i0] = bitangents[i0] + b;
        bitangents[i1] = bitangents[i1] + b;
        bitangents[i2] = bitangents[i2] + b;
    }

    for (size_t v = 0; v < vertices.size(); ++v) {
        tangents[v].normaliza();
        bitangents[v].normaliza();
    }
    return hasTangents();
}