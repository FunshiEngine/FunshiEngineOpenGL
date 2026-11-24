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
#ifndef BNODO_H
#define BNODO_H

#include "../Position/Position.h"

// Nodo de un arbol binario enlazado: cada nodo referencia a su padre y a sus
// (a lo sumo dos) hijos. Es dueno de su subarbol: el destructor libera a los
// hijos recursivamente, por lo que borrar la raiz libera el arbol completo.
template<typename E>
class BNodo : public Position<E> {
private:
    E element;
    BNodo<E>* left;
    BNodo<E>* right;
    BNodo<E>* dad;

public:
    BNodo(E elem) : BNodo(elem, nullptr) {
    }

    BNodo(E elem, BNodo<E>* padre) {
        element = elem;
        left = nullptr;
        right = nullptr;
        dad = padre;
    }

    E getElement() override { return element; }

    void setElement(E e) { element = e; }

    // Accesores "hacia abajo" (hijos) y "hacia arriba" (padre).
    BNodo<E>* getLeft() { return left; }
    BNodo<E>* getRight() { return right; }
    BNodo<E>* getDad() { return dad; }

    void setLeft(BNodo<E>* l) { left = l; }
    void setRight(BNodo<E>* r) { right = r; }
    void setDad(BNodo<E>* d) { dad = d; }

    // Destructor recursivo: libera el subarbol completo. Los elementos E no
    // son propiedad del nodo (no se borran aqui).
    ~BNodo() {
        delete left;
        delete right;
        left = nullptr;
        right = nullptr;
    }
};

#endif