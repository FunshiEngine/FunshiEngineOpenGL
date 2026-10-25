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
#include "GameObjectFactory.h"

#include <algorithm>
#include <cstdio>

#include "NombreUnico.h"
#include "SimpleObject.h"

std::unique_ptr<GameObject> GameObjectFactory::createSimpleObject(
    const GameObject* raiz) {
    auto objeto = std::make_unique<SimpleObject>();
    // Contador de proceso como pista de arranque: el primer "Objeto N" no
    // tiene que recorrer el arbol entero para saber que `Objeto 1` ya existe.
    static int contadorObjetos = 0;
    const std::string nombre =
        NombresUnicos::porDefecto("Objeto", contadorObjetos, raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "%s",
                  nombre.c_str());
    return objeto;
}

std::unique_ptr<GameObject> GameObjectFactory::createSimpleObject(
    const GameObject* raiz, const std::string& nombre) {
    auto objeto = std::make_unique<SimpleObject>();
    std::string elegido = nombre;
    if (elegido.empty()) {
        static int contadorObjetos = 0;
        elegido = NombresUnicos::porDefecto("Objeto", contadorObjetos, raiz);
    } else {
        // El nombre pedido se respeta tal cual si esta libre; si ya lo usa
        // otro objeto del arbol, se le anade un sufijo numerico.
        const std::vector<std::string> nombres = NombresUnicos::enUso(raiz);
        for (int sufijo = 2;; ++sufijo) {
            const bool libre =
                std::find(nombres.begin(), nombres.end(), elegido) == nombres.end();
            if (libre) break;
            elegido = nombre + " " + std::to_string(sufijo);
        }
    }
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "%s",
                  elegido.c_str());
    return objeto;
}
