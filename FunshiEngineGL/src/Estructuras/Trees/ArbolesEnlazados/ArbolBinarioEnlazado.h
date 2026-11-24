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
#ifndef ARBOLBINARIOENLAZADO_H
#define ARBOLBINARIOENLAZADO_H

#include <iostream>
#include "../../../ExcepcionesCPP/InvalidOperationException.h"
#include "../../../ExcepcionesCPP/ExcepcionesEstructuras/ArbolesEnlazados/EmptyTreeException.h"
#include "../Tree.h"
#include "../../Nodos/BNodo.h"

using namespace std;

// Arbol binario enlazado (cada nodo con 0, 1 o 2 hijos). Satisface la
// interfaz generica Tree<E> y ademas expone las operaciones binarias
// (addLeft/addRight/leftOf/rightOf/hasLeft/hasRight).
//
// Los nodos son propiedad del arbol (BNodo destuye su subarbol en cascada);
// los elementos E NO son propiedad (no se borran).
template<typename E>
class ArbolBinarioEnlazado : public Tree<E> {
private:
    BNodo<E>* root;
    int size;

    BNodo<E>* checkPosition(Position<E>* p) {
        if (isEmpty()) {
            throw InvalidPositionException("ArbolBinarioEnlazado::checkPosition:ElArbolEstaVacio");
        }
        if (p == nullptr) {
            throw InvalidPositionException("ArbolBinarioEnlazado::checkPosition:LaPosicionEsNullPtr");
        }
        try {
            return (BNodo<E>*)p;
        }
        catch (std::exception& e) {
            throw InvalidPositionException("ArbolBinarioEnlazado::checkPosition:LaPosicionNoEsDelArbol");
        }
    }

    void intentarAgregar(BNodo<E>* dad, BNodo<E>* hijo, bool aLaDerecha,
                         const char* mensaje) {
        if (aLaDerecha) {
            if (dad->getRight() != nullptr) {
                delete hijo;
                throw InvalidOperationException(mensaje);
            }
            dad->setRight(hijo);
        }
        else {
            if (dad->getLeft() != nullptr) {
                delete hijo;
                throw InvalidOperationException(mensaje);
            }
            dad->setLeft(hijo);
        }
    }

public:
    ArbolBinarioEnlazado() {
        root = nullptr;
        size = 0;
    }

    ArbolBinarioEnlazado(E e) {
        root = new BNodo<E>(e);
        size = 1;
    }

    ~ArbolBinarioEnlazado() {
        // ~BNodo libera el subarbol completo de forma recursiva.
        delete root;
        root = nullptr;
    }

    //==============consultas de la estructura==============
    virtual int tam() const override { return size; }
    virtual bool isEmpty() const override { return size == 0; }

    virtual Position<E>* rootOfTree() override {
        if (isEmpty()) {
            throw EmptyTreeException("ArbolBinarioEnlazado::root:NoHayRoot");
        }
        return root;
    }

    //==============consultas de una posicion==============
    virtual ListaDE<Position<E>*>* childsOf(Position<E>* p) override {
        BNodo<E>* node = checkPosition(p);
        ListaDE<Position<E>*>* resultado = new ListaDE<Position<E>*>();
        if (node->getLeft() != nullptr) resultado->addLast(node->getLeft());
        if (node->getRight() != nullptr) resultado->addLast(node->getRight());
        return resultado;
    }

    virtual Position<E>* dadOf(Position<E>* p) override {
        BNodo<E>* node = checkPosition(p);
        if (isRoot(node)) {
            throw InvalidOperationException("ArbolBinarioEnlazado::dadOf:LaPosicionEsRoot");
        }
        return node->getDad();
    }

    // Hijos binarios: pueden ser nullptr (posicion no ocupada).
    Position<E>* leftOf(Position<E>* p) {
        return checkPosition(p)->getLeft();
    }
    Position<E>* rightOf(Position<E>* p) {
        return checkPosition(p)->getRight();
    }

    bool hasLeft(Position<E>* p) { return leftOf(p) != nullptr; }
    bool hasRight(Position<E>* p) { return rightOf(p) != nullptr; }

    //==============consultas tipo de nodo==============
    virtual bool isRoot(Position<E>* p) override {
        return checkPosition(p) == root;
    }

    virtual bool isInternal(Position<E>* p) override {
        BNodo<E>* node = checkPosition(p);
        return node->getLeft() != nullptr || node->getRight() != nullptr;
    }

    virtual bool isExternal(Position<E>* p) override {
        return !isInternal(p);
    }

    //==============modificar la estructura==============
    //=>agregar
    virtual Position<E>* createRoot(E e) override {
        if (root != nullptr) {
            throw InvalidOperationException("ArbolBinarioEnlazado::createRoot:YaExisteUnRoot");
        }
        root = new BNodo<E>(e);
        size++;
        return root;
    }

