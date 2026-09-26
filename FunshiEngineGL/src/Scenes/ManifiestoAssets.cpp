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
#include "ManifiestoAssets.h"

#include <functional>
#include <map>

#include "ManifiestoAssetsCore.h"
#include "Objetos/Componentes/Material.h"
#include "Objetos/Componentes/Model.h"
#include "Objetos/Componentes/Script.h"
#include "Objetos/GameObject.h"
#include "Objetos/Modelos3D.h"

namespace {

// Fotografia los paths de asset de un GameObject (malla + 4 texturas +
// script). La malla sale del propio objeto cuando es un Modelos3D; si no,
// de su componente Model (si lo lleva). La lista de componentes es del
// objeto, no se libera aqui.
EntradaAssets recogerDeObjeto(GameObject& objeto) {
    EntradaAssets entrada;

    if (auto* modelo = dynamic_cast<Modelos3D*>(&objeto))
        entrada.malla = modelo->getPath();

    auto* componentes = objeto.getComponents();
    if (componentes && !componentes->isEmpty()) {
        auto* pos = componentes->first();
        while (pos != nullptr) {
            if (Component* componente = pos->getElement(); componente != nullptr) {
                if (auto* material = dynamic_cast<Material*>(componente)) {
                    entrada.texturas[0] = material->getDiffuseMapPath();
                    entrada.texturas[1] = material->getSpecularMapPath();
                    entrada.texturas[2] = material->getNormalMapPath();
                    entrada.texturas[3] = material->getEmissionMapPath();
                } else if (auto* model = dynamic_cast<Model*>(componente)) {
                    if (entrada.malla.empty())
                        entrada.malla = model->getPath();
                } else if (auto* script = dynamic_cast<Script*>(componente)) {
                    entrada.script = script->getPath();
                }
            }
            pos = (pos == componentes->last()) ? nullptr : componentes->next(pos);
        }
    }
    return entrada;
}

// Aplica una ruta persistida (ya absolutizada) a un solo campo: se asigna
// solo si es no vacia y difiere de la vigente. Devuelve 1 si se asigno.
int aplicarCampo(const std::string& persistida, const std::string& vigente,
                 const std::function<void(const std::string&)>& asignar) {
    std::string resolucion = ManifiestoAssetsCore::resolverRuta(persistida, vigente);
    if (resolucion == vigente)
        return 0;
    asignar(resolucion);
    return 1;
}

// Vuelca una entrada persistida (absoluta) sobre el objeto, asignando solo
// los campos que cambiaron (guarda de recargas innecesarias de malla/script).
int aplicarEnObjeto(GameObject& objeto, const EntradaAssets& entrada) {
    int cambios = 0;

    if (auto* modelo = dynamic_cast<Modelos3D*>(&objeto)) {
        cambios += aplicarCampo(entrada.malla, modelo->getPath(),
                                [modelo](const std::string& p) { modelo->setPath(p); });
    }

    auto* componentes = objeto.getComponents();
    if (componentes && !componentes->isEmpty()) {
        auto* pos = componentes->first();
        while (pos != nullptr) {
            if (Component* componente = pos->getElement(); componente != nullptr) {
                if (auto* material = dynamic_cast<Material*>(componente)) {
                    cambios += aplicarCampo(
                        entrada.texturas[0], material->getDiffuseMapPath(),
                        [material](const std::string& p) { material->setDiffuseMapPath(p); });
                    cambios += aplicarCampo(
                        entrada.texturas[1], material->getSpecularMapPath(),
                        [material](const std::string& p) { material->setSpecularMapPath(p); });
                    cambios += aplicarCampo(
                        entrada.texturas[2], material->getNormalMapPath(),
                        [material](const std::string& p) { material->setNormalMapPath(p); });
                    cambios += aplicarCampo(
                        entrada.texturas[3], material->getEmissionMapPath(),
                        [material](const std::string& p) { material->setEmissionMapPath(p); });
                } else if (auto* model = dynamic_cast<Model*>(componente)) {
                    cambios += aplicarCampo(entrada.malla, model->getPath(),
                                            [model](const std::string& p) { model->setPath(p); });
                } else if (auto* script = dynamic_cast<Script*>(componente)) {
                    cambios += aplicarCampo(entrada.script, script->getPath(),
                                            [script](const std::string& p) { script->setDllPath(p); });
                }
            }
            pos = (pos == componentes->last()) ? nullptr : componentes->next(pos);
        }
    }
    return cambios;
}

} // namespace

bool ManifiestoAssets::guardar(const std::string& rutaArchivo,
                               ListaDE<GameObject*>* objetos) {
    if (!objetos || objetos->isEmpty())
        return ManifiestoAssetsCore::escribirArchivo(rutaArchivo,
                                                     std::map<int, EntradaAssets>());

    std::map<int, EntradaAssets> entradas;
    auto* pos = objetos->first();
    while (pos != nullptr) {
        if (GameObject* objeto = pos->getElement(); objeto != nullptr) {
            EntradaAssets entrada = recogerDeObjeto(*objeto);
            if (!ManifiestoAssetsCore::entradaVacia(entrada)) {
                ManifiestoAssetsCore::relativizarEntrada(entrada);
                entradas[objeto->getId()] = std::move(entrada);
            }
        }
        pos = (pos == objetos->last()) ? nullptr : objetos->next(pos);
    }
    return ManifiestoAssetsCore::escribirArchivo(rutaArchivo, entradas);
}

bool ManifiestoAssets::cargar(const std::string& rutaArchivo,
                              ListaDE<GameObject*>* objetos) {
    if (!objetos || objetos->isEmpty())
        return false;

    std::map<int, EntradaAssets> entradas;
    if (!ManifiestoAssetsCore::leerArchivo(rutaArchivo, entradas))
        return false;

    int aplicados = 0;
    for (const auto& [id, persistida] : entradas) {
        GameObject* objetivo = nullptr;
        auto* pos = objetos->first();
        while (pos != nullptr) {
            if (GameObject* candidato = pos->getElement();
                candidato != nullptr && candidato->getId() == id) {
                objetivo = candidato;
                break;
            }
            pos = (pos == objetos->last()) ? nullptr : objetos->next(pos);
        }
        if (objetivo == nullptr)
            continue;

        EntradaAssets entrada = persistida;
        ManifiestoAssetsCore::absolutizarEntrada(entrada);
        aplicados += aplicarEnObjeto(*objetivo, entrada);
    }
    return aplicados > 0;
}