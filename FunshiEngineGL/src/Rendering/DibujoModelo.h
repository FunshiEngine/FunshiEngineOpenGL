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
#ifndef DIBUJOMODELO_H
#define DIBUJOMODELO_H

class GameObject;
class AssetManager;
class Mesh;

// Resuelve, para un GameObject con componente Model, que malla se dibuja y con
// que matriz mundo. Vive fuera de SceneRenderer y no toca la pila grafica, para
// poder verificarse headless: la matriz sale del Transform GLOBAL del objeto
// (getGlobalTransform), de modo que la jerarquia padre/hijo se respeta. Antes
// se fabricaba un GameObject temporal y se le copiaba el Transform local, lo
// que duplicaba el Transform y perdia la cadena de ancestros.
struct DibujoModelo
{
    const Mesh* malla = nullptr; // prestada: la retiene el AssetManager
    float modelo[16] = {0.f};
    bool valido = false;
};

// Devuelve valido=true solo si el objeto tiene componente Model con path, la
// malla carga y hay Transform. Si la carga falla se registra la causa y se deja
// valido=false en vez de propagar: el objeto simplemente no se dibuja.
DibujoModelo resolverDibujoModelo(GameObject* objeto, AssetManager& assets);

#endif
