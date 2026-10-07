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
#include "ClonadorObjetos.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <typeinfo>

#include "GameObject.h"
#include "Modelos3D.h"
#include "SimpleObject.h"
#include "Componentes/ComponentFactory.h"
#include "Componentes/Transform.h"
#include "../Herramientas/TypeUtils.h"
#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

namespace {

bool copiarComponente(GameObject* destino, Component* origen) {
    if (!destino || !origen) return false;
    namespace fs = std::filesystem;
    static int contadorClon = 0;
    const fs::path temporal =
        fs::temp_directory_path() /
        ("funshi_clon_" + std::to_string(++contadorClon) + ".bin");
    {
        std::ofstream salida(temporal, std::ios::binary);
        if (!salida) return false;
        origen->saveComponent(&salida);
        if (!salida) return false;
    }
    std::ifstream entrada(temporal, std::ios::binary);
    if (!entrada) {
        std::error_code ec;
        fs::remove(temporal, ec);
        return false;
    }
    const std::string tipo = demangle(typeid(*origen).name());
    bool ok = false;
    if (tipo == "Transform") {
        // El objeto ya nace con su Transform (lo pone Entity): se copian los
        // datos sobre el existente en vez de agregar un segundo.
        if (Transform* t = destino->getComponent<Transform>()) {
            t->loadComponent(&entrada);
            ok = static_cast<bool>(entrada);
        }
    } else if (std::unique_ptr<Component> nuevo =
                   ComponentFactory::create(tipo, *destino)) {
        nuevo->loadComponent(&entrada);
        if (entrada) {
            destino->addComponent(std::move(nuevo));
            ok = true;
        }
    } else {
        std::cerr << "[clon] componente sin fabrica: " << tipo << std::endl;
    }
    std::error_code ec;
    fs::remove(temporal, ec);
    return ok;
}

void clonarNodo(GameObject* original, GameObject* padre,
                std::vector<std::string>& tomados,
                std::vector<std::pair<std::unique_ptr<GameObject>, GameObject*>>& salida) {
    std::unique_ptr<GameObject> copia;
    if (dynamic_cast<Modelos3D*>(original))
        copia = std::make_unique<Modelos3D>();
    else
        copia = std::make_unique<SimpleObject>();
    copia->setTag(original->getTag());
    copia->setState(original->getState());
    const std::string unico =
        ClonadorObjetos::nombreLibre(original->inputName, tomados);
    std::snprintf(copia->inputName, sizeof(copia->inputName), "%s",
                  unico.c_str());
    if (ListaDE<Component*>* comps = original->getComponents()) {
        if (!comps->isEmpty()) {
            for (auto* nodo = comps->first(); nodo;
                 nodo = (nodo != comps->last()) ? comps->next(nodo)
                                                : nullptr) {
                if (Component* c = nodo->getElement())
                    copiarComponente(copia.get(), c);
            }
        }
    }
    GameObject* crudo = copia.get();
    salida.emplace_back(std::move(copia), padre);
    for (Entity* hijo : original->getChildEntities()) {
        if (GameObject* hijoObjeto = dynamic_cast<GameObject*>(hijo))
            clonarNodo(hijoObjeto, crudo, tomados, salida);
    }
}

} // namespace

std::string ClonadorObjetos::nombreLibre(const std::string& base,
                                         std::vector<std::string>& tomados) {
    if (base.empty()) return "";
    std::string candidato = base;
    int sufijo = 1;
    for (;;) {
        bool libre = true;
        for (const std::string& tomado : tomados) {
            if (tomado == candidato) {
                libre = false;
                break;
            }
        }
        if (libre) break;
        candidato = base + " " + std::to_string(sufijo++);
    }
    tomados.push_back(candidato);
    return candidato;
}

std::vector<std::pair<std::unique_ptr<GameObject>, GameObject*>>
ClonadorObjetos::clonar(GameObject* original, GameObject* padre,
                        std::vector<std::string> tomados) {
    std::vector<std::pair<std::unique_ptr<GameObject>, GameObject*>> salida;
    if (!original) return salida;
    clonarNodo(original, padre, tomados, salida);
    return salida;
}