    // Agrega un hijo al primer hueco disponible (izquierda, luego derecha).
    virtual Position<E>* addNodeChildOf(Position<E>* p, E e) override {
        BNodo<E>* dad = checkPosition(p);
        BNodo<E>* hijo = new BNodo<E>(e, dad);
        if (dad->getLeft() == nullptr) {
            dad->setLeft(hijo);
        }
        else if (dad->getRight() == nullptr) {
            dad->setRight(hijo);
        }
        else {
            delete hijo;
            throw InvalidOperationException("ArbolBinarioEnlazado::addNodeChildOf:ElPadreYaTieneDosHijos");
        }
        size++;
        return hijo;
    }

    // En binario "despues de un hijo izquierdo" == ocupar la ranura derecha.
    virtual Position<E>* addNodeChildAfterOf(Position<E>* dad, Position<E>* plChild, E e) override {
        BNodo<E>* nodeDad = checkPosition(dad);
        BNodo<E>* nodePlChild = checkPosition(nodeDad);
        if (nodeDad != nodePlChild->getDad() || nodeDad->getLeft() != nodePlChild) {
            throw InvalidPositionException("ArbolBinarioEnlazado::addNodeChildAfterOf:ElHijoDeReferenciaNoEsElHijoIzquierdo");
        }
        BNodo<E>* hijo = new BNodo<E>(e, nodeDad);
        intentarAgregar(nodeDad, hijo, true,
                        "ArbolBinarioEnlazado::addNodeChildAfterOf:ElPadreYaTieneHijoDerecho");
        size++;
        return hijo;
    }

    // En binario "antes de un hijo derecho" == ocupar la ranura izquierda.
    virtual Position<E>* addNodeChildBeforeOf(Position<E>* dad, Position<E>* prChild, E e) override {
        BNodo<E>* nodeDad = checkPosition(dad);
        BNodo<E>* nodePrChild = checkPosition(nodeDad);
        if (nodeDad != nodePrChild->getDad() || nodeDad->getRight() != nodePrChild) {
            throw InvalidPositionException("ArbolBinarioEnlazado::addNodeChildBeforeOf:ElHijoDeReferenciaNoEsElHijoDerecho");
        }
        BNodo<E>* hijo = new BNodo<E>(e, nodeDad);
        intentarAgregar(nodeDad, hijo, false,
                        "ArbolBinarioEnlazado::addNodeChildBeforeOf:ElPadreYaTieneHijoIzquierdo");
        size++;
        return hijo;
    }

    // Accesos especificos para construir un binario con slot explicito.
    Position<E>* addLeft(Position<E>* dad, E e) {
        BNodo<E>* nodeDad = checkPosition(dad);
        BNodo<E>* hijo = new BNodo<E>(e, nodeDad);
        intentarAgregar(nodeDad, hijo, false,
                        "ArbolBinarioEnlazado::addLeft:ElPadreYaTieneHijoIzquierdo");
        size++;
        return hijo;
    }

    Position<E>* addRight(Position<E>* dad, E e) {
        BNodo<E>* nodeDad = checkPosition(dad);
        BNodo<E>* hijo = new BNodo<E>(e, nodeDad);
        intentarAgregar(nodeDad, hijo, true,
                        "ArbolBinarioEnlazado::addRight:ElPadreYaTieneHijoDerecho");
        size++;
        return hijo;
    }

    //=>eliminar
    // En un binario no existe "promover al primer hijo" si la raiz tiene hijos:
    // deleteRoot solo elimina la raiz si es hoja, preservando la estructura.
    virtual E deleteRoot() override {
        if (isEmpty()) {
            throw InvalidOperationException("ArbolBinarioEnlazado::deleteRoot:NoHayRoot");
        }
        if (isInternal(root)) {
            throw InvalidOperationException("ArbolBinarioEnlazado::deleteRoot:LaRaizTieneHijos");
        }
        E saveElement = root->getElement();
        delete root;
        root = nullptr;
        size--;
        return saveElement;
    }

    virtual E deleteInternalNode(Position<E>* p) override {
        BNodo<E>* theDeleteable = checkPosition(p);
        if (!isInternal(theDeleteable)) {
            throw InvalidOperationException("ArbolBinarioEnlazado::deleteInternalNode:LaPosicionEsExternal");
        }
        E saveElement = theDeleteable->getElement();

        BNodo<E>* leftChild = theDeleteable->getLeft();
        BNodo<E>* rightChild = theDeleteable->getRight();
        BNodo<E>* sustituto = leftChild != nullptr ? leftChild : rightChild;

        if (sustituto != nullptr) {
            // Suelto a los hijos del nodo antes de reparentarlos (~BNodo
            // liberaria el subarbol si quedaran enlazados al eliminarlo).
            theDeleteable->setLeft(nullptr);
            theDeleteable->setRight(nullptr);

            if (theDeleteable != root) {
                BNodo<E>* dad = theDeleteable->getDad();
                if (dad->getLeft() == theDeleteable) {
                    dad->setLeft(sustituto);
                }
                else {
                    dad->setRight(sustituto);
                }
                sustituto->setDad(dad);
            }
            else {
                root = sustituto;
                sustituto->setDad(nullptr);
            }

            if (leftChild != nullptr && rightChild != nullptr) {
                // Con ambos hijos: el subarbol derecho cuelga del descendiente
                // mas a la derecha del subarbol izquierdo.
                BNodo<E>* masDerecho = leftChild;
                while (masDerecho->getRight() != nullptr) {
                    masDerecho = masDerecho->getRight();
                }
                masDerecho->setRight(rightChild);
                rightChild->setDad(masDerecho);
            }
        }

        delete theDeleteable;
        size--;
        return saveElement;
    }

