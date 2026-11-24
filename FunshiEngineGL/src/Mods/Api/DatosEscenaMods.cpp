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
#include "DatosEscenaMods.h"

#include <cstring>
#include <string>
#include <typeinfo>

#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Escena/Transform.h"
#include "../../Objetos/Componentes/Colliders/Collider.h"
#include "../../Scenes/GameScene.h"
#include "../../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../../Herramientas/TypeUtils.h"

namespace FunshiMods {

namespace {

GameScene* g_escena = nullptr;

// Copia un texto a un buffer de tamano fijo, recortando lo que no quepa y
// garantizando el terminador.
void copiarTexto(char* destino, size_t capacidad, const char* origen) {
    if (!destino || capacidad == 0) return;
    if (!origen) {
        destino[0] = '\0';
        return;
    }
    const size_t largo = std::strlen(origen);
    const size_t aCopiar = largo < capacidad - 1 ? largo : capacidad - 1;
    std::memcpy(destino, origen, aCopiar);
    destino[aCopiar] = '\0';
}

ListaDE<GameObject*>* listaObjetos() {
    return g_escena ? g_escena->getGameObjectsScene() : nullptr;
}

// Recorrido seguro de la lista: next() lanza al llegar al ultimo, asi que el
// avance se hace con el idiom de la casa (comparar con last()).
template <typename Lista, typename E>
Position<E>* avanzar(Lista* lista, Position<E>* nodo) {
    return (nodo != lista->last()) ? lista->next(nodo) : nullptr;
}

GameObject* objetoEnIndice(int indice) {
    auto* lista = listaObjetos();
    if (!lista || lista->isEmpty() || indice < 0) return nullptr;
    int actual = 0;
    for (auto* nodo = lista->first(); nodo; nodo = avanzar(lista, nodo)) {
        if (actual == indice) return nodo->getElement();
        ++actual;
    }
    return nullptr;
}

Component* componenteEnIndice(GameObject* objeto, int indice) {
    if (!objeto) return nullptr;
    auto* componentes = objeto->getComponents();
    if (!componentes || componentes->isEmpty() || indice < 0) return nullptr;
    int actual = 0;
    for (auto* nodo = componentes->first(); nodo;
         nodo = avanzar(componentes, nodo)) {
        if (actual == indice) return nodo->getElement();
        ++actual;
    }
    return nullptr;
}

bool objetoTieneCollider(GameObject* objeto) {
    if (!objeto) return false;
    auto* componentes = objeto->getComponents();
    if (!componentes || componentes->isEmpty()) return false;
    for (auto* nodo = componentes->first(); nodo;
         nodo = avanzar(componentes, nodo)) {
        Component* componente = nodo->getElement();
        if (componente && dynamic_cast<Collider*>(componente)) return true;
    }
    return false;
}

// --- callbacks de la fuente de datos ---

int contarObjetos(void* /*contexto*/) {
    auto* lista = listaObjetos();
    return lista ? lista->tam() : 0;
}

bool obtenerObjeto(void* /*contexto*/, int indice, ApiModGameObject* salida) {
    if (!salida) return false;
    GameObject* objeto = objetoEnIndice(indice);
    if (!objeto) return false;

    salida->id = objeto->getId();
    copiarTexto(salida->nombre, sizeof(salida->nombre), objeto->inputName);
    copiarTexto(salida->etiqueta, sizeof(salida->etiqueta),
                objeto->getTag().c_str());

    if (Transform* transform = objeto->getGlobalTransform()) {
        const float* translate = transform->getTranslatef();
        const float* escala = transform->getScalef();
        const float* rotacion = transform->getRotatef();
        for (int eje = 0; eje < 3; ++eje) {
            salida->posicion[eje] = translate[eje];
            salida->escala[eje] = escala[eje];
        }
        for (int eje = 0; eje < 4; ++eje) salida->rotacion[eje] = rotacion[eje];
    }

    auto* componentes = objeto->getComponents();
    salida->cantidadComponentes = componentes ? componentes->tam() : 0;
    salida->tieneCollider = objetoTieneCollider(objeto) ? 1 : 0;
    return true;
}

int contarComponentes(void* /*contexto*/, int indice) {
    GameObject* objeto = objetoEnIndice(indice);
    if (!objeto) return 0;
    auto* componentes = objeto->getComponents();
    return componentes ? componentes->tam() : 0;
}

bool obtenerComponente(void* /*contexto*/, int indiceObjeto,
                        int indiceComponente, ApiModComponente* salida) {
    if (!salida) return false;
    Component* componente = componenteEnIndice(objetoEnIndice(indiceObjeto),
                                               indiceComponente);
    if (!componente) return false;
    const std::string tipo = demangle(typeid(*componente).name());
    copiarTexto(salida->tipo, sizeof(salida->tipo), tipo.c_str());
    return true;
}

} // namespace

void inyectarEscenaMods(GameScene* escena) { g_escena = escena; }

const FuenteDatosEscena* fuenteDatosEscenaMods() {
    static const FuenteDatosEscena fuente = {
        /* .contarObjetos     = */ contarObjetos,
        /* .obtenerObjeto     = */ obtenerObjeto,
        /* .contarComponentes = */ contarComponentes,
        /* .obtenerComponente = */ obtenerComponente,
        /* .contexto          = */ nullptr,
    };
    return &fuente;
}

} // namespace FunshiMods
