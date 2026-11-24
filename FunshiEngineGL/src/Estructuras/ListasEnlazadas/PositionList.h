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
#ifndef POSITIONLIST_H
#define POSITIONLIST_H

#include <iostream>
#include "../../Estructuras/Position/Position.h"

using namespace std;

template<typename E>
class PositionList {
public:
    virtual bool isEmpty() = 0;
    virtual int tam() = 0;
    virtual bool isElement(E p) = 0;
    virtual Position<E>* whatElementPosition(E p) = 0;

    virtual Position<E>* first() = 0;
    virtual Position<E>* last() = 0;

    virtual void addFirst(E e) = 0;
    virtual void addLast(E e) = 0;
    virtual void addAfter(Position<E>* pl, E e) = 0;
    virtual void addBefore(Position<E>* pr, E e) = 0;

    virtual Position<E>* next(Position<E>* p) = 0;
    virtual Position<E>* prev(Position<E>* p) = 0;
    virtual E remove(Position<E>* p) = 0;
    virtual E remplace(Position<E>* p, E e) = 0;
    virtual void swapPositions(Position<E>* p1, Position<E>* p2) = 0;
    virtual void clear() = 0;
    virtual void deleteByElement(E e) = 0;
};
#endif