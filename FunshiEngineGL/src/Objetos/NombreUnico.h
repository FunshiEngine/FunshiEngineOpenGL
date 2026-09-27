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
#ifndef NOMBREUNICO_H
#define NOMBREUNICO_H

// Nombre por defecto de los objetos nuevos, sin repetir los del arbol de
// escena.
//
// Mismo criterio que GameScene ya usaba para "Camara N": un prefijo y un
// sufijo numerico creciente hasta encontrar un nombre libre. Vive en un header
// (mismo criterio que SoltarEnCarpeta.h: pocas lineas sin dependencias) para
// que lo compartan la factoria, la escena y los tests.
//
// Motivo: sin nombre, el arbol de escena cae al nombre de clase
// (SceneObjectTree: `if (etiqueta.empty()) etiqueta = demangle(...)`), asi que
// el usuario ve "SimpleObject" en vez del nombre del objeto, y como el vacio
// se guarda y se recarga fielmente, se queda asi para siempre.

#include <string>
#include <vector>

#include "GameObject.h"

namespace NombresUnicos {

// Todos los nombres no vacios de `raiz` y TODO su subarbol (raiz puede ser
// nula: el caller que todavia no tiene escena sigue pidiendo un nombre libre).
inline void recolectar(const GameObject* nodo, std::vector<std::string>& nombres) {
    if (!nodo) return;
    const std::string nombre(nodo->inputName);
    if (!nombre.empty()) nombres.push_back(nombre);
    for (auto* hijo : nodo->getChildEntities()) {
        if (hijo) recolectar(dynamic_cast<const GameObject*>(hijo), nombres);
    }
}

inline std::vector<std::string> enUso(const GameObject* raiz) {
    std::vector<std::string> nombres;
    recolectar(raiz, nombres);
    return nombres;
}

// `base` si no esta tomada, si no `base 1`, `base 2`, ... hasta quedar libre.
// `contador` es una pista de arranque (se actualiza al final) para no escanear
// desde 1 cada vez: el mismo truco que el contador de camaras.
inline std::string porDefecto(const std::string& base, int& contador,
                              const GameObject* raiz) {
    const std::vector<std::string> nombres = enUso(raiz);
    int sufijo = contador > 0 ? contador : 1;
    for (;;) {
        const std::string candidato = base + " " + std::to_string(sufijo);
        bool libre = true;
        for (const std::string& n : nombres)
            if (n == candidato) {
                libre = false;
                break;
            }
        if (libre) break;
        ++sufijo;
    }
    contador = sufijo;
    return base + " " + std::to_string(sufijo);
}

} // namespace NombresUnicos

#endif // NOMBREUNICO_H
