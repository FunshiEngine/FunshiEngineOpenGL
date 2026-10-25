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
// Pruebas headless de las Estructuras de datos del motor (sin pila grafica).
// Cubren: ListaDE (incl. el addFirst corregido y la liberacion de nodos en
// remove), ArbolEnlazado (deleteRoot/deleteInternalNode/deleteExternalNode con
// la nueva gestion de memoria), PriorityListaDE, MinHeap/MaxHeap,
// ListMergeSort y ArbolBinarioEnlazado, y el contrato de las excepciones
// del motor (el mensaje de Throwable llega intacto a quien la captura).
//
// Las estructuras enlazadas del motor usan E=puntero (p. ej.
// TNodo<GameObject*>): checkPosition exige comparables contra nullptr, por lo
// que aqui las pruebas usan int* (valores referencia, duenos de los tests,
// que se liberan al final; los nodos/arboles no son duenos de los elementos).

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/ExcepcionesCPP/Throwable.h"
#include "../FunshiEngineGL/src/Estructuras/ListasEnlazadas/ListasConPrioridad/PriorityListaDE.h"
#include "../FunshiEngineGL/src/Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../FunshiEngineGL/src/Estructuras/MetodosDeOrdenamiento/ListMergeSort.h"
#include "../FunshiEngineGL/src/Estructuras/Trees/ArbolesEnlazados/ArbolBinarioEnlazado.h"
#include "../FunshiEngineGL/src/Estructuras/Trees/ArbolesEnlazados/ArbolEnlazado.h"
#include "../FunshiEngineGL/src/Estructuras/Trees/Heap/MaxHeap.h"
#include "../FunshiEngineGL/src/Estructuras/Trees/Heap/MinHeap.h"

