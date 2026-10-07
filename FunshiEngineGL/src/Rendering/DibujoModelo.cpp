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
#include "DibujoModelo.h"

#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "../Assets/AssetManager.h"
#include "../Assets/Mesh.h"
#include "../Objetos/Componentes/Model.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Objetos/GameObject.h"

DibujoModelo resolverDibujoModelo(GameObject* objeto, AssetManager& assets) {
    DibujoModelo resultado;
    if (!objeto) return resultado;

    Model* model = objeto->getComponent<Model>();
    if (!model) return resultado;

    const std::string path = model->getPath();
    if (path.empty()) return resultado;

    Transform* transform = objeto->getGlobalTransform();
    if (!transform) return resultado;

    try {
        const std::shared_ptr<const Mesh> mesh = assets.getMesh(path);
        if (!mesh) return resultado;
        resultado.malla = mesh.get();
    } catch (const std::exception& e) {
        // Una ruta que no se puede importar (script asignado por error, archivo
        // movido) no debe tumbar el editor: se avisa y el objeto no se dibuja.
        std::cerr << "[Model] no se pudo cargar la malla '" << path
                  << "': " << e.what() << std::endl;
        return resultado;
    }

    buildMatrixFromTransform(transform, resultado.modelo);
    resultado.valido = true;
    return resultado;
}
