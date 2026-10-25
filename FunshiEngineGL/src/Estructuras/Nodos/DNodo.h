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
#ifndef DNODO_H
#define DNODO_H
#include "../Position/Position.h"
#include <iostream>
using namespace std;

template<typename E>
class DNodo : public Position<E> {
private:
    DNodo<E>* left;
    DNodo<E>* right;
    E element;

public:
    // Constructor correcto: deja left/right inicializados a nullptr para que
    // las operaciones de ListaDE nunca lean punteros sin inicializar (los
    // sentinela front/tail solo se enlazan al construirlos).
    DNodo(E elem){
        element = elem;
        left = nullptr;
        right = nullptr;
    }

    // Implementación del método virtual puro
    E getElement() override {
        return element;
    }

    void setElement(E element) {
        this->element = element;
    }

    // Métodos para acceder a los nodos hijos
    DNodo<E>* getLeft() { return left; }
    DNodo<E>* getRight() { return right; }

    // Métodos para modificar los nodos hijos
    void setLeft(DNodo<E>* l) { left = l; }
    void setRight(DNodo<E>* r) { right = r; }

    // Destructor
    ~DNodo() {
        // Aquí normalmente se manejará la liberación de memoria
        // si los nodos hijos son propiedad de este nodo
    }
};


#endif