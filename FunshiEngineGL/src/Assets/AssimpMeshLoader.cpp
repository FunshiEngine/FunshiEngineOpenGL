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
#include "AssimpMeshLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "AssetException.h"
#include "AssetPath.h"

std::shared_ptr<Mesh> AssimpMeshLoader::load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                  aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices |
                  aiProcess_GenUVCoords | aiProcess_CalcTangentSpace);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
        !scene->mRootNode) {
        throw AssetLoadException(path, importer.GetErrorString());
    }
    if (scene->mNumMeshes == 0)
        throw AssetLoadException(path, "el archivo no contiene mallas");

    auto mesh = std::make_shared<Mesh>();
    mesh->name = AssetPath::basename(path);

    // El importador pide aiProcess_GenSmoothNormals, pero no siempre puede
    // generarlas (mallas no manifoldes, caras degeneradas): las sub-mallas que
    // llegan sin ellas dejan ceros, y hasNormals() solo mira el tamano, asi que
    // pasarian por validas y el shader las dibujaria apagadas. La deteccion es
    // por vertice y no por sub-malla porque un asset puede mezclar unas con
    // normales y otras sin ellas.
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* src = scene->mMeshes[m];
        if (!src) continue;

        const unsigned int base = static_cast<unsigned int>(mesh->vertices.size());
        mesh->vertices.reserve(mesh->vertices.size() + src->mNumVertices);
        mesh->normals.reserve(mesh->normals.size() + src->mNumVertices);
        mesh->uvs.reserve(mesh->uvs.size() + src->mNumVertices);
        mesh->indices.reserve(mesh->indices.size() + src->mNumFaces * 3);

        const bool hasNormals = src->HasNormals();
        const bool hasUvs = src->HasTextureCoords(0);
        const bool hasTangentes = src->HasTangentsAndBitangents();
        if (hasTangentes) {
            // El marco tangente se copia en paralelo a vertices (una fila por
            // vertice); sin el, la malla no tiene normal mapping.
            mesh->tangents.reserve(mesh->tangents.size() + src->mNumVertices);
            mesh->bitangents.reserve(mesh->bitangents.size() + src->mNumVertices);
        }

        for (unsigned int v = 0; v < src->mNumVertices; ++v) {
            const aiVector3D& p = src->mVertices[v];
            mesh->vertices.emplace_back(p.x, p.y, p.z);
            if (hasNormals) {
                const aiVector3D& n = src->mNormals[v];
                mesh->normals.emplace_back(n.x, n.y, n.z);
            } else {
                // Sin normales: ceros para mantener una por vertice (el
                // renderer las regenera si hace falta).
                mesh->normals.emplace_back(0.f, 0.f, 0.f);
            }
            if (hasTangentes) {
                const aiVector3D& t = src->mTangents[v];
                const aiVector3D& b = src->mBitangents[v];
                mesh->tangents.emplace_back(t.x, t.y, t.z);
                mesh->bitangents.emplace_back(b.x, b.y, b.z);
            } else {
                mesh->tangents.emplace_back(0.f, 0.f, 0.f);
                mesh->bitangents.emplace_back(0.f, 0.f, 0.f);
            }
            if (hasUvs) {
                const aiVector3D& uv = src->mTextureCoords[0][v];
                mesh->uvs.emplace_back(uv.x, uv.y);
            } else {
                mesh->uvs.emplace_back(0.f, 0.f);
            }
        }

        for (unsigned int f = 0; f < src->mNumFaces; ++f) {
            const aiFace& face = src->mFaces[f];
            if (face.mNumIndices == 3) {
                mesh->indices.push_back(base + face.mIndices[0]);
                mesh->indices.push_back(base + face.mIndices[1]);
                mesh->indices.push_back(base + face.mIndices[2]);
            }
        }
    }

    if (mesh->vertices.empty())
        throw AssetLoadException(path, "la malla resultante no tiene vertices");

    // Si quedo algun vertice con normal nula, se le recalcula por cara
    // (area-weighted) solo a el: el asset entra al shader iluminado en vez de
    // caer al dibujado legacy de cara plana. Con "soloFaltantes" se respetan las
    // normales que si trajo el importador, que son las que definen las aristas
    // duras del modelo.
    for (const vec3& n : mesh->normals) {
        if (n.magnitude() > 1e-8f) continue;
        mesh->computeNormals(true);
        break;
    }

    return mesh;
}