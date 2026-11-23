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
#ifndef PRIMITIVAS2D_H
#define PRIMITIVAS2D_H

#include "Mesh.h"

// Primitivas geometricas 2D en CPU, independientes de OpenGL: son la "malla"
// base que el renderer de sprites sube a GPU una sola vez. Viven como Mesh para
// reutilizar el mismo camino que MeshRenderer (AssetManager/Assimp generan Mesh;
// esto genera una sin cargar archivo) y para poder probarse en los targets
// headless.
namespace Primitivas2D {

// Quad unitario (1x1) centrado en el origen, contenido en el plano XY. Las UVs
// van de (0,0) a (1,1) y los indices arman dos triangulos. La posicion en Z es 0
// porque el sprite no es un volumen: el orden en pantalla lo decide la posicion
// Z del Transform del objeto (ver SceneRenderer).
inline Mesh quad() {
    Mesh malla;
    malla.name = "quad2D";
    malla.vertices = {
        vec3(-0.5f, -0.5f, 0.0f),
        vec3( 0.5f, -0.5f, 0.0f),
        vec3( 0.5f,  0.5f, 0.0f),
        vec3(-0.5f,  0.5f, 0.0f),
    };
    malla.uvs = {
        vec2(0.0f, 0.0f),
        vec2(1.0f, 0.0f),
        vec2(1.0f, 1.0f),
        vec2(0.0f, 1.0f),
    };
    malla.indices = {0, 1, 2, 2, 3, 0};
    return malla;
}

} // namespace Primitivas2D

#endif