namespace {

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Fabrica de elementos: devuelve un int* nuevo y lo registra como dueno del
// test para liberarlo al final (evita fugas; el build usa ASan/LSan).
std::vector<std::unique_ptr<int>>& duenosGlobales() {
    static std::vector<std::unique_ptr<int>> duenos;
    return duenos;
}

int* nuevoInt(int v) {
    duenosGlobales().push_back(std::make_unique<int>(v));
    return duenosGlobales().back().get();
}

// Elemento para PriorityListaDE (que exige *->distanciaA(ref)).
struct Punto {
    int x, y;
    int distanciaA(const Punto* ref) const {
        return std::abs(x - ref->x) + std::abs(y - ref->y);
    }
};

std::vector<std::unique_ptr<Punto>>& duenosPuntos() {
    static std::vector<std::unique_ptr<Punto>> duenos;
    return duenos;
}

Punto* nuevoPunto(int x, int y) {
    duenosPuntos().push_back(std::make_unique<Punto>(Punto{x, y}));
    return duenosPuntos().back().get();
}

// Recorre una ListaDE en orden y vuelca los elementos (punteros).
template <typename E>
std::vector<E> volcar(ListaDE<E>& l) {
    std::vector<E> v;
    if (l.isEmpty()) return v;
    Position<E>* pos = l.first();
    while (pos != nullptr) {
        v.push_back(pos->getElement());
        pos = (pos != l.last()) ? l.next(pos) : nullptr;
    }
    return v;
}

// Convierte vector<int*> a vector<int> (comparaciones legibles).
std::vector<int> valores(const std::vector<int*>& v) {
    std::vector<int> r;
    for (int* p : v) r.push_back(*p);
    return r;
}

void testListaDE() {
    ListaDE<int*> l;
    int* uno = nuevoInt(1);
    int* dos = nuevoInt(2);
    int* tres = nuevoInt(3);
    l.addLast(uno);
    l.addLast(dos);
    l.addLast(tres);
    CHECK(l.tam() == 3, "tam tras addLast x3");
    CHECK(*l.first()->getElement() == 1, "first tras addLast");
    CHECK(*l.last()->getElement() == 3, "last tras addLast");

    // addFirst (regresion del fix: se inserta tras front, no tras tail).
    int* cero = nuevoInt(0);
    l.addFirst(cero);
    CHECK(l.tam() == 4, "tam tras addFirst");
    CHECK(*l.first()->getElement() == 0, "addFirst coloca al frente");
    CHECK(*l.last()->getElement() == 3, "addFirst no toca el final");

    // addAfter/addBefore.
    int* veinticinco = nuevoInt(25);
    l.addAfter(l.whatElementPosition(dos), veinticinco);
    l.addBefore(l.whatElementPosition(cero), nuevoInt(-1));
    const std::vector<int> orden = valores(volcar(l));
    CHECK((orden == std::vector<int>{-1, 0, 1, 2, 25, 3}),
          "addAfter/addBefore reordenan correcto");

    // remplace conserva la posicion.
    int* viejo = l.remplace(l.whatElementPosition(veinticinco), nuevoInt(9));
    CHECK(*viejo == 25, "remplace devuelve el elemento anterior");
    const std::vector<int> trasRemplace = valores(volcar(l));
    CHECK((trasRemplace == std::vector<int>{-1, 0, 1, 2, 9, 3}),
          "remplace conserva el orden");

    // remove: elimina el nodo del medio y deja la lista coherente.
    int* extraido = l.remove(l.whatElementPosition(dos));
    CHECK(*extraido == 2, "remove devuelve el elemento");
    const std::vector<int> trasRemove = valores(volcar(l));
    CHECK((trasRemove == std::vector<int>{-1, 0, 1, 9, 3}), "remove reenlaza");
    l.addLast(nuevoInt(99)); // lista usable tras remove (nodo liberado)
    CHECK(*l.last()->getElement() == 99, "addLast tras remove");

    // isElement / deleteByElement (por identidad de puntero).
    CHECK(l.isElement(cero), "isElement encuentra el 0");
    l.deleteByElement(cero);
    CHECK(!l.isElement(cero), "deleteByElement elimino el 0");

    // clear + destructor (~ListaDE libera la cadena de nodos).
    l.clear();
    CHECK(l.isEmpty(), "clear vacia la lista");
    l.addFirst(uno);
    CHECK(*l.first()->getElement() == 1, "lista reutilizable tras clear");

    // remove sobre lista vacia debe lanzar (no crashear).
    ListaDE<int*> vacia;
    bool lanzo = false;
    try {
        vacia.remove(vacia.first());
    } catch (const std::exception&) {
        lanzo = true;
    }
    CHECK(lanzo, "remove en lista vacia lanza excepcion");
}

void testArbolEnlazado() {
    // Arbol: 1 -> {2, 3}
    ArbolEnlazado<int*> arbol;
    Position<int*>* raiz = arbol.createRoot(nuevoInt(1));
    Position<int*>* h2 = arbol.addNodeChildOf(raiz, nuevoInt(2));
    Position<int*>* h3 = arbol.addNodeChildOf(raiz, nuevoInt(3));
    CHECK(arbol.tam() == 3, "tam del arbol");
    CHECK(arbol.isInternal(raiz), "raiz interna con 2 hijos");
    CHECK(arbol.isExternal(h2), "h2 hoja");
    CHECK(arbol.isRoot(raiz), "isRoot verdadero en la raiz");

    auto* hijos = arbol.childsOf(raiz);
    CHECK(hijos->tam() == 2, "childsOf devuelve 2 hijos");
    Position<int*>* hijoPrimero = hijos->first()->getElement();
    Position<int*>* hijoUltimo = hijos->last()->getElement();
    CHECK(*hijoPrimero->getElement() == 2, "primer hijo es 2");
    CHECK(*hijoUltimo->getElement() == 3, "ultimo hijo es 3");
    delete hijos;

    CHECK(*arbol.dadOf(h2)->getElement() == 1, "dadOf(h2) es la raiz");

    // Borrado de hoja: pierde el indice, el arbol se mantiene coherente.
    int* e2 = arbol.deleteExternalNode(h2);
    CHECK(*e2 == 2, "deleteExternalNode devuelve el elemento");
    CHECK(arbol.tam() == 2, "tam tras borrar hoja");
    Position<int*>* h3b = arbol.whatIsPositionOf(h3->getElement());
    CHECK(h3b != nullptr, "el 3 sigue presente");

    // Borrado de nodo interno: el primer hijo (3) ocupa el lugar de la raiz.
    int* e1 = arbol.deleteInternalNode(raiz);
    CHECK(*e1 == 1, "deleteInternalNode devuelve el elemento");
    CHECK(*arbol.rootOfTree()->getElement() == 3, "primer hijo pasa a ser raiz");
    CHECK(arbol.isExternal(arbol.rootOfTree()), "nueva raiz es hoja");

    // deleteRoot sobre la raiz hoja, queda vacio.
    int* e3 = arbol.deleteRoot();
    CHECK(*e3 == 3, "deleteRoot devuelve el elemento");
    CHECK(arbol.isEmpty(), "arbol vacio tras deleteRoot");

    // deleteExternalNode sobre raiz hoja en otro arbol.
    ArbolEnlazado<int*> solitario;
    Position<int*>* unica = solitario.createRoot(nuevoInt(7));
    CHECK(*solitario.deleteExternalNode(unica) == 7, "deleteExternalNode raiz hoja");
    CHECK(solitario.isEmpty(), "arbol vacio");

    // whatIsPositionOf en vacio: nullptr, sin lanzar.
    CHECK(solitario.whatIsPositionOf(nuevoInt(7)) == nullptr, "busqueda en vacio da nullptr");
}

void testPriorityListaDE() {
    Punto* origen = nuevoPunto(0, 0);
    Punto* p1 = nuevoPunto(1, 0); // dist 1
    Punto* p2 = nuevoPunto(1, 1); // dist 2
    Punto* p5 = nuevoPunto(3, 2); // dist 5
    Punto* p9 = nuevoPunto(5, 4); // dist 9

    PriorityListaDE<Punto*> pl(origen);
    pl.insertarOrdenado(p9);
    pl.insertarOrdenado(p2);
    pl.insertarOrdenado(p5);
    pl.insertarOrdenado(p1);
    CHECK(pl.tam() == 4, "tam de la lista de prioridad");

    // El menos pesado (menor distancia) queda al frente.
    CHECK(pl.first()->getElement()->distanciaA(origen) == 1, "first es el mas liviano");
    CHECK(pl.last()->getElement()->distanciaA(origen) == 9, "last es el mas pesado");

    // Ordenamiento completo: distancias 1,2,5,9.
    const std::vector<Punto*> v = volcar(pl);
    CHECK(v[0]->distanciaA(origen) == 1, "orden[0] dist 1");
    CHECK(v[1]->distanciaA(origen) == 2, "orden[1] dist 2");
    CHECK(v[2]->distanciaA(origen) == 5, "orden[2] dist 5");
    CHECK(v[3]->distanciaA(origen) == 9, "orden[3] dist 9");
}

void testHeaps() {
    MinHeap<int> minh;
    CHECK(minh.isEmpty(), "MinHeap comienza vacio");
    minh.push(5);
    minh.push(3);
    minh.push(8);
    minh.push(1);
    minh.push(4);
    CHECK(minh.tam() == 5, "MinHeap tam");
    CHECK(minh.top() == 1, "MinHeap top es el minimo");
    CHECK(minh.pop() == 1, "MinHeap pop minimo");
    CHECK(minh.pop() == 3, "MinHeap pop segundo");
    CHECK(minh.pop() == 4, "MinHeap pop tercero");
    CHECK(minh.pop() == 5, "MinHeap pop cuarto");
    CHECK(minh.pop() == 8, "MinHeap pop quinto");
    CHECK(minh.isEmpty(), "MinHeap vacio tras pops");
    minh.push(2);
    minh.clear();
    CHECK(minh.isEmpty(), "MinHeap clear");

    MaxHeap<int> maxh;
    maxh.push(5);
    maxh.push(3);
    maxh.push(8);
    maxh.push(1);
    CHECK(maxh.top() == 8, "MaxHeap top es el maximo");
    CHECK(maxh.pop() == 8, "MaxHeap pop maximo");
    CHECK(maxh.pop() == 5, "MaxHeap pop segundo");
    CHECK(maxh.pop() == 3, "MaxHeap pop tercero");
    CHECK(maxh.pop() == 1, "MaxHeap pop cuarto");
    CHECK(maxh.isEmpty(), "MaxHeap vacio");

    // 0..100 desordenados: extrae en orden decreciente.
    MaxHeap<int> all;
    for (int i = 100; i >= 0; --i) all.push(i);
    bool ordenado = true;
    int anterior = all.pop();
    while (!all.isEmpty()) {
        const int actual = all.pop();
        if (anterior < actual) ordenado = false;
        anterior = actual;
    }
    CHECK(ordenado, "MaxHeap extrae en orden decreciente (0..100)");
}

struct MenorPorValor {
    bool operator()(int* a, int* b) const { return *a < *b; }
};
struct MayorPorValor {
    bool operator()(int* a, int* b) const { return *a > *b; }
};

void testListMergeSort() {
    ListaDE<int*> l;
    for (int v : {9, 1, 8, 2, 7, 3, 6, 4, 5, 0}) l.addLast(nuevoInt(v));

    ListMergeSort<int*, MenorPorValor>::sort(l);
    const std::vector<int> orden = valores(volcar(l));
    CHECK((orden == std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}),
          "ListMergeSort ordena ascendente");

