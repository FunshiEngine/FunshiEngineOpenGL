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
#include "RutasReescritura.h"

#include <filesystem>
#include <functional>
#include <iostream>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "Configuracion/EditorConfig.h"
#include "Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "Objetos/Componentes/Material.h"
#include "Objetos/Componentes/Model.h"
#include "Objetos/Componentes/Script.h"
#include "Objetos/Componentes/Skybox.h"
#include "Objetos/GameObject.h"
#include "Objetos/Modelos3D.h"

namespace {

namespace fs = std::filesystem;

// Reescribe la ruta de un asset cuyo prefijo coincida con `anterior`; aplica
// el reemplazo via `asignar` y suma las ocurrencias. El lambda de escritura
// no toca el objeto si el cambio exige recompilar (Script usa setDllPath, que
// invalida la carga y refresca el nombre de clase); el resto de componentes
// solo almacenan el string.
int reescribirPath(const std::string& ruta, const std::string& anterior,
                   const std::string& reemplazo,
                   const std::function<void(const std::string&)>& asignar) {
    if (ruta.empty()) return 0;
    const std::string recalculada =
        EditorConfig::reemplazarPrefijoRuta(ruta, anterior, reemplazo);
    if (recalculada.empty() || recalculada == ruta) return 0;
    asignar(recalculada);
    return 1;
}

// ---------------------------------------------------------------------------
// Sanado de referencias rotas al cargar
// ---------------------------------------------------------------------------

// Indice de los assets que EXISTEN bajo la raiz del proyecto: nombre base ->
// rutas encontradas. Se construye UNA sola vez por invocacion de sanado y solo
// si alguna referencia esta rota: el escaneo es el costo fijo y despues cada
// consulta es una busqueda en hash.
struct IndiceAssets {
    bool construido = false;
    std::unordered_map<std::string, std::vector<std::string>> porNombre;
};

void construirIndice(IndiceAssets& indice) {
    indice.construido = true;
    if (!EditorConfig::hayRaizAssets()) return;

    const fs::path raiz(EditorConfig::raizAssetsFijada());
    std::error_code ec;
    if (!fs::is_directory(raiz, ec) || ec) return;

    // Los permisos denegados se saltean y no se siguen los symlinks de
    // carpeta (evita ciclos); un error de iteracion corta el indexado sin
    // romper el sanado: lo que no se indexo simplemente no sera candidato.
    fs::recursive_directory_iterator it(
        raiz, fs::directory_options::skip_permission_denied, ec);
    if (ec) return;
    const fs::recursive_directory_iterator fin;
    for (; it != fin; it.increment(ec)) {
        if (ec) break;
        std::error_code ecTipo;
        if (!(*it).is_regular_file(ecTipo) || ecTipo) continue;
        const std::string nombre = (*it).path().filename().string();
        if (!nombre.empty())
            indice.porNombre[nombre].push_back((*it).path().string());
    }
}

// Repara una referencia cuyo archivo ya no existe: si el nombre base aparece
// exactamente UNA vez bajo la raiz, la referencia pasa a esa ruta (1); con
// varias o con ninguna NO se adivina (0) y el caso queda en el log: fallar
// en silencio es lo que dejo la escena apuntando a un archivo que ya no
// existia.
int sanarPath(const std::string& ruta, IndiceAssets& indice,
              const std::function<void(const std::string&)>& asignar) {
    if (ruta.empty()) return 0;
    std::error_code ec;
    if (fs::exists(ruta, ec)) return 0; // ya resuelve: nada que sanar

    if (!indice.construido) construirIndice(indice);
    const std::string nombre = fs::path(ruta).filename().string();
    if (nombre.empty()) return 0;

    const auto encontrado = indice.porNombre.find(nombre);
    if (encontrado == indice.porNombre.end() || encontrado->second.empty()) {
        std::cerr << "[escena] la ruta no existe y no hay ningun '" << nombre
                  << "' bajo la raiz de assets; se deja tal cual: " << ruta
                  << "\n";
        return 0;
    }
    if (encontrado->second.size() > 1) {
        std::cerr << "[escena] la ruta no existe y hay "
                  << encontrado->second.size() << " archivos llamados '"
                  << nombre << "' bajo la raiz de assets; no se repara para no "
                  << "adivinar: " << ruta << "\n";
        return 0;
    }

    const std::string reparada = encontrado->second.front();
    std::cerr << "[escena] ruta reparada: '" << ruta << "' -> '" << reparada
              << "'\n";
    asignar(reparada);
    return 1;
}

// ---------------------------------------------------------------------------
// Recorridos compartidos: los slots de ruta de un componente/objeto y la lista
// lineal de la escena. Reescritura y sanado son la misma baja sobre los mismos
// slots, con dos operaciones distintas; mantener una sola lista de slots evita
// que un slot nuevo (p. ej. una textura mas) se agregue en un camino y no en
// el otro.
// ---------------------------------------------------------------------------

// Aplica `operar(ruta, asignar)` a cada slot de ruta de un componente. La lista
// de slots es la de la reescritura original: los cuatro mapas del material, la
// malla del Model y la fuente del Script.
template <typename Operar>
int paraSlotsDe(Component& componente, Operar&& operar) {
    if (auto* material = dynamic_cast<Material*>(&componente)) {
        int cambios = 0;
        cambios += operar(
            material->getDiffuseMapPath(),
            [material](const std::string& p) {
                material->setDiffuseMapPath(p);
            });
        cambios += operar(
            material->getSpecularMapPath(),
            [material](const std::string& p) {
                material->setSpecularMapPath(p);
            });
        cambios += operar(
            material->getNormalMapPath(),
            [material](const std::string& p) {
                material->setNormalMapPath(p);
            });
        cambios += operar(
            material->getEmissionMapPath(),
            [material](const std::string& p) {
                material->setEmissionMapPath(p);
            });
        return cambios;
    }
    if (auto* model = dynamic_cast<Model*>(&componente)) {
        return operar(model->getPath(),
                      [model](const std::string& p) { model->setPath(p); });
    }
    if (auto* script = dynamic_cast<Script*>(&componente)) {
        return operar(
            script->getPath(),
            [script](const std::string& p) { script->setDllPath(p); });
    }
    if (auto* skybox = dynamic_cast<Skybox*>(&componente)) {
        int cambios = 0;
        cambios += operar(skybox->getCaraMasX(),
                          [skybox](const std::string& p) { skybox->setCaraMasX(p); });
        cambios += operar(skybox->getCaraMenosX(),
                          [skybox](const std::string& p) { skybox->setCaraMenosX(p); });
        cambios += operar(skybox->getCaraMasY(),
                          [skybox](const std::string& p) { skybox->setCaraMasY(p); });
        cambios += operar(skybox->getCaraMenosY(),
                          [skybox](const std::string& p) { skybox->setCaraMenosY(p); });
        cambios += operar(skybox->getCaraMasZ(),
                          [skybox](const std::string& p) { skybox->setCaraMasZ(p); });
        cambios += operar(skybox->getCaraMenosZ(),
                          [skybox](const std::string& p) { skybox->setCaraMenosZ(p); });
        return cambios;
    }
    return 0;
}

// Lo mismo para un objeto completo: el objeto puede ser en si un Modelos3D,
// cuya ruta de malla se guarda en el propio GameObject (no en un componente).
template <typename Operar>
int paraSlotsDeObjeto(GameObject& objeto, Operar&& operar) {
    int cambios = 0;
    if (auto* modelo = dynamic_cast<Modelos3D*>(&objeto)) {
        cambios += operar(modelo->getPath(),
                          [modelo](const std::string& p) {
                              modelo->setPath(p);
                          });
    }
    auto* componentes = objeto.getComponents(); // propiedad del objeto
    if (componentes && !componentes->isEmpty()) {
        auto* pos = componentes->first();
        while (pos != nullptr) {
            if (Component* componente = pos->getElement(); componente != nullptr)
                cambios += paraSlotsDe(*componente, operar);
            pos = (pos == componentes->last()) ? nullptr
                                               : componentes->next(pos);
        }
    }
    return cambios;
}

// Recorre la lista lineal de entidades de la escena (no propietaria).
template <typename Operar>
int paraCadaObjeto(ListaDE<GameObject*>* objetos, Operar&& operar) {
    if (objetos == nullptr || objetos->isEmpty()) return 0;
    int total = 0;
    auto* pos = objetos->first();
    while (pos != nullptr) {
        if (GameObject* objeto = pos->getElement(); objeto != nullptr)
            total += operar(*objeto);
        pos = (pos == objetos->last()) ? nullptr : objetos->next(pos);
    }
    return total;
}

int reescribirObjeto(GameObject& objeto, const std::string& anterior,
                     const std::string& reemplazo) {
    return paraSlotsDeObjeto(objeto,
                             [&](const std::string& ruta,
                                 const std::function<void(
                                     const std::string&)>& asignar) {
                                 return reescribirPath(ruta, anterior,
                                                       reemplazo, asignar);
                             });
}

int sanarObjeto(GameObject& objeto, IndiceAssets& indice) {
    return paraSlotsDeObjeto(
        objeto,
        [&](const std::string& ruta,
            const std::function<void(const std::string&)>& asignar) {
            return sanarPath(ruta, indice, asignar);
        });
}

} // namespace

int RutasReescritura::reescribirEnEscena(ListaDE<GameObject*>* objetos,
                                         const std::string& anterior,
                                         const std::string& reemplazo) {
    if (objetos == nullptr || objetos->isEmpty() || anterior.empty() ||
        reemplazo.empty())
        return 0;

    return paraCadaObjeto(objetos, [&](GameObject& objeto) {
        return reescribirObjeto(objeto, anterior, reemplazo);
    });
}

int RutasReescritura::sanarRutasInexistentes(ListaDE<GameObject*>* objetos) {
    // Sin raiz de assets fijada no hay donde buscar (sin proyecto abierto, o
    // en un contexto que no la fija): no se repara nada y no se avisa.
    if (objetos == nullptr || objetos->isEmpty() ||
        !EditorConfig::hayRaizAssets())
        return 0;

    IndiceAssets indice;
    return paraCadaObjeto(objetos, [&](GameObject& objeto) {
        return sanarObjeto(objeto, indice);
    });
}
