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
#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>

#include "../Matematicas/StructVec3.h"

// Geometria de una malla en CPU, independiente de OpenGL. Es el asset que
// comparte AssetManager entre objetos: lo genera el loader (Assimp en la
// integracion con Modelos3D) o procedimentalmente, y el renderer moderno lo
// sube a VBO/VAO. Ademas de vertices/normales/UVs, guarda el marco tangente
// (tangente + bitangente por vertice) para normal mapping en el shader.
struct Mesh {
    std::string name;
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<vec3> tangents;
    std::vector<vec3> bitangents;
    std::vector<vec2> uvs;
    std::vector<unsigned int> indices;

    bool isEmpty() const { return vertices.empty(); }

    // Las normales valen solo si hay una por vertice.
    bool hasNormals() const {
        return !normals.empty() && normals.size() == vertices.size();
    }

    // El marco tangente vale solo si hay tangente Y bitangente por vertice.
    bool hasTangents() const {
        return !tangents.empty() && tangents.size() == vertices.size() &&
               !bitangents.empty() && bitangents.size() == vertices.size();
    }

    // Las UVs valen solo si hay una por vertice.
    bool hasUvs() const {
        return !uvs.empty() && uvs.size() == vertices.size();
    }

    // Calcula una normal por vertice a partir de las caras: acumula la normal
    // de cada triángulo (producto cruz, que ya viene ponderado por el area) en
    // sus tres vertices y normaliza al final, promediando por area. Es el mismo
    // criterio que computeTangents(). Requiere vertices e indices triangulados.
    // Los vertices que no pertenecen a ninguna cara conservan la normal nula en
    // vez de un NaN. Devuelve false si no hay indices triangulados.
    //
    // "soloFaltantes" recalcula SOLO los vertices cuya normal es nula y respeta
    // las que ya traen normal (no nula). Es el modo para assets que mezclan
    // sub-mallas con y sin normales: recomputar todas las dejaria planas
    // (promediando aristas duras) y se perderia el suavizado que dio el
    // importador.
    bool computeNormals(bool soloFaltantes = false);

    // Calcula tangente/bitangente por vertice a partir de vertices+UVs
    // (algoritmo de cara estandar de Lengyel/iMDesmarais), acumulando por
    // vertice y normalizando. Requiere hasUvs() y hasNormals(). Las caras con
    // area de UV degenerada se omiten. Devuelve false si no hay UVs/normales.
    bool computeTangents();

    // AABB en espacio local. Devuelve false si la malla no tiene vertices.
    bool computeBounds(vec3& outMin, vec3& outMax) const;
};

#endif