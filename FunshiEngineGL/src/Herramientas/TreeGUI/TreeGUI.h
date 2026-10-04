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
#ifndef TREE_GUI_H
#define TREE_GUI_H

#include <set>
#include <imgui.h>

#include "../Estructuras/Trees/ArbolesEnlazados/ArbolEnlazado.h"

// Desduplicacion entre jerarquias (escena, explorador de archivos,...):
// un unico recorrido pre-orden de ArbolEnlazado<T> que dibuja un
// TreeNodeEx por nodo, con estado de colapso persistente, PushID por nodo y
// restore de la pila de widgets. El contenido de la fila lo decide el
// renderizador (drawRow), que aporta la logica especifica de cada dominio.
namespace TreeIG {

// Estado de colapso compartido. Se indexa con el puntero del elemento: clave
// estable mientras el elemento viva en el arbol.
using OpenState = std::set<const void*>;

// Resultado del renderizador de una fila.
struct RowResult {
    bool open = false;    // true -> el nodo quedo abierto (se recorren hijos)
    bool toggled = false; // true -> el usuario toco la flecha este frame
};

// Recorre en pre-orden un ArbolEnlazado<T> (T = tipo de elemento, p.ej.
// GameObject*). La raiz se trata como contenedor: sus hijos se recorren sin
// dibujar la raiz misma. drawRow(elemento, wasOpen) devuelve RowResult.
// IMPORTANTE: drawRow debe leer IsItemToggledOpen() justo despues de dibujar
// su TreeNodeEx, antes de emitir menus/drag&drop (sobreescriben el "last item").
//
// Los hijos se recorren sobre la lista interna del TNodo (getChilds()) en
// lugar de childsOf(): esta estructura clasica nunca libera sus nodos y
// childsOf() construye una ListaDE nueva en cada llamada, con lo que con
// muchos objetos anidados se fugaba memoria por frame.
template <typename T, typename Fn>
void forEachChild(TNodo<T>* node, Fn&& fn) {
    ListaDE<TNodo<T>*>* children = node->getChilds();
    Position<TNodo<T>*>* child = children->first();
    while (child) {
        fn(child->getElement());
        child = (child != children->last()) ? children->next(child) : nullptr;
    }
}
template <typename T, typename Row, typename OpenSet, typename KeyOf>
void drawTreeKeyed(ArbolEnlazado<T>* tree, Position<T>* pos,
                   OpenSet& openState, Row&& drawRow, KeyOf&& keyOf,
                   bool drawRoot = false) {
    if (!tree || !pos) return;

    if (pos == tree->rootOfTree() && !drawRoot) {
        if (!((TNodo<T>*)pos)->getChilds()->isEmpty()) {
            forEachChild((TNodo<T>*)pos, [&](Position<T>* child) {
                drawTreeKeyed(tree, child, openState, drawRow, keyOf, false);
            });
        }
        return;
    }

    // Solo los nodos hoja-vacia-nula no se dibujan; el resto, siempre.
    TNodo<T>* node = (TNodo<T>*)pos;
    T element = node->getElement();
    if (!element) return;

    // PushID por nodo: mientras el elemento viva, su puntero es una clave
    // estable y unica (evita IDs de ImGui compartidos entre nodos iguales).
    ImGui::PushID((const void*)element);

    // La clave de colapso NO tiene que ser el puntero del elemento: clave =
    // keyOf(element) admite claves que sobreviven a una reconstruccion del
    // arbol (p.ej. la ruta de un archivo) o punteros (como antes).
    const auto key = keyOf(element);
    const bool wasOpen = openState.count(key) != 0;
    const RowResult result = drawRow(element, wasOpen);

    if (result.toggled) {
        if (wasOpen)
            openState.erase(key);
        else
            openState.insert(key);
    }

    // Contrato de ImGui: si TreeNodeEx devolvio true (nodo abierto) SE DEBE
    // llamar TreePop() esta misma frame, tenga o no hijos. Desplegar un nodo
    // sin hijos sin el TreePop() descuadra el TreeDepth interno de ImGui y
    // corrompe el dibujo/crash posterior de la ventana.
    if (result.open) {
        if (!node->getChilds()->isEmpty()) {
            forEachChild(node, [&](Position<T>* child) {
                drawTreeKeyed(tree, child, openState, drawRow, keyOf);
            });
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

// Version clasica: el estado de colapso se indexa con el puntero del elemento.
template <typename T, typename Row>
void drawTree(ArbolEnlazado<T>* tree, Position<T>* pos, OpenState& openNodes,
              Row&& drawRow) {
    drawTreeKeyed(tree, pos, openNodes, drawRow,
                  [](T element) -> const void* {
                      return static_cast<const void*>(element);
                  });
}

} // namespace TreeIG

#endif