    // Repetidos se conservan (estable y sin perder elementos; ordena por valor).
    ListaDE<int*> dups;
    int* dosB = nuevoInt(2);
    dups.addLast(dosB);
    dups.addLast(nuevoInt(1));
    dups.addLast(dosB);
    dups.addLast(nuevoInt(1));
    ListMergeSort<int*, MenorPorValor>::sort(dups);
    const std::vector<int> ordenDups = valores(volcar(dups));
    CHECK((ordenDups == std::vector<int>{1, 1, 2, 2}), "ListMergeSort con duplicados");

    // Comparador custom (descendente).
    ListaDE<int*> d;
    d.addLast(nuevoInt(3));
    d.addLast(nuevoInt(1));
    d.addLast(nuevoInt(2));
    ListMergeSort<int*, MayorPorValor>::sort(d);
    CHECK(valores(volcar(d)) == (std::vector<int>{3, 2, 1}), "ListMergeSort comparador desc");
}

void testArbolBinario() {
    ArbolBinarioEnlazado<int*> arbol;
    Position<int*>* raiz = arbol.createRoot(nuevoInt(1));
    Position<int*>* n2 = arbol.addLeft(raiz, nuevoInt(2));
    Position<int*>* n3 = arbol.addRight(raiz, nuevoInt(3));
    Position<int*>* n4 = arbol.addLeft(n2, nuevoInt(4));
    Position<int*>* n5 = arbol.addRight(n2, nuevoInt(5));
    CHECK(arbol.tam() == 5, "ArbolBinario tam");
    CHECK(arbol.hasLeft(raiz) && arbol.hasRight(raiz), "raiz con ambos hijos");
    CHECK(*arbol.leftOf(raiz)->getElement() == 2, "leftOf raiz");
    CHECK(*arbol.rightOf(raiz)->getElement() == 3, "rightOf raiz");
    CHECK(arbol.isExternal(n5), "n5 es hoja");
    CHECK(arbol.isInternal(n2), "n2 es interno");
    CHECK(*arbol.dadOf(n2)->getElement() == 1, "dadOf(n2) es la raiz");

    // childsOf de un nodo binario: hijos existentes.
    auto* hijosIzq = arbol.childsOf(n2);
    CHECK(hijosIzq->tam() == 2, "childsOf devuelve 2 hijos");
    delete hijosIzq;

    // preorden RID: 1,2,4,5,3.
    auto* pre = arbol.preorden();
    CHECK(pre->tam() == 5, "preorden tam");
    std::vector<int> esperado;
    Position<Position<int*>*>* p = pre->first();
    while (p != nullptr) {
        esperado.push_back(*p->getElement()->getElement());
        p = (p != pre->last()) ? pre->next(p) : nullptr;
    }
    CHECK((esperado == std::vector<int>{1, 2, 4, 5, 3}), "preorden RID");
    delete pre;

    // deleteExternalNode sobre una hoja interna (n4=4): se desvincula sola.
    int* e4 = arbol.deleteExternalNode(n4);
    CHECK(*e4 == 4, "deleteExternalNode binario devuelve el elemento");
    CHECK(!arbol.hasLeft(n2), "n2 quedo sin hijo izquierdo");
    CHECK(arbol.hasRight(n2), "n2 conserva el derecho (5)");
    CHECK(arbol.tam() == 4, "tam tras borrar hoja");

    // deleteInternalNode sobre la raiz con ambos hijos: el izquierdo (2) ocupa
    // el lugar y el derecho (3) cuelga del descendiente mas a la derecha (5).
    int* e1 = arbol.deleteInternalNode(raiz);
    CHECK(*e1 == 1, "deleteInternalNode binario devuelve el elemento");
    CHECK(*arbol.rootOfTree()->getElement() == 2, "raiz binaria ahora es 2");
    CHECK(*arbol.rightOf(arbol.rootOfTree())->getElement() == 5, "nueva raiz derecha 5");
    CHECK(*arbol.rightOf(arbol.rightOf(arbol.rootOfTree()))->getElement() == 3,
          "3 cuelga del mas derecho");
    CHECK(arbol.tam() == 3, "tam tras deleteInternalNode");

    // deleteExternalNode sobre raiz hoja -> queda vacio.
    ArbolBinarioEnlazado<int*> solitario;
    Position<int*>* unica = solitario.createRoot(nuevoInt(7));
    CHECK(*solitario.deleteExternalNode(unica) == 7, "raiz hoja binaria");
    CHECK(solitario.isEmpty(), "binario vacio");

    // deleteRoot se niega si la raiz tiene hijos.
    ArbolBinarioEnlazado<int*> conHijo;
    Position<int*>* r = conHijo.createRoot(nuevoInt(1));
    conHijo.addLeft(r, nuevoInt(2));
    bool lanzo = false;
    try {
        conHijo.deleteRoot();
    } catch (const std::exception&) {
        lanzo = true;
    }
    CHECK(lanzo, "deleteRoot con hijos lanza");

    // deleteRoot sobre raiz hoja funciona (conHijo se libera en su dtor).
    ArbolBinarioEnlazado<int*> hoja;
    Position<int*>* rh = hoja.createRoot(nuevoInt(9));
    CHECK(*hoja.deleteRoot() == 9, "deleteRoot binario hoja");
    CHECK(hoja.isEmpty(), "binario vacio tras deleteRoot");
}

} // namespace

// La excepcion base del motor conserva el mensaje con el que se lanzo: la
// clase base ya guarda el texto y la anulacion de what() no lo pierde.
void testExcepcionThrowable() {
    Throwable error("fallo al cargar la escena");
    CHECK(std::string(error.what()) == "fallo al cargar la escena",
          "Throwable devuelve el mensaje con el que se lanzo");
    CHECK(error.std::runtime_error::what() ==
              std::string("fallo al cargar la escena"),
          "el mensaje tambien llega a la clase base");
    try {
        throw error;
    } catch (const std::exception& excepcion) {
        CHECK(std::string(excepcion.what()) == "fallo al cargar la escena",
              "al capturar como std::exception se conserva el mensaje");
    }

    Throwable sinMensaje;
    CHECK(std::string(sinMensaje.what()).empty(),
          "una excepcion sin mensaje no inventa texto");
}

int main() {
    testListaDE();
    testArbolEnlazado();
    testPriorityListaDE();
    testHeaps();
    testListMergeSort();
    testArbolBinario();
    testExcepcionThrowable();

    std::cout << "EstructurasTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}