    virtual E deleteExternalNode(Position<E>* p) override {
        BNodo<E>* theDeleteable = checkPosition(p);
        if (!isExternal(theDeleteable)) {
            throw InvalidOperationException("ArbolBinarioEnlazado::deleteExternalNode:LaPosicionEsInternal");
        }
        E saveElement = theDeleteable->getElement();
        if (theDeleteable != root) {
            BNodo<E>* dad = theDeleteable->getDad();
            if (dad->getLeft() == theDeleteable) {
                dad->setLeft(nullptr);
            }
            else {
                dad->setRight(nullptr);
            }
        }
        else {
            root = nullptr;
        }
        delete theDeleteable;
        size--;
        return saveElement;
    }

    virtual E deleteNode(Position<E>* p) override {
        E resultado = nullptr;
        if (isInternal(p)) {
            resultado = deleteInternalNode(p);
        }
        else if (isExternal(p)) {
            resultado = deleteExternalNode(p);
        }
        return resultado;
    }

    //==============busquedas==============
    virtual Position<E>* whatIsPositionOf(E e) override {
        Position<E>* resultado = nullptr;
        if (!isEmpty() && e != nullptr) {
            resultado = busquedaDeElementoPreOrden(root, e);
        }
        return resultado;
    }

    virtual void positionToChildOf(Position<E>* dad, Position<E>* pChild) {
        BNodo<E>* nodeDad = checkPosition(dad);
        BNodo<E>* nodePChild = checkPosition(pChild);
        if (nodeDad == nodePChild->getDad()) {
            return; // ya es hijo directo
        }
        // Desconectar de su padre actual.
        if (nodePChild->getDad() != nullptr) {
            BNodo<E>* oldDad = nodePChild->getDad();
            if (oldDad->getLeft() == nodePChild) oldDad->setLeft(nullptr);
            else if (oldDad->getRight() == nodePChild) oldDad->setRight(nullptr);
        }
        else if (nodePChild == root) {
            // Mover la raiz: el primer hijo (izquierdo o derecho) ocupa el lugar.
            BNodo<E>* nuevoRoot = nodePChild->getLeft() != nullptr
                ? nodePChild->getLeft() : nodePChild->getRight();
            nodePChild->setLeft(nullptr);
            nodePChild->setRight(nullptr);
            root = nuevoRoot;
            if (nuevoRoot != nullptr) nuevoRoot->setDad(nullptr);
        }
        nodePChild->setDad(nullptr);
        BNodo<E>* hijo = nodePChild;
        BNodo<E>* hueco = nodeDad;
        if (hueco->getLeft() == nullptr) {
            hueco->setLeft(hijo);
            hijo->setDad(hueco);
        }
        else if (hueco->getRight() == nullptr) {
            hueco->setRight(hijo);
            hijo->setDad(hueco);
        }
        else {
            throw InvalidOperationException("ArbolBinarioEnlazado::positionToChildOf:ElPadreYaTieneDosHijos");
        }
    }

    // Recorrido preorden: devuelve (nueva lista) las posiciones en orden
    // raiz, izquierda, derecha. La lista es propiedad del llamador.
    ListaDE<Position<E>*>* preorden() {
        ListaDE<Position<E>*>* resultado = new ListaDE<Position<E>*>();
        if (!isEmpty()) {
            recorridoPreOrden(root, resultado);
        }
        return resultado;
    }

private:
    void recorridoPreOrden(BNodo<E>* node, ListaDE<Position<E>*>* lista) {
        lista->addLast(node);
        if (node->getLeft() != nullptr) {
            recorridoPreOrden(node->getLeft(), lista);
        }
        if (node->getRight() != nullptr) {
            recorridoPreOrden(node->getRight(), lista);
        }
    }

    Position<E>* busquedaDeElementoPreOrden(BNodo<E>* node, E e) {
        Position<E>* resultado = nullptr;
        if (node->getElement() == e) {
            resultado = node;
        }
        else {
            if (node->getLeft() != nullptr) {
                resultado = busquedaDeElementoPreOrden(node->getLeft(), e);
            }
            if (resultado == nullptr && node->getRight() != nullptr) {
                resultado = busquedaDeElementoPreOrden(node->getRight(), e);
            }
        }
        return resultado;
    }
};
#endif