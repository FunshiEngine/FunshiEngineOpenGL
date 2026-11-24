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
#ifndef TNODO_H
#define TNODO_H
#include "../Position/Position.h"
#include <iostream>
#include "../ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

using namespace std;
template<typename E>
//La relacion con Position es puramente polimorfica
//No uso Position pq la generacion de instancias TNodo esta encapsulada
//Dicho esto siempre operaremos con TNodo por conveniencia tecnica
class TNodo : public Position<E> {
protected:
    E element;
    TNodo<E>* rootDad;
    ListaDE<TNodo<E>*>* childs;
public:

    TNodo(E element, TNodo<E>* rootDad) {
        this->element = element;
        this->rootDad = rootDad;
        childs = new ListaDE<TNodo<E>*>();
    }

    TNodo(E element) : TNodo(element, nullptr) {
    }

    // Destructor recursivo: libera la ListaDE de hijos y cada hijo TNodo.
    // Los elementos E del arbol NO son propiedad del nodo (no se borran aqui).
    ~TNodo() {
        if (childs) {
            while (!childs->isEmpty()) {
                TNodo<E>* hijo = childs->remove(childs->first());
                delete hijo;
            }
            delete childs;
            childs = nullptr;
        }
    }

    void setRootDad(TNodo<E>* rootDad) { this->rootDad = rootDad; }
    void setElement(E element) { this->element = element; }
    
    //los modifiadores setters para childs los tiene la estructura
    ListaDE<TNodo<E>*>* getChilds() { return childs; }
    TNodo<E>* getRootDad() { return rootDad; }
    E getElement() override { return element; }
};
#endif